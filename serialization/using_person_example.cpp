#include <iostream>
#include <magic_enum.hpp>

#include "make_serializable.hpp"
#include "person_example.h"
#include "serialization.h"
#include "util/span.h"

int main(int argc, char* argv[]) {
    PersonExample henry;
    henry.gender = PersonGender::MALE;
    henry.age = 25;
    henry.name = "Henry";
    henry.wealth = 100;

    uint8_t henry_blob[] = {'s', 'e', 'c', 'r', 'e', 't', 0};
    henry.blob = reinterpret_span<std::byte>(henry_blob);

    henry.position[0] = 0.123f;
    henry.position[1] = -2.333f;
    henry.position[2] = 4.113f;
    henry.position[3] = 9.119f;

    henry.pets.push_back({.name = "Fido"});
    henry.pets.push_back({.name = "Romeo"});
    henry.pets.push_back({.name = "Tokyo"});

    uint8_t buf[1024];
    size_t buf_size = 0;
    CHECK(serialization::serialize_cbor(henry, buf, sizeof(buf), &buf_size));

    PersonExample someone;
    serialization::deserialize_cbor(someone, buf, buf_size);

    std::cout << "someone.gender " << magic_enum::enum_name(someone.gender) << "\n";
    std::cout << "someone.age " << someone.age << "\n";
    std::cout << "someone.name " << someone.name
              << ", size=" << someone.name.size() << "\n";
    std::cout << "someone.wealth " << someone.wealth << "\n";

    // the blob points into the cbor buffer, which must be kept alive
    // while the blob is being read
    std::cout << "someone.blob size " << someone.blob.size() << "\n";
    std::cout << "someone.blob " << (char*)someone.blob.data() << "\n";

    std::cout << "someone.position " << seq_to_string(henry.position)
              << "\n";

		std::cout << "Pets (" << someone.pets.size() << ") \n";
    for (auto& pet : someone.pets) {
        std::cout << "\t" << pet.name << "\n";
    }

    return 0;
}
