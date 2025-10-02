// matrix.cpp - 矩阵高效实现，ARM数学库集成
#include "../math_lib.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <type_traits>

#ifdef USE_ARM_MATH
#include "arm_math.h"
#endif

namespace math {


// 矩阵-向量乘法
template<size_t Rows, size_t Cols>
Vector<Rows> operator*(const Matrix<Rows, Cols>& mat, const Vector<Cols>& vec) {
    // 修复：使用完全限定的math::use_arm_math
    if constexpr (math::use_arm_math) {
#ifdef USE_ARM_MATH
        Vector<Rows> result;
        arm_matrix_instance_f32 matInst;
        arm_mat_init_f32(&matInst, Rows, Cols, const_cast<float*>(mat.data()));
        arm_mat_vec_mult_f32(&matInst, const_cast<float*>(vec.data()), result.data());
        return result;
#else
        Vector<Rows> result;
        for(size_t i = 0; i < Rows; ++i) {
            float sum = 0.0f;
            for(size_t j = 0; j < Cols; ++j) sum += mat(i, j) * vec[j];
            result[i] = sum;
        }
        return result;
#endif
    } else {
        Vector<Rows> result;
        for(size_t i = 0; i < Rows; ++i) {
            float sum = 0.0f;
            for(size_t j = 0; j < Cols; ++j) sum += mat(i, j) * vec[j];
            result[i] = sum;
        }
        return result;
    }
}

// 矩阵数学函数
template<size_t Rows, size_t Cols>
Matrix<Cols, Rows> transpose(const Matrix<Rows, Cols>& mat) {
    Matrix<Cols, Rows> result;
    for(size_t i = 0; i < Rows; ++i) {
        for(size_t j = 0; j < Cols; ++j) result(j, i) = mat(i, j);
    }
    return result;
}

template<size_t N>
float determinant(const Matrix<N, N>& mat) {
    if constexpr (N == 2) {
        return mat(0,0) * mat(1,1) - mat(0,1) * mat(1,0);
    } else if constexpr (N == 3) {
        return mat(0,0) * (mat(1,1)*mat(2,2) - mat(1,2)*mat(2,1)) -
               mat(0,1) * (mat(1,0)*mat(2,2) - mat(1,2)*mat(2,0)) +
               mat(0,2) * (mat(1,0)*mat(2,1) - mat(1,1)*mat(2,0));
    } else {
        throw std::invalid_argument("Determinant only implemented for 2x2 and 3x3 matrices");
    }
}

template<size_t N>
Matrix<N, N> inverse(const Matrix<N, N>& mat) {
    float det = determinant(mat);
    if(std::abs(det) < EPSILON) throw std::invalid_argument("Matrix is singular");
    
    if constexpr (N == 2) {
        Matrix<2, 2> result;
        float inv_det = 1.0f / det;
        result(0,0) = mat(1,1) * inv_det;
        result(0,1) = -mat(0,1) * inv_det;
        result(1,0) = -mat(1,0) * inv_det;
        result(1,1) = mat(0,0) * inv_det;
        return result;
    } else {
        throw std::invalid_argument("Inverse only implemented for 2x2 matrices");
    }
}

template<size_t N>
Matrix<N, N> identity() {
    Matrix<N, N> result;
    for(size_t i = 0; i < N; ++i) {
        for(size_t j = 0; j < N; ++j) result(i, j) = (i == j) ? 1.0f : 0.0f;
    }
    return result;
}

// 显式实例化
template Vector<2> operator*(const Matrix<2,2>&, const Vector<2>&);
template Vector<3> operator*(const Matrix<3,3>&, const Vector<3>&);
template Vector<4> operator*(const Matrix<4,4>&, const Vector<4>&);

template Matrix<2,2> transpose(const Matrix<2,2>&);
template Matrix<3,3> transpose(const Matrix<3,3>&);
template Matrix<4,4> transpose(const Matrix<4,4>&);

template float determinant(const Matrix<2,2>&);
template float determinant(const Matrix<3,3>&);

template Matrix<2,2> inverse(const Matrix<2,2>&);

template Matrix<2,2> identity();
template Matrix<3,3> identity();
template Matrix<4,4> identity();

} // namespace math