/*
 * This file is open source software, licensed to you under the terms
 * of the Apache License, Version 2.0 (the "License").  See the NOTICE file
 * distributed with this work for additional information regarding copyright
 * ownership.  You may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
 * Copyright (C) 2017 ScyllaDB Ltd.
 */

#pragma once

#include <utility>
#include <type_traits>
#include <functional>

namespace common {

template <typename Signature>
class ncpy_func;

namespace internal {

class noncopyable_function_base {
private:
    noncopyable_function_base() = default;
    static constexpr size_t nr_direct = 32;
    union [[gnu::may_alias]] storage {
        char direct[nr_direct];
        void* indirect;
    };
    using move_type = void (*)(noncopyable_function_base* from, noncopyable_function_base* to);
    using destroy_type = void (*)(noncopyable_function_base* func);

    static void empty_move(noncopyable_function_base* from, noncopyable_function_base* to) {}
    static void empty_destroy(noncopyable_function_base* func) {}

    static void indirect_move(noncopyable_function_base* from, noncopyable_function_base* to) {
        using void_ptr = void*;
        new (&to->_storage.indirect) void_ptr(from->_storage.indirect);
    }

    template <size_t N>
    static void trivial_direct_move(noncopyable_function_base* from, noncopyable_function_base* to) {
        // Avoid including <algorithm> just for this
        for (unsigned i = 0; i != N; ++i) {
            to->_storage.direct[i] = from->_storage.direct[i];
        }
    }

    static void trivial_direct_destroy(noncopyable_function_base* func) {
    }

private:
    storage _storage;

    template <typename Signature>
    friend class common::ncpy_func;
};

}

/// A clone of \c std::function, but only invokes the move constructor
/// of the contained function.
template <typename Ret, typename... Args>
class ncpy_func<Ret (Args...)> : private internal::noncopyable_function_base {
    using call_type = Ret (*)(const ncpy_func* func, Args...);
    struct vtable {
        const call_type call;
        const move_type move;
        const destroy_type destroy;
    };
private:
    const vtable* _vtable;
private:
    static Ret empty_call(const ncpy_func* func, Args... args) {
        throw std::bad_function_call();
    }

    static constexpr vtable _s_empty_vtable = {empty_call, empty_move, empty_destroy};

    template <typename Func>
    struct direct_vtable_for {
        static Func* access(ncpy_func* func) { return reinterpret_cast<Func*>(func->_storage.direct); }
        static const Func* access(const ncpy_func* func) { return reinterpret_cast<const Func*>(func->_storage.direct); }
        static Func* access(noncopyable_function_base* func) { return access(static_cast<ncpy_func*>(func)); }
        static Ret call(const ncpy_func* func, Args... args) {
            return (*access(const_cast<ncpy_func*>(func)))(std::forward<Args>(args)...);
        }
        static void move(noncopyable_function_base* from, noncopyable_function_base* to) {
            new (access(to)) Func(std::move(*access(from)));
            destroy(from);
        }
        static constexpr move_type select_move_thunk() {
            bool can_trivially_move = std::is_trivially_move_constructible<Func>::value
                    && std::is_trivially_destructible<Func>::value;
            return can_trivially_move ? trivial_direct_move<sizeof(Func)> : move;
        }
        static void destroy(noncopyable_function_base* func) {
            access(func)->~Func();
        }
        static constexpr destroy_type select_destroy_thunk() {
            return std::is_trivially_destructible<Func>::value ? trivial_direct_destroy : destroy;
        }
        static void initialize(Func&& from, ncpy_func* to) {
            new (access(to)) Func(std::move(from));
        }
        static constexpr vtable make_vtable() { return { call, select_move_thunk(), select_destroy_thunk() }; }
        static const vtable s_vtable;
    };
    template <typename Func>
    struct indirect_vtable_for {
        static Func* access(ncpy_func* func) { return reinterpret_cast<Func*>(func->_storage.indirect); }
        static const Func* access(const ncpy_func* func) { return reinterpret_cast<const Func*>(func->_storage.indirect); }
        static Func* access(noncopyable_function_base* func) { return access(static_cast<ncpy_func*>(func)); }
        static Ret call(const ncpy_func* func, Args... args) {
            return (*access(const_cast<ncpy_func*>(func)))(std::forward<Args>(args)...);
        }
        static void destroy(noncopyable_function_base* func) {
            delete access(func);
        }
        static void initialize(Func&& from, ncpy_func* to) {
            to->_storage.indirect = new Func(std::move(from));
        }
        static constexpr vtable make_vtable() { return { call, indirect_move, destroy }; }
        static const vtable s_vtable;
    };
    template <typename Func, bool Direct = true>
    struct select_vtable_for : direct_vtable_for<Func> {};
    template <typename Func>
    struct select_vtable_for<Func, false> : indirect_vtable_for<Func> {};
    template <typename Func>
    static constexpr bool is_direct() {
        return sizeof(Func) <= nr_direct && alignof(Func) <= alignof(storage)
                && std::is_nothrow_move_constructible<Func>::value;
    }
    template <typename Func>
    struct vtable_for : select_vtable_for<Func, is_direct<Func>()> {};
public:
    ncpy_func() noexcept : _vtable(&_s_empty_vtable) {}
    template <typename Func>
    ncpy_func(Func func) {
        vtable_for<Func>::initialize(std::move(func), this);
        _vtable = &vtable_for<Func>::s_vtable;
    }
    template <typename Object, typename... AllButFirstArg>
    ncpy_func(Ret (Object::*member)(AllButFirstArg...)) : ncpy_func(std::mem_fn(member)) {}
    template <typename Object, typename... AllButFirstArg>
    ncpy_func(Ret (Object::*member)(AllButFirstArg...) const) : ncpy_func(std::mem_fn(member)) {}

    ~ncpy_func() {
        _vtable->destroy(this);
    }

    ncpy_func(const ncpy_func&) = delete;
    ncpy_func& operator=(const ncpy_func&) = delete;

    ncpy_func(ncpy_func&& x) noexcept : _vtable(std::exchange(x._vtable, &_s_empty_vtable)) {
        _vtable->move(&x, this);
    }

    ncpy_func& operator=(ncpy_func&& x) noexcept {
        if (this != &x) {
            this->~ncpy_func();
            new (this) ncpy_func(std::move(x));
        }
        return *this;
    }

    Ret operator()(Args... args) const {
        return _vtable->call(this, std::forward<Args>(args)...);
    }

    explicit operator bool() const {
        return _vtable != &_s_empty_vtable;
    }
};


template <typename Ret, typename... Args>
constexpr typename ncpy_func<Ret (Args...)>::vtable ncpy_func<Ret (Args...)>::_s_empty_vtable;

template <typename Ret, typename... Args>
template <typename Func>
const typename ncpy_func<Ret (Args...)>::vtable ncpy_func<Ret (Args...)>::direct_vtable_for<Func>::s_vtable
        = ncpy_func<Ret (Args...)>::direct_vtable_for<Func>::make_vtable();


template <typename Ret, typename... Args>
template <typename Func>
const typename ncpy_func<Ret (Args...)>::vtable ncpy_func<Ret (Args...)>::indirect_vtable_for<Func>::s_vtable
        = ncpy_func<Ret (Args...)>::indirect_vtable_for<Func>::make_vtable();

}

