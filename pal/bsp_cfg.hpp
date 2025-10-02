// bsp_cfg.hpp
#pragma once



#include <array>
#include <cstddef>
#include <cstdint>


namespace cfg {
    /******************************usart*******************************/
    inline constexpr std::array<std::uint8_t, 3> usart_en{1, 2, 6};
    inline constexpr std::size_t usart_rx_sz = 64;
    inline constexpr std::size_t usart_tx_sz = 64;
    /******************************************************************/
}  // namespace cfg


