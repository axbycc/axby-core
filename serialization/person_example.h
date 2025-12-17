#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <span>
#include <vector>

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
    std::span<const std::byte> blob;

    std::vector<PetExample> pets;

    std::array<float, 4> position;
};
