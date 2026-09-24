#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>

// ═════════════════════════════════════════════════════════════════════════════
// Mécanisme d'annulation (Token)
// ═════════════════════════════════════════════════════════════════════════════
class CancellationToken {
public:
    CancellationToken() : m_cancelled(false) {}
    void Cancel() noexcept { m_cancelled.store(true, std::memory_order_relaxed); }
    void Reset() noexcept { m_cancelled.store(false, std::memory_order_relaxed); }
    bool IsCancelled() const noexcept { return m_cancelled.load(std::memory_order_relaxed); }
private:
    std::atomic<bool> m_cancelled;
};
using CancellationTokenPtr = std::shared_ptr<CancellationToken>;

// ═════════════════════════════════════════════════════════════════════════════
// Pool de Threads générique et thread-safe
// ═════════════════════════════════════════════════════════════════════════════
class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::max(1u, std::thread::hardware_concurrency())) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                            });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
                });
        }
    }

    template<class F, class... Args>
    auto Enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop)
                throw std::runtime_error("Enqueue sur un ThreadPool arrêté");
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable())
                worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};
