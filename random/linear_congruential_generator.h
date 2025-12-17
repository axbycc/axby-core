#include <cstdint>

#include "seq/seq.h"

namespace axby {
struct LinearCongruentialGenerator {
    uint32_t seed = 0;
    float generate();

    // fills the out span with random indices in the range 0 to
    // out.size(), without replacement. this is useful for choosing a
    // random subset of a collection.
    void generate_sample_idxs(size_t collection_size, Seq<size_t> out);
};
}  // namespace axby
