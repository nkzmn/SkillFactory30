#pragma once
#include <future>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>
#include <deque>
#include <memory>

struct ThreadPool {
    std::mutex cout_mutex;

    std::vector<std::thread> threads;
    std::deque<std::unique_ptr<std::packaged_task<void()>>> tasks;
    std::condition_variable condition;
    std::atomic<bool> stop;

    ThreadPool() : stop(false) {
        unsigned int const thread_count = std::thread::hardware_concurrency();

        for (unsigned int i = 0; i < thread_count; ++i) {
            threads.emplace_back(std::thread([this] {
                while (true) {
                    std::unique_ptr<std::packaged_task<void()>> task;

                    {
                        std::unique_lock<std::mutex> lock(cout_mutex);
                        condition.wait(lock, [this] {
                            return !tasks.empty() || stop;
                            });

                        if (stop && tasks.empty()) {
                            return;
                        }

                        task = std::move(tasks.front());
                        tasks.pop_front();
                    }

                    (*task)();
                }
                }));
        }
    }

    ~ThreadPool() {
        stop = true;
        condition.notify_all();

        for (std::thread& thread : threads) {
            thread.join();
        }
    }

    template<typename Task, typename... Args>
    std::future<typename std::result_of<Task(Args...)>::type> push_task(Task&& task, Args&&... args) {
        using ResultType = typename std::result_of<Task(Args...)>::type;

        auto packagedTask = std::make_unique<std::packaged_task<ResultType()>>(std::bind(std::forward<Task>(task), std::forward<Args>(args)...));
        std::future<ResultType> future = packagedTask->get_future();

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            tasks.emplace_back(std::move(packagedTask));
        }

        condition.notify_one();
        return future;
    }
};