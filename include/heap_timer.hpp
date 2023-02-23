#pragma once

#include <unordered_map>
#include <vector>
#include <functional>
#include <chrono>
#include <time.h>
#include <iostream>

struct timer {
    std::function<void()> m_cb_func{};
    time_t m_expire{};
    bool operator<(const timer& t) {
        return m_expire < t.m_expire;
    }
};

class heap_timer {
  public:
    explicit heap_timer(int init_capacity = 100);
    ~heap_timer();

    void add_timer(timer* t);
    void adjust_timer(timer* t);
    void delete_timer(timer* t);
    void tick();
  
  private:
    // timer* pop();
    void percolate_down(int hole);
    void percolate_up(int hole);

  private:
    std::vector<timer*> m_array{};
    std::unordered_map<timer*, int> m_map{};
    int m_size{};

};


