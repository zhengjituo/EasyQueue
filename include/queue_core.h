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
    explicit QueueCore(size_t capacity);

    void enqueue(const std::array<T, N>& item);
    bool try_enqueue(const std::array<T, N>& item);

    void batchEnqueue(const std::vector<std::array<T, N>>& items);
    std::vector<std::array<T, N>> batchDequeue(size_t maxItems);

    std::array<T, N> dequeue();
    bool try_dequeue(std::array<T, N>& out);

    size_t size() const;
    size_t available() const;

    std::vector<std::array<T, N>> flush();
    void clear();

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
    bool dequeue_unsafe(std::array<T, N>& out);
};

#endif // QUEUE_CORE_H