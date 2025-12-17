#include "gtest/gtest.h"
#include "app/timing.h"
#include "concurrency/single_item.h"

using namespace axby;

TEST(SingleItem_nonblocking, empty) {
    SingleItem<int> item;
    int result = 123;

    EXPECT_FALSE(item.read(result, /*blocking=*/false));
    EXPECT_EQ(result, 123);

    EXPECT_FALSE(item.read(result, /*blocking=*/false));
    EXPECT_EQ(result, 123);

    EXPECT_FALSE(item.read(result, /*blocking=*/false));
    EXPECT_EQ(result, 123);
}

TEST(SingleItem_nonblocking, four_items) {
    SingleItem<int> item;
    item.write(1);
    item.write(2);
    item.write(3);
    item.write(4);

    int result = 123;
    EXPECT_TRUE(item.read(result, /*blocking=*/false));
    EXPECT_EQ(result, 4);
}

TEST(SingleItem_blocking, four_items) {
    SingleItem<int> item;
    item.write(1);
    item.write(2);
    item.write(3);
    item.write(4);

    int result = 123;
    EXPECT_TRUE(item.read(result, /*blocking=*/true));
    EXPECT_EQ(result, 4);
}

TEST(SingleItem_blocking, empty_then_nonempty) {
    SingleItem<int> item;

    std::thread writer {
        [&]() {
            sleep_ms(200);
            item.write(999);
        }
    };

    int result = 0;
    EXPECT_TRUE(item.read(result, /*blocking=*/true));
    EXPECT_EQ(result, 999);

    writer.join();
}
