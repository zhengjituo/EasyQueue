#ifndef OBTAIN_RELEASE_H
#define OBTAIN_RELEASE_H

#include <memory>  // 提供 shared_ptr 和 weak_ptr
#include <string>
#include <unordered_map>
#include <mutex>
#include "queue_core.h"

using QueueHandle = void*;

template<typename T, size_t N>
class SharedQueue {
public:
    using InternalPtr = std::shared_ptr<QueueCore<T, N>>;

    static QueueHandle ObtainQueue(size_t capacity) {
        auto q = std::make_shared<QueueCore<T, N>>(capacity);
        return reinterpret_cast<QueueHandle>(new std::shared_ptr<QueueCore<T, N>>(q));
    }

    static void ReleaseQueue(QueueHandle handle) {
        if (!handle) return;
        auto sp = reinterpret_cast<std::shared_ptr<QueueCore<T, N>>*>(handle);
        delete sp;
    }

    static void Enqueue(QueueHandle handle, const std::array<T, N>& item) {
        auto sp = get_underlying(handle);
        sp->enqueue(item);
    }

    static bool TryEnqueue(QueueHandle handle, const std::array<T, N>& item) {
        auto sp = get_underlying(handle);
        return sp->try_enqueue(item);
    }

    static std::array<T, N> Dequeue(QueueHandle handle) {
        auto sp = get_underlying(handle);
        return sp->dequeue();
    }

    static bool TryDequeue(QueueHandle handle, std::array<T, N>& out) {
        auto sp = get_underlying(handle);
        return sp->try_dequeue(out);
    }

    static std::vector<std::array<T, N>> Flush(QueueHandle handle) {
        auto sp = get_underlying(handle);
        return sp->flush();
    }

    static size_t Size(QueueHandle handle) {
        auto sp = get_underlying(handle);
        return sp->size();
    }

    static size_t Available(QueueHandle handle) {
        auto sp = get_underlying(handle);
        return sp->available();
    }

    /**
     * 获取队列句柄对应的内部指针
     * 
     * @param handle 队列句柄，必须是有效的非空句柄
     * @return 返回与句柄关联的内部指针
     * @throws std::invalid_argument 如果句柄无效
     */
    static InternalPtr get_underlying(QueueHandle handle) {
        if (!handle) throw std::invalid_argument("Invalid queue handle");
        return *reinterpret_cast<InternalPtr*>(handle);
    }

    // 批量入队
    static void BatchEnqueue(QueueHandle handle, const std::vector<std::array<T, N>>& items) {
        auto sp = get_underlying(handle);
        sp->batchEnqueue(items);
    }

    // 批量出队
    static std::vector<std::array<T, N>> BatchDequeue(QueueHandle handle, size_t maxItems) {
        auto sp = get_underlying(handle);
        return sp->batchDequeue(maxItems);
    }
};

// 全局句柄管理器
class QueueHandleManager {
public:
    template<typename T, size_t N>
    static QueueHandle ObtainNamedQueue(const std::string& name, size_t capacity = 0);

    template<typename T, size_t N>
    static void Enqueue(QueueHandle handle, const std::array<T, N>& item);

    template<typename T, size_t N>
    static std::array<T, N> Dequeue(QueueHandle handle);

    static void ReleaseQueue(QueueHandle handle);

private:
    template<typename T, size_t N>
    static std::shared_ptr<QueueCore<T, N>> get_underlying(QueueHandle handle);

    static std::unordered_map<std::string, std::weak_ptr<void>> named_queues_;
    static std::mutex mutex_;
};

#endif // OBTAIN_RELEASE_H