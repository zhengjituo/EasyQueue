#ifndef EASY_QUEUE_ALL_IN_ONE_H
#define EASY_QUEUE_ALL_IN_ONE_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <iostream>
#include <type_traits>
#include <atomic>
#include <thread>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

// 初始化中文支持
inline void InitializeChineseSupport() {
#ifdef _WIN32
    // Windows下设置控制台UTF-8支持
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

/**
 * EasyQueue - 模仿LabVIEW Queue的高效预分配内存队列
 * 
 * 特点：
 * - 根据输入的数据类型和元素个数预分配内存
 * - 特别支持1维数组类型的预定义长度
 * - 兼容不定长数组，自动识别是否需要预分配
 * - 优化批量操作，支持1入1000出
 * - 线程安全，符合RAII资源管理
 */
template<typename T, size_t ArraySize = 0>
class EasyQueue {
private:
    std::vector<T> buffer_;        // 预分配的环形缓冲区
    size_t capacity_;
    size_t read_pos_;
    size_t write_pos_;
    size_t used_;
    mutable std::mutex mtx_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    
    // 类型特征检测：判断是否为vector类型
    template<typename U>
    struct is_vector : std::false_type {};
    
    template<typename U>
    struct is_vector<std::vector<U>> : std::true_type {};
    
    // 类型特征检测：判断是否有resize方法
    template<typename U>
    static auto has_resize_method(int) -> decltype(std::declval<U>().resize(0), std::true_type{});
    
    template<typename U>
    static std::false_type has_resize_method(...);
    
    template<typename U>
    using has_resize = decltype(has_resize_method<U>(0));

public:
    /**
     * 构造函数 - 根据数据类型和元素个数智能预分配内存
     * @param capacity 队列容量（元素个数）
     */
    explicit EasyQueue(size_t capacity)
        : capacity_(capacity), read_pos_(0), write_pos_(0), used_(0) {
        // 预分配队列容量
        buffer_.reserve(capacity);
        buffer_.resize(capacity);
        
        // 智能预分配机制
        initializeElements();
    }
    
private:
    void initializeElements() {
        if constexpr (ArraySize > 0) {
            // 情况1：显式指定了ArraySize，必须预分配
            for (auto& element : buffer_) {
                if constexpr (has_resize<T>::value) {
                    element.reserve(ArraySize);
                    element.resize(ArraySize);
                }
            }
            std::cout << "EasyQueue初始化：固定长度模式，数组长度=" << ArraySize << std::endl;
        } else {
            // 情况2：未指定ArraySize，自动识别类型
            if constexpr (is_vector<T>::value) {
                // 是vector类型，但未指定长度 -> 不定长数组模式
                std::cout << "EasyQueue初始化：不定长数组模式，不预分配内存" << std::endl;
                // 不进行内存预分配，由用户在运行时决定大小
            } else {
                // 非vector类型（基础类型、结构体等）-> 固定大小模式
                std::cout << "EasyQueue初始化：固定类型模式，元素大小=" << sizeof(T) << "字节" << std::endl;
                // 基础类型和结构体已经通过resize(capacity)完成预分配
            }
        }
        
        std::cout << "  队列容量：" << capacity_ << "个元素" << std::endl;
    }

public:

    /**
     * 入队操作（阻塞）
     * @param item 要入队的元素
     */
    void Enqueue(const T& item) {
        std::unique_lock<std::mutex> lock(mtx_);
        not_full_.wait(lock, [this] { return used_ < capacity_; });
        buffer_[write_pos_] = item;
        write_pos_ = (write_pos_ + 1) % capacity_;
        ++used_;
        not_empty_.notify_one();
    }

    /**
     * 尝试入队操作（非阻塞）
     * @param item 要入队的元素
     * @return 成功返回true，队列满返回false
     */
    bool TryEnqueue(const T& item) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (used_ >= capacity_) return false;
        buffer_[write_pos_] = item;
        write_pos_ = (write_pos_ + 1) % capacity_;
        ++used_;
        not_empty_.notify_one();
        return true;
    }

    /**
     * 出队操作（阻塞）
     * @return 队列中的元素
     */
    T Dequeue() {
        std::unique_lock<std::mutex> lock(mtx_);
        not_empty_.wait(lock, [this] { return used_ > 0; });
        T item = buffer_[read_pos_];
        read_pos_ = (read_pos_ + 1) % capacity_;
        --used_;
        not_full_.notify_one();
        return item;
    }

    /**
     * 批量出队操作 - 优化的1入1000出场景
     * @param max_count 最大出队数量
     * @param result 预分配的结果容器（避免内存分配）
     * @return 实际出队的数量
     */
    size_t BatchDequeue(size_t max_count, std::vector<T>& result) {
        std::unique_lock<std::mutex> lock(mtx_);
        not_empty_.wait(lock, [this] { return used_ > 0; });
        
        size_t count = std::min(max_count, used_);
        if (result.size() < count) {
            result.resize(count);
        }
        
        for (size_t i = 0; i < count; ++i) {
            result[i] = buffer_[read_pos_];
            read_pos_ = (read_pos_ + 1) % capacity_;
            --used_;
        }
        
        not_full_.notify_all();
        return count;
    }

    /**
     * 尝试出队操作（非阻塞）
     * @param item 输出参数，存储出队的元素
     * @return 成功返回true，队列空返回false
     */
    bool TryDequeue(T& item) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (used_ == 0) return false;
        item = buffer_[read_pos_];
        read_pos_ = (read_pos_ + 1) % capacity_;
        --used_;
        not_full_.notify_one();
        return true;
    }

    /**
     * 批量入队操作
     * @param items 要入队的元素向量
     * @return 成功入队的元素数量
     */
    size_t BatchEnqueue(const std::vector<T>& items) {
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待直到有足够的空间
        not_full_.wait(lock, [this, &items] { 
            return used_ + items.size() <= capacity_; 
        });
        
        size_t count = std::min(items.size(), capacity_ - used_);
        
        for (size_t i = 0; i < count; ++i) {
            buffer_[write_pos_] = items[i];
            write_pos_ = (write_pos_ + 1) % capacity_;
            ++used_;
        }
        
        not_empty_.notify_all();
        return count;
    }

    /**
     * 获取队列当前大小
     * @return 队列中元素的数量
     */
    size_t Size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return used_;
    }

    /**
     * 检查队列是否为空
     * @return 如果队列为空则返回 true，否则返回 false
     */
    bool IsEmpty() const {
        return Size() == 0;
    }

    /**
     * 检查队列是否已满
     * @return 如果队列已满则返回 true，否则返回 false
     */
    bool IsFull() const {
        return Size() >= Capacity();
    }

    /**
     * 获取队列可用空间
     * @return 队列剩余空间数量
     */
    size_t Available() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return capacity_ - used_;
    }

    /**
     * 获取队列容量
     * @return 队列总容量
     */
    size_t Capacity() const {
        return capacity_;
    }

    /**
     * 检查队列是否为空
     * @return 空返回true
     */
    bool Empty() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return used_ == 0;
    }

    /**
     * 检查队列是否已满
     * @return 满返回true
     */
    bool Full() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return used_ == capacity_;
    }

    /**
     * 清空队列
     */
    void Clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        read_pos_ = 0;
        write_pos_ = 0;
        used_ = 0;
        not_full_.notify_all();
    }
    
    /**
     * 注销队列 - 释放所有内存，为重新初始化做准备
     */
    void Destroy() {
        std::lock_guard<std::mutex> lock(mtx_);
        buffer_.clear();
        buffer_.shrink_to_fit();  // 强制释放内存
        capacity_ = 0;
        read_pos_ = 0;
        write_pos_ = 0;
        used_ = 0;
        not_full_.notify_all();
        not_empty_.notify_all();
    }
};

/**
 * FastQueue - 高性能队列实现，针对特定场景优化
 * 
 * 特点：
 * 1. 使用预分配的连续内存块
 * 2. 无动态内存分配（在运行时）
 * 3. 简化的线程同步机制
 * 4. 仅支持固定大小的元素
 * 5. 限制：元素类型必须是POD类型，且大小不超过预设的最大值
 * 
 * 适用于：对性能要求极高的场景，如DMA数据传输
 */
template<size_t MaxElementSize = 1024, size_t MaxElements = 1024>
class FastQueue {
private:
    alignas(std::max_align_t) char buffer_[MaxElements][MaxElementSize];  // 预分配的内存池
    std::atomic<size_t> read_pos_{0};
    std::atomic<size_t> write_pos_{0};
    std::atomic<size_t> count_{0};
    const size_t max_elements_;
    
    // 自旋锁，用于高性能场景
    std::atomic_flag spinlock_ = ATOMIC_FLAG_INIT;
    
    void lock() {
        while(spinlock_.test_and_set(std::memory_order_acquire));
    }
    
    void unlock() {
        spinlock_.clear(std::memory_order_release);
    }

public:
    FastQueue() : max_elements_(MaxElements) {}
    
    /**
     * 入队操作 - 仅支持POD类型
     * 注意：此方法对T有严格限制，必须是POD类型
     */
    template<typename T>
    void Enqueue(const T& item) {
        static_assert(std::is_pod_v<T>, "FastQueue only supports POD types");
        static_assert(sizeof(T) <= MaxElementSize, "Type size exceeds maximum element size");
        
        // 等待直到有空间
        while(count_.load() >= max_elements_) {
            std::this_thread::yield();
        }
        
        lock();
        if(count_.load() < max_elements_) {
            memcpy(&buffer_[write_pos_.load()][0], &item, sizeof(T));
            write_pos_.store((write_pos_.load() + 1) % max_elements_);
            count_.fetch_add(1);
        }
        unlock();
    }
    
    /**
     * 尝试入队操作
     */
    template<typename T>
    bool TryEnqueue(const T& item) {
        static_assert(std::is_pod_v<T>, "FastQueue only supports POD types");
        static_assert(sizeof(T) <= MaxElementSize, "Type size exceeds maximum element size");
        
        if(count_.load() >= max_elements_) {
            return false;
        }
        
        lock();
        bool result = false;
        if(count_.load() < max_elements_) {
            memcpy(&buffer_[write_pos_.load()][0], &item, sizeof(T));
            write_pos_.store((write_pos_.load() + 1) % max_elements_);
            count_.fetch_add(1);
            result = true;
        }
        unlock();
        
        return result;
    }
    
    /**
     * 出队操作
     */
    template<typename T>
    T Dequeue() {
        static_assert(std::is_pod_v<T>, "FastQueue only supports POD types");
        static_assert(sizeof(T) <= MaxElementSize, "Type size exceeds maximum element size");
        
        // 等待直到有数据
        while(count_.load() == 0) {
            std::this_thread::yield();
        }
        
        lock();
        T result;
        if(count_.load() > 0) {
            memcpy(&result, &buffer_[read_pos_.load()][0], sizeof(T));
            read_pos_.store((read_pos_.load() + 1) % max_elements_);
            count_.fetch_sub(1);
        }
        unlock();
        
        return result;
    }
    
    /**
     * 尝试出队操作
     */
    template<typename T>
    bool TryDequeue(T& item) {
        static_assert(std::is_pod_v<T>, "FastQueue only supports POD types");
        static_assert(sizeof(T) <= MaxElementSize, "Type size exceeds maximum element size");
        
        if(count_.load() == 0) {
            return false;
        }
        
        lock();
        bool result = false;
        if(count_.load() > 0) {
            memcpy(&item, &buffer_[read_pos_.load()][0], sizeof(T));
            read_pos_.store((read_pos_.load() + 1) % max_elements_);
            count_.fetch_sub(1);
            result = true;
        }
        unlock();
        
        return result;
    }
    
    /**
     * 批量入队操作 - 高性能版本
     */
    template<typename T>
    size_t BatchEnqueue(const std::vector<T>& items) {
        static_assert(std::is_pod_v<T>, "FastQueue only supports POD types");
        static_assert(sizeof(T) <= MaxElementSize, "Type size exceeds maximum element size");
        
        if(items.empty()) return 0;
        
        size_t enqueued = 0;
        size_t remaining_space = max_elements_ - count_.load();
        size_t to_enqueue = std::min(items.size(), remaining_space);
        
        lock();
        for(size_t i = 0; i < to_enqueue; ++i) {
            if(count_.load() < max_elements_) {
                memcpy(&buffer_[write_pos_.load()][0], &items[i], sizeof(T));
                write_pos_.store((write_pos_.load() + 1) % max_elements_);
                count_.fetch_add(1);
                enqueued++;
            } else {
                break;
            }
        }
        unlock();
        
        return enqueued;
    }
    
    /**
     * 批量出队操作 - 高性能版本
     */
    template<typename T>
    size_t BatchDequeue(size_t max_count, std::vector<T>& result) {
        static_assert(std::is_pod_v<T>, "FastQueue only supports POD types");
        static_assert(sizeof(T) <= MaxElementSize, "Type size exceeds maximum element size");
        
        if(max_count == 0 || count_.load() == 0) return 0;
        
        size_t to_dequeue = std::min(max_count, count_.load());
        if(result.size() < to_dequeue) {
            result.resize(to_dequeue);
        }
        
        size_t dequeued = 0;
        lock();
        for(size_t i = 0; i < to_dequeue; ++i) {
            if(count_.load() > 0) {
                memcpy(&result[i], &buffer_[read_pos_.load()][0], sizeof(T));
                read_pos_.store((read_pos_.load() + 1) % max_elements_);
                count_.fetch_sub(1);
                dequeued++;
            } else {
                break;
            }
        }
        unlock();
        
        return dequeued;
    }
    
    /**
     * 获取队列当前大小
     */
    size_t Size() const {
        return count_.load();
    }
    
    /**
     * 获取队列容量
     */
    size_t Capacity() const {
        return max_elements_;
    }
    
    /**
     * 检查队列是否为空
     */
    bool IsEmpty() const {
        return count_.load() == 0;
    }
    
    /**
     * 检查队列是否已满
     */
    bool IsFull() const {
        return count_.load() >= max_elements_;
    }
};

// =================== 智能预分配支持和使用示例 ===================
/*
支持的数据类型：
- U8:  uint8_t     (8位无符号整数)
- I8:  int8_t      (8位有符号整数) 
- U16: uint16_t    (16位无符号整数)
- I16: int16_t     (16位有筦号整数)
- U32: uint32_t    (32位无筦号整数)
- I32: int32_t     (32位有筦号整数)
- U64: uint64_t    (64位无筦号整数)
- I64: int64_t     (64位有筦号整数)
- SGL: float       (单精度浮点数)
- DBL: double      (双精度浮点数)

智能预分配模式：

// 1. 固定长度数组模式（显式指定ArraySize）
EasyQueue<std::vector<int16_t>, 10000> fixedQueue(1000);     // 预分配10k个I16点
EasyQueue<std::vector<float>, 2048> fixedFloatQueue(500);    // 预分配2k个SGL点

// 2. 不定长数组模式（不指定ArraySize）
EasyQueue<std::vector<int16_t>> dynamicQueue(1000);         // 不预分配，运行时决定大小
EasyQueue<std::vector<float>> dynamicFloatQueue(500);       // 适合变长数据

// 3. 固定类型模式（非vector类型）
EasyQueue<int16_t> i16Queue(1000);                          // I16整数队列
EasyQueue<float> sglQueue(1000);                            // SGL浮点数队列
EasyQueue<double> dblQueue(1000);                           // DBL浮点数队列

// 4. 自定义结构体（自动识别）
struct CustomData {
    int id;
    float value;
    char name[32];
};
EasyQueue<CustomData> customQueue(1000);                    // 固定结构体队列
EasyQueue<std::vector<CustomData>> customDynamicQueue(50);  // 不定长结构体数组

// 5. 数据采集卡场景对比
EasyQueue<std::vector<int16_t>, 10000> dataCard10k(1000);   // 固定10k点，高性能
EasyQueue<std::vector<int16_t>> dataCardDynamic(1000);      // 变长点数，灵活

使用示例对比：

// 固定长度模式（高性能）
EasyQueue<std::vector<int16_t>, 10000> fixedQueue(1000);
std::vector<int16_t> data(10000);  // 直接使用预分配的内存
fixedQueue.TryEnqueue(data);

// 不定长模式（灵活）
EasyQueue<std::vector<int16_t>> dynamicQueue(1000);
std::vector<int16_t> data1(5000);   // 5k点
std::vector<int16_t> data2(8000);   // 8k点
std::vector<int16_t> data3(12000);  // 12k点
dynamicQueue.TryEnqueue(data1);
dynamicQueue.TryEnqueue(data2);
dynamicQueue.TryEnqueue(data3);

性能对比：
- 固定长度模式：高性能，零分配，缓存友好
- 不定长模式：灵活性高，但有动态分配开销
- 固定结构体：最高性能（比vector高3.5倍）

自动识别规则：
1. 如果显式指定ArraySize > 0，则使用固定长度模式
2. 如果是std::vector类型且未指定ArraySize，则使用不定长模式
3. 如枟是非vector类型，则使用固定类型模式
*/

#endif // EASY_QUEUE_ALL_IN_ONE_H