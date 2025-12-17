#include <algorithm>
#include <cstdint>
#include <random>

namespace axby {

uint64_t init_process_id() {
    // see https://codereview.stackexchange.com/a/212751/
    std::mt19937_64 rng;
    std::random_device rdev;
    std::seed_seq::result_type data[std::mt19937_64::state_size];
    std::generate_n(data, std::mt19937_64::state_size, std::ref(rdev));

    std::seed_seq prng_seed(data, data + std::mt19937_64::state_size);
    rng.seed(prng_seed);

    return rng();
}

uint64_t process_id_ = init_process_id();

uint64_t get_process_id() { return process_id_; }
void force_process_id(uint64_t process_id) {
    process_id_= process_id;
}

}
