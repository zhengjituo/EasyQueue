#ifndef QUEUE_CORE_H
#define QUEUE_CORE_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <array>

template<typename T, size_t N>
class QueueCore {
public:
    explicit QueueCore(size_t capacity)
        : buffer(capacity), capacity(capacity), readPos(0), writePos(0), used(0) {}

    void enqueue(const std::array<T, N>& item) {
        std::unique_lock<std::mutex> lock(mtx);
        cond_not_full.wait(lock, [this] { return used < capacity; });

        buffer[writePos] = item;
        writePos = (writePos + 1) % capacity;
        ++used;

        cond_not_empty.notify_one();
    }

    bool try_enqueue(const std::array<T, N>& item) {
        std::unique_lock<std::mutex> lock(mtx);
        if (used >= capacity) {
            // 队列已满，丢弃最前面的数据
            std::array<T, N> dummy;
            dequeue_unsafe(dummy);
        }

        buffer[writePos] = item;
        writePos = (writePos + 1) % capacity;
        ++used;

        cond_not_empty.notify_one();
        return true;
    }

    std::array<T, N> dequeue() {
        std::unique_lock<std::mutex> lock(mtx);
        cond_not_empty.wait(lock, [this] { return used > 0; });

        auto value = buffer[readPos];
        readPos = (readPos + 1) % capacity;
        --used;

        cond_not_full.notify_one();
        return value;
    }

    bool try_dequeue(std::array<T, N>& out) {
        std::unique_lock<std::mutex> lock(mtx);
        if (used == 0) return false;

        out = buffer[readPos];
        readPos = (readPos + 1) % capacity;
        --used;

        cond_not_full.notify_one();
        return true;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return used;
    }

    size_t available() const {
        std::lock_guard<std::mutex> lock(mtx);
        return capacity - used;
    }

    std::vector<std::array<T, N>> flush() {
        std::lock_guard<std::mutex> lock(mtx);

        std::vector<std::array<T, N>> result;
        result.reserve(used);

        for (size_t i = 0; i < used; ++i) {
            result.push_back(buffer[(readPos + i) % capacity]);
        }

        readPos = 0;
        writePos = 0;
        used = 0;

        return result;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mtx);
        readPos = 0;
        writePos = 0;
        used = 0;
    }

private:
    std::vector<std::array<T, N>> buffer;
    size_t capacity;
    size_t readPos, writePos, used;

    mutable std::mutex mtx;
    std::condition_variable cond_not_empty;
    std::condition_variable cond_not_full;

    template<typename U, size_t M>
    friend class SharedQueue;

    // 辅助函数用于丢弃数据
    bool dequeue_unsafe(std::array<T, N>& out) {
        if (used == 0) return false;
        out = buffer[readPos];
        readPos = (readPos + 1) % capacity;
        --used;
        return true;
    }
};

#endif // QUEUE_CORE_H