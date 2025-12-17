#include "gtest/gtest.h"
#include "small_string.h"
#include <string_view>

TEST(small_string, set_with_c_str) {
		SmallString<10> ss = "hello";
		EXPECT_TRUE(ss == "hello");
}

TEST(small_string, set_with_string) {
		SmallString<10> ss = std::string("hello");
		EXPECT_TRUE(ss == "hello");
}

TEST(small_string, set_with_string_view) {
		SmallString<10> ss = std::string_view("hello");
		EXPECT_TRUE(ss == "hello");
}

TEST(small_string, empty) {
		SmallString<10> ss;
		EXPECT_TRUE(ss.empty());
		EXPECT_EQ(ss.size(), 0);
}

TEST(small_string, size) {
		SmallString<10> ss = "123";
		EXPECT_EQ(ss.size(), 3);
}

TEST(small_string, nullterminate) {
		SmallString<10> ss = "123";
		EXPECT_EQ(ss.buffer[3], '\0');
}

TEST(small_string, copyable) {
		SmallString<10> ss1 = "123";
		SmallString<10> ss2 = "345";

		ss2 = ss1;
		
		EXPECT_EQ(ss2, ss1);
}

TEST(small_string, equals_respects_length) {
		SmallString<10> ss1 = "123";
		SmallString<10> ss2 = "123";

		// write some garbage past the null terminator
		ss2.buffer[4] = 'd';
		ss2.buffer[5] = 'e';
		ss2.buffer[6] = 'f';

		ss2.buffer[4] = 'a';
		ss2.buffer[5] = 'b';
		ss2.buffer[6] = 'c';

		EXPECT_EQ(ss1, ss2);
}
