//
// kiwakiwaaにより 2026/02/25 に作成されました。
//

#pragma once

#include <algorithm>
#include <atomic>
#include <exception>
#include <mutex>
#include <thread>

namespace MKD::detail
{
    void parallel_dispatch(size_t count, void* context, void (*function)(size_t, void*));

    template <typename Function>
    void runBatches(const size_t count, const size_t concurrency, Function&& function)
    {
        if (count == 0)
            return;

        const auto available = std::max(1u, std::thread::hardware_concurrency());
        const auto workers = std::min(count, concurrency == 0 ? static_cast<size_t>(available) : concurrency);
        if (workers == 1)
        {
            for (size_t i = 0; i < count; ++i)
                function(i);
            return;
        }
        std::atomic<size_t> next{0};
        std::atomic<bool> failed{false};
        std::exception_ptr failure;
        std::mutex mutex;
        auto work = [&](size_t)
        {
            try
            {
                while (!failed.load(std::memory_order_relaxed))
                {
                    const auto index = next.fetch_add(1, std::memory_order_relaxed);
                    if (index >= count)
                        break;
                    function(index);
                }
            }
            catch (...)
            {
                std::lock_guard lock(mutex);
                if (!failure)
                    failure = std::current_exception();
                failed.store(true, std::memory_order_relaxed);
            }
        };
        parallel_dispatch(workers, &work, [](size_t index, void* context)
        {
            (*static_cast<decltype(work)*>(context))(index);
        });
        if (failure)
            std::rethrow_exception(failure);
    }
}
