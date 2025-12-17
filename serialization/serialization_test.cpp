#include "serialization.h"
#include "serialization/small_string.h"

#include <cbor.h>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "make_serializable.hpp"
#include "seq/seq.h"

using namespace axby;

enum class PersonGender {
    NONBINARY,
    MALE,
    FEMALE,
};

struct PetExample {
    std::string name;
};

struct PersonExample {
    PersonGender gender;
    uint16_t age;
    uint64_t wealth;
    std::string name;

    // pointer to a buffer of data
    // that should be serialized
    // with the person
    std::span<const uint8_t> blob;
    std::vector<PetExample> pets;
    std::array<float, 4> position;
};

TEST(serialize_cbor, string) {
    CborEncoder encoder;
    std::array<std::byte, 1024> buf;
    cbor_encoder_init(&encoder, (uint8_t*)buf.data(), buf.size(), 0);

    std::string item;
    auto result = serialization::cbor_encode(&encoder, item);
    EXPECT_EQ(result, CborNoError);
}

TEST(serialize_cbor, vec_of_strings) {
    CborEncoder encoder;
    std::array<std::byte, 1024> buf;
    cbor_encoder_init(&encoder, (uint8_t*)buf.data(), buf.size(), 0);

    std::vector<std::string> items{"a", "b", "c"};
    auto result = serialization::cbor_encode(&encoder, items);
    EXPECT_EQ(result, CborNoError);
}

TEST(serialize_cbor, boolean) {
    CborEncoder encoder;
    std::array<std::byte, 1024> buf;
    cbor_encoder_init(&encoder, (uint8_t*)buf.data(), buf.size(), 0);

    bool item = false;
    auto result = serialization::cbor_encode(&encoder, item);
    EXPECT_EQ(result, CborNoError);
}

struct EmptyStruct {};
TEST(serialize_cbor, emptystruct) {
    CborEncoder encoder;
    std::array<std::byte, 1024> buf;
    cbor_encoder_init(&encoder, (uint8_t*)buf.data(), buf.size(), 0);

    EmptyStruct item;
    auto result = serialization::cbor_encode(&encoder, item);
    EXPECT_EQ(result, CborNoError);
}

struct StructWithString {
    std::string val;
};

TEST(serialize_cbor, structwstring) {
    CborEncoder encoder;
    std::array<std::byte, 1024> buf;
    cbor_encoder_init(&encoder, (uint8_t*)buf.data(), buf.size(), 0);

    StructWithString item{.val = "hello"};
    auto result = serialization::cbor_encode(&encoder, item);
    EXPECT_EQ(result, CborNoError);
}

TEST(serialize_cbor, arr_float) {
    CborEncoder encoder;
    std::array<std::byte, 1024> buf;
    cbor_encoder_init(&encoder, (uint8_t*)buf.data(), buf.size(), 0);

    const std::array<float, 4> item = {1, 2, 3, 4};
    auto result = serialization::cbor_encode(&encoder, item);
    EXPECT_EQ(result, CborNoError);
}

TEST(serialize_cbor, span_const_uint8) {
    CborEncoder encoder;
    std::array<std::byte, 1024> buf;
    cbor_encoder_init(&encoder, (uint8_t*)buf.data(), buf.size(), 0);

    const std::array<uint8_t, 4> item = {1, 2, 3, 4};
    auto result = serialization::cbor_encode(&encoder, to_cspan(item));
    EXPECT_EQ(result, CborNoError);
}

TEST(PersonExample, round_trip) {
    PersonExample henry;
    henry.gender = PersonGender::MALE;
    henry.age = 25;
    henry.name = "Henry";
    henry.wealth = 100;

    uint8_t henry_blob[] = {'s', 'e', 'c', 'r', 'e', 't', 0};
    henry.blob = henry_blob;

    henry.position[0] = 0.123f;
    henry.position[1] = -2.333f;
    henry.position[2] = 4.113f;
    henry.position[3] = 9.119f;

    henry.pets.push_back({.name = "Fido"});
    henry.pets.push_back({.name = "Romeo"});
    henry.pets.push_back({.name = "Tokyo"});

    std::byte buf[1024];
    size_t buf_size = 0;
    EXPECT_TRUE(
        serialization::serialize_cbor(henry, buf, sizeof(buf), &buf_size));

    PersonExample someone;
    serialization::deserialize_cbor(someone, buf, buf_size);

    EXPECT_EQ(henry.gender, someone.gender);
    EXPECT_EQ(henry.age, someone.age);
    EXPECT_EQ(henry.name, someone.name);
    EXPECT_EQ(henry.wealth, someone.wealth);

    // the blob points into the cbor buffer, which must be kept alive
    // while the blob is being read
    EXPECT_EQ(henry.blob.size(), someone.blob.size());
    for (size_t i = 0; i < someone.blob.size(); ++i) {
        EXPECT_EQ(someone.blob[i], henry.blob[i]);
    }

    EXPECT_EQ(someone.position, henry.position);
    EXPECT_EQ(someone.pets.size(), henry.pets.size());
    for (size_t i = 0; i < someone.pets.size(); ++i) {
        EXPECT_EQ(someone.pets[i].name, henry.pets[i].name);
    }
}

TEST(PersonExample, estimate_size) {
    PersonExample henry;
    henry.gender = PersonGender::MALE;
    henry.age = 25;
    henry.name = "Henry";
    henry.wealth = 100;

    uint8_t henry_blob[] = {'s', 'e', 'c', 'r', 'e', 't', 0};
    henry.blob = henry_blob;

    henry.position[0] = 0.123f;
    henry.position[1] = -2.333f;
    henry.position[2] = 4.113f;
    henry.position[3] = 9.119f;

    henry.pets.push_back({.name = "Fido"});
    henry.pets.push_back({.name = "Romeo"});
    henry.pets.push_back({.name = "Tokyo"});

    std::byte buf[1024];
    size_t buf_size = 0;
    EXPECT_TRUE(
        serialization::serialize_cbor(henry, buf, sizeof(buf), &buf_size));

    size_t size_estimate = serialization::estimate_cbor_size(henry);
    EXPECT_GE(1.5*size_estimate, buf_size);
}

TEST(Bytes, round_trip) {
    std::vector<std::byte> my_bytes{std::byte{0}, std::byte{1}, std::byte{3}};
    std::byte buf[1024];
    size_t buf_size = 0;
    EXPECT_TRUE(
        serialization::serialize_cbor(my_bytes, buf, sizeof(buf), &buf_size));

    std::vector<std::byte> my_bytes_out;
    serialization::deserialize_cbor(my_bytes_out, buf, buf_size);

    EXPECT_TRUE(axby::seq_equals(my_bytes, my_bytes_out));
}

struct MyThing {
    std::string name;
};
TEST(serialize, FastResizableVector) {
    FastResizableVector<MyThing> things;
    things.push_back({ .name = "Alice" });
    things.push_back({.name = "Bob"});

    FastResizableVector<std::byte> serialized;
    serialization::serialize_cbor(things, serialized);
}

TEST(serialize, SmallString) {
    SmallString<10> ss = "hello";
    FastResizableVector<std::byte> serialized;
    serialization::serialize_cbor(ss, serialized);
}
