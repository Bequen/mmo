#pragma once

#include <thread>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>

class ThreadPool {
public:
    uint32_t size() {
        return m_workers.size();
    }

    explicit ThreadPool(size_t count = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < count; ++i) {
            m_workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(m_mutex);
                        m_cv.wait(lock, [this]{ return m_stop || !m_tasks.empty(); });
                        if (m_stop && m_tasks.empty()) return;
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    void submit(std::function<void()> task) {
        {
            std::lock_guard lock(m_mutex);
            m_tasks.push(std::move(task));
        }
        m_cv.notify_one();
    }

    void wait_idle() {
        // Simple barrier: submit N no-op tasks and wait on a counter
        std::atomic<size_t> remaining{m_workers.size()};
        std::mutex done_mtx;
        std::condition_variable done_cv;
        for (size_t i = 0; i < m_workers.size(); ++i) {
            submit([&]() {
                if (--remaining == 0) {
                    done_cv.notify_one();
                }
            });
        }
        std::unique_lock lock(done_mtx);
        done_cv.wait(lock, [&]{ return remaining.load() == 0; });
    }

    ~ThreadPool() {
        { std::lock_guard lock(m_mutex); m_stop = true; }
        m_cv.notify_all();
        for (auto& w : m_workers) w.join();
    }

private:
    std::vector<std::thread>          m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex                        m_mutex;
    std::condition_variable           m_cv;
    bool                              m_stop = false;
};
