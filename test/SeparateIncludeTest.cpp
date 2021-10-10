#include <TnTThreadPool.h>


namespace Concurrency {
   std::size_t getThreadCountTest() {
      TnTThreadPool::TnTThreadPool tp;

      return tp.getThreadCount();
   }
}