#include "include/easy_queue_all_in_one.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <vector>
#include <random>
#include <cstring>

// 生成模拟数据的辅助函数
void generateData(std::vector<int16_t>& data, int frameNum) {
    std::iota(data.begin(), data.end(), static_cast<int16_t>(frameNum * 1000));  // 填充递增数据
}

int main() {
    std::cout << "🧪 开始测试EasyQueue的所有功能..." << std::endl;

    // 1. 基本功能测试
    std::cout << "\n📝 基本入队出队测试:" << std::endl;
    EasyQueue<int> queue(5);
    
    for(int i = 1; i <= 5; ++i) {
        queue.Enqueue(i * 10);
        std::cout << "入队: " << i * 10 << std::endl;
    }
    
    for(int i = 0; i < 5; ++i) {
        int val = queue.Dequeue();
        std::cout << "出队: " << val << std::endl;
        assert(val == (i+1) * 10);
    }
    
    std::cout << "✅ 基本功能测试通过" << std::endl;

    // 2. 尝试入队/出队测试
    std::cout << "\n🔒 尝试入队/出队测试:" << std::endl;
    EasyQueue<int> tryQueue(3);
    
    // 填满队列
    for(int i = 1; i <= 3; ++i) {
        bool success = tryQueue.TryEnqueue(i * 10);
        std::cout << "尝试入队 " << i * 10 << ": " << (success ? "成功" : "失败") << std::endl;
        assert(success);
    }
    
    // 再尝试入队应该失败
    bool fullResult = tryQueue.TryEnqueue(40);
    std::cout << "队列满时尝试入队: " << (fullResult ? "成功" : "失败") << std::endl;
    assert(!fullResult);
    
    // 出队
    for(int i = 0; i < 3; ++i) {
        int val;
        bool success = tryQueue.TryDequeue(val);
        std::cout << "尝试出队: " << (success ? std::to_string(val) : "失败") << std::endl;
        assert(success && val == (i+1) * 10);
    }
    
    // 再尝试出队应该失败
    int dummy;
    bool emptyResult = tryQueue.TryDequeue(dummy);
    std::cout << "队列空时尝试出队: " << (emptyResult ? "成功" : "失败") << std::endl;
    assert(!emptyResult);
    
    std::cout << "✅ 尝试入队/出队测试通过" << std::endl;

    // 3. 批量操作测试
    std::cout << "\n📦 批量操作测试:" << std::endl;
    EasyQueue<int> batchQueue(10);
    
    std::vector<int> toEnqueue = {100, 200, 300, 400, 500};
    size_t enqueuedCount = batchQueue.BatchEnqueue(toEnqueue);  // 使用BatchEnqueue
    std::cout << "批量入队 " << enqueuedCount << " 个元素" << std::endl;
    
    std::vector<int> batchResults(5);
    size_t dequeuedCount = batchQueue.BatchDequeue(5, batchResults);
    std::cout << "批量出队 " << dequeuedCount << " 个元素，首元素: " << batchResults[0] << std::endl;
    
    assert(enqueuedCount == 5);
    assert(dequeuedCount == 5);
    assert(batchResults[0] == 100);
    assert(batchResults[4] == 500);
    
    std::cout << "✅ 批量操作测试通过" << std::endl;

    // 4. 多线程测试
    std::cout << "\n🧵 多线程测试:" << std::endl;
    EasyQueue<std::string> threadQueue(5);
    volatile bool finished = false;
    
    std::thread producer([&threadQueue, &finished]() {
        for(int i = 0; i < 5; ++i) {
            std::string msg = "消息_" + std::to_string(i);
            threadQueue.Enqueue(msg);
            std::cout << "  📤 生产者: 发送 " << msg << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    std::thread consumer([&threadQueue, &finished]() {
        for(int i = 0; i < 5; ++i) {
            std::string msg = threadQueue.Dequeue();
            std::cout << "  📥 消费者: 接收 " << msg << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    });
    
    producer.join();
    consumer.join();
    std::cout << "✅ 多线程测试通过" << std::endl;

    // 5. 不等长数组测试
    std::cout << "\n📐 不等长数组测试:" << std::endl;
    EasyQueue<std::vector<uint8_t>> arrayQueue(5);
    
    std::vector<uint8_t> array1 = {1, 2, 3};
    std::vector<uint8_t> array2 = {4, 5, 6, 7, 8};
    std::vector<uint8_t> array3 = {9, 10};
    
    std::vector<std::vector<uint8_t>> arraysToEnqueue = {array1, array2, array3};
    size_t arrayEnqueuedCount = arrayQueue.BatchEnqueue(arraysToEnqueue);  // 使用BatchEnqueue
    std::cout << "批量入队 " << arrayEnqueuedCount << " 个不等长数组，长度分别为: " 
              << array1.size() << ", " << array2.size() << ", " << array3.size() << std::endl;
    
    std::vector<std::vector<uint8_t>> arraysDequeued(3);
    size_t arrayDequeuedCount = arrayQueue.BatchDequeue(3, arraysDequeued);
    std::cout << "批量出队 " << arrayDequeuedCount << " 个数组，首数组首元素: " << (int)arraysDequeued[0][0] << std::endl;
    
    assert(arrayEnqueuedCount == 3);
    assert(arrayDequeuedCount == 3);
    assert(arraysDequeued[0].size() == 3);
    assert(arraysDequeued[1].size() == 5);
    assert(arraysDequeued[2].size() == 2);
    assert(arraysDequeued[0][0] == 1);
    assert(arraysDequeued[1][0] == 4);
    assert(arraysDequeued[2][0] == 9);
    
    std::cout << "✅ 不等长数组测试通过" << std::endl;

    // 6. 数据采集卡模拟测试
    std::cout << "\n🔬 模拟采集卡数据采集与批量处理:" << std::endl;
    EasyQueue<std::vector<int16_t>, 10000> dataCardQueue(100);
    
    for (int frame = 0; frame < 3; ++frame) {
        std::vector<int16_t> dataFrame(10000);
        generateData(dataFrame, frame);
        bool success = dataCardQueue.TryEnqueue(dataFrame);
        assert(success);
        std::cout << "✅ 采集卡帧 " << frame << " 入队成功，数据大小：" << dataFrame.size() << " 点" << std::endl;
    }
    
    // 批量出队处理
    std::vector<std::vector<int16_t>> dataCardBatchResults(3);  // 修正变量名避免冲突
    size_t dequeued_count = dataCardQueue.BatchDequeue(3, dataCardBatchResults);
    assert(dequeued_count == 3);
    std::cout << "✅ 批量出队 " << dequeued_count << " 帧，首帧数据[0]=" << dataCardBatchResults[0][0] << std::endl;

    // 7. DMA场景模拟测试
    std::cout << "\n📡 模拟DMA数据传输场景:" << std::endl;
    EasyQueue<std::vector<int16_t>> dmaQueue(10);
    
    // 检查初始状态
    std::cout << "📊 初始队列状态 - 大小: " << dmaQueue.Size() << ", 容量: " << dmaQueue.Capacity() 
              << ", 空: " << (dmaQueue.IsEmpty() ? "是" : "否") 
              << ", 满: " << (dmaQueue.IsFull() ? "是" : "否") << std::endl;
    
    // 模拟DMA传输数据
    std::vector<std::vector<int16_t>> dmaFrames;
    for (int frame = 0; frame < 3; ++frame) {
        std::vector<int16_t> dmaFrame(10000);
        generateData(dmaFrame, frame + 10);  // 使用不同的基础值
        dmaFrames.push_back(dmaFrame);
    }
    
    // 批量入队
    size_t dmaEnqueuedCount = dmaQueue.BatchEnqueue(dmaFrames);
    std::cout << "✅ DMA批量入队 " << dmaEnqueuedCount << " 帧，当前队列大小: " << dmaQueue.Size() << std::endl;
    
    // 批量出队处理前先检查队列状态
    std::cout << "📊 批量出队前队列状态 - 大小: " << dmaQueue.Size() << ", 容量: " << dmaQueue.Capacity() 
              << ", 空: " << (dmaQueue.IsEmpty() ? "是" : "否") 
              << ", 满: " << (dmaQueue.IsFull() ? "是" : "否") << std::endl;
    
    // 批量出队处理
    std::vector<std::vector<int16_t>> dmaBatchResults(3);
    size_t dmaDequeuedCount = dmaQueue.BatchDequeue(3, dmaBatchResults);
    assert(dmaDequeuedCount == 3);
    std::cout << "✅ DMA批量出队 " << dmaDequeuedCount << " 帧，首帧数据[0]=" << dmaBatchResults[0][0] << std::endl;
    
    // 检查出队后状态
    std::cout << "📊 DMA批量出队后队列状态 - 大小: " << dmaQueue.Size() << ", 空: " 
              << (dmaQueue.IsEmpty() ? "是" : "否") << std::endl;

    // 8. 新的采集卡模拟测试（使用新的API）
    std::cout << "\n🔬 模拟采集卡数据采集与批量处理 (新API):" << std::endl;
    EasyQueue<std::vector<int16_t>> newDataCardQueue(10);
    
    // 检查初始状态
    std::cout << "📊 初始队列状态 - 大小: " << newDataCardQueue.Size() << ", 容量: " << newDataCardQueue.Capacity() 
              << ", 空: " << (newDataCardQueue.IsEmpty() ? "是" : "否") 
              << ", 满: " << (newDataCardQueue.IsFull() ? "是" : "否") << std::endl;
    
    // 批量入队
    std::vector<std::vector<int16_t>> dataCardFrames;
    for (int frame = 0; frame < 3; ++frame) {
        std::vector<int16_t> dataFrame(10000);
        generateData(dataFrame, frame);
        dataCardFrames.push_back(dataFrame);
    }
    
    size_t newEnqueuedCount = newDataCardQueue.BatchEnqueue(dataCardFrames);
    std::cout << "✅ 批量入队 " << newEnqueuedCount << " 帧，当前队列大小: " << newDataCardQueue.Size() << std::endl;
    
    // 批量出队处理前先检查队列状态
    std::cout << "📊 批量出队前队列状态 - 大小: " << newDataCardQueue.Size() << ", 容量: " << newDataCardQueue.Capacity() 
              << ", 空: " << (newDataCardQueue.IsEmpty() ? "是" : "否") 
              << ", 满: " << (newDataCardQueue.IsFull() ? "是" : "否") << std::endl;
    
    // 批量出队处理
    std::vector<std::vector<int16_t>> newBatchResults(3);
    size_t newDequeuedCount = newDataCardQueue.BatchDequeue(3, newBatchResults);
    assert(newDequeuedCount == 3);
    std::cout << "✅ 批量出队 " << newDequeuedCount << " 帧，首帧数据[0]=" << newBatchResults[0][0] << std::endl;
    
    // 检查出队后状态
    std::cout << "📊 批量出队后队列状态 - 大小: " << newDataCardQueue.Size() << ", 空: " 
              << (newDataCardQueue.IsEmpty() ? "是" : "否") << std::endl;

    std::cout << "\n🎉 所有测试完成!" << std::endl;
    return 0;
}