//
// Created by null on 20-2-3.
//

#ifndef PROJECT_UNALIGNED_CAST_HH
#define PROJECT_UNALIGNED_CAST_HH

#pragma once

#include <type_traits>

namespace data{
    template <typename T>
    struct unaligned {
        T raw;
        unaligned() = default;
        unaligned(T x) : raw(x) {}
        unaligned& operator=(const T& x) { raw = x; return *this; }
        operator T() const { return raw; }
    } __attribute__((packed));

    template <typename T, typename F>
    inline auto unaligned_cast(F* p) {
        return reinterpret_cast<unaligned<std::remove_pointer_t<T>>*>(p);
    }

    template <typename T, typename F>
    inline auto unaligned_cast(const F* p) {
        return reinterpret_cast<const unaligned<std::remove_pointer_t<T>>*>(p);
    }
}
#endif //PROJECT_UNALIGNED_CAST_HH
