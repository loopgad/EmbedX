#include "test.h"
#include "bsp_cfg.hpp"
uint8_t my_buf[8] = {0};
const uint8_t* temp_const_buf =  usart::UsartAPI::rx_ptr(2);
uint8_t my_buf_[64] = {0};
uint8_t data_length = 64;
uint8_t _my_buf[1] = {0};
uint8_t my_buf2[8] = {0,5,3,5,2,5,6,2};

    // 向量测试结果
    math::Vector<3> vector_addition_result;
    math::Vector<3> vector_subtraction_result;
    math::Vector<3> vector_scalar_mul_result;
    float vector_dot_result;
    math::Vector<3> vector_cross_result;
    math::Vector<3> vector_normalize_result;
    float vector_norm_result;
    
    // 矩阵测试结果
    math::Matrix<3, 3> matrix_addition_result;
    math::Matrix<3, 3> matrix_multiplication_result;
    math::Matrix<3, 3> matrix_transpose_result;
    math::Matrix<3, 3> matrix_identity_result;
    float matrix_determinant_result;
    math::Matrix<2, 2> matrix_inverse_result;
    
    // 矩阵向量乘法测试结果
    math::Vector<3> matrix_vector_mul_result;
    
    // 便捷函数测试结果
    math::Vector<2> vec2_result;
    math::Vector<3> vec3_result;
    math::Vector<4> vec4_result;
    math::Matrix<2, 2> mat2x2_result;
    
    // 编译期数学函数测试结果
    constexpr float pi = math::PI;
    constexpr float sqrt2 = math::sqrt(2.0f);
    constexpr float pow3 = math::pow(2.0f, 3);
    constexpr unsigned long long fact5 = math::factorial(5);
    
    // 测试状态
    bool all_tests_passed = true;
	
extern "C" {
// 全局变量用于存储测试结果




// 测试向量功能
void test_vectors() {

    
    // 创建向量
    math::Vector<3> v1(1.0f, 2.0f, 3.0f);
    math::Vector<3> v2(4.0f, 5.0f, 6.0f);
    
    // 向量加法
    vector_addition_result = v1 + v2;
    
    // 向量减法
    vector_subtraction_result = v2 - v1;
    
    // 标量乘法
    vector_scalar_mul_result = v1 * 2.0f;
    
    // 点积
    vector_dot_result = math::dot(v1, v2);
    
    // 叉积 (仅适用于3D向量)
    vector_cross_result = math::cross(v1, v2);
    
    // 向量归一化
    vector_normalize_result = math::normalize(v1);
    
    // 向量长度
    vector_norm_result = math::norm(v1);
}

// 测试矩阵功能
void test_matrices() {
    
    // 创建矩阵
    math::Matrix<3, 3> m1(1.0f, 2.0f, 3.0f,
                          4.0f, 5.0f, 6.0f,
                          7.0f, 8.0f, 9.0f);
    
    math::Matrix<3, 3> m2(9.0f, 8.0f, 7.0f,
                          6.0f, 5.0f, 4.0f,
                          3.0f, 2.0f, 1.0f);
    
    // 矩阵加法
    matrix_addition_result = m1 + m2;
    
    // 矩阵乘法
    matrix_multiplication_result = m1 * m2;
    
    // 矩阵转置
    matrix_transpose_result = math::transpose(m1);
    
    // 单位矩阵
    matrix_identity_result = math::identity<3>();
    
    // 2x2矩阵的行列式和逆矩阵
    math::Matrix<2, 2> m2x2(4.0f, 7.0f,
                            2.0f, 6.0f);
    
    matrix_determinant_result = math::determinant(m2x2);
    
    if(matrix_determinant_result != 0) {
        matrix_inverse_result = math::inverse(m2x2);
    }
}

// 测试矩阵向量乘法
void test_matrix_vector_multiplication() {

    // 创建矩阵和向量
    math::Matrix<3, 3> m(1.0f, 2.0f, 3.0f,
                         4.0f, 5.0f, 6.0f,
                         7.0f, 8.0f, 9.0f);
    
    math::Vector<3> v(2.0f, 1.0f, 3.0f);
    
    // 矩阵乘以向量
    matrix_vector_mul_result = m * v;
}

// 测试便捷函数和别名
void test_convenience_functions() {

    // 使用便捷函数创建向量
    vec2_result = math::vec2(1.0f, 2.0f);
    vec3_result = math::vec3(1.0f, 2.0f, 3.0f);
    vec4_result = math::vec4(1.0f, 2.0f, 3.0f, 4.0f);
    
    // 使用便捷函数创建矩阵
    mat2x2_result = mat2x2(1.0f, 2.0f, 3.0f, 4.0f);
}

// 运行所有测试
void run_all_tests() {
    test_vectors();
    test_matrices();
    test_matrix_vector_multiplication();
    test_convenience_functions();
    
    // 编译期数学函数已经在全局变量初始化时计算
}
    void test_fn(void){
	run_all_tests();

    }
}