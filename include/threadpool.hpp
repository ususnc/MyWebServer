#pragma once

#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include <semaphore>

#include <iostream>

#include "block_queue.hpp"

class thread_pool {
  public:
    thread_pool(int num_threads = 8) : m_done(false), m_work_queue{}, m_threads{} {
        for (int i{0}; i < num_threads; ++i) {
            m_threads.push_back(std::thread(&thread_pool::worker_thread, this));
            // should be detached?
        }
        for (auto& thread : m_threads) {
            thread.detach();
        }
    }

    ~thread_pool() {
        // std::cout << "~thread_pool" << std::endl;
        m_done.store(true, std::memory_order_relaxed);

        // for (auto& thread : m_threads) {
        //     if (thread.joinable()) {
        //         thread.join();
        //     }
        // }
    }

    void submit(std::function<void()> work) {
        m_work_queue.push(work);
    }

  private:
    std::atomic<bool> m_done;
    block_queue<std::function<void()>> m_work_queue;
    std::vector<std::thread> m_threads;

  private:
    void worker_thread() {
        while (!m_done.load(std::memory_order_relaxed)) {
            auto task = m_work_queue.pop();
            task();
        }
        // std::cout << "worker-thread-ended" << std::endl;
    }

};