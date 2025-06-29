#include "../include/queue_core.h"

template<typename T, size_t N>
QueueCore<T, N>::QueueCore(size_t capacity)
    : buffer(capacity), capacity(capacity), readPos(0), writePos(0), used(0) {}

template<typename T, size_t N>
void QueueCore<T, N>::enqueue(const std::array<T, N>& item) {
    std::unique_lock<std::mutex> lock(mtx);
    cond_not_full.wait(lock, [this] { return used < capacity; });
    buffer[writePos] = item;
    writePos = (writePos + 1) % capacity;
    ++used;
    cond_not_empty.notify_one();
}

template<typename T, size_t N>
bool QueueCore<T, N>::try_enqueue(const std::array<T, N>& item) {
    std::unique_lock<std::mutex> lock(mtx);
    if (used >= capacity) return false;
    buffer[writePos] = item;
    writePos = (writePos + 1) % capacity;
    ++used;
    cond_not_empty.notify_one();
    return true;
}

template<typename T, size_t N>
void QueueCore<T, N>::batchEnqueue(const std::vector<std::array<T, N>>& items) {
    std::unique_lock<std::mutex> lock(mtx);
    cond_not_full.wait(lock, [this, &items] { return used + items.size() <= capacity; });
    for (const auto& item : items) {
        buffer[writePos] = item;
        writePos = (writePos + 1) % capacity;
        ++used;
    }
    cond_not_empty.notify_all();
}

template<typename T, size_t N>
std::vector<std::array<T, N>> QueueCore<T, N>::batchDequeue(size_t count) {
    std::unique_lock<std::mutex> lock(mtx);
    cond_not_empty.wait(lock, [this] { return used > 0; });
    count = std::min(count, used);
    std::vector<std::array<T, N>> result(count);
    for (size_t i = 0; i < count; ++i) {
        result[i] = buffer[readPos];
        readPos = (readPos + 1) % capacity;
        --used;
    }
    cond_not_full.notify_all();
    return result;
}

template<typename T, size_t N>
std::array<T, N> QueueCore<T, N>::dequeue() {
    std::unique_lock<std::mutex> lock(mtx);
    cond_not_empty.wait(lock, [this] { return used > 0; });
    std::array<T, N> item = buffer[readPos];
    readPos = (readPos + 1) % capacity;
    --used;
    cond_not_full.notify_one();
    return item;
}

template<typename T, size_t N>
bool QueueCore<T, N>::try_dequeue(std::array<T, N>& item) {
    std::unique_lock<std::mutex> lock(mtx);
    if (used == 0) return false;
    item = buffer[readPos];
    readPos = (readPos + 1) % capacity;
    --used;
    cond_not_full.notify_one();
    return true;
}

// 显式实例化常用模板类
template class QueueCore<int, 3>;