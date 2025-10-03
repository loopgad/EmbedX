// vector.cpp - 向量高效实现，ARM数学库集成
#include "../math_lib.hpp"
#include <cmath>
#include <algorithm>
#include <cassert> // 添加 assert 头文件
#include <type_traits>

#ifdef USE_ARM_MATH
#include "arm_math.h"
#endif

namespace math {

namespace {
    
		// ARM数学库优化实现
		struct ArmOps {
			template<size_t N>
			static float dot(const Vector<N>& a, const Vector<N>& b) {
	#ifdef USE_ARM_MATH
				float result;
				arm_dot_prod_f32(a.data_ptr(), b.data_ptr(), N, &result);
				return result;
	#else
				float sum = 0.0f;
				for(size_t i = 0; i < N; ++i) sum += a[i] * b[i];
				return sum;
	#endif
			}
		};
		
		// 标准实现
		struct StandardOps {
			template<size_t N>
			static float dot(const Vector<N>& a, const Vector<N>& b) {
				float sum = 0.0f;
				for(size_t i = 0; i < N; ++i) sum += a[i] * b[i];
				return sum;
			}
		};
		
		// 编译期分发
		using SelectedOps = std::conditional_t<use_arm_math, ArmOps, StandardOps>;
	}

	// 数学函数实现
	template<size_t N>
	float dot(const Vector<N>& a, const Vector<N>& b) {
		if constexpr (use_arm_math) {
			return ArmOps::dot<N>(a, b);
		} else {
			float sum = 0.0f;
			for(size_t i = 0; i < N; ++i) sum += a[i] * b[i];
			return sum;
		}
	}

	template<size_t N>
	float norm(const Vector<N>& a) {
		return std::sqrt(dot(a, a));
	}

	template<size_t N>
	Vector<N> normalize(const Vector<N>& a) {
		float n = norm(a);
		// 移除异常，使用断言
		assert(n >= 1e-6f && "Cannot normalize zero vector");
		return Vector<N>(a * (1.0f / n));
	}

	// 3D向量叉积特化实现
	template<>
	Vector<3> cross<3>(const Vector<3>& a, const Vector<3>& b) {
		return Vector<3>(
			a[1] * b[2] - a[2] * b[1],
			a[2] * b[0] - a[0] * b[2],
			a[0] * b[1] - a[1] * b[0]
		);
	}

	// 其他维度的叉积使用 static_assert
	template<size_t N>
	Vector<N> cross(const Vector<N>& a, const Vector<N>& b) {
		static_assert(N == 3, "Cross product is only defined for 3D vectors");
		// 删除抛出语句
		return Vector<3>(); // 不会执行，但为了编译
	}

	template<size_t N>
	float distance(const Vector<N>& a, const Vector<N>& b) {
		Vector<N> diff = Vector<N>(a - b);
		return norm(diff);
	}

	// 显式实例化常用维度
	template float dot(const Vector<2>&, const Vector<2>&);
	template float dot(const Vector<3>&, const Vector<3>&);
	template float dot(const Vector<4>&, const Vector<4>&);

	template float norm(const Vector<2>&);
	template float norm(const Vector<3>&);
	template float norm(const Vector<4>&);

	template Vector<2> normalize(const Vector<2>&);
	template Vector<3> normalize(const Vector<3>&);
	template Vector<4> normalize(const Vector<4>&);

	template float distance(const Vector<2>&, const Vector<2>&);
	template float distance(const Vector<3>&, const Vector<3>&);
	template float distance(const Vector<4>&, const Vector<4>&);

	// 显式实例化叉积
	template Vector<3> cross<3>(const Vector<3>&, const Vector<3>&);

} // namespace math