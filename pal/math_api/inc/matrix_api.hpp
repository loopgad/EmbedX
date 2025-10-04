// matrix.hpp - 简化矩阵数学接口
#pragma once
#include <cstddef>
#include "../math_lib.hpp"

namespace math {

// 矩阵表达式模板基类
template<typename Expr>
struct MatrixExpr {
    constexpr auto operator()(size_t row, size_t col) const { 
        return static_cast<const Expr&>(*this)(row, col); 
    }
    constexpr size_t rows() const { 
        return static_cast<const Expr&>(*this).rows(); 
    }
    constexpr size_t cols() const { 
        return static_cast<const Expr&>(*this).cols(); 
    }
};

// 主矩阵类
template<size_t Rows, size_t Cols>
struct Matrix : public MatrixExpr<Matrix<Rows, Cols>> {
    float data[Rows * Cols];
    
    constexpr Matrix() {
        for(size_t i = 0; i < Rows * Cols; ++i) data[i] = 0.0f;
    }
    
    template<typename... Args>
    constexpr Matrix(Args... args) : data{static_cast<float>(args)...} {}
    
    // 从表达式构造
    template<typename Expr>
    constexpr Matrix(const MatrixExpr<Expr>& expr) {
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                data[i * Cols + j] = expr(i, j);
            }
        }
    }
    
    constexpr float& operator()(size_t row, size_t col) { 
        return data[row * Cols + col]; 
    }
    
    constexpr const float& operator()(size_t row, size_t col) const { 
        return data[row * Cols + col]; 
    }
    
    constexpr size_t rows() const { return Rows; }
    constexpr size_t cols() const { return Cols; }
    
    // 赋值运算符支持表达式模板
    template<typename Expr>
    constexpr Matrix& operator=(const MatrixExpr<Expr>& expr) {
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                data[i * Cols + j] = expr(i, j);
            }
        }
        return *this;
    }
};

// 矩阵加法表达式模板
template<typename E1, typename E2, size_t Rows, size_t Cols>
struct MatrixAddExpr : public MatrixExpr<MatrixAddExpr<E1, E2, Rows, Cols>> {
    const E1& lhs;
    const E2& rhs;
    
    constexpr MatrixAddExpr(const E1& l, const E2& r) : lhs(l), rhs(r) {}
    
    constexpr auto operator()(size_t row, size_t col) const { 
        return lhs(row, col) + rhs(row, col); 
    }
    
    constexpr size_t rows() const { return Rows; }
    constexpr size_t cols() const { return Cols; }
};

// 矩阵乘法表达式模板
template<typename E1, typename E2, size_t M, size_t N, size_t P>
struct MatrixMulExpr : public MatrixExpr<MatrixMulExpr<E1, E2, M, N, P>> {
    const E1& lhs;
    const E2& rhs;
    
    constexpr MatrixMulExpr(const E1& l, const E2& r) : lhs(l), rhs(r) {}
    
    constexpr auto operator()(size_t row, size_t col) const {
        float sum = 0.0f;
        for (size_t k = 0; k < N; ++k) {
            sum += lhs(row, k) * rhs(k, col);
        }
        return sum;
    }
    
    constexpr size_t rows() const { return M; }
    constexpr size_t cols() const { return P; }
};

// 矩阵向量乘法表达式模板
template<typename MatExpr, typename VecExpr, size_t Rows, size_t Cols>
struct MatrixVectorMulExpr : public math::VectorExpr<MatrixVectorMulExpr<MatExpr, VecExpr, Rows, Cols>> {
    const MatExpr& mat;
    const VecExpr& vec;
    
    constexpr MatrixVectorMulExpr(const MatExpr& m, const VecExpr& v) : mat(m), vec(v) {}
    
    constexpr auto operator[](size_t row) const {
        float sum = 0.0f;
        for (size_t k = 0; k < Cols; ++k) {
            sum += mat(row, k) * vec[k];
        }
        return sum;
    }
    
    constexpr size_t size() const { return Rows; }
};

// 运算符重载
template<size_t Rows, size_t Cols>
constexpr auto operator+(const MatrixExpr<Matrix<Rows, Cols>>& lhs,
                        const MatrixExpr<Matrix<Rows, Cols>>& rhs) {
    return MatrixAddExpr<Matrix<Rows, Cols>, Matrix<Rows, Cols>, Rows, Cols>(
        static_cast<const Matrix<Rows, Cols>&>(lhs),
        static_cast<const Matrix<Rows, Cols>&>(rhs)
    );
}

template<size_t M, size_t N, size_t P>
constexpr auto operator*(const MatrixExpr<Matrix<M, N>>& lhs,
                        const MatrixExpr<Matrix<N, P>>& rhs) {
    return MatrixMulExpr<Matrix<M, N>, Matrix<N, P>, M, N, P>(
        static_cast<const Matrix<M, N>&>(lhs),
        static_cast<const Matrix<N, P>&>(rhs)
    );
}

template<size_t Rows, size_t Cols>
constexpr auto operator*(const MatrixExpr<Matrix<Rows, Cols>>& mat,
                        const math::VectorExpr<math::Vector<Cols>>& vec) {
    return MatrixVectorMulExpr<Matrix<Rows, Cols>, math::Vector<Cols>, Rows, Cols>(
        static_cast<const Matrix<Rows, Cols>&>(mat),
        static_cast<const math::Vector<Cols>&>(vec)
    );
}

// 矩阵数学函数声明
template<size_t Rows, size_t Cols>
Matrix<Cols, Rows> transpose(const Matrix<Rows, Cols>& mat);

template<size_t N>
Matrix<N, N> inverse(const Matrix<N, N>& mat);

template<size_t N>
float determinant(const Matrix<N, N>& mat);

template<size_t N>
Matrix<N, N> identity();

// 便捷类型别名
using Matrix2x2 = Matrix<2, 2>;
using Matrix3x3 = Matrix<3, 3>;
using Matrix4x4 = Matrix<4, 4>;

} // namespace math