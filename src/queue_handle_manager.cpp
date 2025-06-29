#include "../include/queue_handle_manager.h"

// 初始化静态成员
std::unordered_map<std::string, std::weak_ptr<void>> QueueHandleManager::named_queues_;
std::mutex QueueHandleManager::mutex_;

template<typename T, size_t N>
QueueHandle QueueHandleManager::ObtainNamedQueue(const std::string& name, size_t capacity) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = named_queues_.find(name);
    if (it != named_queues_.end()) {
        auto shared = it->second.lock();
        if (shared) {
            return reinterpret_cast<QueueHandle>(new std::shared_ptr<void>(shared));
        }
    }

    // 创建新队列，默认容量 10
    auto queue_core = std::make_shared<QueueCore<T, N>>(capacity ? capacity : 10);
    auto shared_from_this = std::static_pointer_cast<void>(queue_core);

    named_queues_[name] = shared_from_this;
    return reinterpret_cast<QueueHandle>(new std::shared_ptr<void>(shared_from_this));
}

template<typename T, size_t N>
void QueueHandleManager::Enqueue(QueueHandle handle, const std::array<T, N>& item) {
    auto sp = get_underlying<T, N>(handle);
    sp->enqueue(item);
}

template<typename T, size_t N>
std::array<T, N> QueueHandleManager::Dequeue(QueueHandle handle) {
    auto sp = get_underlying<T, N>(handle);
    return sp->dequeue();
}

void QueueHandleManager::ReleaseQueue(QueueHandle handle) {
    if (!handle) return;
    auto sp = reinterpret_cast<std::shared_ptr<void>*>(handle);
    delete sp;
}

template<typename T, size_t N>
std::shared_ptr<QueueCore<T, N>> QueueHandleManager::get_underlying(QueueHandle handle) {
    if (!handle) throw std::invalid_argument("Invalid queue handle");
    auto sp = *reinterpret_cast<std::shared_ptr<void>*>(handle);
    return std::static_pointer_cast<QueueCore<T, N>>(sp);
}

// 显式实例化常用模板类型
template
QueueHandle QueueHandleManager::ObtainNamedQueue<int, 3>(const std::string& name, size_t capacity);

template
void QueueHandleManager::Enqueue<int, 3>(QueueHandle handle, const std::array<int, 3>& item);

template
std::array<int, 3> QueueHandleManager::Dequeue<int, 3>(QueueHandle handle);
