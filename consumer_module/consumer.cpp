#include "consumer.h"
#include <iostream>
#include <thread>

void start_consumer(QueueHandle handle) {
    for (int i = 0; i < 5; ++i) {
        std::array<int, 3> item = QueueHandleManager::Dequeue<int, 3>(handle);
        std::cout << "Consumer received: [" << item[0] << ", " << item[1] << ", " << item[2] << "]\n";
    }
}