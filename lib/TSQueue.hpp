#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

template <typename T> class TSQueue {
  public:
    TSQueue() = default;
    ~TSQueue() = default;

    void push(T item);
    bool pop(T &item);
    void setFinished();
    bool empty();
    size_t size();

  private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_finished = false;
};

template <typename T> void TSQueue<T>::setFinished() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_finished = true;
    m_cond.notify_all();
}

template <typename T> void TSQueue<T>::push(T item) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.push(item);

    m_cond.notify_one(); // Notify one thread that is waiting
}

template <typename T> bool TSQueue<T>::pop(T &item) {
    std::unique_lock<std::mutex> lock(m_mutex);

    m_cond.wait(lock, [this] { return !m_queue.empty() || m_finished; });

    if (m_finished && m_queue.empty()) {
        return false;
    }

    item = m_queue.front();
    m_queue.pop();
    return true;
}

template <typename T> bool TSQueue<T>::empty() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

template <typename T> size_t TSQueue<T>::size() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}
