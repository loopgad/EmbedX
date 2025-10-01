#include "usart_api.hpp"
#include "stm32h7xx_ll_usart.h"
#include <cstring> // 用于memcpy

namespace usart {
    // 定义静态成员
    template <std::uint8_t No>
    alignas(4) std::uint8_t RxBuf<No>::data[cfg::usart_rx_sz];

    template <std::uint8_t No>
    alignas(4) std::uint8_t TxBuf<No>::data[cfg::usart_tx_sz];

    template <std::uint8_t No>
    std::atomic_flag TxBuf<No>::lock = ATOMIC_FLAG_INIT;

    // 编译时查找表：将串口号映射到基地址
    template<std::uint8_t No>
    constexpr uint32_t get_usart_base_address() {
        // 默认返回0，表示不支持该串口
        return 0;
    }

    // 为每个支持的串口提供特化版本
    template<> constexpr uint32_t get_usart_base_address<0>() { return LPUART1_BASE; }
    template<> constexpr uint32_t get_usart_base_address<1>() { return USART1_BASE; }
    template<> constexpr uint32_t get_usart_base_address<2>() { return USART2_BASE; }
    template<> constexpr uint32_t get_usart_base_address<3>() { return USART3_BASE; }
    template<> constexpr uint32_t get_usart_base_address<4>() { return UART4_BASE; }
    template<> constexpr uint32_t get_usart_base_address<5>() { return UART5_BASE; }
    template<> constexpr uint32_t get_usart_base_address<6>() { return USART6_BASE; }
    template<> constexpr uint32_t get_usart_base_address<7>() { return UART7_BASE; }
    template<> constexpr uint32_t get_usart_base_address<8>() { return UART8_BASE; }
    template<> constexpr uint32_t get_usart_base_address<9>() { return UART9_BASE; }
    template<> constexpr uint32_t get_usart_base_address<10>() { return USART10_BASE; }
    
    // 编译时检查所有配置的串口是否有有效映射
    // 使用折叠表达式和索引序列来检查每个配置的串口
    static_assert(
        []<std::size_t... I>(std::index_sequence<I...>) {
            return ((get_usart_base_address<cfg::usart_en[I]>() != 0) && ...);
        }(std::make_index_sequence<cfg::usart_en.size()>{}),
        "All configured USARTs must have valid base address mappings"
    );

    // 获取 USART 实例（内联函数，零运行时开销）
    template<std::uint8_t No>
    constexpr USART_TypeDef* get_usart_instance() {
        return reinterpret_cast<USART_TypeDef*>(get_usart_base_address<No>());
    }

    // 辅助函数用于实例化模板
    namespace {
    template <std::size_t... I>
    void instantiate_templates(std::index_sequence<I...>) {
            // 使用逗号运算符和空表达式来实例化所有模板
            ((void)RxBuf<cfg::usart_en[I]>::data, ...);
            ((void)TxBuf<cfg::usart_en[I]>::data, ...);
            ((void)TxBuf<cfg::usart_en[I]>::lock, ...);
    }
    } // namespace

    // 调用辅助函数实例化模板
    static auto _ = []() {
            instantiate_templates(std::make_index_sequence<cfg::usart_en.size()>{});
            return 0;
    }();

    /* ---------- 发送实现（内部用锁） ---------- */
    template <std::uint8_t No>
    bool UsartAPI::send(const std::uint8_t* ptr, std::size_t len) {
            // 使用宽松内存序，减少内存屏障开销
            if (TxBuf<No>::lock.test_and_set(std::memory_order_relaxed))
                    return false;                       // 已被锁定
            
            std::size_t n = (len > cfg::usart_tx_sz) ? cfg::usart_tx_sz : len;
            
            // 使用memcpy替代循环拷贝，编译器会优化为高效指令
            std::memcpy(TxBuf<No>::data, ptr, n);
            
            tx_count<No> = n;                       // 通知中断长度
            tx_tail<No>  = 0;                       // 复位发送指针
            
            /* 触发发送中断 */
            // 使用查找函数获取实例（零运行时开销）
            USART_TypeDef* inst = get_usart_instance<No>();
            
            LL_USART_EnableIT_TXE(inst);            // 中断会解锁
            return true;                            // 锁成功
    }

    template <std::uint8_t No>
    std::size_t UsartAPI::recv(std::uint8_t* ptr, std::size_t max_len) {
            const std::uint8_t* rx = RxBuf<No>::data;
            static std::size_t tail{0};                       // 中断里写 head
            std::size_t head = rx_head<No>;
            
            // 计算可读取的数据量
            std::size_t available = (head >= tail) ? (head - tail) : 
                                   (cfg::usart_rx_sz - tail + head);
            std::size_t n = (available > max_len) ? max_len : available;
            
            if (n == 0) return 0;
            
            // 分段拷贝以提高效率
            if (head > tail || head == 0) {
                std::memcpy(ptr, rx + tail, n);
            } else {
                std::size_t first_chunk = cfg::usart_rx_sz - tail;
                std::memcpy(ptr, rx + tail, first_chunk);
                std::memcpy(ptr + first_chunk, rx, n - first_chunk);
            }
            
            // 使用位操作替代模运算
            tail = (tail + n) & (cfg::usart_rx_sz - 1);
            return n;                                 // 实际字节数
    }

    // usart中断bsp层
    template <std::uint8_t No>
    static void usart_irq_handler() {
            // 使用查找函数获取实例（零运行时开销）
            USART_TypeDef* inst = get_usart_instance<No>();

            /* RX：环形推进 */
            if (LL_USART_IsActiveFlag_RXNE(inst)) {
                    auto* rx = RxBuf<No>::data;
                    rx[rx_head<No>] = LL_USART_ReceiveData8(inst);
                    // 使用位操作替代模运算
                    rx_head<No> = (rx_head<No> + 1) & (cfg::usart_rx_sz - 1);
            }

            /* TX：发完自动解锁 */
            if (LL_USART_IsActiveFlag_TXE(inst)) {
                    if (tx_count<No> != 0) {
                            auto* tx = TxBuf<No>::data;
                            std::size_t tail = tx_tail<No>;
                            LL_USART_TransmitData8(inst, tx[tail]);
                            // 使用位操作替代模运算
                            tx_tail<No> = (tail + 1) & (cfg::usart_tx_sz - 1);
                            --tx_count<No>;
                    } else {
                            LL_USART_DisableIT_TXE(inst);          // 空
                            // 使用宽松内存序
                            TxBuf<No>::lock.clear(std::memory_order_relaxed); // **解锁！**
                    }
            }
    }

    // 辅助函数用于实例化中断处理模板
    namespace {
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