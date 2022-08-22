#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

template <typename T>
struct slice {
private:
    T* _data = nullptr;
    uintptr_t _len = 0;

    using T_NonConst = typename std::remove_const<T>::type;

    constexpr slice(T* data, uintptr_t len) noexcept : _data(data), _len(len) {}

public:
    constexpr slice() noexcept = default;

    constexpr slice(const slice<T>&) = default;

    slice(std::vector<T_NonConst>& vec) noexcept : _data(vec.data()), _len(vec.size()) {}

    template <typename = typename std::enable_if<std::is_const_v<T>>::type>
    slice(const std::vector<T_NonConst>& vec) noexcept : _data(vec.data()), _len(vec.size()) {}

    template <size_t N>
    slice(std::array<T_NonConst, N>& array) noexcept : _data(array.data()), _len(array.size()) {}

    template <size_t N, typename = typename std::enable_if<std::is_const_v<T>>::type>
    slice(const std::array<T_NonConst, N>& array) noexcept : _data(array.data()), _len(array.size()) {}

    T* data() const noexcept {
        return _data;
    }

    uintptr_t len() const noexcept {
        return _len;
    }

    bool isEmpty() const noexcept {
        return _len == 0;
    }

    static slice<T> fromRaw(T* data, uintptr_t len) noexcept {
        return {data, len};
    }

    // TODO: maybe add utility methods

    //    T& at_unchecked(uintptr_t i) const {
    //        return *(_data + i);
    //    }
    //
    //    T& at(uintptr_t i) const {
    //        if (_len <= i) {
    //            throw std::runtime_error("bruh");
    //        }
    //        return this->at_unchecked(i);
    //    }
    //
    //    T& operator[](uintptr_t i) const {
    //        return this->at(i);
    //    }
    //
    T* begin() const {
        return _data;
    }

    T* end() const {
        return _data + _len;
    }
};

template <typename T>
using immslice = slice<const T>;
