#include <iostream>
#include "../include/obtain_release.h"
#include <array>
#include <vector>

int main() {
    // 创建一个容量为 5 的 std::array<int, 3> 类型队列
    QueueHandle handle = SharedQueue<int, 3>::ObtainQueue(5);

    std::cout << "Available: " << SharedQueue<int, 3>::Available(handle) << "\n";

    // 测试批量写入
    std::vector<std::array<int, 3>> items = {
        {10, 20, 30},
        {40, 50, 60},
        {70, 80, 90}
    };

    SharedQueue<int, 3>::BatchEnqueue(handle, items);
    std::cout << "Size after batch enqueue: " << SharedQueue<int, 3>::Size(handle) << "\n";

    // 测试批量读取
    auto batch_result = SharedQueue<int, 3>::BatchDequeue(handle, 2);
    std::cout << "Batch Dequeued: \n";
    for (const auto& arr : batch_result) {
        std::cout << "[";
        for (int val : arr) {
            std::cout << val << ", ";
        }
        std::cout << "], ";
    }
    std::cout << "\n";

    // 再次测试批量写入
    std::vector<std::array<int, 3>> more_items = {
        {100, 200, 300},
        {400, 500, 600}
    };
    SharedQueue<int, 3>::BatchEnqueue(handle, more_items);
    std::cout << "Size after second batch enqueue: " << SharedQueue<int, 3>::Size(handle) << "\n";

    // 读取剩余数据
    while (SharedQueue<int, 3>::Size(handle) > 0) {
        std::array<int, 3> out;
        if (SharedQueue<int, 3>::TryDequeue(handle, out)) {
            std::cout << "Dequeued: [";
            for (int val : out) {
                std::cout << val << ", ";
            }
            std::cout << "]\n";
        }
    }

    // 清理资源
    SharedQueue<int, 3>::ReleaseQueue(handle);

    return 0;
}