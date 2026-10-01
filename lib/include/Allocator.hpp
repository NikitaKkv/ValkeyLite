#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "State.hpp"

namespace val_key {
    template <typename T>
    class CountingAllocator {
    public:
        using value_type = T; 

        CountingAllocator() noexcept : state_(nullptr) {}

        CountingAllocator(std::shared_ptr<State> other) : state_(other) {}

        CountingAllocator(const CountingAllocator& other) noexcept : state_(other.state_) {}

        CountingAllocator(CountingAllocator&& other) noexcept : state_(other.state_) {}

        template<typename Other>
        friend class CountingAllocator;

        template <typename U>
        CountingAllocator(const CountingAllocator<U>& other) noexcept : state_(other.state_) {}

        template <typename U>
        CountingAllocator(CountingAllocator<U>&& other) noexcept : state_(other.state_) {}

        T* allocate(std::size_t n) {
            std::size_t bytes = n * sizeof(T);
            state_->cur_mem_bytes += bytes; 
            return static_cast<T*>(operator new(bytes));
        }

        void deallocate(T* p, std::size_t n) noexcept {
            state_->cur_mem_bytes -= n * sizeof(T); 
            operator delete(p);
        }

        bool operator== (const CountingAllocator& other) const noexcept = default;
        
        bool operator!= (const CountingAllocator& other) const noexcept = default;

    private:
        std::shared_ptr<State> state_;
    };

    using CountingString = std::basic_string<char, std::char_traits<char>, CountingAllocator<char>>;

    template<typename T>
    using CountingList = std::list<T, CountingAllocator<T>>;

    template<typename T, typename U>
    using CountingHashMap = 
        std::unordered_map<T, U, std::hash<T>, std::equal_to<T>, CountingAllocator<std::pair<const T, U>>>;

    template<typename T, typename U>
    using CountingMultiMap = 
        std::multimap<T, U, std::equal_to<T>, CountingAllocator<std::pair<const T, U>>>;


    template<typename T>
    using CountingHashSet = std::unordered_set<T, std::hash<T>, std::equal_to<T>, CountingAllocator<T>>;

} // namespace val_key
