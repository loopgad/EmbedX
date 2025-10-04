// math_lib.hpp - 零成本抽象嵌入式数学库总头文件
#pragma once
#include <cstddef>
#include "bsp_cfg.hpp"

// 编译期配置
#if defined(__ARM_ARCH) && defined(USE_ARM_MATH)
#define MATH_LIB_USE_ARM_MATH 1
#else
#define MATH_LIB_USE_ARM_MATH 0
#endif

namespace math {
    // 编译期特性检测
    constexpr bool use_arm_math = MATH_LIB_USE_ARM_MATH;

    // 性能优化标记
    struct fast_t {};
    constexpr fast_t fast{};

    // 常数定义
    constexpr float PI = 3.14159265358979323846f;
    constexpr float EPSILON = 1e-6f;

    // 前置声明
    template<size_t N> struct Vector;
    template<size_t Rows, size_t Cols> struct Matrix;

    // 通用数学函数（全部使用constexpr）
    constexpr float abs(float x) { return x < 0 ? -x : x; }
    constexpr float pow(float base, int exponent) {
        if (exponent == 0) return 1.0f;
        bool negative = exponent < 0;
        exponent = negative ? -exponent : exponent;
        float result = 1.0f;
        while (exponent > 0) {
            if (exponent & 1) result *= base;
            base *= base;
            exponent >>= 1;
        }
        return negative ? 1.0f / result : result;
    }

    constexpr float sqrt(float x) {
        if (x < 0) return 0.0f;
        if (x == 0 || x == 1) return x;
        float guess = x * 0.5f;
        for (int i = 0; i < 32; ++i)
            guess = 0.5f * (guess + x / guess);
        return guess;
    }

    constexpr float nth_root(float x, int n) {
        if (n <= 0) return 0.0f;
        if (n == 1) return x;
        if (x < 0 && n % 2 == 0) return 0.0f;
        float guess = x * 0.5f;
        float inv_n = 1.0f / n;
        float nm1_over_n = (n - 1.0f) * inv_n;
        for (int i = 0; i < 20; ++i) {
            float power_nm1 = pow(guess, n - 1);
            guess = nm1_over_n * guess + inv_n * x / power_nm1;
        }
        return guess;
    }

    constexpr unsigned long long factorial(unsigned n) {
        return (n <= 1) ? 1ULL : n * factorial(n - 1);
    }
}

// 包含核心组件
#include "inc/vector_api.hpp"
#include "inc/matrix_api.hpp"