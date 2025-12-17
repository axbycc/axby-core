// clang-format off
#include "gtest/gtest.h"

#include "info.h"
// clang-format on

using namespace axby;

TEST(type_to_glenum, float) {
    EXPECT_EQ(gl::type_to_glenum<float>(), GL_FLOAT);
    EXPECT_EQ(gl::type_to_glenum<const float>(), GL_FLOAT);
}

TEST(type_to_glenum, double) {
    EXPECT_EQ(gl::type_to_glenum<double>(), GL_DOUBLE);
    EXPECT_EQ(gl::type_to_glenum<const double>(), GL_DOUBLE);
}

TEST(type_to_glenum, int) {
    EXPECT_EQ(gl::type_to_glenum<int>(), GL_INT);
    EXPECT_EQ(gl::type_to_glenum<const int>(), GL_INT);
}

TEST(type_to_glenum, unsigned_int) {
    EXPECT_EQ(gl::type_to_glenum<unsigned int>(), GL_UNSIGNED_INT);
    EXPECT_EQ(gl::type_to_glenum<const unsigned int>(), GL_UNSIGNED_INT);
}

TEST(type_to_glenum, short) {
    EXPECT_EQ(gl::type_to_glenum<short>(), GL_SHORT);
    EXPECT_EQ(gl::type_to_glenum<const short>(), GL_SHORT);
}

TEST(type_to_glenum, unsigned_short) {
    EXPECT_EQ(gl::type_to_glenum<unsigned short>(), GL_UNSIGNED_SHORT);
    EXPECT_EQ(gl::type_to_glenum<const unsigned short>(), GL_UNSIGNED_SHORT);
}

TEST(type_to_glenum, byte) {
    EXPECT_EQ(gl::type_to_glenum<int8_t>(), GL_BYTE);
    EXPECT_EQ(gl::type_to_glenum<const int8_t>(), GL_BYTE);
}

TEST(type_to_glenum, unsigned_byte) {
    EXPECT_EQ(gl::type_to_glenum<uint8_t>(), GL_UNSIGNED_BYTE);
    EXPECT_EQ(gl::type_to_glenum<const uint8_t>(), GL_UNSIGNED_BYTE);
}
