# High-Performance Concurrency & Parallel Algorithms Library
### 基于工作窃取与无锁环形队列的高性能 C++ 并发与并行算法库

本库是一套基于现代 **C++17** 标准设计并实现的并发算法与同步库。系统以**工作窃取 (Work-Stealing) 线程池**为核心引擎，消除了全局任务队列的并发竞争瓶颈，并实现了非阻塞的任务协同推进机制；在此基础上，提供了 `parallel_for_each`、`parallel_accumulate`、`parallel_quick_sort` 等高性能并行算法。

为了满足高并发通信与同步需求，本项目还实现并封装了**多生产者-多消费者 (MPMC) 有界缓冲区**的双重架构（基于 Mutex/CV 的背压版本与基于原子序列号的无锁环形队列版本），并提供了在 100ms 内快速响应的线程中断标志位机制 (`interrupt_flag`)。

---

## 🌟 核心特性

- **工作窃取线程池 (`thread_pool`)**：
  - **分层双端队列**：全局任务队列与每线程私有的任务双端队列相结合。
  - **低冲突窃取**：本线程从本地队头 LIFO 获取任务以保护局部性，空闲线程从其他线程队尾 FIFO 窃取任务以降低摩擦。
  - **协作式推进 (`run_pending_task`)**：等待 `future` 就绪期间主动推进池内其他就绪任务，绝不空转挂起，消除死锁隐患。
- **高性能并行算法 (`parallel_algorithms`)**：
  - **自适应划分**：支持“处理前划分” (for_each / accumulate) 与“递归分治” (quick_sort)。
  - **深度与阈值控制**：`parallel_quick_sort` 引入递归深度和数据规模阈值限制，防止小任务过载，自动退化为极速串行排序。
- **有界双实现缓冲区 (`bounded_buffer`)**：
  - **锁版本 (`bounded_buffer_lock`)**：使用互斥锁与双条件变量实现，严格执行谓词等待以防御虚假唤醒。
  - **原子无锁版本 (`bounded_buffer_atomic`)**：基于单调递增的原子索引与序列号环形数组实现，在结构上免疫 ABA 并发缺陷，吞吐量提升高达两个数量级。
- **优雅停启与中断机制 (`interrupt_flag`)**：
  - 支持 `packaged_task` / `future` 的多线程异常捕获与主线程透明重新抛出。
  - 支持基于 RAII 的 `join_threads` 汇合回收。
  - 重载支持中断的 `push/pop` 接口，保证阻塞线程在 100ms 内平稳退出。

---

## 📂 项目结构

```
parallel-algorithms/
├── include/parallel/          # 头文件目录
│   ├── thread_pool.h          # 工作窃取线程池声明与submit实现
│   ├── parallel_algorithms.h  # 并行算法（for_each, accumulate, quick_sort, partial_sum）
│   ├── bounded_buffer.h       # MPMC有界缓冲区（锁版/原子无锁环形版）
│   ├── interrupt.h            # 线程中断同步标志位 (interrupt_flag)
│   ├── join_threads.h         # RAII 线程平稳汇合回收工具
│   └── utils.h                # 高精度计时器与基准测试工具函数
├── src/                       # 源文件目录
│   ├── Makefile
│   └── thread_pool.cpp        # 线程池工作逻辑与窃取算法实现
├── tests/                     # 单元测试 (包含60个高强度正确性断言)
│   ├── Makefile
│   ├── test_assert.h          # 单元测试轻量级框架
│   ├── test_main.cpp
│   ├── test_thread_pool.cpp
│   ├── test_for_each.cpp
│   ├── test_accumulate.cpp
│   ├── test_quick_sort.cpp
│   ├── test_exception_safety.cpp
│   └── test_bounded_buffer.cpp # 锁版/原子无锁版MPMC有界缓冲区与中断机制测试
├── benchmarks/                # 性能基准测试
│   ├── Makefile
│   ├── benchmark_main.cpp     # 跑分主入口
│   ├── benchmark_for_each.cpp
│   ├── benchmark_accumulate.cpp
│   ├── benchmark_quick_sort.cpp
│   ├── benchmark_thread_count.cpp
│   └── benchmark_bounded_buffer.cpp # 有界缓冲区吞吐率及容量/配比性能基准
└── Makefile                   # 全局编译构建脚本
```

---

## 🛠️ 快速开始

### 1. 编译构建

在 `parallel-algorithms` 目录下运行 `make` 工具链：

```bash
make clean && make all   # 完整清理并重新编译库、测试和基准测试
make lib                 # 仅编译静态库 (libparallel.a)
make tests               # 仅编译单元测试
make benchmarks          # 仅编译基准测试
```

### 2. 运行单元测试

单元测试包含了 **60** 个严谨的断言用例，覆盖并发安全性、边界条件、异常流转、MPMC 并发与中断时效：

```bash
make run_tests
# 或者直接运行可执行文件
./build/bin/tests/test_runner
```

### 3. 运行跑分基准

基准测试将自动探测 CPU 核心数，并对并行算法和有界缓冲区进行多维度的吞吐率与加速比跑分：

```bash
make run_benchmarks
# 或者直接运行可执行文件
./build/bin/benchmarks/benchmark_runner
```

---

## 💻 使用示例

### 1. 任务窃取线程池与异常传播
```cpp
#include "parallel/thread_pool.h"
#include <iostream>

int main() {
    // 创建契合当前 CPU 核心数的线程池
    parallel::thread_pool pool; 

    // 提交带返回值的任务
    auto future = pool.submit([](int x, int y) {
        if (y == 0) throw std::runtime_error("Division by zero!");
        return x / y;
    }, 10, 2);

    try {
        // 主线程在推进其他任务的同时，安全获取结果。如有异常，会在当前线程重新抛出
        int result = parallel::wait_for_future(future, pool);
        std::cout << "Result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Caught Exception: " << e.what() << std::endl;
    }
    
    return 0;
}
```

### 2. 高性能并行算法
```cpp
#include "parallel/parallel_algorithms.h"
#include <vector>
#include <numeric>

void run_algorithms() {
    parallel::thread_pool pool;
    std::vector<int> data(1000000);
    std::iota(data.begin(), data.end(), 1);

    // 1. 并行 for_each
    parallel::parallel_for_each(data.begin(), data.end(), [](int& x) {
        x *= 2;
    }, pool);

    // 2. 并行 accumulate (自适应分块)
    auto sum = parallel::parallel_accumulate(data.begin(), data.end(), 0LL, pool);

    // 3. 并行 quick_sort (分治递归划分+退化控制)
    parallel::parallel_quick_sort(data.begin(), data.end(), pool);
}
```

### 3. MPMC 有界缓冲区与中断响应
```cpp
#include "parallel/bounded_buffer.h"
#include "parallel/interrupt.h"
#include <thread>
#include <iostream>

void mpmc_demo() {
    // 实例化一个容量为 64 的无锁环形有界队列
    parallel::bounded_buffer_atomic<int> buffer(64);
    parallel::interrupt_flag flag;

    // 消费者线程
    std::thread consumer([&]() {
        int val;
        // 使用支持中断的接口，一旦 flag 触发，线程将在 100ms 内跳出阻塞并平稳退出
        while (buffer.pop(val, flag)) {
            std::cout << "Consumed: " << val << std::endl;
        }
        std::cout << "Consumer thread cleanly exited." << std::endl;
    });

    // 生产数据
    buffer.push(42);
    buffer.push(100);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 主动触发中断，终止所有阻塞在 pop/push 的工作线程
    flag.interrupt();
    consumer.join();
}
```

---

## 📈 性能跑分指标

以下数据运行于 AMD Ryzen 7 5800X (8核/16线程)，Ubuntu 22.04 LTS，GCC 11.4.0。

### 1. 并行算法加速比对照 ($10^8$ 规模数据，16个线程)

| 算法类型 | 数据规模 | 串行耗时 (ms) | 并行耗时 (ms) | 实测加速比 | 瓶颈分析 |
|---|---|---|---|---|---|
| **parallel_for_each** | $10^8$ | 135.47 ms | 54.02 ms | **2.507x** | 内存带宽瓶颈 (Memory Bound) |
| **parallel_quick_sort**| $10^7$ | 539.06 ms | 360.52 ms | **1.495x** | 递归分治优秀调度效果 |
| **parallel_accumulate**| $10^8$ | 32.35 ms | 58.73 ms | 0.551x | 内存墙拥堵与极低计算密度 |

### 2. 有界缓冲区吞吐率对照 (锁版 vs 原子无锁版，单位：Mops/s)

| 队列容量 | P:C 线程配比 | 锁版本吞吐率 (`lock`) | 原子无锁版吞吐率 (`atomic`) | 性能倍数提升 |
|---|---|---|---|---|
| **8 (极小容量)** | 1:1 | 0.12 Mops/s | 7.74 Mops/s | **64.5x** |
| | 2:1 | 0.05 Mops/s | 7.75 Mops/s | **155.0x** |
| **64 (小容量)** | 1:1 | 1.03 Mops/s | 38.37 Mops/s | **37.2x** |
| | 2:1 | 0.22 Mops/s | 18.21 Mops/s | **82.7x** |
| **1024 (大容量)** | 1:1 | 13.23 Mops/s | 34.62 Mops/s | **2.62x** |
| | 2:1 | 1.91 Mops/s | 17.12 Mops/s | **8.96x** |

> **数据洞察**：
> 1. 在极小容量和高不对称竞争（2:1）场景下，锁版受困于内核级条件变量挂起与上下文切换，吞吐量跌落。原子版利用 CAS 轮询及分级退避自旋在用户态，跑出了 **155倍** 的超强吞吐率。
> 2. 随着容量的增大，锁竞争与空/满背压概率降低，锁版吞吐量大幅回升，但原子版依然在所有维度下对锁版形成全面碾压。

---

## 💻 系统与编译依赖

- **操作系统**：Linux (Ubuntu 20.04+ 推荐) 或 macOS
- **编译器**：支持 C++17 的 GCC (g++ 9+) 或 Clang (clang++ 10+)
- **系统库**：POSIX Threads (`-lpthread`)
- **构建工具**：GNU Make

---