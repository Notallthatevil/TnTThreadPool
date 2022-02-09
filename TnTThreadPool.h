#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <future>
#include <condition_variable>

namespace TnTThreadPool {

   namespace Details {

      static std::mutex d_jobQueueMutex;
      static std::vector<std::thread> d_threads;
      static std::queue<std::function<void()>> d_jobQueue;

      static std::atomic_bool d_execute;
      static std::atomic_bool d_pause;
      static std::atomic_size_t d_runningTasks{ 0 };
      static std::atomic_size_t d_queuedTasks{ 0 };

      static std::once_flag d_initialized;

      static std::condition_variable d_cv;

      inline void executor() {
         std::function<void()> currentJob;
         while (d_execute) {
            {
               std::scoped_lock lock{ d_jobQueueMutex };
               if (!d_queuedTasks) {
                  d_cv.notify_all();
                  std::this_thread::yield();
               }
               else {
                  ++d_runningTasks;
                  currentJob = d_jobQueue.front();
                  d_jobQueue.pop();
                  --d_queuedTasks;
               }
            }
            if (currentJob) {
               currentJob();
               currentJob = {};
               --d_runningTasks;
            }
         }
      }

      inline void init() {
         std::call_once(d_initialized, []() {
            for (auto i = 0u; i < std::thread::hardware_concurrency(); ++i) {
               d_threads.emplace_back(executor);
            }
            d_execute = true;
            });
      }

      template<typename Job>
      inline void queueJob(Job&& job) {
         std::scoped_lock lock{ Details::d_jobQueueMutex };
         ++d_queuedTasks;
         Details::d_jobQueue.emplace(job);
      }
   }


   /// @brief Submits a job to the thread pool queue for execution.
/// @tparam Job A callable of some type. I.e. lambda, function, or class/struct with operator() overloaded. 
/// @tparam Args [Optional] Arguments to provide to the job.
/// @param job The job to execute.
/// @param args Arguments to provide to the job.
/// @remarks If providing a lambda, it cannot be initialized in the function parameters. It must be assigned to a variable and then that variable passed in. 
/// Otherwise, you may get a compiler error indicating that a non-const reference must be an l-value.
   template<typename Job, typename... Args>
   inline void submit(const Job& job, Args &&...args) {
      Details::init();
      if constexpr (sizeof...(Args) == 0) {
         Details::queueJob(job);
      }
      else {
         // Convert job and args to lambda calling job with the args provided so that it matches the signature of void().
         submit([&job, &args...]{ job(std::forward<Args>(args)...); });
      }
   }

   /// @brief Submits a job to the thread pool queue for execution.
   /// @tparam Job A callable of some type. I.e. lambda, function, or class/struct with operator() overloaded. 
   /// @tparam Args [Optional] Arguments to provide to the job.
   /// @param job The job to execute.
   /// @param args Arguments to provide to the job.
   /// @remarks If providing a lambda, it cannot be initialized in the function parameters. It must be assigned to a variable and then that variable passed in. 
   /// Otherwise, you may get a compiler error indicating that a non-const reference must be an l-value.
   template<typename Job, typename... Args>
   inline void submit(Job& job, Args &&...args) {
      Details::init();
      if constexpr (sizeof...(Args) == 0) {
         Details::queueJob(job);
      }
      else {
         // Convert job and args to lambda calling job with the args provided so that it matches the signature of void().
         submit([&job, &args...]{ job(std::forward<Args>(args)...); });
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
   inline std::future<ReturnValue> submitForReturn(const Job& job, Args &&...args) {
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
   inline std::future<void> submitWaitable(const Job& job, Args &&...args) {
      return submitForReturn<void>(job, std::forward<Args>(args)...);
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

   inline void finishAllJobs() {
      Details::init();
      Details::d_execute = true;
      std::unique_lock lock{ Details::d_jobQueueMutex };
      Details::d_cv.wait(lock, [] {return Details::d_runningTasks == 0 && Details::d_queuedTasks == 0; });
      //}
   }

   inline void pause() {
      Details::d_execute = false;
   }

   inline void setThreadCount(std::size_t newThreadCount) {
      Details::init();
      if (newThreadCount == 0) {
         pause();
         return;
      }
   }

   inline std::size_t getThreadCount() {
      Details::init();
      std::scoped_lock lock{ Details::d_jobQueueMutex };
      return Details::d_threads.size();
   }

   inline void resume() {
      if (!Details::d_execute) {
         Details::d_execute = true;
      }
      for (auto& thread : Details::d_threads) {
         thread = std::thread{ Details::executor };
      }
   }

}
