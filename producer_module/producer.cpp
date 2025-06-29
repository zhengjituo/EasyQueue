#include "producer.h"
#include <iostream>
#include <thread>
#include <chrono>

void start_producer(QueueHandle handle) {
    for (int i = 0; i < 5; ++i) {
        std::array<int, 3> item = {i * 10, i * 10 + 1, i * 10 + 2};
        QueueHandleManager::Enqueue<int, 3>(handle, item);
        std::cout << "Producer sent: [" << item[0] << ", " << item[1] << ", " << item[2] << "]\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}