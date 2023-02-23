#include "heap_timer.hpp"

heap_timer::heap_timer(int init_capacity) : m_array(init_capacity + 1),
                                            m_map{}, m_size{0} {

}

heap_timer::~heap_timer() {
    // for (int i{1}; i <= m_size; ++i) {
    //     delete m_array[i];
    // }
    
}

void heap_timer::add_timer(timer* t) {
    if (m_size == m_array.size() - 1) {
        m_array.resize(m_array.size() * 2);
    }

    m_array[++m_size] = t;
    percolate_up(m_size);
}

void heap_timer::adjust_timer(timer* t) {
    int hole = m_map[t];
    if (hole > 1 && *m_array[hole] < *m_array[hole / 2]) {
        percolate_up(hole);
    }
    else {
        percolate_down(hole);
    }
    // percolate_down(hole);
}

void heap_timer::delete_timer(timer* t) {
    int hole = m_map[t];
    m_array[hole] = m_array[m_size--];

    m_map.erase(t);
    // delete t;

    if (hole > 1 && *m_array[hole] < *m_array[hole / 2]) {
        percolate_up(hole);
    }
    else {
        percolate_down(hole);
    }
}

void heap_timer::tick() {
    time_t cur = time(nullptr);
    while (m_size > 0) {
        timer* top = m_array[1];
        if (top->m_expire <= cur) {
            top->m_cb_func();
            delete_timer(top);
        }
        else {
            break;
        }
    } 
}

// timer* heap_timer::pop() {
//     timer* ret = m_array[1];
//     m_array[1] = m_array[m_size--];
//     percolate_down(1);
//     return ret;
// }

void heap_timer::percolate_down(int hole) {
    timer* tmp = m_array[hole];
    int child{};
    for (; hole * 2 <= m_size; hole = child) {
        child = hole * 2;
        if (child != m_size && *m_array[child + 1] < *m_array[child]) {
            ++child;
        }
        if (*m_array[child] < *tmp) {
            m_array[hole] = m_array[child];
            m_map[m_array[hole]] = hole;
        }
        else {
            break;
        }
    }
    m_array[hole] = tmp;
    m_map[tmp] = hole;
}

void heap_timer::percolate_up(int hole) {
    timer* tmp = m_array[hole];
    int parent{};
    for (; hole / 2 >= 1; hole = parent) {
        parent = hole / 2;
        if (*tmp < *m_array[parent]) {
            m_array[hole] = m_array[parent];
            m_map[m_array[hole]] = hole;
        }
        else {
            break;
        }
    }
    m_array[hole] = tmp;
    m_map[tmp] = hole;
}