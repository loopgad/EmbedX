// matrix.cpp - 矩阵高效实现，ARM数学库集成
#include "../math_lib.hpp"
#include <cmath>
#include <cassert>
#include <type_traits>

#ifdef USE_ARM_MATH
#include "arm_math.h"
#endif

namespace math {

// 矩阵-向量乘法  (Rows×Cols) * (Cols×1) -> (Rows×1)
template<size_t Rows, size_t Cols>
Vector<Rows> operator*(const Matrix<Rows,Cols>& mat, const Vector<Cols>& vec)
{
    if constexpr (math::use_arm_math) {
#ifdef USE_ARM_MATH
        /* 把向量包装成Cols×1矩阵 */
        arm_matrix_instance_f32 A, B, C;
        alignas(8) float tmpOut[Rows];          // 存结果
        alignas(8) float tmpIn [Cols];          // 可写副本
        std::copy(vec.data_ptr(), vec.data_ptr()+Cols, tmpIn);

        arm_mat_init_f32(&A, Rows, Cols, const_cast<float*>(mat.data_ptr()));
        arm_mat_init_f32(&B, Cols, 1, tmpIn);
        arm_mat_init_f32(&C, Rows, 1, tmpOut);

        arm_mat_mult_f32(&A, &B, &C);          // CMSIS-DSP 矩阵×矩阵

        Vector<Rows> result;
        std::copy(tmpOut, tmpOut+Rows, result.data_ptr());
        return result;
#else
        /*  Fallback：朴素循环  */
        Vector<Rows> result;
        for (size_t i = 0; i < Rows; ++i) {
            float s = 0.0f;
            for (size_t j = 0; j < Cols; ++j) s += mat(i,j)*vec[j];
            result[i] = s;
        }
        return result;
#endif
    } else {
        /*  非ARM_MATH路径  */
        Vector<Rows> result;
        for (size_t i = 0; i < Rows; ++i) {
            float s = 0.0f;
            for (size_t j = 0; j < Cols; ++j) s += mat(i,j)*vec[j];
            result[i] = s;
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
        static_assert(N == 2 || N == 3, "Determinant only implemented for 2x2 and 3x3 matrices");
        return 0.0f;
    }
}

template<size_t N>
Matrix<N, N> inverse(const Matrix<N, N>& mat) {
    float det = determinant(mat);
    assert(std::abs(det) >= EPSILON && "Matrix is singular");
    
    if constexpr (N == 2) {
        Matrix<2, 2> result;
        float inv_det = 1.0f / det;
        result(0,0) = mat(1,1) * inv_det;
        result(0,1) = -mat(0,1) * inv_det;
        result(1,0) = -mat(1,0) * inv_det;
        result(1,1) = mat(0,0) * inv_det;
        return result;
    } else {
        static_assert(N == 2, "Inverse only implemented for 2x2 matrices");
        return identity<N>();
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