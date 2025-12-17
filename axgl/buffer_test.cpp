// clang-format off
#include "gtest/gtest.h"

#include "debug/check.h"

#include "gl_test_fixture.h"

#include "buffer.h"
#include "seq/seq.h"
// clang-format on

using namespace axby;

TEST_F(GlTestFixture, gl_buffer_float) {
  std::vector<float> buffer_data{0.0, 0.1, 0.2};

  gl::BufferOptions opts = gl::BufferOptions().set_data_type<float>();
  gl::Buffer buffer(opts);
  EXPECT_EQ(buffer.options.buffer_type, GL_ARRAY_BUFFER);
  EXPECT_EQ(buffer.options.data_type, GL_FLOAT);

  buffer.upload(buffer_data);
  EXPECT_EQ(buffer.length, buffer_data.size());

  std::array<float, 3> downloaded_data{1.337, 1.337, 1.337};
  buffer.download(downloaded_data);

  EXPECT_TRUE(axby::seq_equals(downloaded_data, buffer_data));
}

TEST_F(GlTestFixture, gl_buffer_float_modify) {
  std::vector<float> buffer_data1{0.0, 0.1, 0.2};
  std::vector<float> buffer_data2{0.1, 0.2, 0.3, 0.4};

  gl::BufferOptions opts = gl::BufferOptions().set_data_type<float>();
  gl::Buffer buffer(opts);
  buffer.upload(buffer_data1);
  buffer.upload(buffer_data2);
  EXPECT_EQ(buffer.length, buffer_data2.size());

  std::array<float, 4> downloaded_data{1.337, 1.337, 1.337, 1.337};
  buffer.download(downloaded_data);

  EXPECT_FALSE(axby::seq_equals(downloaded_data, buffer_data1));
  EXPECT_TRUE(axby::seq_equals(downloaded_data, buffer_data2));
}

TEST_F(GlTestFixture, gl_buffer_uint8) {
  std::vector<uint8_t> buffer_data{6, 7, 67};

  gl::BufferOptions opts = gl::BufferOptions().set_data_type<uint8_t>();
  gl::Buffer buffer(opts);
  EXPECT_EQ(buffer.options.buffer_type, GL_ARRAY_BUFFER);
  EXPECT_EQ(buffer.options.data_type, GL_UNSIGNED_BYTE);

  buffer.upload(buffer_data);
  EXPECT_EQ(buffer.length, buffer_data.size());

  std::array<uint8_t, 3> downloaded_data{1, 2, 3};
  buffer.download(downloaded_data);

  EXPECT_TRUE(axby::seq_equals(downloaded_data, buffer_data));
}
