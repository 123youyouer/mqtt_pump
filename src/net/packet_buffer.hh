//
// Created by null on 20-1-6.
//

#ifndef PROJECT_packet_bufFER_HH
#define PROJECT_packet_bufFER_HH
#include <atomic>
#include <assert.h>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <unistd.h>

#include <sys/mman.h>

namespace net {
    template<typename Size>
    class linear_ringbuffer_ {
    public:
        typedef unsigned char value_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef value_type* iterator;
        typedef const value_type* const_iterator;
        typedef std::ptrdiff_t difference_type;
        typedef std::size_t size_type;

        struct delayed_init {};

        // "640KiB should be enough for everyone."
        //   - Not Bill Gates.
        explicit linear_ringbuffer_(size_t minsize = 640*1024);
        ~linear_ringbuffer_();

        // Noexcept initialization interface, see description above.
        explicit linear_ringbuffer_(const delayed_init) noexcept;
        int initialize(size_t minsize) noexcept;

        void commit(size_t n) noexcept;
        void consume(size_t n) noexcept;
        iterator read_head() noexcept;
        iterator write_head() noexcept;
        void clear() noexcept;

        bool empty() const noexcept;
        size_t size() const noexcept;
        size_t capacity() const noexcept;
        size_t free_size() const noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;

        // Plumbing

        linear_ringbuffer_(linear_ringbuffer_&& other) noexcept;
        linear_ringbuffer_& operator=(linear_ringbuffer_&& other) noexcept;
        void swap(linear_ringbuffer_& other) noexcept;

        linear_ringbuffer_(const linear_ringbuffer_&) = delete;
        linear_ringbuffer_& operator=(const linear_ringbuffer_&) = delete;

    private:
        unsigned char* buffer_;
        size_t capacity_;
        size_t head_;
        size_t tail_;
        Size size_;
    };


    template<typename Count>
    void swap(
            linear_ringbuffer_<Count>& lhs,
            linear_ringbuffer_<Count>& rhs) noexcept;


    struct initialization_error : public std::runtime_error
    {
        initialization_error(int error);
        int error;
    };


    using linear_ringbuffer_st = linear_ringbuffer_<int64_t>;
    using linear_ringbuffer_mt = linear_ringbuffer_<std::atomic<int64_t>>;
    using linear_ringbuffer = linear_ringbuffer_mt;


// Implementation.

    template<typename T>
    void linear_ringbuffer_<T>::commit(size_t n) noexcept {
        assert(n <= (capacity_-size_));
        tail_ = (tail_ + n) % capacity_;
        size_ += n;
    }


    template<typename T>
    void linear_ringbuffer_<T>::consume(size_t n) noexcept {
        assert(n <= size_);
        head_ = (head_ + n) % capacity_;
        size_ -= n;
    }


    template<typename T>
    void linear_ringbuffer_<T>::clear() noexcept {
        tail_ = head_ = size_ = 0;
    }


    template<typename T>
    size_t linear_ringbuffer_<T>::size() const noexcept {
        return size_;
    }


    template<typename T>
    bool linear_ringbuffer_<T>::empty() const noexcept {
        return size_ == 0;
    }


    template<typename T>
    size_t linear_ringbuffer_<T>::capacity() const noexcept {
        return capacity_;
    }


    template<typename T>
    size_t linear_ringbuffer_<T>::free_size() const noexcept {
        return capacity_ - size_;
    }


    template<typename T>
    auto linear_ringbuffer_<T>::cbegin() const noexcept -> const_iterator
    {
        return buffer_ + head_;
    }


    template<typename T>
    auto linear_ringbuffer_<T>::begin() const noexcept -> const_iterator
    {
        return cbegin();
    }


    template<typename T>
    auto linear_ringbuffer_<T>::read_head() noexcept -> iterator
    {
        return buffer_ + head_;
    }


    template<typename T>
    auto linear_ringbuffer_<T>::cend() const noexcept -> const_iterator
    {
        // Fix up `end` if needed so that [begin, end) is always a
        // valid range.
        return head_ < tail_ ?
               buffer_ + tail_ :
               buffer_ + tail_ + capacity_;
    }


    template<typename T>
    auto linear_ringbuffer_<T>::end() const noexcept -> const_iterator
    {
        return cend();
    }


    template<typename T>
    auto linear_ringbuffer_<T>::write_head() noexcept -> iterator
    {
        return buffer_ + tail_;
    }


    template<typename T>
    linear_ringbuffer_<T>::linear_ringbuffer_(const delayed_init) noexcept
            : buffer_(nullptr)
            , capacity_(0)
            , head_(0)
            , tail_(0)
            , size_(0)
    {}


    template<typename T>
    linear_ringbuffer_<T>::linear_ringbuffer_(size_t minsize)
            : buffer_(nullptr)
            , capacity_(0)
            , head_(0)
            , tail_(0)
            , size_(0)
    {
        int res = this->initialize(minsize);
        if (res == -1) {
            throw initialization_error {errno};
        }
    }


    template<typename T>
    linear_ringbuffer_<T>::linear_ringbuffer_(linear_ringbuffer_&& other) noexcept
    {
        linear_ringbuffer_ tmp(delayed_init {});
        tmp.swap(other);
        this->swap(tmp);
    }


    template<typename T>
    auto linear_ringbuffer_<T>::operator=(linear_ringbuffer_&& other) noexcept
    -> linear_ringbuffer_&
    {
        linear_ringbuffer_ tmp(delayed_init {});
        tmp.swap(other);
        this->swap(tmp);
        return *this;
    }


    template<typename T>
    int linear_ringbuffer_<T>::initialize(size_t minsize) noexcept
    {
#ifdef PAGESIZE
        static constexpr unsigned int PAGE_SIZE = PAGESIZE;
#else
        static const unsigned int PAGE_SIZE = ::sysconf(_SC_PAGESIZE);
#endif

        // Use `char*` instead of `void*` because we need to do arithmetic on them.
        unsigned char* addr =nullptr;
        unsigned char* addr2=nullptr;

        // Technically, we could also report sucess here since a zero-length
        // buffer can't be legally used anyways.
        if (minsize == 0) {
            errno = EINVAL;
            return -1;
        }

        // Round up to nearest multiple of page size.
        int bytes = minsize & ~(PAGE_SIZE-1);
        if (minsize % PAGE_SIZE) {
            bytes += PAGE_SIZE;
        }

        // Check for overflow.
        if (bytes*2u < bytes) {
            errno = EINVAL;
            return -1;
        }

        // Allocate twice the buffer size
        addr = static_cast<unsigned char*>(::mmap(NULL, 2*bytes,
                                                  PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

        if (addr == MAP_FAILED) {
            goto errout;
        }

        // Shrink to actual buffer size.
        addr = static_cast<unsigned char*>(::mremap(addr, 2*bytes, bytes, 0));
        if (addr == MAP_FAILED) {
            goto errout;
        }

        // Create the second copy right after the shrinked buffer.
        addr2 = static_cast<unsigned char*>(::mremap(addr, 0, bytes, MREMAP_MAYMOVE,
                                                     addr+bytes));

        if (addr2 == MAP_FAILED) {
            goto errout;
        }

        if (addr2 != addr+bytes) {
            errno = EAGAIN;
            goto errout;
        }

        // Sanity check.
        *(char*)addr = 'x';
        assert(*(char*)addr2 == 'x');

        *(char*)addr2 = 'y';
        assert(*(char*)addr == 'y');

        capacity_ = bytes;
        buffer_ = addr;

        return 0;

        errout:
        int error = errno;
        // We actually have to check for non-null here, since even if `addr` is
        // null, `bytes` might be large enough that this overlaps some actual
        // mappings.
        if (addr) {
            ::munmap(addr, bytes);
        }
        if (addr2) {
            ::munmap(addr2, bytes);
        }
        errno = error;
        return -1;
    }


    template<typename T>
    linear_ringbuffer_<T>::~linear_ringbuffer_()
    {
        // Either `buffer_` and `capacity_` are both initialized properly,
        // or both are zero.
        ::munmap(buffer_, capacity_);
        ::munmap(buffer_+capacity_, capacity_);
    }


    template<typename T>
    void linear_ringbuffer_<T>::swap(linear_ringbuffer_<T>& other) noexcept
    {
        using std::swap;
        swap(buffer_, other.buffer_);
        swap(capacity_, other.capacity_);
        swap(tail_, other.tail_);
        swap(head_, other.head_);
        swap(size_, other.size_);
    }


    template<typename Count>
    void swap(
            linear_ringbuffer_<Count>& lhs,
            linear_ringbuffer_<Count>& rhs) noexcept
    {
        lhs.swap(rhs);
    }


    inline initialization_error::initialization_error(int errno_)
            : std::runtime_error(::strerror(errno_))
            , error(errno_)
    {}

}
#endif //PROJECT_packet_bufFER_HH
