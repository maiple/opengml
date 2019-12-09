#pragma once

#ifdef PARALLEL_COMPILE
#include <mutex>
#include <ThreadPool.h>
#   define WRITE_LOCK(l) std::lock_guard<decltype(l)> lock(l);
#   define READ_LOCK(l)  std::lock_guard<decltype(l)> lock(l);
#else
#   define READ_LOCK(lock) {}
#   define WRITE_LOCK(lock) {}
#endif
