#include "linear_congruential_generator.h"

#include "debug/check.h"

namespace axby {
float LinearCongruentialGenerator::generate() {
    const uint32_t a = 1664525;
    const uint32_t c = 1013904223;
    seed = (a * seed + c);  // modulo 2^32
    return seed / static_cast<float>(4294967296);
}

void LinearCongruentialGenerator::generate_sample_idxs(size_t collection_size,
                                                       Seq<size_t> out) {
    const auto num_indices_needed = out.size();
    CHECK(num_indices_needed <= collection_size);

    size_t num_indices_used = 0;

    for (size_t i = 0; i < collection_size; ++i) {
        const size_t reservoir_size = collection_size - i;
        const size_t num_indices_missing =
            num_indices_needed - num_indices_used;
        CHECK(num_indices_missing <= reservoir_size);

        if (num_indices_missing == 0) break;

        const float prob = float(num_indices_missing) / reservoir_size;
        const float U = generate();
        const bool take = U <= prob;

        if (take) {
            out[num_indices_used] = i;
            num_indices_used++;
        }
    }
}
}
