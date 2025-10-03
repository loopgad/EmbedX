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
        for (int i = 0; i < 32; ++i) {  // 迭代32次获得较好精度
            guess = 0.5f * (guess + x / guess);
        }
        return guess;
    }
    
    constexpr float nth_root(float x, int n) {
        if (n <= 0) return 0.0f;
        if (n == 1) return x;
        if (x < 0 && n % 2 == 0) return 0.0f; // 负数不能开偶次方根
        
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

// 便捷别名 - 提高可读性
using Vector2 = math::Vector<2>;
using Vector3 = math::Vector<3>;
using Vector4 = math::Vector<4>;

using Matrix2x2 = math::Matrix<2, 2>;
using Matrix3x3 = math::Matrix<3, 3>;
using Matrix4x4 = math::Matrix<4, 4>;

// 便捷构造函数
inline Vector2 vec2(float x, float y) { return Vector2{x, y}; }
inline Vector3 vec3(float x, float y, float z) { return Vector3{x, y, z}; }
inline Vector4 vec4(float x, float y, float z, float w) { return Vector4{x, y, z, w}; }

inline Matrix2x2 mat2x2(float m00, float m01, float m10, float m11) {
    return Matrix2x2{m00, m01, m10, m11};
}

inline Matrix3x3 mat3x3(float m00, float m01, float m02,
                       float m10, float m11, float m12,
                       float m20, float m21, float m22) {
    return Matrix3x3{m00, m01, m02, m10, m11, m12, m20, m21, m22};
}