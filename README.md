# EmbedX
A better embedded abstraction framework(Embedded Cross-platform), more readable, without sacrificing performance, from GDUT 9th Rcers

# 规范
## 📋 核心原则

1.  **零抽象成本 (Zero-Cost Abstraction)**：命名应清晰反映实体或操作，确保代码意图明确，避免因命名模糊引入理解开销或运行时检查。
2.  **清晰性与一致性 (Clarity & Consistency)**：使用完整的英文单词或广泛认可的缩写，做到“见名知意”。整个项目必须采用统一的命名风格。
3.  **嵌入式特性优先 (Embedded-First)**：命名应体现嵌入式系统的硬件交互特性，并明确区分变量的作用域和存储期。

##  具体命名规则

| 元素类型 | 命名风格 | 规则说明 | 示例 | 嵌入式相关说明 |
| :--- | :--- | :--- | :--- | :--- |
| **函数 / 方法** | **蛇形命名法**<br>(snake_case) | 动词或动词+名词，描述明确操作。 | `initialize_uart()`<br>`read_temperature()` | 函数应短小精悍，功能单一。 |
| **类 / 结构体** | **帕斯卡命名法**<br>(PascalCase) | 名词或形容词+名词，体现实体或概念。 | `class GpioPin`<br>`struct PacketHeader` | 结构体若主要用作数据聚合，可考虑蛇形法，但项目内需统一。 |
| **类成员变量** | **蛇形命名法**<br>+ **下划线后缀** | 明确区分局部变量，提高可读性。 | `private: int counter_;` | 避免使用`m_`前缀，以保持简洁。 |
| **普通变量**<br>(局部、参数) | **蛇形命名法**<br>(snake_case) | 全部小写，下划线分隔。 | `int error_code;`<br>`const uint8_t* data_buffer` | 循环变量可使用`i`, `j`, `k`等单字母名称。 |
| **全局变量** | **`g_`前缀**<br>+ 蛇形命名法 | 显式标识作用域。静态变量同理 | `volatile uint32_t g_system_tick;` | 硬件相关变量必须用`volatile`修饰。 |
| **常量**<br>(`constexpr`, `const`) | **全大写蛇形命名法**<br>(UPPER_SNAKE_CASE) | 包括编译时常量和全局常量。 | `constexpr int MAX_BUFFER_SIZE = 256;` | 替代宏定义的首选方式，类型安全且零开销。 |
| **枚举** | **帕斯卡命名法**<br>(枚举类型名) | 枚举类型名使用帕斯卡法。 | `enum class GpioState { Low, High };` | 强类型枚举（`enum class`）可避免命名污染。 |
| **枚举值** | **全大写蛇形命名法**<br>(UPPER_SNAKE_CASE) | 枚举值使用全大写蛇形法。 | `enum class State { IDLE, RUNNING, ERROR };` | 与常量命名风格保持一致。 |
| **宏** | **全大写蛇形命名法**<br>(UPPER_SNAKE_CASE) | 仅用于条件编译或无法用C++特性替代的场景。 | `#define ASSERT(x) if (!(x)) {}` | 宏名不得以`_`开头或结尾。 |
| **文件** | **蛇形命名法**<br>(snake_case) | 源文件(.cpp)和头文件(.h)均使用小写。 | `bsp_spi.cpp`, `memory_pool.h` | 利于跨平台兼容。 |
| **命名空间** | **蛇形命名法**<br>(snake_case) | 全部小写，基于项目或模块名。 | `namespace hal {}` // 硬件抽象层 | 帮助组织代码，防止命名冲突。 |

