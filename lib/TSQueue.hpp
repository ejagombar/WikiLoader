#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

template <typename T> class TSQueue {
  public:
    TSQueue() = default;
    ~TSQueue() = default;

    void push(T item);
    T pop();
    bool empty();
    size_t size();

  private:
    size_t length = 0;
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

template <typename T> void TSQueue<T>::push(T item) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.push(item);
    length++;

    m_cond.notify_one(); // Notify one thread that is waiting
}

template <typename T> T TSQueue<T>::pop() {
    std::unique_lock<std::mutex> lock(m_mutex);

    m_cond.wait(lock, [this]() { return !m_queue.empty(); }); // wait until queue is not empty

    T item = m_queue.front();
    m_queue.pop();

    length--;

    return item;
}

template <typename T> bool TSQueue<T>::empty() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

template <typename T> size_t TSQueue<T>::size() { return length; }
