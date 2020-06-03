//
// Created by null on 20-2-6.
//

#ifndef PROJECT_G_DEFINE_HH
#define PROJECT_G_DEFINE_HH

//#define ALWAYS_INLINE [[gnu::always_inline]][[gnu::hot]] inline
//#define ALWAYS_INLINE __attribute__((always_inline)) __attribute__((hot)) inline
#define ALWAYS_INLINE
template <typename T>
using sp=std::shared_ptr<T>;
#endif //PROJECT_G_DEFINE_HH
