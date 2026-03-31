#include "include/easy_queue_all_in_one.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <cassert>
#include <atomic>  // 添加atomic头文件

struct TestData {
    int id;
    double value;
    char data[64];
};

int main() {
    std::cout << "🧪 性能对比测试: EasyQueue vs FastQueue" << std::endl;

    // 测试EasyQueue
    std::cout << "\n📝 测试 EasyQueue:" << std::endl;
    {
        EasyQueue<TestData> easyQueue(1000);
        
        // 测试基本功能
        TestData testData{1, 3.14, "Hello"};
        easyQueue.Enqueue(testData);
        
        TestData result = easyQueue.Dequeue();
        assert(result.id == 1);
        assert(result.value == 3.14);
        std::cout << "✅ EasyQueue 基本功能正常" << std::endl;
        
        // 性能测试
        auto start = std::chrono::high_resolution_clock::now();
        
        const int COUNT = 10000;
        for (int i = 0; i < COUNT; ++i) {
            TestData data{i, static_cast<double>(i) * 1.5, "TestData"};
            easyQueue.Enqueue(data);
        }
        
        for (int i = 0; i < COUNT; ++i) {
            TestData data = easyQueue.Dequeue();
            assert(data.id == i);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "✅ EasyQueue 处理 " << COUNT * 2 << " 次操作耗时: " << duration.count() << " 微秒" << std::endl;
    }
    
    // 测试FastQueue
    std::cout << "\n⚡ 测试 FastQueue:" << std::endl;
    {
        FastQueue<128, 10000> fastQueue;  // 最大元素128字节，最多10000个元素
        
        // 测试基本功能
        TestData testData{1, 3.14, "Hello"};
        fastQueue.Enqueue<TestData>(testData);
        
        TestData result;
        bool success = fastQueue.TryDequeue<TestData>(result);
        assert(success);
        assert(result.id == 1);
        assert(result.value == 3.14);
        std::cout << "✅ FastQueue 基本功能正常" << std::endl;
        
        // 性能测试
        auto start = std::chrono::high_resolution_clock::now();
        
        const int COUNT = 10000;
        for (int i = 0; i < COUNT; ++i) {
            TestData data{i, static_cast<double>(i) * 1.5, "TestData"};
            fastQueue.Enqueue<TestData>(data);
        }
        
        for (int i = 0; i < COUNT; ++i) {
            TestData data;
            bool ok = fastQueue.TryDequeue<TestData>(data);
            assert(ok);
            assert(data.id == i);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "✅ FastQueue 处理 " << COUNT * 2 << " 次操作耗时: " << duration.count() << " 微秒" << std::endl;
    }
    
    // 测试批量操作
    std::cout << "\n📦 测试批量操作:" << std::endl;
    {
        // EasyQueue 批量操作
        EasyQueue<int> easyQueue(1000);
        std::vector<int> toEnqueue = {1, 2, 3, 4, 5};
        size_t enqueued = easyQueue.BatchEnqueue(toEnqueue);
        std::cout << "✅ EasyQueue 批量入队 " << enqueued << " 个元素" << std::endl;
        
        std::vector<int> result(5);
        size_t dequeued = easyQueue.BatchDequeue(5, result);
        std::cout << "✅ EasyQueue 批量出队 " << dequeued << " 个元素，首元素: " << result[0] << std::endl;
        
        // FastQueue 批量操作
        FastQueue<sizeof(int), 1000> fastQueue;
        std::vector<int> fastToEnqueue = {10, 20, 30, 40, 50};
        size_t fastEnqueued = fastQueue.BatchEnqueue<int>(fastToEnqueue);
        std::cout << "✅ FastQueue 批量入队 " << fastEnqueued << " 个元素" << std::endl;
        
        std::vector<int> fastResult;
        size_t fastDequeued = fastQueue.BatchDequeue<int>(5, fastResult);
        std::cout << "✅ FastQueue 批量出队 " << fastDequeued << " 个元素，首元素: " << fastResult[0] << std::endl;
    }
    
    // 测试多线程
    std::cout << "\n🧵 测试多线程:" << std::endl;
    {
        // EasyQueue 多线程
        EasyQueue<std::string> easyQueue(100);
        std::atomic<bool> easyFinished{false};
        
        std::thread easyProducer([&easyQueue, &easyFinished]() {
            for(int i = 0; i < 100; ++i) {
                easyQueue.Enqueue("EasyQueue_Msg_" + std::to_string(i));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        std::thread easyConsumer([&easyQueue, &easyFinished]() {
            for(int i = 0; i < 100; ++i) {
                std::string msg = easyQueue.Dequeue();
                // std::cout << "  📥 EasyQueue 消费: " << msg << std::endl;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        easyProducer.join();
        easyConsumer.join();
        std::cout << "✅ EasyQueue 多线程测试完成" << std::endl;
        
        // FastQueue 多线程
        FastQueue<64, 1000> fastQueue;  // 64字节最大，1000个元素
        std::thread fastProducer([&fastQueue]() {
            for(int i = 0; i < 100; ++i) {
                std::string msg = "FastQueue_Msg_" + std::to_string(i);
                char buffer[64] = {};
                strncpy(buffer, msg.c_str(), sizeof(buffer) - 1);
                fastQueue.Enqueue(buffer);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        std::thread fastConsumer([&fastQueue]() {
            for(int i = 0; i < 100; ++i) {
                char buffer[64] = {};
                bool ok = fastQueue.TryDequeue(buffer);
                // if(ok) std::cout << "  📥 FastQueue 消费: " << buffer << std::endl;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        fastProducer.join();
        fastConsumer.join();
        std::cout << "✅ FastQueue 多线程测试完成" << std::endl;
    }
    
    std::cout << "\n🎉 所有性能对比测试完成!" << std::endl;
    return 0;
}