#pragma once
#include <vector>
#include <thread>

class ThradPool
{
public:
    ThradPool();
    ~ThradPool();
private:
    // 线程池相关成员变量和方法
    int m_thread_count;
    std::vector<std::jthread> m_threads;
};
