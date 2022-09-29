#include <TnTThreadPool.h>
#include <array>
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <numeric>
#include <thread>

using namespace std::chrono_literals;

namespace Concurrency {

   const auto     MAIN_THREAD_ID     = std::this_thread::get_id();
   constexpr auto DEFAULT_STALL_TIME = 10ms;

   void functionNoArgs() {
      ASSERT_NE(MAIN_THREAD_ID, std::this_thread::get_id());
   }
   void functionWithArgs(std::thread::id* id) {
      *id = std::this_thread::get_id();
   }
   void functionWait() {
      std::this_thread::sleep_for(DEFAULT_STALL_TIME);
   }
   decltype(std::this_thread::get_id()) functionReturnValue() {
      return std::this_thread::get_id();
   }
   int functionReturnValueWithArgs(int x) {
      return x * x;
   }

   struct CallableNoArgs {
      void operator()() const { ASSERT_NE(MAIN_THREAD_ID, std::this_thread::get_id()); }   // namespace TEST_NAMESPACE
   };

   struct CallableWithArgs {
      void operator()(std::thread::id* id) const { *id = std::this_thread::get_id(); }
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

   struct TrivialType {
      int i;

     private:
      int j;
   };

   struct TrivialTypeNonCopyable {
      int i;
      TrivialTypeNonCopyable() = default;

     private:
      int j;

      TrivialTypeNonCopyable(const TrivialTypeNonCopyable&)            = delete;
      TrivialTypeNonCopyable& operator=(const TrivialTypeNonCopyable&) = delete;
   };

   /* Submit_NoArgs */
   TEST(Submit_NoArgs, Lamda) {
      TnT::TnTThreadPool tp;

      auto lambda = [] { ASSERT_NE(MAIN_THREAD_ID, std::this_thread::get_id()); };
      tp.submit(lambda);
   }

   TEST(Submit_NoArgs, Lamda_Inline) {
      TnT::TnTThreadPool tp;

      tp.submit([] { ASSERT_NE(MAIN_THREAD_ID, std::this_thread::get_id()); });
   }

   TEST(Submit_NoArgs, Function) {
      TnT::TnTThreadPool tp;

      tp.submit(functionNoArgs);
   }

   TEST(Submit_NoArgs, ClassWithCallable) {
      TnT::TnTThreadPool tp;

      CallableNoArgs callable;
      tp.submit(callable);
   }

   TEST(Submit_NoArgs, ClassWithCallable_InLine) {
      TnT::TnTThreadPool tp;
      tp.submit(CallableNoArgs{});
   }

   ///* Submit_WithArgs */
   TEST(Submit_WithArgs, Lambda) {
      std::thread::id arg;
      {
         TnT::TnTThreadPool tp;

         const auto lambdaWithArgs = [](std::thread::id* o) { *o = std::this_thread::get_id(); };

         tp.submit(lambdaWithArgs, &arg);
      }
      ASSERT_NE(MAIN_THREAD_ID, arg);
      std::thread::id defaultConstructed;
      ASSERT_NE(defaultConstructed, arg);
   }

   TEST(Submit_WithArgs, Lambda_InLine) {
      std::thread::id arg;

      {
         TnT::TnTThreadPool tp;
         tp.submit([](std::thread::id* o) { *o = std::this_thread::get_id(); }, &arg);
      }
      ASSERT_NE(MAIN_THREAD_ID, arg);
      std::thread::id defaultConstructed;
      ASSERT_NE(defaultConstructed, arg);
   }

   TEST(Submit_WithArgs, Function) {
      std::thread::id arg;

      {
         TnT::TnTThreadPool tp;
         tp.submit(functionWithArgs, &arg);
      }
      ASSERT_NE(MAIN_THREAD_ID, arg);
      std::thread::id defaultConstructed;
      ASSERT_NE(defaultConstructed, arg);
   }

   TEST(Submit_WithArgs, ClassWithCallable) {
      std::thread::id arg;

      {

         TnT::TnTThreadPool tp;
         CallableWithArgs   callable;
         tp.submit(callable, &arg);
      }
      ASSERT_NE(MAIN_THREAD_ID, arg);
      std::thread::id defaultConstructed;
      ASSERT_NE(defaultConstructed, arg);
   }

   TEST(Submit_WithArgs, ClassWithCallable_InLine) {
      std::thread::id arg;

      {
         TnT::TnTThreadPool tp;
         tp.submit(CallableWithArgs{}, &arg);
      }
      ASSERT_NE(MAIN_THREAD_ID, arg);
      std::thread::id defaultConstructed;
      ASSERT_NE(defaultConstructed, arg);
   }

   TEST(Submit_WithArgs, PassingNonTrivialTypeByValue_Lambda) {
      std::int32_t value  = 123;
      auto         lambda = [&value](std::int32_t i) { ASSERT_EQ(value, i); };

      TnT::TnTThreadPool tp;
      tp.submit(lambda, value);
   }

   TEST(Submit_WithArgs, PassingNonTrivialTypeByLValueReference_Lambda) {
      const std::int32_t initialValue = 123;
      std::int32_t       value        = initialValue;
      auto               lambda       = [](std::int32_t* i) { *i = *i * *i; };

      {

         TnT::TnTThreadPool tp;
         tp.submit(lambda, &value);
      }

      ASSERT_EQ(initialValue * initialValue, value);
   }

   TEST(Submit_WithArgs, PassingTrivialTypeByValue_Lambda) {
      TrivialType type{};
      type.i      = 123;
      auto lambda = [&type](TrivialType t) { ASSERT_EQ(type.i, t.i); };

      TnT::TnTThreadPool tp;

      tp.submit(lambda, type);
   }

   TEST(Submit_WithArgs, PassingTrivialTypeByPointer_Lambda) {
      const std::int32_t initialValue = 123;
      TrivialType        type{};
      type.i      = initialValue;
      auto lambda = [&type](TrivialType* t) { (*t).i = (*t).i * (*t).i; };

      {
         TnT::TnTThreadPool tp;
         tp.submit(lambda, &type);
      }

      ASSERT_EQ(initialValue * initialValue, type.i);
   }

   TEST(Submit_WithArgs, PassingNonCopyableTrivialTypeByLValueReference_Lambda) {
      const std::int32_t     initialValue = 123;
      TrivialTypeNonCopyable type{};
      type.i      = initialValue;
      auto lambda = [&type](TrivialTypeNonCopyable* t) { (*t).i = (*t).i * (*t).i; };

      {
         TnT::TnTThreadPool tp;
         tp.submit(lambda, &type);
      }

      ASSERT_EQ(initialValue * initialValue, type.i);
   }

   ///* SubmitWaitable */
   TEST(SubmitWaitable, Lamda) {
      auto lambda = [] { std::this_thread::sleep_for(DEFAULT_STALL_TIME); };

      auto start = std::chrono::high_resolution_clock::now();

      TnT::TnTThreadPool tp;

      auto waitable = tp.submitWaitable(lambda);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_GE(end - start, DEFAULT_STALL_TIME);
      ASSERT_NE(std::future_status::timeout, result);
   }

   TEST(SubmitWaitable, Lamda_Inline) {

      auto start = std::chrono::high_resolution_clock::now();

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitWaitable([] { std::this_thread::sleep_for(DEFAULT_STALL_TIME); });

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_GE(end - start, DEFAULT_STALL_TIME);
      ASSERT_NE(std::future_status::timeout, result);
   }

   TEST(SubmitWaitable, Function) {

      auto start = std::chrono::high_resolution_clock::now();

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitWaitable(functionWait);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_GE(end - start, DEFAULT_STALL_TIME);
      ASSERT_NE(std::future_status::timeout, result);
   }

   TEST(SubmitWaitable, ClassWithCallable) {

      CallableWait callable;

      auto start = std::chrono::high_resolution_clock::now();

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitWaitable(callable);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_GE(end - start, DEFAULT_STALL_TIME);
      ASSERT_NE(std::future_status::timeout, result);
   }

   TEST(SubmitWaitable, ClassWithCallable_InLine) {
      auto start = std::chrono::high_resolution_clock::now();

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitWaitable(CallableWait{});

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_GE(end - start, DEFAULT_STALL_TIME);
      ASSERT_NE(std::future_status::timeout, result);
   }

   /* SubmitForReturn_NoArgs */
   TEST(SubmitForReturn_NoArgs, Lambda) {
      auto lambda = [] { return std::this_thread::get_id(); };

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitForReturn<std::thread::id>(lambda);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   TEST(SubmitForReturn_NoArgs, Lambda_InLine) {

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitForReturn<std::thread::id>([] { return std::this_thread::get_id(); });

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   TEST(SubmitForReturn_NoArgs, Function) {
      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitForReturn<std::thread::id>(functionReturnValue);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   TEST(SubmitForReturn_NoArgs, Callable) {
      CallableReturnValue callable;

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitForReturn<std::thread::id>(callable);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   TEST(SubmitForReturn_NoArgs, Callable_InLine) {

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitForReturn<std::thread::id>(CallableReturnValue{});

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_NE(MAIN_THREAD_ID, waitable.get());
   }

   /* SubmitForReturn_WithArgs */
   TEST(SubmitForReturn_WithArgs, Lambda) {
      int  value  = 125;
      auto lambda = [](int x) { return x * x; };

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitForReturn<int>(lambda, value);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   TEST(SubmitForReturn_WithArgs, Lambda_InLine) {
      int value = 125;

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitForReturn<int>([](int x) { return x * x; }, value);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   TEST(SubmitForReturn_WithArgs, Function) {
      int value = 125;

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitForReturn<int>(functionReturnValueWithArgs, value);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   TEST(SubmitForReturn_WithArgs, Callable) {
      int                         value = 125;
      CallableReturnValueWithArgs callable;

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitForReturn<int>(callable, value);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   TEST(SubmitForReturn_WithArgs, Callable_InLine) {
      int value = 125;

      TnT::TnTThreadPool tp;
      auto               waitable = tp.submitForReturn<int>(CallableReturnValueWithArgs{}, value);

      auto result = waitable.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, result);
      ASSERT_EQ(value * value, waitable.get());
   }

   /* Pause */
   TEST(Pause, PauseAndResume) {
      auto value  = 1;
      auto lambda = [](int i) { return i * i; };

      TnT::TnTThreadPool tp;

      auto waitable1 = tp.submitWaitable(lambda, value);
      auto status1   = waitable1.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_NE(std::future_status::timeout, status1);

      tp.pause();
      auto value2        = 2;
      auto waitable2     = tp.submitWaitable(lambda, value2);
      auto statusTimeout = waitable2.wait_for(DEFAULT_STALL_TIME * 5);
      tp.resume();
      auto statusNoTimeout = waitable2.wait_for(DEFAULT_STALL_TIME * 5);

      ASSERT_EQ(std::future_status::timeout, statusTimeout) << "Was apparently ready?";
      ASSERT_NE(std::future_status::timeout, statusNoTimeout) << "Apparently timed out?";
   }

   /* Reset */
   TEST(Reset, ResetTo1ThreadThenBackToFull) {
      std::atomic_int counter    = 0;
      auto            iterations = 150;
      auto            lambda     = [&counter]() {
         std::this_thread::sleep_for(DEFAULT_STALL_TIME);
         counter++;
      };

      TnT::TnTThreadPool tp;

      tp.reset(1);

      auto start = std::chrono::high_resolution_clock::now();

      for(auto i = 0; i < iterations; ++i) {
         tp.submit(lambda);
      }
      tp.finishAllJobs();

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_EQ(iterations, counter);
      ASSERT_LT(DEFAULT_STALL_TIME * iterations, end - start);
      counter = 0;

      tp.reset();

      start = std::chrono::high_resolution_clock::now();

      for(auto i = 0; i < iterations; ++i) {
         tp.submit(lambda);
      }
      tp.finishAllJobs();

      end = std::chrono::high_resolution_clock::now();

      ASSERT_EQ(iterations, counter);
      ASSERT_GT(DEFAULT_STALL_TIME * iterations, end - start);
   }

   /* SetThreadCount */
   TEST(SetThreadCount, SetTo1ThenBackToFull) {
      std::atomic_int counter    = 0;
      auto            iterations = 150;
      auto            lambda     = [&counter]() {
         std::this_thread::sleep_for(DEFAULT_STALL_TIME);
         counter++;
      };

      TnT::TnTThreadPool tp;
      tp.setThreadCount(1);

      auto start = std::chrono::high_resolution_clock::now();

      for(auto i = 0; i < iterations; ++i) {
         tp.submit(lambda);
      }
      tp.finishAllJobs();

      auto end = std::chrono::high_resolution_clock::now();
      ASSERT_EQ(iterations, counter);
      ASSERT_LT(DEFAULT_STALL_TIME * iterations, end - start);
      counter = 0;

      tp.reset();

      start = std::chrono::high_resolution_clock::now();

      for(auto i = 0; i < iterations; ++i) {
         tp.submit(lambda);
      }
      tp.finishAllJobs();

      end = std::chrono::high_resolution_clock::now();

      ASSERT_EQ(iterations, counter);
      ASSERT_GT(DEFAULT_STALL_TIME * iterations, end - start);
   }

   /* Stress Tests */
   TEST(StressTest, AddLargeNumberOfItems) {
      std::mutex         mutex;
      TnT::TnTThreadPool tp;

      auto        iterations = 50000;
      std::size_t value{ 0 };

      auto lambda = [&mutex, &value] {
         std::this_thread::yield();

         std::scoped_lock lock{ mutex };
         value++;
      };

      for(auto i = 0; i < iterations; ++i) {
         tp.submit(lambda);
      }

      tp.finishAllJobs();

      ASSERT_EQ(iterations, value);
   }

   TEST(StressTest, AddLargeNumberOfItems_WithPause) {
      std::mutex mutex;

      TnT::TnTThreadPool tp;

      auto        iterations = 50000;
      std::size_t value{ 0 };

      auto lambda = [&mutex, &value] {
         std::this_thread::yield();

         std::scoped_lock lock{ mutex };
         value++;
      };

      for(auto i = 0; i < iterations; ++i) {
         tp.submit(lambda);
      }

      tp.pause();
      std::this_thread::sleep_for(DEFAULT_STALL_TIME * 5);
      tp.resume();

      tp.finishAllJobs();

      ASSERT_EQ(iterations, value);
   }

   TEST(StressTest, AddingItemsToVector) {
      std::mutex mutex;
      auto       iterations  = 5000;
      auto       strLoopSize = 100;

      TnT::TnTThreadPool tp;

      std::vector<std::string> vec;

      auto lambda = [&mutex, &vec, strLoopSize]() {
         std::stringstream ss;
         for(auto i = 0; i < strLoopSize; ++i) {
            ss << i;
         }

         {
            std::scoped_lock lock{ mutex };
            vec.emplace_back(ss.str());
         }
      };

      for(auto i = 0; i < iterations; ++i) {
         tp.submit(lambda);
      }

      tp.finishAllJobs();

      ASSERT_EQ(iterations, vec.size());

      std::stringstream expected;
      for(auto i = 0; i < strLoopSize; ++i) {
         expected << i;
      }

      auto expectedStr = expected.str();
      for(auto i = 0; i < vec.size(); ++i) {
         ASSERT_EQ(expectedStr, vec[i]) << " Failed at index " << i;
      }
   }

   TEST(StressTest, QueueingMultipleWithConstArg) {
      std::mutex mutex;

      auto iterations = 500;

      TnT::TnTThreadPool tp;

      std::array nums{ 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97 };
      auto       expected = std::accumulate(nums.begin(), nums.end(), 0);

      for(auto i = 0; i < iterations; ++i) {

         auto accumulator = 0;

         auto lambda = [&mutex, &accumulator](const std::int32_t num) {
            std::scoped_lock lock{ mutex };
            accumulator += num;
         };

         for(auto num: nums) {
            tp.submit(lambda, num);
         }

         tp.finishAllJobs();

         ASSERT_EQ(expected, accumulator);
      }
   }

   TEST(StressTest, QueueingMultipleWithNoneConstArg) {
      std::mutex mutex;

      auto iterations = 500;

      std::array nums{ 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97 };
      auto       expected = std::accumulate(nums.begin(), nums.end(), 0);

      for(auto i = 0; i < iterations; ++i) {

         auto accumulator = 0;

         auto lambda = [&mutex, &accumulator](std::int32_t num) {
            std::scoped_lock lock{ mutex };
            accumulator += num;
         };

         TnT::TnTThreadPool tp;

         for(auto num: nums) {
            tp.submit(lambda, num);
         }

         tp.finishAllJobs();

         ASSERT_EQ(expected, accumulator);
      }
   }

   /* For Each*/

   TEST(ForEachTest, NonTrivialForEach) {
      std::mutex mutex;

      std::array nums{ 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97 };
      auto       expected = std::accumulate(nums.begin(), nums.end(), 0);

      TnT::TnTThreadPool tp;

      std::int32_t accumulator{ 0 };
      tp.forEach(
          [&mutex, &accumulator](std::int32_t num) {
             std::scoped_lock lock{ mutex };
             accumulator += num;
          },
          nums);

      ASSERT_EQ(expected, accumulator);
   }

   /* For Each Indexed */
   TEST(ForEachIndexedTest, NonTrivialForEach) {
      std::mutex mutex;
      std::array nums{ 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97 };
      auto       expected = std::accumulate(nums.begin(), nums.end(), 0);

      TnT::TnTThreadPool tp;

      std::int32_t accumulator{ 0 };
      tp.forEachIndexed<std::int32_t>(
          [&mutex, &accumulator, &nums](auto index) {
             std::scoped_lock lock{ mutex };
             accumulator += nums[index];
          },
          0,
          static_cast<std::int32_t>(nums.size()));

      ASSERT_EQ(expected, accumulator);
   }

   TEST(ShutdownThreadPoolThenQueueJob, ShutdownThreadPoolThenQueueJobWithoutReset) {
      std::mutex mutex;

      auto        iterations = 50000;
      std::size_t value{ 0 };

      auto lambda = [&mutex, &value] {
         std::this_thread::yield();

         std::scoped_lock lock{ mutex };
         value++;
      };

      TnT::TnTThreadPool tp;

      for(auto i = 0; i < iterations; ++i) {
         tp.submit(lambda);
      }

      tp.shutdown();

      ASSERT_EQ(iterations, value);

      value = 0;

      auto statement = [&]() {
         for(auto i = 0; i < iterations; ++i) {
            tp.submit(lambda);
         }

         tp.finishAllJobs();

         ASSERT_EQ(iterations, value);
      };
      ASSERT_THROW(statement(), std::runtime_error);
   }

   TEST(ShutdownThreadPoolThenQueueJob, ShutdownThreadPoolThenQueueJobWithReset) {
      std::mutex mutex;

      auto        iterations = 50000;
      std::size_t value{ 0 };

      auto lambda = [&mutex, &value] {
         std::this_thread::yield();

         std::scoped_lock lock{ mutex };
         value++;
      };

      TnT::TnTThreadPool tp;

      for(auto i = 0; i < iterations; ++i) {
         tp.submit(lambda);
      }

      tp.shutdown();

      ASSERT_EQ(iterations, value);

      value = 0;

      tp.reset();
      for(auto i = 0; i < iterations; ++i) {
         tp.submit(lambda);
      }

      tp.finishAllJobs();

      ASSERT_EQ(iterations, value);
   }

}   // namespace Concurrency
