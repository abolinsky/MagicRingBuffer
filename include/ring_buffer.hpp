#pragma once

#include "magic.hpp"

#include <utility>
#include <exception>
#include <iostream>
#include <cstdint>
#include <stdexcept>
#include <span>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cerrno>
#include <cstring>
#include <string>

namespace magic {

template<typename T, std::size_t N>
class RingBuffer {
public:
    class iterator {
    public:
        iterator(T* ptr) : ptr_(ptr) {}
        auto operator*() const -> T& { return *ptr_; }
        auto operator++() -> iterator& { ++ptr_; return *this; }
        auto operator++(int) -> iterator { iterator tmp { *this }; ++ptr_; return tmp; }
        auto operator!=(const iterator& other) const -> bool { return ptr_ != other.ptr_; }
        auto operator==(const iterator& other) const -> bool { return ptr_ == other.ptr_; }
    private:
        T* ptr_;
    };

    RingBuffer();
    ~RingBuffer();

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&& other) noexcept;

    auto operator=(const RingBuffer&) = delete;
    auto operator=(RingBuffer&& other) noexcept;

    auto front() const -> const T&;
    auto front() -> T&;

    auto back() const -> const T&;
    auto back() -> T&;

    auto empty() const -> bool;
    auto size() const -> size_t;

    auto push(const T& value) -> void;
    auto push(T&& value) -> void;
    auto pop() -> void;

    auto peek() -> std::span<T>;
    auto c_peek() const -> std::span<const T>;

    auto begin() const -> iterator;
    auto end() const -> iterator;

private:
    std::span<T> buffer_;
    std::size_t read_pos_ { 0 };
    std::size_t write_pos_ { 0 };
};

template<typename T, std::size_t N>
RingBuffer<T, N>::RingBuffer()
try : buffer_(create_memory_mapped_buffer<T>(N)) {
} catch (...) {
    this->~RingBuffer();
    throw;
}

template<typename T, std::size_t N>
RingBuffer<T, N>::~RingBuffer() {
    delete_memory_mapped_buffer<T>(buffer_);
}

template<typename T, std::size_t N>
RingBuffer<T, N>::RingBuffer(RingBuffer&& other) noexcept
    : buffer_(other.buffer_), read_pos_(other.read_pos_), write_pos_(other.write_pos_) {
    other.buffer_ = std::span<T>();
}

template<typename T, std::size_t N>
auto RingBuffer<T, N>::operator=(RingBuffer&& other) noexcept {
    if (this != &other) {
        this->~RingBuffer();
        buffer_ = other.buffer_;
        read_pos_ = other.read_pos_;
        write_pos_ = other.write_pos_;
        other.buffer_ = std::span<T>();
    }
    return *this;
}

template<typename T, std::size_t N>
auto RingBuffer<T, N>::front() const -> const T& {
    return buffer_[read_pos_];
}

template<typename T, std::size_t N>
auto RingBuffer<T, N>::front() -> T& {
    return buffer_[read_pos_];
}

template<typename T, std::size_t N>
auto RingBuffer<T, N>::back() const -> const T& {
    return buffer_[write_pos_ - 1];
}

template<typename T, std::size_t N>
auto RingBuffer<T, N>::back() -> T& {
    return buffer_[write_pos_ - 1];
}

template<typename T, std::size_t N>
auto RingBuffer<T, N>::empty() const -> bool {
    return read_pos_ == write_pos_;
}

template<typename T, std::size_t N>
auto RingBuffer<T, N>::size() const -> size_t {
    return write_pos_ >= read_pos_ ? write_pos_ - read_pos_ : (write_pos_ + buffer_.size()) - read_pos_;
}

template<typename T, std::size_t N>
auto RingBuffer<T, N>::push(const T& value) -> void {
    buffer_[write_pos_] = value;
    ++write_pos_ %= N;
}

template<typename T, std::size_t N>
auto RingBuffer<T, N>::push(T&& value) -> void {
    buffer_[write_pos_] = value;
    ++write_pos_ %= N;
}

template<typename T, std::size_t N>
auto RingBuffer<T, N>::pop() -> void {
    ++read_pos_ %= N;
}

template<typename T, std::size_t N>
std::span<const T> RingBuffer<T, N>::c_peek() const {
    auto count { write_pos_ >= read_pos_ ? write_pos_ - read_pos_ : (write_pos_ + buffer_.size()) - read_pos_ };
    return { &buffer_[read_pos_], count };
}

template<typename T, std::size_t N>
std::span<T> RingBuffer<T, N>::peek() {
    auto count { read_pos_ > write_pos_ ? read_pos_ - write_pos_ : (read_pos_ + buffer_.size()) - write_pos_ };
    return { &buffer_[write_pos_], count - 1 };
}

template<typename T, std::size_t N>
auto RingBuffer<T, N>::begin() const -> typename RingBuffer<T, N>::iterator {
    auto read_pos { write_pos_ >= read_pos_ ? read_pos_ : read_pos_ + buffer_.size() };
    return iterator { &buffer_[read_pos] };
};

template<typename T, std::size_t N>
auto RingBuffer<T, N>::end() const -> typename RingBuffer<T, N>::iterator {
    auto write_pos { write_pos_ >= read_pos_ ? write_pos_ : write_pos_ + buffer_.size() };
    return iterator { &buffer_[write_pos] };
};

} /* namespace magic */
