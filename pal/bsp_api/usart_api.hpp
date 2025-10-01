#pragma once
#include "../bsp_cfg.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace usart {
    // 确保缓冲区大小为2的幂，以便使用位操作替代模运算
    static_assert((cfg::usart_rx_sz & (cfg::usart_rx_sz - 1)) == 0, 
                 "usart_rx_sz must be a power of two");
    static_assert((cfg::usart_tx_sz & (cfg::usart_tx_sz - 1)) == 0, 
                 "usart_tx_sz must be a power of two");

    /* 对偶静态实体 */
    template <std::uint8_t No>
    struct RxBuf {
        alignas(4) static std::uint8_t data[cfg::usart_rx_sz];
    };

    template <std::uint8_t No>
    struct TxBuf {
        alignas(4) static std::uint8_t data[cfg::usart_tx_sz];
        static std::atomic_flag lock;
    };

    /* 中断友好变量（头文件内联） */
    template <std::uint8_t No> inline std::size_t rx_head{0};
    template <std::uint8_t No> inline std::size_t tx_tail{0};
    template <std::uint8_t No> inline std::size_t tx_count{0};

    /* 初始化函数 */
    inline void init() {
        // 使用折叠表达式初始化所有使能的USART缓冲区
        []<std::size_t... I>(std::index_sequence<I...>) {
            ((TxBuf<cfg::usart_en[I]>::lock.clear()), ...);
        }(std::make_index_sequence<cfg::usart_en.size()>{});
    }

    /* 对外 API：声明 + 内联实现 */
    class UsartAPI {
        UsartAPI() = delete;
    public:
        /* 返回 Rx 只读指针（内联，中断可用） */
        template <std::uint8_t No>
        [[nodiscard]] static const std::uint8_t* rx_ptr() {
            return RxBuf<No>::data;
        }

        /* 发送：内部上锁，返回锁状态（声明） */
        template <std::uint8_t No>
        [[nodiscard]] static bool send(const std::uint8_t* ptr, std::size_t len);

        /* 接收：从环形缓冲区拷出（声明） */
        template <std::uint8_t No>
        static std::size_t recv(std::uint8_t* ptr, std::size_t max_len);
    };

    // 初始化所有使能的USART
    inline bool initialized = (init(), true);
}  // namespace usart