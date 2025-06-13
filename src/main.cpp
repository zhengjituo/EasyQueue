#include <iostream>
#include "../include/obtain_release.h"
#include <array>

int main() {
    // 创建一个容量为 5 的 std::array<int, 3> 类型队列
    QueueHandle handle = SharedQueue<int, 3>::ObtainQueue(5);

    std::cout << "Available: " << SharedQueue<int, 3>::Available(handle) << "\n";

    // 写入数据
    for (int i = 0; i < 3; ++i) {
        std::array<int, 3> arr = {{i, i+1, i+2}};
        SharedQueue<int, 3>::Enqueue(handle, arr);
    }

    std::cout << "Size after enqueue: " << SharedQueue<int, 3>::Size(handle) << "\n";

    // 读取数据
    for (int i = 0; i < 3; ++i) {
        auto value = SharedQueue<int, 3>::Dequeue(handle);
        std::cout << "Dequeued: [" << value[0] << ", " << value[1] << ", " << value[2] << "]\n";
    }

    std::cout << "Size after dequeue: " << SharedQueue<int, 3>::Size(handle) << "\n";

    // 尝试 Lossy 入队
    for (int i = 100; i < 105; ++i) {
        std::array<int, 3> arr = {{i, i+1, i+2}};
        SharedQueue<int, 3>::TryEnqueue(handle, arr);
    }

    auto flushed = SharedQueue<int, 3>::Flush(handle);
    std::cout << "Flushed data count: " << flushed.size() << "\n";

    // 释放队列
    SharedQueue<int, 3>::ReleaseQueue(handle);

    return 0;
}