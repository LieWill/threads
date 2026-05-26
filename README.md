# Modern C++ Concurrency & Parallelism Workspace
### 多核并发与并行计算实践工作空间

这是一个基于 **C++17** 的多核并发编程与高性能并行计算实践工作空间。整个项目围绕“高性能、低锁竞争、强异常安全与优雅停启”的并发准则展开。

核心工程位于主目录下的 `parallel-algorithms/` 目录中。为了提供极佳的开箱即用体验，工程根目录下已配置好**全局 Master Makefile** 与**全局 Git 过滤规则**，支持直接从根目录对子模块进行一键式编译、清理、测试与性能跑分。

---

## 📂 空间结构布局

```
threads/ (Workspace Root)
├── parallel-algorithms/        # 核心子项目 (并行算法与并发同步库)
│   ├── include/parallel/       # 并行库核心头文件目录 (工作窃取线程池/有界队列/中断标志等)
│   ├── src/                    # 线程池工作逻辑源文件目录
│   ├── tests/                  # 包含 60 个高强度的正确性与安全单元测试
│   └── benchmarks/             # 算法加速比与有界队列吞吐率性能跑分基准
├── .gitignore                  # 全局 Git 忽略配置 (过滤编译产物、IDE配置、草稿与论文)
├── Makefile                    # 全局 Master Makefile (一键向下分发编译和测试指令)
├── README.md                   # 本主空间说明文档
└── thread_pool.h / .cpp        # 根目录的临时模板备份 (核心实现见 parallel-algorithms 目录)
```

---

## ⚡ 核心组件概览

整个子项目 `parallel-algorithms/` 包含以下四大并发支柱模块：

1. **工作窃取线程池 (`thread_pool`)**：
   - 采用私有任务双端队列限制锁碰撞，执行 LIFO 本地出队以保障缓存局部性。
   - 实现低锁冲突的 FIFO 窃取算法，平衡多核负载。
   - 协作式非阻塞挂起机制 (`run_pending_task`)，在等待期间推进池内其他就绪任务以消除死锁。
2. **自适应并行算法 (`parallel_algorithms`)**：
   - `parallel_for_each` 与 `parallel_accumulate`：采用“处理前划分”的自适应分块，最小化入队摩擦。
   - `parallel_quick_sort`：采用递归分治动态分发，配备深度与规模阈值退化控制，杜绝微小任务过载。
3. **有界双实现缓冲区 (`bounded_buffer`)**：
   - 锁版本 (`bounded_buffer_lock`)：Mutex 结合双条件变量谓词等待，杜绝虚假唤醒。
   - 原子无锁版 (`bounded_buffer_atomic`)：基于原子序列号环形数组与 relaxed 抢占位置指针实现，性能较锁版提升达两个数量级（高频竞争下跑出 **38.37 Mops/s** 超强吞吐）。
4. **中断控制机制 (`interrupt_flag`)**：
   - 支持多线程下的 `std::packaged_task` 异常自动捕获与 `std::future::get` 主线程跨线程透明重抛。
   - RAII 机制平稳 join 回收。
   - 队列接口提供超时协作式轮询中断检测，保证所有线程在 **100ms** 内平稳退出。

---

## 🛠️ 全局一键式构建与运行

在工作空间根目录下（即当前目录），可以直接使用 `make` 指令一键下发任务到子项目：

### 1. 编译构建
```bash
make clean && make all   # 清理旧编译产物，并一键编译静态库、测试集和基准跑分
make lib                 # 仅编译子项目并发静态库 (libparallel.a)
make tests               # 仅编译单元测试程序
make benchmarks          # 仅编译基准跑分程序
```

### 2. 运行 60 个单元测试
```bash
make run_tests           # 自动编译并运行 60 个高强度单元测试用例，验证并发安全性
```

### 3. 运行性能跑分基准
```bash
make run_benchmarks      # 自动编译并运行算法加速比、不同容量与线程配比下的队列吞吐跑分
```

### 4. 彻底清理
```bash
make clean               # 清除所有子目录下的对象文件 (.o)、静态库与编译出的二进制程序
```

---

## 🔍 子项目文档入口

- 有关底层并发原语的使用示例、异常传播、中断控制、详细跑分对照以及各系统依赖，请参阅：  
  👉 [**parallel-algorithms/README.md**](./parallel-algorithms/README.md)
- 有关 Happens-Before 锁链、Release Sequence 释放序列语义、CAS 内存序选择、ABA 防护原理、Amdahl 定律理论计算与伪共享消除策略等深层学术分析，请参阅工程目录下的学术论文实践报告（已被 Git 排除在代码库之外）：  
  👉 [**parallel-algorithms/论文-基于线程池的C++并行算法库设计与实现.md**](./parallel-algorithms/论文-基于线程池的C++并行算法库设计与实现.md)

