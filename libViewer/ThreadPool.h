#pragma once
#include <queue>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

namespace Regards::Viewer
{
    class CThreadPool
    {
    public:
        explicit CThreadPool(size_t numThreads = std::thread::hardware_concurrency())
            : m_stop(false)
        {
            for (size_t i = 0; i < numThreads; ++i)
            {
                m_workers.emplace_back([this]
                {
                    for (;;)
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(m_queueMutex);
                            m_condition.wait(lock, [this] {
                                return m_stop || !m_tasks.empty();
                            });
                            if (m_stop && m_tasks.empty())
                                return;
                            task = std::move(m_tasks.front());
                            m_tasks.pop();
                        }
                        task();
                    }
                });
            }
        }

        ~CThreadPool()
        {
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_stop = true;
            }
            m_condition.notify_all();
            for (auto& worker : m_workers)
                worker.join();
        }

        template<class F, class... Args>
        auto Dequeue(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
        {
            using ReturnType = decltype(f(args...));
            std::unique_lock<std::mutex> lock(m_queueMutex);
            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
            std::future<ReturnType> result = task->get_future();
            {
                m_condition.wait(lock, [this]
                    {
                        return m_stop || !m_tasks.empty();
                    });

                if (m_stop && m_tasks.empty())
                    return result;

                std::move(m_tasks.front());
                m_tasks.pop();
            }


            return result;
        }

        template<class F, class... Args>
        auto Enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
        {
            using ReturnType = decltype(f(args...));
            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
            std::future<ReturnType> result = task->get_future();
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                if (m_stop)
                    throw std::runtime_error("Enqueue on stopped ThreadPool");
                m_tasks.emplace([task]() { (*task)(); });
            }
            m_condition.notify_one();
            return result;
        }

        size_t PendingTasks() const
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            return m_tasks.size();
        }

        void RequestStop() { m_stop = true; m_condition.notify_all(); }

    private:
        std::vector<std::thread>          m_workers;
        std::queue<std::function<void()>> m_tasks;
        mutable std::mutex                m_queueMutex;
        std::condition_variable           m_condition;
        std::atomic<bool>                 m_stop;
    };
}
