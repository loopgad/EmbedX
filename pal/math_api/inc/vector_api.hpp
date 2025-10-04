// vector.hpp - 简化向量数学接口
#pragma once
#include <cstddef>
#include <type_traits>

namespace math {

// 向量表达式模板基类
template<typename Expr>
struct VectorExpr {
    constexpr auto operator[](size_t i) const { 
        return static_cast<const Expr&>(*this)[i]; 
    }
    constexpr size_t size() const { 
        return static_cast<const Expr&>(*this).size(); 
    }
    
    // 添加转换操作符，允许表达式模板隐式转换为float
    constexpr operator float() const {
        static_assert(Expr::size() == 1, "Only single-element vectors can be converted to float");
        return static_cast<const Expr&>(*this)[0];
    }
};

// 主向量类
template<size_t N>
struct Vector : public VectorExpr<Vector<N>> {
    float data[N];
    
    constexpr Vector() {
        for(size_t i = 0; i < N; ++i) data[i] = 0.0f;
    }
    
    // 使用SFINAE限制参数类型，只允许算术类型
    template<typename... Args, typename = std::enable_if_t<(std::is_arithmetic_v<Args> && ...)>>
    constexpr Vector(Args... args) : data{static_cast<float>(args)...} {}
    
    // 从表达式构造
    template<typename Expr>
    constexpr Vector(const VectorExpr<Expr>& expr) {
        for (size_t i = 0; i < N; ++i) {
            data[i] = expr[i];
        }
    }
    
    constexpr float& operator[](size_t i) { return data[i]; }
    constexpr const float& operator[](size_t i) const { return data[i]; }
    static constexpr size_t size() { return N; }
    
    // 提供数据访问接口
    constexpr float* data_ptr() { return data; }
    constexpr const float* data_ptr() const { return data; }
    
    // 赋值运算符支持表达式模板
    template<typename Expr>
    constexpr Vector& operator=(const VectorExpr<Expr>& expr) {
        for (size_t i = 0; i < N; ++i) {
            data[i] = expr[i];
        }
        return *this;
    }
    
    // 向量运算作为成员函数
    constexpr Vector& operator+=(const Vector& rhs) {
        for(size_t i = 0; i < N; ++i) data[i] += rhs[i];
        return *this;
    }
    
    constexpr Vector& operator-=(const Vector& rhs) {
        for(size_t i = 0; i < N; ++i) data[i] -= rhs[i];
        return *this;
    }
    
    constexpr Vector& operator*=(float scalar) {
        for(size_t i = 0; i < N; ++i) data[i] *= scalar;
        return *this;
    }
    
    constexpr Vector& operator/=(float scalar) {
        for(size_t i = 0; i < N; ++i) data[i] /= scalar;
        return *this;
    }
};

// 向量加法表达式模板
template<typename E1, typename E2>
struct VectorAddExpr : public VectorExpr<VectorAddExpr<E1, E2>> {
    const E1& lhs;
    const E2& rhs;
    
    constexpr VectorAddExpr(const E1& l, const E2& r) : lhs(l), rhs(r) {}
    
    constexpr auto operator[](size_t i) const { return lhs[i] + rhs[i]; }
    constexpr size_t size() const { return lhs.size(); }
};

// 向量减法表达式模板
template<typename E1, typename E2>
struct VectorSubExpr : public VectorExpr<VectorSubExpr<E1, E2>> {
    const E1& lhs;
    const E2& rhs;
    
    constexpr VectorSubExpr(const E1& l, const E2& r) : lhs(l), rhs(r) {}
    
    constexpr auto operator[](size_t i) const { return lhs[i] - rhs[i]; }
    constexpr size_t size() const { return lhs.size(); }
};

// 向量标量乘法表达式模板
template<typename E>
struct VectorScalarMulExpr : public VectorExpr<VectorScalarMulExpr<E>> {
    const E& vec;
    float scalar;
    
    constexpr VectorScalarMulExpr(const E& v, float s) : vec(v), scalar(s) {}
    
    constexpr auto operator[](size_t i) const { return vec[i] * scalar; }
    constexpr size_t size() const { return vec.size(); }
};

// 向量标量除法表达式模板
template<typename E>
struct VectorScalarDivExpr : public VectorExpr<VectorScalarDivExpr<E>> {
    const E& vec;
    float scalar;
    
    constexpr VectorScalarDivExpr(const E& v, float s) : vec(v), scalar(s) {}
    
    constexpr auto operator[](size_t i) const { return vec[i] / scalar; }
    constexpr size_t size() const { return vec.size(); }
};

// 一元负号表达式模板
template<typename E>
struct VectorNegExpr : public VectorExpr<VectorNegExpr<E>> {
    const E& vec;
    
    constexpr VectorNegExpr(const E& v) : vec(v) {}
    
    constexpr auto operator[](size_t i) const { return -vec[i]; }
    constexpr size_t size() const { return vec.size(); }
};

// 运算符重载
template<typename E1, typename E2>
constexpr auto operator+(const VectorExpr<E1>& lhs, const VectorExpr<E2>& rhs) {
    return VectorAddExpr<E1, E2>(
        static_cast<const E1&>(lhs), 
        static_cast<const E2&>(rhs)
    );
}

template<typename E1, typename E2>
constexpr auto operator-(const VectorExpr<E1>& lhs, const VectorExpr<E2>& rhs) {
    return VectorSubExpr<E1, E2>(
        static_cast<const E1&>(lhs), 
        static_cast<const E2&>(rhs)
    );
}

template<typename E>
constexpr auto operator*(const VectorExpr<E>& vec, float scalar) {
    return VectorScalarMulExpr<E>(
        static_cast<const E&>(vec), scalar
    );
}

template<typename E>
constexpr auto operator*(float scalar, const VectorExpr<E>& vec) {
    return vec * scalar;
}

template<typename E>
constexpr auto operator/(const VectorExpr<E>& vec, float scalar) {
    return VectorScalarDivExpr<E>(
        static_cast<const E&>(vec), scalar
    );
}

template<typename E>
constexpr auto operator-(const VectorExpr<E>& vec) {
    return VectorNegExpr<E>(static_cast<const E&>(vec));
}

// 数学函数声明
template<size_t N>
float dot(const Vector<N>& a, const Vector<N>& b);

template<size_t N>
float norm(const Vector<N>& a);

template<size_t N>
Vector<N> normalize(const Vector<N>& a);

template<size_t N>
Vector<N> cross(const Vector<N>& a, const Vector<N>& b);

template<size_t N>
float distance(const Vector<N>& a, const Vector<N>& b);

// 便捷类型别名
using Vector2 = Vector<2>;
using Vector3 = Vector<3>;
using Vector4 = Vector<4>;

// 便捷构造函数
constexpr Vector2 vec2(float x, float y) { return Vector2{x, y}; }
constexpr Vector3 vec3(float x, float y, float z) { return Vector3{x, y, z}; }
constexpr Vector4 vec4(float x, float y, float z, float w) { return Vector4{x, y, z, w}; }

} // namespace math