#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <TnTThreadPool.h>

using namespace std::chrono_literals;

namespace Concurrency {

   const auto            MAIN_THREAD_ID = std::this_thread::get_id();
   const std::thread::id DEFAULT_THREAD_ID{};

   void functionNoArgs() {
      ASSERT_NE(DEFAULT_THREAD_ID, std::this_thread::get_id());
      ASSERT_NE(MAIN_THREAD_ID, std::this_thread::get_id());
   }
   void                                 functionWithArgs(std::thread::id& id) { id = std::this_thread::get_id(); }
   void                                 functionWait() { std::this_thread::sleep_for(100ms); }
   decltype(std::this_thread::get_id()) functionReturnValue() { return std::this_thread::get_id(); }
   int                                  functionReturnValueWithArgs(int x) { return x * x; }

   struct CallableNoArgs {
      void operator()() {
         ASSERT_NE(DEFAULT_THREAD_ID, std::this_thread::get_id());
         ASSERT_NE(MAIN_THREAD_ID, std::this_thread::get_id());
      }   // namespace TEST_NAMESPACE
   };

   struct CallableWithArgs {
      void operator()(std::thread::id& id) { id = std::this_thread::get_id(); }
   };
   struct CallableWait {
      void operator()() { std::this_thread::sleep_for(100ms); }
   };
   struct CallableReturnValue {
      auto operator()() { return std::this_thread::get_id(); }
   };
   struct CallableReturnValueWithArgs {
      auto operator()(int x) { return x * x; }
   };

   TEST(TnTThreadPoolTests, threadPoolDefaultSize) {
      TnTThreadPool::ThreadPool tp;

      ASSERT_EQ(std::thread::hardware_concurrency(), tp.getThreadCount());
   }

   TEST(TnTThreadPoolTests, submit_Lambda) {
      TnTThreadPool::ThreadPool tp;

      auto lambda = [] {
         ASSERT_NE(DEFAULT_THREAD_ID, std::this_thread::get_id());
         ASSERT_NE(MAIN_THREAD_ID, std::this_thread::get_id());
      };
      tp.submit(lambda);
      tp.finishAllJobs();
   }

   TEST(TnTThreadPoolTests, submit_Function) {
      TnTThreadPool::ThreadPool tp;

      tp.submit(functionNoArgs);
      tp.finishAllJobs();
   }

   TEST(TnTThreadPoolTests, submit_Callable) {
      TnTThreadPool::ThreadPool tp;

      std::thread::id other;

      CallableNoArgs c;
      tp.submit(c);
      tp.finishAllJobs();
   }

   TEST(TnTThreadPoolTests, submitWithArgs_Lambda) {
      TnTThreadPool::ThreadPool tp;

      std::thread::id other;

      const auto lambdaWithArgs = [](std::thread::id& o) { o = std::this_thread::get_id(); };

      tp.submit(lambdaWithArgs, other);

      tp.finishAllJobs();
      ASSERT_NE(DEFAULT_THREAD_ID, other);
      ASSERT_NE(MAIN_THREAD_ID, other);
   }

   TEST(TnTThreadPoolTests, submitWithArgs_Function) {
      TnTThreadPool::ThreadPool tp;

      std::thread::id other;

      tp.submit(functionWithArgs, other);

      tp.finishAllJobs();
      ASSERT_NE(DEFAULT_THREAD_ID, other);
      ASSERT_NE(MAIN_THREAD_ID, other);
   }

   TEST(TnTThreadPoolTests, submitWithArgs_Callable) {
      TnTThreadPool::ThreadPool tp;

      std::thread::id other;

      CallableWithArgs c;
      tp.submit(c, other);

      tp.finishAllJobs();
      ASSERT_NE(DEFAULT_THREAD_ID, other);
      ASSERT_NE(MAIN_THREAD_ID, other);
   }

   TEST(TnTThreadPoolTests, submitWaitable_Lambda) {
      TnTThreadPool::ThreadPool tp;

      auto lambda = [] { std::this_thread::sleep_for(100ms); };
      auto start = std::chrono::high_resolution_clock::now();
      auto waitable = tp.submitWaitable(lambda);

      auto result = waitable.wait_for(5000ms);
      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_GE(end - start, 100ms);
      ASSERT_NE(std::future_status::timeout, result);
   }

   TEST(TnTThreadPoolTests, submitWaitable_Function) {
      TnTThreadPool::ThreadPool tp;

      auto start = std::chrono::high_resolution_clock::now();
      auto waitable = tp.submitWaitable(functionWait);

      auto result = waitable.wait_for(5000ms);
      auto end = std::chrono::high_resolution_clock::now();

      ASSERT_GE(end - start, 100ms);
      ASSERT_NE(std::future_status::timeout, result);
   }

   TEST(TnTThreadPoolTests, submitWaitable_Callable) {
      TnTThreadPool::ThreadPool tp;

      CallableWait c;
      auto         start = std::chrono::high_resolution_clock::now();
      auto         waitable = tp.submitWaitable(c);

      auto result = waitable.wait_for(5000ms);
      auto end = std::chrono::high_resolution_clock::now();

      ASSERT_GE(end - start, 100ms);
      ASSERT_NE(std::future_status::timeout, result);
   }

   TEST(TnTThreadPoolTests, submitForReturn_Lambda) {
      TnTThreadPool::ThreadPool tp;

      auto lambda = [] { return std::this_thread::get_id(); };
      auto waitable = tp.submitForReturn<std::thread::id>(lambda);

      auto result = waitable.wait_for(5000ms);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   TEST(TnTThreadPoolTests, submitForReturn_Function) {
      TnTThreadPool::ThreadPool tp;

      auto waitable = tp.submitForReturn<std::thread::id>(functionReturnValue);

      auto result = waitable.wait_for(5000ms);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   TEST(TnTThreadPoolTests, submitForReturn_Callable) {
      TnTThreadPool::ThreadPool tp;

      CallableReturnValue c;
      auto                waitable = tp.submitForReturn<std::thread::id>(c);

      auto result = waitable.wait_for(5000ms);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   TEST(TnTThreadPoolTests, submitForReturn_With_Args_Lambda) {
      TnTThreadPool::ThreadPool tp;

      auto value = 125;

      auto lambda = [](int x) { return x * x; };
      auto waitable = tp.submitForReturn<int>(lambda, value);

      auto result = waitable.wait_for(5000ms);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   TEST(TnTThreadPoolTests, submitForReturn_With_Args_Function) {
      TnTThreadPool::ThreadPool tp;

      auto value = 125;

      auto waitable = tp.submitForReturn<int>(functionReturnValueWithArgs, value);

      auto result = waitable.wait_for(5000ms);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   TEST(TnTThreadPoolTests, submitForReturn_With_Args_Callable) {
      TnTThreadPool::ThreadPool tp;

      auto value = 125;

      CallableReturnValueWithArgs c;
      auto                        waitable = tp.submitForReturn<int>(c, value);

      auto result = waitable.wait_for(5000ms);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   extern std::size_t getThreadCountTest();
   TEST(TnTThreadPoolTests, Get_Thread_Pool_Count_From_Different_File) {
      TnTThreadPool::ThreadPool tp;
      tp.setThreadCount(2);

      ASSERT_EQ(2, getThreadCountTest());

      tp.setThreadCount(0);
   }

}   // namespace TEST_NAMESPACE