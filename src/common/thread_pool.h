#pragma once

#include <future>
#include <functional>
#include <queue>
#include <thread>
#include <vector>

namespace hubsql {

// 轻量线程池：提交任务，异步执行
class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 提交任务，返回 std::future
    template <typename F, typename... Args>
    auto Enqueue(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    size_t Size() const { return workers_.size(); }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_{false};
};

template <typename F, typename... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<ReturnType> res = task->get_future();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_) {
            throw std::runtime_error("Enqueue on stopped ThreadPool");
        }
        tasks_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return res;
}

}  // namespace hubsql
