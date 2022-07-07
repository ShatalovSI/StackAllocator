#include <cstddef>
#include <memory>
#include <limits>
#include <cstdlib>

template <class T, std::size_t Size>
class stack_allocator
{
    char d_[Size];
    char* p_;

public:
    using Allocator = stack_allocator;

    using value_type = T;
    using pointer = value_type*;
    using const_pointer = const value_type*;

    stack_allocator() : p_(d_) {}

    template <class U, size_t _Size>
    stack_allocator(const stack_allocator<U, _Size>&) noexcept {}

    static size_t align_up(std::size_t n) noexcept
    {
        const auto align = alignof(T);
        return (n + (align - 1)) & ~(align - 1);
    }

    pointer allocate(size_t n) noexcept
    {
        const auto bytes = align_up(n * sizeof(T));
        if (static_cast<decltype(bytes)>(d_ + Size - p_) >= bytes)
        {
            auto result = p_;
            p_ += n;

            return reinterpret_cast<pointer>(result);
        }

        return nullptr;
    }

    void deallocate(pointer p, size_t n) noexcept
    {
        auto raw = reinterpret_cast<char*>(p);
        const auto offset = raw - d_;
        if (raw + align_up(n * sizeof(T)) == p_)
        {
            p_ = raw;
        }
    }

    size_t max_size() const noexcept
    {
        return Size / sizeof(value_type);
    }

    bool owns(const_pointer p) const noexcept
    {
        return reinterpret_cast<const char*>(p) >= d_ && reinterpret_cast<const char*>(p) < d_ + Size;
    }

    template <typename U>
    struct rebind
    {
        using other = stack_allocator<U, Size>;
    };

    ~stack_allocator()
    {
        (void)0;
    }
};

template <typename T>
using stack_allocator_1Kibi = stack_allocator<T, 1024>;

template <class T>
struct mallocator
{
    using value_type = T;

    mallocator() = default;

    template <class U>
    mallocator(const mallocator <U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n)
    {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
        {
            throw std::bad_array_new_length();
        }

        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
        {
            return p;
        }

        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        std::free(p);
    }

    ~mallocator()
    {
        (void)0;
    }
};

template <typename T, template <typename> typename Primary, template <typename> typename Fallback>
class fallback_allocator : private Primary<T>, private Fallback<T>
{
public:
    using Allocator = fallback_allocator;

    using value_type = T;
    using pointer = typename std::allocator_traits<Primary<T>>::pointer;
    using const_pointer = typename std::allocator_traits<Primary<T>>::const_pointer;

    fallback_allocator() = default;

    template <typename U, template <typename> typename _Primary, template <typename> typename _Fallback>
    fallback_allocator(const fallback_allocator<U, _Primary, _Fallback>&) {}

    auto allocate(size_t n) noexcept
    {
        auto p = Primary<T>::allocate(n);
        if (!p)
        {
            p = Fallback<T>::allocate(n);
        }

        return p;
    }

    void deallocate(pointer p, size_t n) noexcept(noexcept(Primary<T>::owns(p)))
    {
        if (Primary<T>::owns(p))
        {
            Primary<T>::deallocate(p, n);
        }
        else
        {
            Fallback<T>::deallocate(p, n);
        }
    }

    bool owns(pointer p) noexcept(noexcept(Primary<T>::owns(p))
        && noexcept(Fallback<T>::owns(p)))
    {
        return Primary<T>::owns(p) || Fallback<T>::owns(p);
    }

    template <typename U>
    struct rebind
    {
        using other = fallback_allocator<U, Primary, Fallback>;
    };

    ~fallback_allocator()
    {
        (void)0;
    }
};

#include <numeric>
#include <vector>

template <typename T>
using custom_allocator = fallback_allocator<T, stack_allocator_1Kibi, mallocator>;

int main()
{
    std::vector<int, custom_allocator<int>> vec;
    vec.resize(20);

    std::iota(vec.begin(), vec.end(), 0);

    return 0;
}
