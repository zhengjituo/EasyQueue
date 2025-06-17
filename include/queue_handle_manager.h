#ifndef OBTAIN_RELEASE_H
#define OBTAIN_RELEASE_H

#include <memory>
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

#endif // OBTAIN_RELEASE_H