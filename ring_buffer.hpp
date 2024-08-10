#pragma once

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
#include <filesystem>

auto create_temporary_file() -> int {
    auto temp_dir { std::filesystem::temp_directory_path() };
    auto temp_file { temp_dir / "XXXXXX" };
    char* temp_file_cstr { strdup(temp_file.c_str()) };
    int fd { mkstemp(temp_file_cstr) };
    if (fd == -1) {
        throw std::runtime_error("Failed to create temporary file: " + std::string(strerror(errno)));
    }
    std::filesystem::remove(temp_file);
    return fd;
}

auto get_page_size() -> long {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        throw std::runtime_error("Failed to get page size: " + std::string(strerror(errno)));
    }
    
    return page_size;
}

auto resize_file(int fd, size_t size) -> void {
    if (ftruncate(fd, size) == -1) {
        throw std::runtime_error("Failed to set file size: " + std::string(strerror(errno)));
    }
}

template<typename T, std::size_t N, bool magic = false>
class RingBuffer {
public:
    RingBuffer() {
        if constexpr (!magic) return;
        
        std::cout << "I'm magic" << std::endl;

        try {
            fd_ = create_temporary_file();
            auto page_size { get_page_size() };

            std::cout << "page size: " << page_size << std::endl;

            // Calculate the size to map, rounded up to the nearest page size
            size_t page_size_multiple { ((N * sizeof(T) + page_size - 1) / page_size) };

            std::cout << "size: " << (N * sizeof(T)) << ", page size multiple: " << page_size_multiple << std::endl;

            map_size_ = page_size_multiple * page_size;

            std::cout << "map size: " << map_size_ << std::endl;

            resize_file(fd_, map_size_);

            // Reserve address space for two contiguous mappings
            void* addr = mmap(nullptr, 2 * map_size_, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (addr == MAP_FAILED) {
                throw std::runtime_error("Failed to reserve address space: " + std::string(strerror(errno)));
            }

            // Map the file twice
            buffer_ = static_cast<T*>(mmap(addr, map_size_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd_, 0));
            if (buffer_ == MAP_FAILED) {
                munmap(addr, 2 * map_size_);
                throw std::runtime_error("Failed to map first region: " + std::string(strerror(errno)));
            }

            void* second = mmap(static_cast<char*>(addr) + map_size_, map_size_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd_, 0);
            if (second == MAP_FAILED) {
                munmap(addr, 2 * map_size_);
                throw std::runtime_error("Failed to map second region: " + std::string(strerror(errno)));
            }
        } catch (...) {
            this->~RingBuffer();
            throw;
        }
    }

    ~RingBuffer() {
        if (buffer_) {
            munmap(buffer_, 2 * map_size_);
        }
        if (fd_ != -1) {
            close(fd_);
        }
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    RingBuffer(RingBuffer&& other) noexcept
        : buffer_(other.buffer_), fd_(other.fd_), map_size_(other.map_size_), read_pos_(other.read_pos_), write_pos_(other.write_pos_) {
        other.buffer_ = nullptr;
        other.fd_ = -1;
    }

    RingBuffer& operator=(RingBuffer&& other) noexcept {
        if (this != &other) {
            this->~RingBuffer();
            buffer_ = other.buffer_;
            fd_ = other.fd_;
            map_size_ = other.map_size_;
            read_pos_ = other.read_pos_;
            write_pos_ = other.write_pos_;
            other.buffer_ = nullptr;
            other.fd_ = -1;
        }
        return *this;
    }

    void write(const T& value) {
        buffer_[write_pos_++] = value;
    }

    T read() {
        T value = buffer_[read_pos_++];
        return value;
    }

    std::span<const T> get_read_span(std::size_t count) const {
        return {&buffer_[read_pos_], std::min(count, N - read_pos_)};
    }

    std::span<T> get_write_span(std::size_t count) {
        return {&buffer_[write_pos_], std::min(count, N - write_pos_)};
    }

private:
    T* buffer_ { nullptr };
    int fd_ { -1 };
    std::size_t map_size_ { 0 };
    std::size_t read_pos_ { 0 };
    std::size_t write_pos_ { 0 };
};
