# StackAllocator
C++ has a `std::vector` class template that implements the semantics of a dynamically mutable array. Sometimes the number of elements does not exceed a certain number. The vector, in turn, always allocates memory on the heap by default. I would like to write my own allocator,
which will allow you to take some of the elements onto the stack, and if there are more elements than a given number, then onto the heap. std::vector already has everything you need for customization:
    
    template <typename T, typename Allocator>
    class vector;
    
The second template parameter is an allocator that the class calls to allocate memory and create objects. The solution was taken from [Andrei Alexandrescu](https://www.youtube.com/watch?v=LIb3L4vKZ7U).
