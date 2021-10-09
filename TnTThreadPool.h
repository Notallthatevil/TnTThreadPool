#pragma once

/*
 * Adapted from https://github.com/bshoshany/thread-pool
 *
 * MIT License
 *
 * Copyright (c) 2021 Barak Shoshany
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */



 /*
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
 * SOFTWARE.l
 */



#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace TnTThreadPool {
   namespace Details {
      std::vector<std::thread>          g_threadPool{};
      std::queue<std::function<void()>> g_jobs;
      std::atomic_bool                  g_run{ true };
      std::atomic_bool                  g_pauseExecution{ false };
      std::atomic_size_t                g_totalJobs{ 0 };
      std::mutex                        g_threadPoolLock;
      std::mutex                        g_threadRunLock;

      void executor() {
         while (g_run) {
            if (!g_pauseExecution) {
               std::scoped_lock lock{ g_threadPoolLock };
               if (!g_jobs.empty()) {
                  g_jobs.front()();
                  g_jobs.pop();

                  g_totalJobs--;
               }
            }
            else {
               std::this_thread::yield();
            }
         }
      }
   }

   void        finishAllJobs();
   void        pause();
   void        resume();
   void        setThreadCount(std::uint32_t threadCount = std::thread::hardware_concurrency());
   std::size_t getThreadCount();

   template<typename Job, typename... Args>
   void submit(Job& job, Args &&...args) {
      ++Details::g_totalJobs;
      if constexpr (sizeof...(Args) == 0) {
         std::scoped_lock lock{ Details::g_threadPoolLock };
         Details::g_jobs.push(job);
      }
      else {
         // Convert job and args to lambda calling job with the args provided so that it matches the signature of void().
         auto lambda = [&job, &args...]{ job(std::forward<Args>(args)...); };
         // Lambda cannot be 'inlined' into the function call. Job should not be const. If Job is const then it cannot be used within a operator()() overload unless that operator overload is
         // const.
         submit(lambda);
      }
   }

   template<typename ReturnValue, typename Job, typename... Args>
   std::future<ReturnValue> submitForReturn(Job& job, Args &&...args) {
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

   template<typename Job, typename... Args>
   std::future<void> submitWaitable(Job& job, Args &&...args) {
      return submitForReturn<void>(job, std::forward<Args>(args)...);
   }




   void finishAllJobs() { setThreadCount(getThreadCount()); }

   void pause() { Details::g_pauseExecution = true; }

   void resume() {
      if (Details::g_threadPool.empty()) {
         setThreadCount(std::thread::hardware_concurrency());
      }

      Details::g_pauseExecution = false;
   }

   void setThreadCount(std::uint32_t threadCount) {

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

   std::size_t getThreadCount() { return Details::g_threadPool.size(); }
}   // namespace TnTEngine::ThreadPool