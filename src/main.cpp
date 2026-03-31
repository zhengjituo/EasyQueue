#include <iostream>
#include "../include/queue_handle_manager.h"
#include <array>
#include <thread>

// 引入模块函数声明
#include "../producer_module/producer.h"
#include "../consumer_module/consumer.h"

int main() {
    // 获取命名队列（跨模块共享）
    QueueHandle handle = QueueHandleManager::ObtainNamedQueue<int, 3>("shared_queue", 5);

    // 启动生产者和消费者线程
    std::thread producer_thread([&]() {
        start_producer(handle);
    });

    std::thread consumer_thread([&]() {
        start_consumer(handle);
    });

    // 等待线程结束
    producer_thread.join();
    consumer_thread.join();

    // 最后释放句柄
    QueueHandleManager::ReleaseQueue(handle);

    return 0;
}