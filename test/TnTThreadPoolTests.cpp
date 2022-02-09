#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <TnTThreadPool.h>

using namespace std::chrono_literals;

#pragma warning(push)
#pragma warning(disable: 6326) // Potential comparison of a constant with another constant

namespace Concurrency {

   const auto            MAIN_THREAD_ID = std::this_thread::get_id();
   constexpr auto DEFAULT_STALL_TIME = 10ms;

   void functionNoArgs() {
      ASSERT_NE(MAIN_THREAD_ID, std::this_thread::get_id());
   }
   void                                 functionWithArgs(std::thread::id& id) { id = std::this_thread::get_id(); }
   void                                 functionWait() { std::this_thread::sleep_for(DEFAULT_STALL_TIME); }
   decltype(std::this_thread::get_id()) functionReturnValue() { return std::this_thread::get_id(); }
   int                                  functionReturnValueWithArgs(int x) { return x * x; }

   struct CallableNoArgs {
      void operator()() const {
         ASSERT_NE(MAIN_THREAD_ID, std::this_thread::get_id());
      }   // namespace TEST_NAMESPACE
   };

   struct CallableWithArgs {
      void operator()(std::thread::id& id) const { id = std::this_thread::get_id(); }
   };
   struct CallableWait {
      void operator()() const { std::this_thread::sleep_for(DEFAULT_STALL_TIME); }
   };
   struct CallableReturnValue {
      auto operator()() const { return std::this_thread::get_id(); }
   };
   struct CallableReturnValueWithArgs {
      auto operator()(int x) const { return x * x; }
   };

   /* Initialization */
   TEST(Initialization, DefaultThreadCount) {
      ASSERT_EQ(std::thread::hardware_concurrency(), TnTThreadPool::getThreadCount());
   }

   /* Submit_NoArgs */
   TEST(Submit_NoArgs, Lamda) {
      auto lambda = [] {
         ASSERT_NE(MAIN_THREAD_ID, std::this_thread::get_id());
      };
      TnTThreadPool::submit(lambda);
      TnTThreadPool::finishAllJobs();
   }

   TEST(Submit_NoArgs, Lamda_Inline) {
      TnTThreadPool::submit([] {
         ASSERT_NE(MAIN_THREAD_ID, std::this_thread::get_id());
         });
      TnTThreadPool::finishAllJobs();
   }

   TEST(Submit_NoArgs, Function) {
      TnTThreadPool::submit(functionNoArgs);
      TnTThreadPool::finishAllJobs();
   }

   TEST(Submit_NoArgs, ClassWithCallable) {
      CallableNoArgs callable;
      TnTThreadPool::submit(callable);
      TnTThreadPool::finishAllJobs();
   }

   TEST(Submit_NoArgs, ClassWithCallable_InLine) {
      TnTThreadPool::submit(CallableNoArgs{});
      TnTThreadPool::finishAllJobs();
   }

   /* Submit_WithArgs */
   TEST(Submit_WithArgs, Lambda) {
      std::thread::id arg;
      const auto lambdaWithArgs = [](std::thread::id& o) { o = std::this_thread::get_id(); };

      TnTThreadPool::submit(lambdaWithArgs, arg);

      TnTThreadPool::finishAllJobs();
      ASSERT_NE(MAIN_THREAD_ID, arg);
   }

   TEST(Submit_WithArgs, Lambda_InLine) {
      std::thread::id arg;

      TnTThreadPool::submit([](std::thread::id& o) { o = std::this_thread::get_id(); }, arg);

      TnTThreadPool::finishAllJobs();
      ASSERT_NE(MAIN_THREAD_ID, arg);
   }

   TEST(Submit_WithArgs, Function) {
      std::thread::id arg;

      TnTThreadPool::submit(functionWithArgs, arg);

      TnTThreadPool::finishAllJobs();
      ASSERT_NE(MAIN_THREAD_ID, arg);
   }

   TEST(Submit_WithArgs, ClassWithCallable) {
      std::thread::id arg;

      CallableWithArgs callable;
      TnTThreadPool::submit(callable, arg);

      TnTThreadPool::finishAllJobs();
      ASSERT_NE(MAIN_THREAD_ID, arg);
   }

   TEST(Submit_WithArgs, ClassWithCallable_InLine) {
      std::thread::id arg;

      TnTThreadPool::submit(CallableWithArgs{}, arg);

      TnTThreadPool::finishAllJobs();
      ASSERT_NE(MAIN_THREAD_ID, arg);
   }

   /* SubmitWaitable */
   TEST(SubmitWaitable, Lamda) {
      auto lambda = [] {
         std::this_thread::sleep_for(DEFAULT_STALL_TIME);
      };

      auto start = std::chrono::high_resolution_clock::now();
      auto waitable = TnTThreadPool::submitWaitable(lambda);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_GE(end - start, DEFAULT_STALL_TIME);
      ASSERT_NE(std::future_status::timeout, result);
   }

   TEST(SubmitWaitable, Lamda_Inline) {

      auto start = std::chrono::high_resolution_clock::now();
      auto waitable = TnTThreadPool::submitWaitable([] {
         std::this_thread::sleep_for(DEFAULT_STALL_TIME);
         });

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_GE(end - start, DEFAULT_STALL_TIME);
      ASSERT_NE(std::future_status::timeout, result);
   }

   TEST(SubmitWaitable, Function) {

      auto start = std::chrono::high_resolution_clock::now();
      auto waitable = TnTThreadPool::submitWaitable(functionWait);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_GE(end - start, DEFAULT_STALL_TIME);
      ASSERT_NE(std::future_status::timeout, result);
   }

   TEST(SubmitWaitable, ClassWithCallable) {

      CallableWait callable;

      auto start = std::chrono::high_resolution_clock::now();
      auto waitable = TnTThreadPool::submitWaitable(callable);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_GE(end - start, DEFAULT_STALL_TIME);
      ASSERT_NE(std::future_status::timeout, result);
   }

   TEST(SubmitWaitable, ClassWithCallable_InLine) {
      auto start = std::chrono::high_resolution_clock::now();
      auto waitable = TnTThreadPool::submitWaitable(CallableWait{});

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_GE(end - start, DEFAULT_STALL_TIME);
      ASSERT_NE(std::future_status::timeout, result);
   }

   /* SubmitForReturn_NoArgs */
   TEST(SubmitForReturn_NoArgs, Lambda) {
      auto lambda = [] { return std::this_thread::get_id(); };
      auto waitable = TnTThreadPool::submitForReturn<std::thread::id>(lambda);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   TEST(SubmitForReturn_NoArgs, Lambda_InLine) {
      auto waitable = TnTThreadPool::submitForReturn<std::thread::id>([] { return std::this_thread::get_id(); });

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   TEST(SubmitForReturn_NoArgs, Function) {
      auto waitable = TnTThreadPool::submitForReturn<std::thread::id>(functionReturnValue);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   TEST(SubmitForReturn_NoArgs, Callable) {
      CallableReturnValue callable;
      auto waitable = TnTThreadPool::submitForReturn<std::thread::id>(callable);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   TEST(SubmitForReturn_NoArgs, Callable_InLine) {
      auto waitable = TnTThreadPool::submitForReturn<std::thread::id>(CallableReturnValue{});

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   /* SubmitForReturn_WithArgs */
   TEST(SubmitForReturn_WithArgs, Lambda) {
      int value = 125;
      auto lambda = [](int x) { return x * x; };
      auto waitable = TnTThreadPool::submitForReturn<int>(lambda, value);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   TEST(SubmitForReturn_WithArgs, Lambda_InLine) {
      int value = 125;
      auto waitable = TnTThreadPool::submitForReturn<int>([](int x) { return x * x; }, value);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   TEST(SubmitForReturn_WithArgs, Function) {
      int value = 125;
      auto waitable = TnTThreadPool::submitForReturn<int>(functionReturnValueWithArgs, value);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   TEST(SubmitForReturn_WithArgs, Callable) {
      int value = 125;
      CallableReturnValueWithArgs callable;
      auto waitable = TnTThreadPool::submitForReturn<int>(callable, value);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   TEST(SubmitForReturn_WithArgs, Callable_InLine) {
      int value = 125;
      auto waitable = TnTThreadPool::submitForReturn<int>(CallableReturnValueWithArgs{}, value);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }


   /* Stress Tests */
   TEST(StressTest, AddLargeNumberOfItems) {
      std::mutex mutex;

      auto iterations = 50000;
      std::size_t value{ 0 };

      auto lambda = [&mutex, &value] {
         std::this_thread::yield();

         std::scoped_lock lock{ mutex };
         value++;
      };

      for (auto i = 0; i < iterations; ++i) {
         TnTThreadPool::submit(lambda);
      }

      TnTThreadPool::finishAllJobs();

      ASSERT_EQ(iterations, value);
   }

   TEST(StressTest, AddingItemsToVector) {
      std::mutex mutex;
      auto iterations = 5000;
      auto strLoopSize = 100;

      std::vector<std::string> vec;

      auto lambda = [&mutex, &vec, strLoopSize]() {
         std::stringstream ss;
         for (auto i = 0; i < strLoopSize; ++i) {
            ss << i;
         }

         {
            std::scoped_lock lock{ mutex };
            vec.emplace_back(ss.str());
         }
      };

      for (auto i = 0; i < iterations; ++i) {
         TnTThreadPool::submit(lambda);
      }

      TnTThreadPool::finishAllJobs();


      ASSERT_EQ(iterations, vec.size());

      std::stringstream expected;
      for (auto i = 0; i < strLoopSize; ++i) {
         expected << i;
      }

      auto expectedStr = expected.str();
      for (auto i = 0; i < vec.size(); ++i) {
         ASSERT_EQ(expectedStr, vec[i]) << " Failed at index " << i;
      }
   }


   //EST(TnTThreadPoolTests, Submit1000JobsAndWait) {
   //  TnTThreadPool::ThreadPool tp;
   //
   //  std::atomic_int64_t val{ 0 };
   //
   //  constexpr auto numToRun = 1000;
   //
   //  auto job = [&val]() {
   //     ++val;
   //  };
   //
   //  for (auto i = 0; i < numToRun; ++i) {
   //     tp.submit(job);
   //  }
   //  ASSERT_NE(numToRun, val.load());
   //
   //  tp.finishAllJobs();
   //
   //  ASSERT_EQ(numToRun, val.load());
   //
   //
   //
   //EST(TnTThreadPoolTests, Submit1000MoreComplexJobsAndWait) {
   //  TnTThreadPool::ThreadPool tp;
   //
   //  struct S {
   //     std::uint64_t i{ 0 };
   //  };
   //
   //
   //  std::vector<std::unique_ptr<S>> vec;
   //
   //  auto job = [](S* s) {
   //     s->i = s->i * s->i;
   //  };
   //
   //  for (std::size_t i = 0; i < vec.size(); ++i) {
   //     vec.emplace_back(new S{ i });
   //     tp.submit(job, vec[i].get());
   //  }
   //
   //  tp.finishAllJobs();
   //
   //  for (std::size_t i = 0; i < vec.size(); ++i) {
   //     ASSERT_EQ(i * i, vec[i]->i);
   //  }*/
   //
   //
   // }
}   // namespace TEST_NAMESPACE

#pragma warning(pop)