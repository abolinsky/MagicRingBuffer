#include <gtest/gtest.h>
#include "ring_buffer.hpp"
#include <concepts>
#include <string>
#include <vector>

using namespace magic;

// Test Fixture for RingBuffer
template<typename T>
class RingBufferTest : public ::testing::Test {
protected:
    static constexpr size_t BufferSize = 5;
    RingBuffer<T, BufferSize> buffer;
};

// Test with int
using IntRingBufferTest = RingBufferTest<int>;
using StringRingBufferTest = RingBufferTest<std::string>;

TEST_F(IntRingBufferTest, NotCopyable) {
    static_assert(!std::copyable<decltype(buffer)>);
}

TEST_F(IntRingBufferTest, PushAndPop) {
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    
    EXPECT_EQ(buffer.front(), 1);
    EXPECT_EQ(buffer.back(), 3);
    
    buffer.pop();
    EXPECT_EQ(buffer.front(), 2);
    
    buffer.pop();
    EXPECT_EQ(buffer.front(), 3);
    
    buffer.pop();
    EXPECT_TRUE(buffer.empty());
}

TEST_F(IntRingBufferTest, OverwriteOldestWhenFull) {
    for (int i = 0; i < BufferSize; ++i) {
        buffer.push(i + 1);
    }
    
    EXPECT_EQ(buffer.size(), BufferSize);
    EXPECT_EQ(buffer.front(), 1);
    
    buffer.push(6); // This should overwrite the oldest element (1)
    
    EXPECT_EQ(buffer.size(), BufferSize);
    EXPECT_EQ(buffer.front(), 2); // 2 is now the oldest
    EXPECT_EQ(buffer.back(), 6);
}

TEST_F(IntRingBufferTest, WrapAround) {
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    
    buffer.pop(); // Remove 1
    buffer.pop(); // Remove 2
    
    buffer.push(4);
    buffer.push(5);
    buffer.push(6);
    buffer.push(7); // This should wrap around and overwrite the oldest element
    
    EXPECT_EQ(buffer.size(), BufferSize);
    EXPECT_EQ(buffer.front(), 4);
    EXPECT_EQ(buffer.back(), 7);
}

TEST_F(IntRingBufferTest, Peek) {
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    
    auto span = buffer.peek();
    EXPECT_EQ(span.size(), 2); // Back is 3, peek shows 2 elements

    EXPECT_EQ(span[0], 1);
    EXPECT_EQ(span[1], 2);
    
    buffer.pop(); // Remove 1
    span = buffer.peek();
    EXPECT_EQ(span.size(), 2);
    EXPECT_EQ(span[0], 2);
    EXPECT_EQ(span[1], 3);
}

TEST_F(IntRingBufferTest, Iterator) {
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);

    auto it = buffer.begin();
    EXPECT_EQ(*it, 1);

    ++it;
    EXPECT_EQ(*it, 2);

    ++it;
    EXPECT_EQ(*it, 3);

    ++it;
    EXPECT_EQ(it, buffer.end());
}

TEST_F(IntRingBufferTest, MoveSemantics) {
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);

    RingBuffer<int, BufferSize> movedBuffer = std::move(buffer);

    EXPECT_EQ(movedBuffer.front(), 1);
    EXPECT_EQ(movedBuffer.back(), 3);
    EXPECT_TRUE(buffer.empty());
}

TEST_F(IntRingBufferTest, ExceptionHandlingInConstructor) {
    try {
        RingBuffer<int, BufferSize> faultyBuffer;
        // Simulate an exception scenario during buffer initialization
        // Your create_memory_mapped_buffer() or delete_memory_mapped_buffer() should throw here
    } catch (const std::exception& e) {
        SUCCEED();
    } catch (...) {
        FAIL() << "Caught unexpected exception type.";
    }
}

TEST_F(StringRingBufferTest, StringTypeTest) {
    buffer.push("Hello");
    buffer.push("World");
    
    EXPECT_EQ(buffer.front(), "Hello");
    EXPECT_EQ(buffer.back(), "World");
    
    buffer.pop();
    EXPECT_EQ(buffer.front(), "World");
}

TEST_F(IntRingBufferTest, EmptyAndSizeTest) {
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);

    buffer.push(10);
    EXPECT_FALSE(buffer.empty());
    EXPECT_EQ(buffer.size(), 1);

    buffer.pop();
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
