#include "ring_buffer.hpp"

auto demonstrate_ring_buffer() -> void {
    RingBuffer<int, 4096, true> buffer;

    for (int i = 0; i < 4100; ++i) {
        buffer.write(i);
    }

    for (int i = 0; i < 10; ++i) {
        std::cout << buffer.read() << " ";
    }
    std::cout << std::endl;

    auto write_span = buffer.get_write_span(19);
    for (std::size_t i = 0; i < write_span.size(); ++i) {
        write_span[i] = i + 10;
    }

    auto read_span = buffer.get_read_span(100);
    for (const auto& value : read_span) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
}

int main() {
    try {
        demonstrate_ring_buffer();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
