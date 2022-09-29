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

#include <condition_variable>
#include <cstdlib>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <functional>

namespace TnT {

   class TnTThreadPool {
     public:
      TnTThreadPool(std::size_t threadCount = static_cast<std::size_t>(std::thread::hardware_concurrency())) : m_threadCount(threadCount) { init(); }

      ~TnTThreadPool() { cleanUp(); }

      /// @brief Submits a job to the thread pool queue for execution.
      /// @tparam Job A callable of some type. I.e. lambda, function, or class/struct with operator() overloaded.
      /// @tparam Args [Optional] Arguments to provide to the job.
      /// @param job The job to execute.
      /// @param args Arguments to provide to the job.
      template<typename Job, typename... Args>
      inline void submit(Job&& job, Args&&... args) {
         if constexpr(sizeof...(Args) == 0) {
            queueJob(std::forward<Job>(job));
         }
         else {
            // Convert job and args to lambda calling job with the args provided so that it matches the signature of void().
            submit([job = std::forward<Job>(job), ... args = std::forward<Args>(args)]() mutable { job(args...); });
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
      [[nodiscard]] inline std::future<ReturnValue> submitForReturn(Job&& job, Args&&... args) {
         std::promise<ReturnValue>* promise = new std::promise<ReturnValue>{};
         std::future<ReturnValue>   future  = promise->get_future();

         auto lambda = [job, ... args = std::forward<Args>(args), promise]() mutable {
            if constexpr(std::is_void_v<ReturnValue>) {
               job(args...);
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
      [[nodiscard]] inline std::future<void> submitWaitable(Job&& job, Args&&... args) {
         return submitForReturn<void>(std::forward<Job>(job), std::forward<Args>(args)...);
      }

      /// @brief Creates a job for each item in a container, passing the item as the only parameter to job.
      /// @tparam Job A callable of some type. I.e. lambda, function, or class/struct with operator() overloaded.
      /// @tparam Container A container of some sort, must be support a for each loop.
      /// @param job The job to execute.
      /// @param container The container to iterate over.
      /// @remarks This function blocks until each job created from each container item is complete. Do NOT modify the container during this call. The job's parameter can be a non-const lvalue
      /// reference to modify each element, however, the owning container should never be modified during this call.
      template<typename Job, typename Container>
      inline void forEach(Job&& job, const Container& container) {
         std::vector<std::future<void>> submittedJobs;

         for(auto& item: container) {
            submittedJobs.push_back(submitWaitable(job, item));
         }

         for(const auto& running: submittedJobs) {
            running.wait();
         }
      }

      /// @brief Creates N number of jobs where N is the number of times a number P can be incremented by @paramref increment from @paramref from to @paramref to.
      /// @tparam Numeric A numeric value to increment from and to.
      /// @tparam Job A callable of some type. I.e. lambda, function, or class/struct with operator() overloaded.
      /// @param job The job to execute, taking one parameter of type @see Numeric.
      /// @param from The starting value to increment from (inclusive).
      /// @param to The ending value to increment to (exclusive).
      /// @param increment [Optional; Default=1] The value to increment by for each job.
      /// @remarks This function blocks until all submitted jobs have completed. Do NOT modify an containers that may be backing the stored data. The job should take one parameter whose type is
      /// @see Numeric.
      template<typename Numeric, typename Job>
      inline void forEachIndexed(Job&& job, Numeric from, Numeric to, Numeric increment = 1) requires(std::is_arithmetic_v<Numeric>) {
         std::vector<std::future<void>> submittedJobs;

         for(Numeric index = from; index < to; index += increment) {
            submittedJobs.push_back(submitWaitable(job, index));
         }

         for(const auto& running: submittedJobs) {
            running.wait();
         }
      }

      /// @brief Causes the caller to wait for all currently queued jobs to complete before continuing.
      inline void finishAllJobs() { auto _ = finishAllJobsImpl(); }

      /// @brief Pauses the thread pools execution of queued jobs.
      inline void pause() { auto _ = pauseImpl(); }

      /// @brief Resumes the thread pools job execution.
      inline void resume() { m_pause = false; }

      /// @brief Completes all jobs in the queue then joins all the threads.
      inline void shutdown() { auto _ = shutdownImpl(); }

      /// @brief Completes all jobs in the queue, joins all the threads, then starts up a set number of threads.
      /// @param newThreadCount The number of threads to create in the pool.
      inline void reset(std::size_t newThreadCount = std::thread::hardware_concurrency()) {
         auto lock     = shutdownImpl();
         m_threadCount = newThreadCount;
         init();
      }

      /// @brief Pauses execution of the thread pool, waiting for all in-flight jobs to finish but retaining whats still in the queue. Joins all the threads then spins
      /// up new ones before resuming execution.
      /// @param newThreadCount The number of threads in the thread pool.
      /// @remarks If newThreadCount is 0, this is equivalent to calling shutdown.
      inline void setThreadCount(std::size_t newThreadCount) {
         if(newThreadCount == 0) {
            shutdown();
            return;
         }
         m_threadCount = newThreadCount;
         auto lock = pauseImpl();
         m_execute = false;
         lock.unlock();
         joinThreadsImpl();
         lock.lock();
         init();
         resume();
      }

      /// @brief Returns the number of threads in the pool.
      [[nodiscard]] inline std::size_t getThreadCount() {
         std::scoped_lock lock{ m_jobQueueMutex };
         return m_threads.size();
      }

     private:
      inline void init() {
         m_execute = true;
         for(std::size_t i = 0; i < m_threadCount; ++i) {
            m_threads.emplace_back(std::bind(&TnTThreadPool::executor, this));
         }
      }

      inline void executor() {
         std::function<void()> currentJob;
         while(m_execute) {
            {
               std::scoped_lock lock{ m_jobQueueMutex };
               if(m_queuedTasks == 0 || m_pause) {
                  m_cv.notify_all();
                  std::this_thread::yield();
               }
               else {
                  ++m_runningTasks;
                  currentJob.swap(m_jobQueue.front());
                  m_jobQueue.pop();
                  --m_queuedTasks;
               }
            }
            if(currentJob) {
               currentJob();
               currentJob = {};
               --m_runningTasks;
            }
         }
      }

      template<typename Job>
      inline void queueJob(Job&& job) {
         std::scoped_lock lock{ m_jobQueueMutex };
         if (m_threads.empty()) {
            throw std::runtime_error("Attempted to queue a job, but the thread pool was shutdown. Call reset before queuing jobs.");
         }
         
         ++m_queuedTasks;
         m_jobQueue.emplace(std::forward<Job>(job));
      }

      inline void joinThreadsImpl() { m_threads.clear(); }

      [[nodiscard]] inline std::unique_lock<std::mutex> finishAllJobsImpl() {
         m_execute = true;
         std::unique_lock lock{ m_jobQueueMutex };
         m_cv.wait(lock, [this] { return m_runningTasks == 0 && m_queuedTasks == 0; });
         return lock;
      }

      [[nodiscard]] inline std::unique_lock<std::mutex> shutdownImpl() {
         m_execute = true;
         m_pause   = false;
         auto lock = finishAllJobsImpl();
         m_execute = false;
         lock.unlock();
         joinThreadsImpl();
         lock.lock();
         return lock;
      }

      inline void cleanUp() { auto _ = shutdownImpl(); }

      [[nodiscard]] inline std::unique_lock<std::mutex> pauseImpl() {
         m_pause = true;
         std::unique_lock lock{ m_jobQueueMutex };
         m_cv.wait(lock, [this] { return m_runningTasks == 0; });
         return lock;
      }

     private:
      std::mutex                        m_jobQueueMutex;
      std::vector<std::jthread>         m_threads;
      std::queue<std::function<void()>> m_jobQueue;

      std::atomic_bool   m_execute{ true };
      std::atomic_bool   m_pause{ false };
      std::atomic_size_t m_runningTasks{ 0 };
      std::atomic_size_t m_queuedTasks{ 0 };

      std::size_t m_threadCount;

      std::condition_variable m_cv;
   };

   /// @brief Creates a job for each item in a container, passing the item as the only parameter to job.
   /// @tparam Job A callable of some type. I.e. lambda, function, or class/struct with operator() overloaded.
   /// @tparam Container A container of some sort, must be support a for each loop.
   /// @param job The job to execute.
   /// @param container The container to iterate over.
   /// @remarks This function blocks until each job created from each container item is complete. Do NOT modify the container during this call. The job's parameter can be a non-const lvalue
   /// reference to modify each element, however, the owning container should never be modified during this call.
   template<typename Job, typename Container>
   static inline void forEach(Job&& job, const Container& container, std::size_t threadCount = static_cast<std::size_t>(std::thread::hardware_concurrency())) {
      TnTThreadPool threadPool{ threadCount };
      threadPool.forEach(std::forward(job), container);
   }

   /// @brief Creates N number of jobs where N is the number of times a number P can be incremented by @paramref increment from @paramref from to @paramref to.
   /// @tparam Numeric A numeric value to increment from and to.
   /// @tparam Job A callable of some type. I.e. lambda, function, or class/struct with operator() overloaded.
   /// @param job The job to execute, taking one parameter of type @see Numeric.
   /// @param from The starting value to increment from (inclusive).
   /// @param to The ending value to increment to (exclusive).
   /// @param increment [Optional; Default=1] The value to increment by for each job.
   /// @remarks This function blocks until all submitted jobs have completed. Do NOT modify an containers that may be backing the stored data. The job should take one parameter whose type is
   /// @see Numeric.
   template<typename Numeric, typename Job>
   static inline void forEachIndexed(Job&&       job,
                                     Numeric     from,
                                     Numeric     to,
                                     Numeric     increment   = 1,
                                     std::size_t threadCount = static_cast<std::size_t>(std::thread::hardware_concurrency())) requires(std::is_arithmetic_v<Numeric>) {
      TnTThreadPool threadPool{ threadCount };
      threadPool.forEachIndexed(std::forward(job), from, to, increment, threadCount);
   }

}   // namespace TnT

#endif
