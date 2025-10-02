#pragma once

#include "../bsp_cfg.hpp"
#include <cstddef>
#include <cstdint>

namespace usart {
    // 确保缓冲区大小为2的幂，以便使用位操作替代模运算
    static_assert((cfg::usart_rx_sz & (cfg::usart_rx_sz - 1)) == 0, 
                 "usart_rx_sz must be a power of two");
    static_assert((cfg::usart_tx_sz & (cfg::usart_tx_sz - 1)) == 0, 
                 "usart_tx_sz must be a power of two");

    /* 对外 API：非模板版本，只有声明 */
    class UsartAPI {
        UsartAPI() = delete;
    public:
        /* 返回 Rx 只读指针 */
        [[nodiscard]] static const std::uint8_t* rx_ptr(std::uint8_t no);

        /* 发送：内部上锁，返回锁状态 */
        [[nodiscard]] static bool send(std::uint8_t no, const std::uint8_t* ptr, std::size_t len);

        /* 接收：从环形缓冲区拷出 */
        static std::size_t recv(std::uint8_t no, std::uint8_t* ptr, std::size_t max_len);
    };
}  // namespace usart