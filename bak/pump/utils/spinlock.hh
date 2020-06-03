//
// Created by null on 20-1-22.
//

#ifndef PROJECT_SPINLOCK_HH
#define PROJECT_SPINLOCK_HH

#include <atomic>
#include <assert.h>
#include <xmmintrin.h>

namespace common{
    class spinlock {
        std::atomic<bool> _busy = { false };
    public:
        spinlock() = default;
        spinlock(const spinlock&) = delete;
        ~spinlock() { assert(!_busy.load(std::memory_order_relaxed)); }
        void lock() noexcept {
            while (_busy.exchange(true, std::memory_order_acquire)) {
                _mm_pause();
            }
        }
        void unlock() noexcept {
            _busy.store(false, std::memory_order_release);
        }
    };
}
#endif //PROJECT_SPINLOCK_HH
