#include "test.h"
#include "bsp_cfg.hpp"
#include "stm32h7xx_hal.h"
uint8_t my_buf[8] = {0};
const uint8_t* temp_const_buf =  usart::UsartAPI::rx_ptr(2);
uint8_t my_buf_[64] = {0};
uint8_t data_length = 64;
uint8_t _my_buf[1] = {0};
uint8_t my_buf2[8] = {0,5,3,5,2,5,6,2};

#include <random>
#include <cmath>
#include <cstdint>

// 全局随机数生成器
static std::mt19937 rng(std::random_device{}());
static std::uniform_real_distribution<float> dist(-100.0f, 100.0f);
	

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

// 性能测试相关的全局变量
uint32_t iterations = 1000000; // 运行次数
volatile uint32_t vector_test_time = 0;
volatile uint32_t matrix_test_time = 0;
volatile uint32_t matrix_vector_mul_test_time = 0;
volatile uint32_t convenience_test_time = 0;
volatile uint32_t total_test_time = 0;

// 测试向量功能（添加迭代参数）
void test_vectors(uint32_t iterations) {
    // 动态生成随机向量
    math::Vector<3> v1{dist(rng), dist(rng), dist(rng)};
    math::Vector<3> v2{dist(rng), dist(rng), dist(rng)};
    
    for(uint32_t i = 0; i < iterations; i++) {
        vector_addition_result = v1 + v2;
        vector_subtraction_result = v2 - v1;
        vector_scalar_mul_result = v1 * 2.0f;
        vector_dot_result = math::dot(v1, v2);
        vector_cross_result = math::cross(v1, v2);
        vector_normalize_result = math::normalize(v1);
        vector_norm_result = math::norm(v1);
    }
}

// 测试矩阵功能（添加迭代参数）
void test_matrices(uint32_t iterations) {
    // 动态生成随机矩阵
    math::Matrix<3, 3> m1{
        dist(rng), dist(rng), dist(rng),
        dist(rng), dist(rng), dist(rng),
        dist(rng), dist(rng), dist(rng)
    };
    
    math::Matrix<3, 3> m2{
        dist(rng), dist(rng), dist(rng),
        dist(rng), dist(rng), dist(rng),
        dist(rng), dist(rng), dist(rng)
    };
    
    math::Matrix<2, 2> m2x2{
        dist(rng), dist(rng),
        dist(rng), dist(rng)
    };

    for(uint32_t i = 0; i < iterations; i++) {
        matrix_addition_result = m1 + m2;
        matrix_multiplication_result = m1 * m2;
        matrix_transpose_result = math::transpose(m1);
        matrix_identity_result = math::identity<3>();
        matrix_determinant_result = math::determinant(m2x2);
        
        if(matrix_determinant_result != 0) {
            matrix_inverse_result = math::inverse(m2x2);
        }
    }
}

// 测试矩阵向量乘法（添加迭代参数）
void test_matrix_vector_multiplication(uint32_t iterations) {
    // 动态生成随机矩阵和向量
	 for(uint32_t i = 0; i < iterations; i++) {
		math::Matrix<3, 3> m{
			dist(rng), dist(rng), dist(rng),
			dist(rng), dist(rng), dist(rng),
			dist(rng), dist(rng), dist(rng)
		};
		
		math::Vector<3> v{dist(rng), dist(rng), dist(rng)};

   
        matrix_vector_mul_result = m * v;
    }
}

// 测试便捷函数和别名（添加迭代参数）
void test_convenience_functions(uint32_t iterations) {
    for(uint32_t i = 0; i < iterations; i++) {
        vec2_result = math::vec2(dist(rng), dist(rng));
        vec3_result = math::vec3(dist(rng), dist(rng), dist(rng));
        vec4_result = math::vec4(dist(rng), dist(rng), dist(rng), dist(rng));
        mat2x2_result = math::mat2x2(
            dist(rng), dist(rng),
            dist(rng), dist(rng)
        );
    }
}
// 运行所有测试（添加迭代参数）
void run_all_tests(uint32_t iterations) {
    uint32_t start, end;
    
    // 测试向量性能
    start = HAL_GetTick();
    test_vectors(iterations);
    end = HAL_GetTick();
    vector_test_time = end - start;
    
    // 测试矩阵性能
    start = HAL_GetTick();
    test_matrices(iterations);
    end = HAL_GetTick();
    matrix_test_time = end - start;
    
    // 测试矩阵向量乘法性能
    start = HAL_GetTick();
    test_matrix_vector_multiplication(iterations);
    end = HAL_GetTick();
    matrix_vector_mul_test_time = end - start;
    
    // 测试便捷函数性能
    start = HAL_GetTick();
    test_convenience_functions(iterations);
    end = HAL_GetTick();
    convenience_test_time = end - start;
    
    // 计算总时间
    total_test_time = vector_test_time + matrix_test_time + 
                     matrix_vector_mul_test_time + convenience_test_time;
}

extern "C" {
void test_fn(void) {
    
    // 运行性能测试
    run_all_tests(iterations);
    

}
}