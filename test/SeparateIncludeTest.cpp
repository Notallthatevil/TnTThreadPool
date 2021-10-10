#include <TnTThreadPool.h>


namespace Concurrency {
   std::size_t getThreadCountTest() {
      TnTThreadPool::ThreadPool tp;

      return tp.getThreadCount();
   }
}