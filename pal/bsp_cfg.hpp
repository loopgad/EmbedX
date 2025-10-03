// bsp_cfg.hpp
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace cfg {
    /******************************usart*******************************/
    inline constexpr std::array<std::uint8_t, 3> usart_en{1, 2, 6};
    inline constexpr std::size_t usart_rx_sz = 64;
    inline constexpr std::size_t usart_tx_sz = 64;
    
    // 是否使用串口dma，默认循环接收和单次发送
    #define USART_DMA_ENABLED 1
    #if USART_DMA_ENABLED
    // 每个 USART 的发送/接收 DMA 使能配置（必须是 usart_en 的子集）
    inline constexpr std::array<std::uint8_t, 1> usart_tx_dma_en{2};
    inline constexpr std::array<std::uint8_t, 1> usart_rx_dma_en{2};

    // 静态断言：确保 DMA 配置是 usart_en 的子集
    static_assert([]() {
        const auto& en = usart_en;
        for (auto tx : usart_tx_dma_en) {
            bool found = false;
            for (auto e : en) {
                if (e == tx) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        for (auto rx : usart_rx_dma_en) {
            bool found = false;
            for (auto e : en) {
                if (e == rx) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }(), "DMA configuration must be a subset of enabled USARTs");
    #endif
    /******************************************************************/

    // 获取总串口端口数量的常量
    inline constexpr std::size_t uart_count = 11;


}  // namespace cfg

/*********************if use arm math or math lib**********************/
//#define USE_ARM_MATH 
#define USE_MATH_LIB

#ifdef USE_MATH_LIB
#include "math_lib.hpp"
#endif
/**********************************************************************/

/*************************if use usart api*****************************/
#define USE_USART_API

#ifdef USE_USART_API
#include "usart_api.hpp"
#endif
/**********************************************************************/