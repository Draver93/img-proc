#ifndef MEMORY_PROFILER_H
#define MEMORY_PROFILER_H

#ifdef TRACK_MEMORY

#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <new>

static size_t g_TotalAllocated = 0, g_TotalFreed = 0;
static std::mutex g_AllocMutex;

inline void* operator new(std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    { std::lock_guard<std::mutex> lock(g_AllocMutex); ++g_TotalAllocated; }
    return ptr;
}
inline void* operator new[](std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    { std::lock_guard<std::mutex> lock(g_AllocMutex); ++g_TotalAllocated; }
    return ptr;
}

inline void operator delete(void* ptr) noexcept {
    if (ptr) { std::lock_guard<std::mutex> lock(g_AllocMutex); ++g_TotalFreed; std::free(ptr); }
}
inline void operator delete[](void* ptr) noexcept {
    if (ptr) { std::lock_guard<std::mutex> lock(g_AllocMutex); ++g_TotalFreed; std::free(ptr); }
}

inline void operator delete(void* ptr, std::size_t) noexcept {
    if (ptr) { std::lock_guard<std::mutex> lock(g_AllocMutex); ++g_TotalFreed; std::free(ptr); }
}
inline void operator delete[](void* ptr, std::size_t) noexcept {
    if (ptr) { std::lock_guard<std::mutex> lock(g_AllocMutex); ++g_TotalFreed; std::free(ptr); }
}

struct LeakChecker {
    ~LeakChecker() {
        std::lock_guard<std::mutex> lock(g_AllocMutex);
        std::cerr << "[Memory Usage Summary]\n"
                  << "  Total allocated: " << g_TotalAllocated << " times\n"
                  << "  Total freed:     " << g_TotalFreed << " times\n"
                  << "  Net allocated:   " << (g_TotalAllocated - g_TotalFreed) << " times\n";
    }
};

inline LeakChecker g_LeakChecker;


#endif //!TRACK_MEMORY

#endif //!MEMORY_PROFILER_H
