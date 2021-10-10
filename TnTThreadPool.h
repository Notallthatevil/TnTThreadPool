#ifndef TNT_THREAD_POOL_H
#define TNT_THREAD_POOL_H
/*
 * Adapted from https://github.com/bshoshany/thread-pool
 *
 * MIT License
 *
 * Copyright(c) 2021 Nate Tripp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */



#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

 /// @brief Simple thread pool implementation. To begin using create a TnTThreadPool object and call one of its submit functions. 
namespace TnTThreadPool {
   namespace Details {

      inline std::vector<std::thread>          g_threadPool{};
      inline std::queue<std::function<void()>> g_jobs;
      inline std::atomic_bool                  g_run{ true };
      inline std::atomic_bool                  g_pauseExecution{ false };
      inline std::atomic_uint32_t              g_refCount{ 0 };
      inline std::mutex                        g_threadPoolLock;
      inline std::mutex                        g_threadRunLock;


      inline void executor() {
         while (g_run) {
            if (!g_pauseExecution) {
               std::scoped_lock lock{ g_threadPoolLock };
               if (!g_jobs.empty()) {
                  g_jobs.front()();
                  g_jobs.pop();
               }
            }
            else {
               std::this_thread::yield();
            }
         }
      }
   }

   /// @brief Thread pool object to manage lifetime of global thread pool. All instances of this object submit jobs to the same global thread pool.Creating more then one thread pool 
   /// object safely increments the number of references to the global pool. Once all thread pool objects have been deleted then the global thread pool is destroyed. 
   struct TnTThreadPool {
      /// @brief Constructs a TnTThreadPool for use and increments the global thread pool ref count.
      TnTThreadPool() {
         ++Details::g_refCount;
         if (Details::g_threadPool.empty()) {
            setThreadCount(std::thread::hardware_concurrency());
         }
      };

      /// @brief Decrements the global ref count. Once all references have been destroyed then each thread is joined and the pool cleared.
      ~TnTThreadPool() {
         --Details::g_refCount;
         if (Details::g_refCount == 0) {
            std::scoped_lock lock{ Details::g_threadRunLock, Details::g_threadPoolLock };

            Details::g_pauseExecution = false;

            while (!Details::g_jobs.empty()) {
               std::this_thread::yield();
            }

            Details::g_run = false;

            for (auto& t : Details::g_threadPool) {
               t.join();
            }

            Details::g_threadPool.clear();
         }
      }
      TnTThreadPool(const TnTThreadPool&) = delete;
      TnTThreadPool(TnTThreadPool&&) = delete;
      TnTThreadPool& operator=(const TnTThreadPool&) = delete;
      TnTThreadPool& operator=(TnTThreadPool&&) = delete;

      /// @brief Submits a job to the thread pool queue for execution.
      /// @tparam Job A callable of some type. I.e. lambda, function, or class/struct with operator() overloaded. 
      /// @tparam Args [Optional] Arguments to provide to the job.
      /// @param job The job to execute.
      /// @param args Arguments to provide to the job.
      /// @remarks If providing a lambda, it cannot be initialized in the function parameters. It must be assigned to a variable and then that variable passed in. 
      /// Otherwise, you may get a compiler error indicating that a non-const reference must be an l-value.
      template<typename Job, typename... Args>
      inline void submit(Job& job, Args &&...args) {
         if constexpr (sizeof...(Args) == 0) {
            std::scoped_lock lock{ Details::g_threadPoolLock };
            Details::g_jobs.emplace(job);
         }
         else {
            // Convert job and args to lambda calling job with the args provided so that it matches the signature of void().
            auto lambda = [&job, &args...]{ job(std::forward<Args>(args)...); };
            // Lambda cannot be 'inlined' into the function call. Job should not be const. If Job is const then it cannot be used within a operator() overload unless that operator overload is
            // const.
            submit(lambda);
         }
      }

      /// @brief Submits a job to the thread pool queue and allows the user to retrieve a return value from the job.
      /// @tparam ReturnValue The return value of the job.
      /// @tparam Job A callable of some type. I.e. lambda, function, or class/struct with operator() overloaded. 
      /// @tparam Args [Optional] Arguments to provide to the job.
      /// @param job The job to execute.
      /// @param args Arguments to provide to the job.
      /// @returns An std::future of the return value. To wait for the return value use future.wait() or one of its alternate forms. 
      template<typename ReturnValue, typename Job, typename... Args>
      inline std::future<ReturnValue> submitForReturn(Job& job, Args &&...args) {
         std::promise<ReturnValue>* promise = new std::promise<ReturnValue>{};
         std::future<ReturnValue>   future = promise->get_future();

         auto lambda = [&job, &args..., promise]{
            if constexpr (std::is_void_v<ReturnValue>) {
               job(std::forward<Args>(args)...);
               promise->set_value();
            }
            else {
               promise->set_value(job(std::forward<Args>(args)...));
            }
            delete promise;
         };
         submit(lambda);
         return future;
      }

      /// @brief Specialization of @see submitForReturn. Uses void as the return value, but unlike @see submit, this function allows that caller to wait for completion.
      /// @tparam Job A callable of some type. I.e. lambda, function, or class/struct with operator() overloaded. 
      /// @tparam Args [Optional] Arguments to provide to the job.
      /// @param job The job to execute.
      /// @param args Arguments to provide to the job.
      /// @returns An std::future<void>. To wait for the job to finish use future.wait() or one of its alternate forms. 
      template<typename Job, typename... Args>
      inline std::future<void> submitWaitable(Job& job, Args &&...args) {
         return submitForReturn<void>(job, std::forward<Args>(args)...);
      }

      /// @brief Causes all queued jobs to be completed and updates the thread pool size to threadCount.
      /// @param threadCount The new thread pool size. If this value is equal to zero or greater then std::thread::hardware_concurrency(), it is defaulted to 
      /// std::thread::hardware_concurrency().
      inline void setThreadCount(std::uint32_t threadCount) {

         if (threadCount > std::thread::hardware_concurrency() || threadCount == 0) {
            threadCount = std::thread::hardware_concurrency();
         }

         std::scoped_lock lock{ Details::g_threadRunLock };

         Details::g_run = false;

         for (auto& t : Details::g_threadPool) {
            t.join();
         }

         Details::g_threadPool.clear();

         Details::g_run = true;
         for (decltype(threadCount) i = 0; i < threadCount; ++i) {
            Details::g_threadPool.emplace_back(std::thread(Details::executor));
         }
      }

      /// @brief Returns number of threads in the pool.
      /// @returns The number of threads in the pool.
      inline  std::size_t getThreadCount() { return Details::g_threadPool.size(); }

      /// @brief Causes the calling thread to wait here until all jobs in the queue are finished.
      inline void finishAllJobs() { setThreadCount(getThreadCount()); }

      /// @brief Joins all threads in the pool and shuts down further execution.
      inline void shutdown() {

      }

      /// @brief Pauses thread pool execution.
      inline void pause() { Details::g_pauseExecution = true; }

      /// @brief Resumes thread pool execution.
      inline void resume() {
         if (Details::g_threadPool.empty()) {
            setThreadCount(std::thread::hardware_concurrency());
         }

         Details::g_pauseExecution = false;
      }
   };


}   // namespace TnTEngine::ThreadPool

#endif