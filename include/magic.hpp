#pragma once

#include <filesystem>
#include <cstdint>
#include <stdexcept>
#include <span>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cerrno>

namespace magic {

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

template<typename T>
auto create_memory_mapped_buffer(std::size_t N) -> std::span<T> {
    auto page_size { get_page_size() };
    auto page_size_multiple { ((N * sizeof(T) + page_size - 1) / page_size) };
    auto map_size { page_size_multiple * page_size };
    auto capacity { map_size / sizeof(T) };

    auto fd { create_temporary_file() };
    resize_file(fd, map_size);

    auto* addr { mmap(nullptr, 2 * map_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) };
    if (addr == MAP_FAILED) {
        throw std::runtime_error("Failed to reserve address space: " + std::string(strerror(errno)));
    }

    auto* first_buffer { static_cast<T*>(mmap(addr, map_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0)) };
    if (first_buffer == MAP_FAILED) {
        munmap(addr, 2 * map_size);
        throw std::runtime_error("Failed to map first region: " + std::string(strerror(errno)));
    }

    auto* second_buffer { mmap(static_cast<char*>(addr) + map_size, map_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) };
    if (second_buffer == MAP_FAILED) {
        munmap(addr, 2 * map_size);
        throw std::runtime_error("Failed to map second region: " + std::string(strerror(errno)));
    }

    close(fd);

    return { first_buffer, capacity };
}

template<typename T>
auto delete_memory_mapped_buffer(std::span<T> s) {
    if (!s.empty()) {
        munmap(s.data(), 2 * s.size_bytes());
    }
}


} /* namespace magic */
