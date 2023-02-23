#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>
#include <optional>
#include <functional>


template <typename T>
class block_queue {
  public:
    
    void push(const T& item) {
        std::lock_guard lg{m_mutex};
        m_queue.push(item);
        m_cond_var.notify_one();
    }

    T pop() {
        std::unique_lock ul{m_mutex};
        m_cond_var.wait(ul, [this](){return !m_queue.empty();});

        T tmp = std::move(m_queue.front());
        m_queue.pop();
        return tmp;
    }

    void des_all_elem(std::function<void(T)> fun) {
        std::lock_guard lg{m_mutex};
        while (!m_queue.empty()) {
            T tmp = std::move(m_queue.front());
            m_queue.pop();
            fun(tmp);
        }
    }

  private:
    std::queue<T> m_queue{};
    mutable std::mutex m_mutex{};
    std::condition_variable m_cond_var{};
};