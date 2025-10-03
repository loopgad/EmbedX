#include "usart_api.hpp"
#include "usart.h"
#include <cstring>
#include <atomic>
#include <array>
#include <utility>



namespace usart {
    // 内部实现细节
    namespace detail {
        // 对偶静态实体
        template <std::uint8_t No>
        struct RxBuf {
            alignas(4) static std::uint8_t data[cfg::usart_rx_sz];
        };

        template <std::uint8_t No>
        struct TxBuf {
            alignas(4) static std::uint8_t data[cfg::usart_tx_sz];
            static std::atomic_flag lock;
        };

        // 中断友好变量（声明为volatile确保每次从内存读取）
        template <std::uint8_t No> 
        inline volatile std::size_t rx_head{0};
        
        template <std::uint8_t No> 
        inline volatile std::size_t rx_tail{0};
        
        template <std::uint8_t No> 
        inline volatile std::size_t tx_tail{0};
        
        template <std::uint8_t No> 
        inline volatile std::size_t tx_count{0};

		/* DMA 状态变量 */
		template<std::uint8_t No> 
		inline volatile bool dma_tx_in_progress{false};
		template<std::uint8_t No> 
		inline volatile bool dma_rx_in_progress{false};
		
        // 检查USART编号是否有效
        constexpr bool is_usart_enabled(std::uint8_t no) {
            for (std::size_t i = 0; i < cfg::usart_en.size(); ++i) {
                if (cfg::usart_en[i] == no) return true;
            }
            return false;
        }

        // 编译时查找表：将串口号映射到基地址
        constexpr uint32_t get_usart_base_address(std::uint8_t no) {
            switch (no) {
                case 0: return LPUART1_BASE;
                case 1: return USART1_BASE;
                case 2: return USART2_BASE;
                case 3: return USART3_BASE;
                case 4: return UART4_BASE;
                case 5: return UART5_BASE;
                case 6: return USART6_BASE;
                case 7: return UART7_BASE;
                case 8: return UART8_BASE;
                case 9: return UART9_BASE;
                case 10: return USART10_BASE;
                default: return 0;
            }
        }
        
        // 编译时检查所有配置的串口是否有有效映射
        static_assert(
            []() {
                for (std::size_t i = 0; i < cfg::usart_en.size(); ++i) {
                    if (get_usart_base_address(cfg::usart_en[i]) == 0) return false;
                }
                return true;
            }(),
            "All configured USARTs must have valid base address mappings"
        );

        // 获取 USART 实例
        USART_TypeDef* get_usart_instance(std::uint8_t no) {
            return reinterpret_cast<USART_TypeDef*>(get_usart_base_address(no));
        }

        // 内部实现函数
        template <std::uint8_t No>
        const std::uint8_t* rx_ptr_impl() {
            return RxBuf<No>::data;
        }
        
		/* 编译期检查 */
		constexpr bool is_usart_tx_dma_enabled(std::uint8_t no) {
			for (std::size_t i = 0; i < cfg::usart_tx_dma_en.size(); ++i)
				if (cfg::usart_tx_dma_en[i] == no) return true;
			return false;
		}
		constexpr bool is_usart_rx_dma_enabled(std::uint8_t no) {
			for (std::size_t i = 0; i < cfg::usart_rx_dma_en.size(); ++i)
				if (cfg::usart_rx_dma_en[i] == no) return true;
			return false;
		}

		/* HAL 句柄映射 */
		inline UART_HandleTypeDef* get_uart_handle(std::uint8_t no) {
			switch (no) {
				case 2: return &huart2;   /* 按需扩展其他端口 */
				default: return nullptr;
			}
		}

		/* DMA 发送实现（仅当 TX DMA 使能时编译） */
		template<std::uint8_t No>
		bool send_dma_impl(const std::uint8_t* ptr, std::size_t len) {
			if (len == 0) return true;
			
			UART_HandleTypeDef* huart = get_uart_handle(No);
			if (!huart) return false;
			
			// 检查是否有正在进行的 DMA 传输
			if (HAL_UART_GetState(huart) == HAL_UART_STATE_BUSY_TX) {
				// 已经有发送在进行中，无法启动新的发送
				return false;
			}
			
			// 确保 DMA 和 USART 状态已重置
			if (huart->gState != HAL_UART_STATE_READY) {
				// 如果状态不是 READY，尝试重置状态
				huart->gState = HAL_UART_STATE_READY;
			}
			
			// 直接使用用户提供的缓冲区启动 DMA 发送
			if (HAL_UART_Transmit_DMA(huart, const_cast<std::uint8_t*>(ptr), len) != HAL_OK) {
				return false;
			}
			
			return true;
		}

		/* DMA 接收实现（循环模式，仅当 RX DMA 使能时编译） */
		template<std::uint8_t No>
		std::size_t recv_dma_impl(std::uint8_t* ptr, std::size_t max_len) {
			UART_HandleTypeDef* huart = get_uart_handle(No);
			if (!huart || !huart->hdmarx) return 0;
			
			// 检查 DMA 接收是否已启动，如果没有则启动
			if (!dma_rx_in_progress<No>) {
				// 启动 DMA 接收（循环模式），直接使用用户提供的缓冲区
				if (HAL_UART_Receive_DMA(huart, ptr, max_len) != HAL_OK) {
					return 0;
				}
				dma_rx_in_progress<No> = true;
				return 0; // 第一次启动，还没有数据
			}

			// 检查 DMA 传输是否完成
			if (HAL_UART_GetState(huart) != HAL_UART_STATE_READY) {
				// DMA 传输仍在进行中
				return 0;
			}

			// DMA 传输已完成，获取实际接收的数据量
			std::size_t received = max_len - __HAL_DMA_GET_COUNTER(huart->hdmarx);
			
			// 重置 DMA 接收状态，准备下一次接收
			dma_rx_in_progress<No> = false;
			
			return received;
		}
		
        template <std::uint8_t No>
        bool send_impl_it(const std::uint8_t* ptr, std::size_t len) {
            if (TxBuf<No>::lock.test_and_set(std::memory_order_relaxed))
                return false;
            
            std::size_t n = (len > cfg::usart_tx_sz) ? cfg::usart_tx_sz : len;
            std::memcpy(TxBuf<No>::data, ptr, n);
            
            tx_count<No> = n;
            tx_tail<No> = 0;

            USART_TypeDef* inst = get_usart_instance(No);
            LL_USART_EnableIT_TXE(inst);
            return true;
        }
        
        template <std::uint8_t No>
        std::size_t available_impl() {
            volatile std::size_t& rx_head_ref = rx_head<No>;
            volatile std::size_t& rx_tail_ref = rx_tail<No>;
            
            std::size_t head = rx_head_ref;
            std::size_t tail = rx_tail_ref;
            
            if (head >= tail) {
                return head - tail;
            } else {
                return cfg::usart_rx_sz - tail + head;
            }
        }
        
		template <std::uint8_t No>
		std::size_t recv_impl_it(std::uint8_t* ptr, std::size_t max_len) {
			// 使用寄存器引用优化访问
			volatile std::size_t& rx_head_ref = detail::rx_head<No>;
			volatile std::size_t& rx_tail_ref = detail::rx_tail<No>;
			const std::uint8_t* rx_data = detail::RxBuf<No>::data;

			std::size_t head, tail, available, n;

			// 暂时关闭USART接收中断，防止在读取指针过程中被中断修改
			// 使用LL库函数获取USART实例并禁用RXNE中断
			USART_TypeDef* inst = detail::get_usart_instance(No);
			LL_USART_DisableIT_RXNE(inst); // 进入临界区

			// 安全地读取指针状态
			head = rx_head_ref;
			tail = rx_tail_ref;

			// 计算可用数据量
			if (head >= tail) {
				available = head - tail;
			} else {
				available = cfg::usart_rx_sz - tail + head;
			}

			n = (available > max_len) ? max_len : available;

			if (n == 0) {
				LL_USART_EnableIT_RXNE(inst); // 退出临界区前恢复中断
				return 0;
			}

			// 安全地进行数据拷贝
			if (tail + n <= cfg::usart_rx_sz) {
				std::memcpy(ptr, rx_data + tail, n);
			} else {
				std::size_t first_chunk = cfg::usart_rx_sz - tail;
				std::memcpy(ptr, rx_data + tail, first_chunk);
				std::memcpy(ptr + first_chunk, rx_data, n - first_chunk);
			}
			std::size_t new_tail = (tail + n) % cfg::usart_rx_sz;
			rx_tail_ref = new_tail;

			// 操作完成，重新使能USART接收中断
			LL_USART_EnableIT_RXNE(inst); // 退出临界区

			return n;
		}
        
        /* 新增：单字节FIFO接口实现 - 放入一个字节 */
        template <std::uint8_t No>
        bool putc_impl(std::uint8_t byte) {
            if (TxBuf<No>::lock.test_and_set(std::memory_order_relaxed))
                return false;
            
            // 直接将字节放入发送缓冲区
            TxBuf<No>::data[0] = byte;
            
            tx_count<No> = 1;
            tx_tail<No> = 0;

            USART_TypeDef* inst = get_usart_instance(No);
            LL_USART_EnableIT_TXE(inst);
            return true;
        }
        
        /* 新增：单字节FIFO接口实现 - 取出一个字节 */
        template <std::uint8_t No>
        bool getc_impl(std::uint8_t& byte) {
            // 使用寄存器引用优化访问
            volatile std::size_t& rx_head_ref = detail::rx_head<No>;
            volatile std::size_t& rx_tail_ref = detail::rx_tail<No>;
            const std::uint8_t* rx_data = detail::RxBuf<No>::data;

            std::size_t head, tail;

            // 暂时关闭USART接收中断，防止在读取指针过程中被中断修改
            USART_TypeDef* inst = detail::get_usart_instance(No);
            LL_USART_DisableIT_RXNE(inst); // 进入临界区

            // 安全地读取指针状态
            head = rx_head_ref;
            tail = rx_tail_ref;

            // 检查是否有数据可用
            if (head == tail) {
                LL_USART_EnableIT_RXNE(inst); // 退出临界区前恢复中断
                return false;
            }

            // 读取一个字节
            byte = rx_data[tail];
            std::size_t new_tail = (tail + 1) % cfg::usart_rx_sz;
            rx_tail_ref = new_tail;

            // 操作完成，重新使能USART接收中断
            LL_USART_EnableIT_RXNE(inst); // 退出临界区

            return true;
        }
    } // namespace detail

    // 定义静态成员
    template <std::uint8_t No>
    alignas(4) std::uint8_t detail::RxBuf<No>::data[cfg::usart_rx_sz];

    template <std::uint8_t No>
    alignas(4) std::uint8_t detail::TxBuf<No>::data[cfg::usart_tx_sz];

    template <std::uint8_t No>
    std::atomic_flag detail::TxBuf<No>::lock = ATOMIC_FLAG_INIT;

    // 辅助函数用于实例化模板
    namespace {
        template <std::size_t... I>
        void instantiate_templates(std::index_sequence<I...>) {
            ((void)detail::RxBuf<cfg::usart_en[I]>::data, ...);
            ((void)detail::TxBuf<cfg::usart_en[I]>::data, ...);
            ((void)detail::TxBuf<cfg::usart_en[I]>::lock, ...);
        }
    } // namespace

    // 调用辅助函数实例化模板
    static auto _ = []() {
        instantiate_templates(std::make_index_sequence<cfg::usart_en.size()>{});
        return 0;
    }();

    /* ---------- 对外接口实现 ---------- */
    const std::uint8_t* UsartAPI::rx_ptr(std::uint8_t no) {
        if (!detail::is_usart_enabled(no)) return nullptr;
        
        switch (no) {
            case 0: return detail::rx_ptr_impl<0>();    
            case 1: return detail::rx_ptr_impl<1>();
            case 2: return detail::rx_ptr_impl<2>();
            case 3: return detail::rx_ptr_impl<3>();
            case 4: return detail::rx_ptr_impl<4>();
            case 5: return detail::rx_ptr_impl<5>();
            case 6: return detail::rx_ptr_impl<6>();
            case 7: return detail::rx_ptr_impl<7>();
            case 8: return detail::rx_ptr_impl<8>();
            case 9: return detail::rx_ptr_impl<9>();
            case 10: return detail::rx_ptr_impl<10>();
            default: return nullptr;
        }
    }

	bool UsartAPI::send(std::uint8_t no, const std::uint8_t* ptr, std::size_t len) {
		if (!detail::is_usart_enabled(no)) return false;

	#if USART_DMA_ENABLED
		/* 如果该端口使能了 TX DMA，则调用 DMA 实现 */
		#define CASE_DMA_TX(n) \
			case n: return detail::is_usart_tx_dma_enabled(n) \
					   ? detail::send_dma_impl<n>(ptr, len) \
					   : detail::send_impl_it<n>(ptr, len);
	#else
		/* 未启用 DMA 宏时全部走中断 */
		#define CASE_DMA_TX(n) case n: return detail::send_impl_it<n>(ptr, len);
	#endif

		switch (no) {
			CASE_DMA_TX(0)
			CASE_DMA_TX(1)
			CASE_DMA_TX(2)
			CASE_DMA_TX(3)
			CASE_DMA_TX(4)
			CASE_DMA_TX(5)
			CASE_DMA_TX(6)
			CASE_DMA_TX(7)
			CASE_DMA_TX(8)
			CASE_DMA_TX(9)
			CASE_DMA_TX(10)
			default: return false;
		}
	#undef CASE_DMA_TX
	}

	std::size_t UsartAPI::recv(std::uint8_t no, std::uint8_t* ptr, std::size_t max_len) {
		if (!detail::is_usart_enabled(no)) return 0;

	#if USART_DMA_ENABLED
		#define CASE_DMA_RX(n) \
			case n: return detail::is_usart_rx_dma_enabled(n) \
					   ? detail::recv_dma_impl<n>(ptr, max_len) \
					   : detail::recv_impl_it<n>(ptr, max_len);
	#else
		#define CASE_DMA_RX(n) case n: return detail::recv_impl_it<n>(ptr, max_len);
	#endif

		switch (no) {
			CASE_DMA_RX(0)
			CASE_DMA_RX(1)
			CASE_DMA_RX(2)
			CASE_DMA_RX(3)
			CASE_DMA_RX(4)
			CASE_DMA_RX(5)
			CASE_DMA_RX(6)
			CASE_DMA_RX(7)
			CASE_DMA_RX(8)
			CASE_DMA_RX(9)
			CASE_DMA_RX(10)
			default: return 0;
		}
	#undef CASE_DMA_RX
	}
    
    std::size_t UsartAPI::available(std::uint8_t no) {
        if (!detail::is_usart_enabled(no)) return 0;
        
        switch (no) {
            case 0: return detail::available_impl<0>();
            case 1: return detail::available_impl<1>();
            case 2: return detail::available_impl<2>();
            case 3: return detail::available_impl<3>();
            case 4: return detail::available_impl<4>();
            case 5: return detail::available_impl<5>();
            case 6: return detail::available_impl<6>();
            case 7: return detail::available_impl<7>();
            case 8: return detail::available_impl<8>();
            case 9: return detail::available_impl<9>();
            case 10: return detail::available_impl<10>();
            default: return 0;
        }
    }
    
    /* 单字节FIFO接口实现 */
    bool UsartAPI::putc(std::uint8_t no, std::uint8_t byte) {
        if (!detail::is_usart_enabled(no)) return false;
        
        switch (no) {
            case 0: return detail::putc_impl<0>(byte);
            case 1: return detail::putc_impl<1>(byte);
            case 2: return detail::putc_impl<2>(byte);
            case 3: return detail::putc_impl<3>(byte);
            case 4: return detail::putc_impl<4>(byte);
            case 5: return detail::putc_impl<5>(byte);
            case 6: return detail::putc_impl<6>(byte);
            case 7: return detail::putc_impl<7>(byte);
            case 8: return detail::putc_impl<8>(byte);
            case 9: return detail::putc_impl<9>(byte);
            case 10: return detail::putc_impl<10>(byte);
            default: return false;
        }
    }
    
    bool UsartAPI::getc(std::uint8_t no, std::uint8_t& byte) {
        if (!detail::is_usart_enabled(no)) return false;
        
        switch (no) {
            case 0: return detail::getc_impl<0>(byte);
            case 1: return detail::getc_impl<1>(byte);
            case 2: return detail::getc_impl<2>(byte);
            case 3: return detail::getc_impl<3>(byte);
            case 4: return detail::getc_impl<4>(byte);
            case 5: return detail::getc_impl<5>(byte);
            case 6: return detail::getc_impl<6>(byte);
            case 7: return detail::getc_impl<7>(byte);
            case 8: return detail::getc_impl<8>(byte);
            case 9: return detail::getc_impl<9>(byte);
            case 10: return detail::getc_impl<10>(byte);
            default: return false;
        }
    }

    // usart中断处理
    namespace {
        template <std::uint8_t No>
        void usart_irq_handler() {
            USART_TypeDef* inst = detail::get_usart_instance(No);

            // 处理过载错误（必须清除ORE标志）
            if (LL_USART_IsActiveFlag_ORE(inst)) {
                LL_USART_ClearFlag_ORE(inst);
                volatile uint8_t temp_discard = LL_USART_ReceiveData8(inst);
                (void)temp_discard;
            }
            
            /* RX：环形推进（带缓冲区满检查） */
            if (LL_USART_IsActiveFlag_RXNE(inst)) {
                auto* rx = detail::RxBuf<No>::data;
                volatile std::size_t& rx_head_ref = detail::rx_head<No>;
                volatile std::size_t& rx_tail_ref = detail::rx_tail<No>;
                
                std::size_t current_head = rx_head_ref;
                std::size_t next_head = (current_head + 1) % cfg::usart_rx_sz;
                
                if (next_head != rx_tail_ref) {
                    rx[current_head] = LL_USART_ReceiveData8(inst);
                    rx_head_ref = next_head;
                } else {
                    volatile uint8_t temp_discard = LL_USART_ReceiveData8(inst);
                    (void)temp_discard;
                }
            }

            /* TX：发完自动解锁 */
            if (LL_USART_IsActiveFlag_TXE(inst)) {
                volatile std::size_t& tx_count_ref = detail::tx_count<No>;
                volatile std::size_t& tx_tail_ref = detail::tx_tail<No>;
                
                if (tx_count_ref != 0) {
                    auto* tx = detail::TxBuf<No>::data;
                    std::size_t tail = tx_tail_ref;
                    LL_USART_TransmitData8(inst, tx[tail]);
                    tx_tail_ref = (tail + 1) % cfg::usart_tx_sz;
                    --tx_count_ref;
                } else {
                    LL_USART_DisableIT_TXE(inst);
                    detail::TxBuf<No>::lock.clear(std::memory_order_relaxed);
                }
            }
        }

        // 辅助函数用于实例化中断处理模板
        template <std::size_t... I>
        void instantiate_irq_handlers(std::index_sequence<I...>) {
            ((void)usart_irq_handler<cfg::usart_en[I]>, ...);
        }
    } // namespace

    // 调用辅助函数实例化中断处理模板
    static auto _irq = []() {
        instantiate_irq_handlers(std::make_index_sequence<cfg::usart_en.size()>{});
        return 0;
    }();
}  // namespace usart

/************************api for IQRHandlers***************************/
extern "C" {
    void lpuart1_irq_handler(void) { usart::usart_irq_handler<0>(); }
    void usart1_irq_handler(void) { usart::usart_irq_handler<1>(); }
    void usart2_irq_handler(void) { usart::usart_irq_handler<2>(); }
    void usart3_irq_handler(void) { usart::usart_irq_handler<3>(); }
    void uart4_irq_handler(void) { usart::usart_irq_handler<4>(); }
    void uart5_irq_handler(void) { usart::usart_irq_handler<5>(); }
    void usart6_irq_handler(void) { usart::usart_irq_handler<6>(); }
    void uart7_irq_handler(void) { usart::usart_irq_handler<7>(); }
    void uart8_irq_handler(void) { usart::usart_irq_handler<8>(); }
    void uart9_irq_handler(void) { usart::usart_irq_handler<9>(); }
    void usart10_irq_handler(void) { usart::usart_irq_handler<10>(); }
}