#include "usart_api.hpp"
#include "stm32h7xx_ll_usart.h"
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

        // 中断友好变量
        template <std::uint8_t No> 
        inline std::size_t rx_head{0};
        
		template <std::uint8_t No> 
        inline std::size_t rx_tail{0};
		
        template <std::uint8_t No> 
        inline std::size_t tx_tail{0};
        
        template <std::uint8_t No> 
        inline std::size_t tx_count{0};

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
        
        template <std::uint8_t No>
        bool send_impl(const std::uint8_t* ptr, std::size_t len) {
            // 使用宽松内存序，减少内存屏障开销
            if (TxBuf<No>::lock.test_and_set(std::memory_order_relaxed))
                return false;                       // 已被锁定
            
            std::size_t n = (len > cfg::usart_tx_sz) ? cfg::usart_tx_sz : len;
            
            // 使用memcpy替代循环拷贝，编译器会优化为高效指令
            std::memcpy(TxBuf<No>::data, ptr, n);
            
            tx_count<No> = n;                       // 通知中断长度
            tx_tail<No>  = 0;                       // 复位发送指针
            
            /* 触发发送中断 */
            USART_TypeDef* inst = get_usart_instance(No);
            
            LL_USART_EnableIT_TXE(inst);            // 中断会解锁
            return true;                            // 锁成功
        }
        
        template <std::uint8_t No>
		std::size_t recv_impl(std::uint8_t* ptr, std::size_t max_len) {
			const std::uint8_t* rx = RxBuf<No>::data;
			std::size_t head, tail, available, n;



			head = rx_head<No>;
			tail = detail::rx_tail<No>; // 使用全局tail

			// 2. 计算可用数据量
			if (head >= tail) {
				available = head - tail;
			} else {
				available = cfg::usart_rx_sz - tail + head;
			}

			n = (available > max_len) ? max_len : available;

			if (n == 0) {
				return 0;
			}

			// 3. 更安全的分段拷贝逻辑
			if (head > tail) {
				// 数据连续
				std::memcpy(ptr, rx + tail, n);
			} else {
				// 数据分两段
				std::size_t first_chunk = cfg::usart_rx_sz - tail;
				if (n > first_chunk) {
					std::memcpy(ptr, rx + tail, first_chunk);
					std::memcpy(ptr + first_chunk, rx, n - first_chunk);
				} else {
					std::memcpy(ptr, rx + tail, n);
				}
			}

			// 4. 更新全局tail指针
			detail::rx_tail<No> = (tail + n) & (cfg::usart_rx_sz - 1);


			return n;
		}
    }

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
            // 使用逗号运算符和空表达式来实例化所有模板
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
        // 运行时检查
        if (!detail::is_usart_enabled(no)) return nullptr;
        
        // 使用switch-case分发到不同的模板实例
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
	// 运行时检查
	if (!detail::is_usart_enabled(no)) return false;
	
	// 使用switch-case分发到不同的模板实例
	switch (no) {
			case 0: return detail::send_impl<0>(ptr, len);
			case 1: return detail::send_impl<1>(ptr, len);
			case 2: return detail::send_impl<2>(ptr, len);
			case 3: return detail::send_impl<3>(ptr, len);
			case 4: return detail::send_impl<4>(ptr, len);
			case 5: return detail::send_impl<5>(ptr, len);
			case 6: return detail::send_impl<6>(ptr, len);
			case 7: return detail::send_impl<7>(ptr, len);
			case 8: return detail::send_impl<8>(ptr, len);
			case 9: return detail::send_impl<9>(ptr, len);
			case 10: return detail::send_impl<10>(ptr, len);
			default: return false;
	}
	}

	std::size_t UsartAPI::recv(std::uint8_t no, std::uint8_t* ptr, std::size_t max_len) {
	// 运行时检查
	if (!detail::is_usart_enabled(no)) return 0;
	
	// 使用switch-case分发到不同的模板实例
	switch (no) {
			case 0: return detail::recv_impl<0>(ptr, max_len);
			case 1: return detail::recv_impl<1>(ptr, max_len);
			case 2: return detail::recv_impl<2>(ptr, max_len);
			case 3: return detail::recv_impl<3>(ptr, max_len);
			case 4: return detail::recv_impl<4>(ptr, max_len);
			case 5: return detail::recv_impl<5>(ptr, max_len);
			case 6: return detail::recv_impl<6>(ptr, max_len);
			case 7: return detail::recv_impl<7>(ptr, max_len);
			case 8: return detail::recv_impl<8>(ptr, max_len);
			case 9: return detail::recv_impl<9>(ptr, max_len);
			case 10: return detail::recv_impl<10>(ptr, max_len);
			default: return 0;
	}
	}
    
    // usart中断处理
    namespace {
        template <std::uint8_t No>
        void usart_irq_handler() {
            USART_TypeDef* inst = detail::get_usart_instance(No);

			if (LL_USART_IsActiveFlag_ORE(inst)) {
			// 清除ORE标志的标准操作序列：先读SR（通过IsActiveFlag完成），再读DR
				LL_USART_ClearFlag_ORE(inst);
				volatile uint8_t temp_discard = LL_USART_ReceiveData8(inst); // 读取DR寄存器，丢弃数据
				(void)temp_discard; // 避免编译器警告
			}
			
            /* RX：环形推进 */
            if (LL_USART_IsActiveFlag_RXNE(inst)) {
                auto* rx = detail::RxBuf<No>::data;
                rx[detail::rx_head<No>] = LL_USART_ReceiveData8(inst);
                // 使用位操作替代模运算
                detail::rx_head<No> = (detail::rx_head<No> + 1) & (cfg::usart_rx_sz - 1);
            }

            /* TX：发完自动解锁 */
            if (LL_USART_IsActiveFlag_TXE(inst)) {
                if (detail::tx_count<No> != 0) {
                    auto* tx = detail::TxBuf<No>::data;
                    std::size_t tail = detail::tx_tail<No>;
                    LL_USART_TransmitData8(inst, tx[tail]);
                    // 使用位操作替代模运算
                    detail::tx_tail<No> = (tail + 1) & (cfg::usart_tx_sz - 1);
                    --detail::tx_count<No>;
                } else {
                    LL_USART_DisableIT_TXE(inst);          // 空
                    // 使用宽松内存序
                    detail::TxBuf<No>::lock.clear(std::memory_order_relaxed); // **解锁！**
                }
            }
        }

        // 辅助函数用于实例化中断处理模板
        template <std::size_t... I>
        void instantiate_irq_handlers(std::index_sequence<I...>) {
            // 使用逗号运算符和空表达式来实例化所有中断处理模板
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