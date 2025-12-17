#include "seq.h"

#include <Eigen/Dense>
#include <iostream>

#include "float_or_double_seq.h"
#include "gtest/gtest.h"
#include "seq.h"

using namespace axby;

size_t Seq_func(Seq<int> input) { return input.size(); }
size_t CSeq_func(Seq<const int> input) { return input.size(); }

struct TestClass {
    TestClass(Seq<const float> f) {}
};

TEST(Seq, converts_vector) {
    std::vector<int> a = {1, 2, 3};
    EXPECT_EQ(Seq_func(a), a.size());
}

TEST(Seq, converts_carray) {
    int a[3] = {1, 2, 3};
    EXPECT_EQ(Seq_func(a), 3);
}

TEST(Seq, converts_ilist) {
    std::initializer_list<int> a = {1, 2, 3};
    EXPECT_EQ(CSeq_func(a), a.size());
}

TEST(Seq, converts_ilist_inline) { EXPECT_EQ(CSeq_func({1, 2, 3}), 3); }

TEST(Seq, converts_ilist_constructor_arg) { TestClass t{{1.0f, 2.0f}}; }

TEST(Seq, converts_array) {
    std::array<int, 3> a = {1, 2, 3};
    EXPECT_EQ(Seq_func(a), a.size());
}

TEST(Seq, converts_c_array) {
    int a[3] = {1, 2, 3};
    EXPECT_EQ(Seq_func(a), 3);
}

TEST(AnySeq, converts_vector_int) {
    std::vector<int> a = {1, 2, 3};

    AnySeq any(a);
    std::span<int> any_int = any.get_span<int>();

    EXPECT_EQ(any_int.size(), a.size());
    for (int i = 0; i < any_int.size(); ++i) {
        EXPECT_EQ(any_int[i], a[i]);
    }
}

TEST(AnySeq, converts_vector_float) {
    std::vector<float> a = {1.2, 2.1, 3.0};

    AnySeq any(a);
    std::span<float> any_float = any.get_span<float>();

    EXPECT_EQ(any_float.size(), a.size());
    for (int i = 0; i < any_float.size(); ++i) {
        EXPECT_EQ(any_float[i], a[i]);
    }
}

TEST(ConstAnySeq, internal_byte_type_is_const) {
    constexpr bool pass = std::is_const_v<ConstAnySeq::maybe_const_byte>;
    EXPECT_TRUE(pass);
}

TEST(ConstAnySeq, converts_vector_float) {
    std::vector<float> a = {1.2, 2.1, 3.0};

    ConstAnySeq any(a);
    std::span<const float> any_float = any.get_span<const float>();

    EXPECT_EQ(any_float.size(), a.size());
    for (int i = 0; i < any_float.size(); ++i) {
        EXPECT_EQ(any_float[i], a[i]);
    }
}

TEST(AnySeq_float, is_type) {
    std::vector<float> a = {1.2, 2.1, 3.0};
    AnySeq any(a);
    EXPECT_TRUE(any.is_type<float>());
    EXPECT_FALSE(any.is_type<int>());
}

TEST(AnySeq_float, logical_size) {
    std::vector<float> a = {1.2, 2.1, 3.0};
    AnySeq any(a);
    EXPECT_EQ(any.logical_size(), 3);
}

TEST(AnySeq_float, mutate) {
    std::vector<float> a = {1.2, 2.1, 3.0};

    AnySeq any(a);
    any.get_span<float>()[1] = -1.99;

    EXPECT_EQ(a[1], any.get_span<float>()[1]);
}

TEST(ConstAnySeq_float, is_type) {
    std::vector<float> a = {1.2, 2.1, 3.0};
    ConstAnySeq any(a);
    EXPECT_TRUE(any.is_type<float>());
    EXPECT_FALSE(any.is_type<int>());
}

TEST(fds_construct, eigen_mat4f) {
    // just test that this function compiles
    Eigen::Matrix4f test;
    FloatOrDoubleSeq fds(test);
}

TEST(cfds_construct, eigen_mat4f) {
    // just test that this function compiles
    Eigen::Matrix4f test;
    ConstFloatOrDoubleSeq fds(test);
}

TEST(fds, converts_c_f_array) {
    // just test that this function compiles
    std::array<float, 3> hello = {1, 2, 3};
    Seq<float> works(hello);
    FloatOrDoubleSeq fdseq(hello);
}

void func_accepting_fds(FloatOrDoubleSeq fds) { return; }
void func_accepting_cfds(ConstFloatOrDoubleSeq fds) { return; }

TEST(fds, converts_eigen_mat4_arg) {
    // just test that this function compiles
    Eigen::Matrix4f mat;
    func_accepting_fds(mat);  
}

TEST(cfds, converts_eigen_mat4_arg) {
    // just test that this function compiles
    Eigen::Matrix4f mat;
    func_accepting_cfds(mat);  
}

TEST(cfds, converts_stdarray_float) {
    // just test that this function compiles
    std::array<float, 16> arr {};
    func_accepting_cfds(arr);  
}

std::array<float, 3> make_test_array() { return {1.0, 2.0, 3.0}; }
TEST(fds, converts_temporary_c_f_array) {
    // just test that this function compiles
    FloatOrDoubleSeq fdseq(make_test_array());
    //warning: don't access the contents since the array was a
    //temporary
}

TEST(fds, write_to) {
    double dst[3] = {-1, -2, -3};
    double src[3] = {1, 2, 3};

    FloatOrDoubleSeq(src).write_to(dst);

    EXPECT_EQ(src[0], dst[0]);
    EXPECT_EQ(src[1], dst[1]);
    EXPECT_EQ(src[2], dst[2]);
}

TEST(const_fds_write_to, d2d) {
    double dst[3] = {-1, -2, -3};
    double src[3] = {1, 2, 3};

    ConstFloatOrDoubleSeq(src).write_to(dst);

    EXPECT_EQ(src[0], dst[0]);
    EXPECT_EQ(src[1], dst[1]);
    EXPECT_EQ(src[2], dst[2]);
}

TEST(fds_copy_from, d2d) {
    double dst[3] = {-1, -2, -3};
    double src[3] = {1, 2, 3};

    FloatOrDoubleSeq(dst).copy_from(src);

    EXPECT_EQ(src[0], dst[0]);
    EXPECT_EQ(src[1], dst[1]);
    EXPECT_EQ(src[2], dst[2]);
}

TEST(fds_copy_from, f2d) {
    double dst[3] = {-1, -2, -3};
    float src[3] = {1, 2, 3};

    FloatOrDoubleSeq(dst).copy_from(src);

    EXPECT_NEAR(src[0], dst[0], 1e-9);
    EXPECT_NEAR(src[1], dst[1], 1e-9);
    EXPECT_NEAR(src[2], dst[2], 1e-9);
}

TEST(fds_copy_from, eigen2f) {
    Eigen::Matrix4f tx;
    tx.setIdentity();

    float dst[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    FloatOrDoubleSeq(dst).copy_from(tx);

    EXPECT_TRUE(seq_equals(tx, dst));
}

TEST(fds_copy_from, eigen2eigen) {
    Eigen::Matrix4f tx1;
    tx1.setIdentity();

    Eigen::Matrix4f tx2;
    tx2.setZero();

    FloatOrDoubleSeq(tx2).copy_from(tx1);

    EXPECT_EQ(tx1, tx2);
}

TEST(fds_copy_from, f2f) {
    float dst[3] = {-1, -2, -3};

    float src[3] = {1, 2, 3};
    float src_alt[3] = {1, 2, 3};

    FloatOrDoubleSeq(dst).copy_from(src);

    EXPECT_NEAR(src[0], dst[0], 1e-9);
    EXPECT_NEAR(src[1], dst[1], 1e-9);
    EXPECT_NEAR(src[2], dst[2], 1e-9);

    // src should not have changed
    EXPECT_NEAR(src[0], src_alt[0], 1e-9);
    EXPECT_NEAR(src[1], src_alt[1], 1e-9);
    EXPECT_NEAR(src[2], src_alt[2], 1e-9);
}

TEST(fds_copy_from, d2f) {
    float dst[3] = {-1, -2, -3};
    double src[3] = {1, 2, 3};

    FloatOrDoubleSeq(dst).copy_from(src);

    EXPECT_NEAR(src[0], dst[0], 1e-9);
    EXPECT_NEAR(src[1], dst[1], 1e-9);
    EXPECT_NEAR(src[2], dst[2], 1e-9);
}

// test that this compiles
TEST(fds, from_other_fds) {
    float data[3] = {1.0, 1.5, 3.0};
    FloatOrDoubleSeqImpl<false> fds_a{data};
    FloatOrDoubleSeqImpl<false> fds_b{fds_a};
}
TEST(constfds, from_other_fds) {
    float data[3] = {1.0, 1.5, 3.0};
    FloatOrDoubleSeqImpl<false> fds_a{data};
    FloatOrDoubleSeqImpl<true> fds_b{fds_a};
}

TEST(seq_equals, vec_char_array_char) {
    std::vector<char> x1 = {'h', 'e', 'l', 'l', 'o'};
    std::array<char, 5> x2 = {'h', 'e', 'l', 'l', 'o'};
    std::array<char, 6> x3 = {'h', 'e', 'l', 'l', 'o', 'o'};

    EXPECT_FALSE(axby::seq_equals(x1, x3));
    EXPECT_TRUE(axby::seq_equals(x1, x2));
}

TEST(seq_copy, float_arr) {
    std::array<float, 2> src = {1.0f, 2.0f};
    std::array<float, 2> dst = {0.0f, 0.0f};
    EXPECT_FALSE(axby::seq_equals(src, dst));

    seq_copy(src, dst);
    EXPECT_TRUE(axby::seq_equals(src, dst));
}

TEST(seq_copy, float_arr_to_vec_arr) {
    std::array<float, 2> src = {1.0f, 2.0f};
    std::vector<float> dst(src.size(), 0);
    EXPECT_FALSE(axby::seq_equals(src, dst));

    seq_copy(src, dst);
    EXPECT_TRUE(axby::seq_equals(src, dst));
}

TEST(reinterprest_span, u8_to_byte) {
    // chekcing it compiles
    std::vector<uint8_t> a{1, 2, 3};
    reinterpret_span<std::byte>(a);
}
TEST(reinterprest_span, u8_to_constbyte) {
    // checking it compiles
    std::vector<uint8_t> a{1, 2, 3};
    reinterpret_span<const std::byte>(a);
}

TEST(seq_to_array, chars) {
    const auto hello_array = seq_to_array<6>("hello");
    EXPECT_EQ(hello_array[0], 'h');
    EXPECT_EQ(hello_array[1], 'e');
    EXPECT_EQ(hello_array[2], 'l');
    EXPECT_EQ(hello_array[3], 'l');
    EXPECT_EQ(hello_array[4], 'o');
    EXPECT_EQ(hello_array[5], 0);
}

TEST(seq_to_array, floats) {
    const float floats_in[6] = {1.0f, 2.0f, 3.0f, 2.1f, 1.1f, 0.1f};
    const auto floats_array = seq_to_array<6>(floats_in);
    EXPECT_EQ(floats_array[0], 1.0f);
    EXPECT_EQ(floats_array[1], 2.0f);
    EXPECT_EQ(floats_array[2], 3.0f);
    EXPECT_EQ(floats_array[3], 2.1f);
    EXPECT_EQ(floats_array[4], 1.1f);
    EXPECT_EQ(floats_array[5], 0.1f);
}

/*
  // should fail since reinterpreting const as non-const

TEST(reinterprest_span, const_issue1) {
    const uint8_t a[3] = {1, 2, 3};
    reinterpret_span<std::byte>(a);
}
*/

/*
// this test should check fail since the type out is not the same as
// the source type

TEST(AnySeq, crash) {
    std::vector<float> a = {1.2, 2.1, 3.0};

    AnySeq any(a);
    any.get_span<int>()[1] = 1;
}

*/
