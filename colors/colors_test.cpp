#include "colors.h"

#include <imgui.h>

#include "gtest/gtest.h"
#include "seq/seq.h"

using namespace axby;

TEST(imgui_compat, red_manual_convert) {
    EXPECT_EQ((uint32_t)IM_COL32(colors::red.red, colors::red.green,
                                 colors::red.blue, 255),
              (uint32_t)IM_COL32(255, 0, 0, 255));
}

TEST(imgui_compat, red) {
    EXPECT_EQ(to_imgui(colors::red), (uint32_t)IM_COL32(255, 0, 0, 255));
}

TEST(RGBA, operator_squarebracket) {
    colors::RGBA color{1, 2, 3, 4};
    EXPECT_EQ(color[0], 1);
    EXPECT_EQ(color[1], 2);
    EXPECT_EQ(color[2], 3);
    EXPECT_EQ(color[3], 4);
}

TEST(RGBAf, operator_squarebracket) {
    colors::RGBAf color{0.1f, 0.2f, 0.3f, 0.4f};
    EXPECT_EQ(color[0], 0.1f);
    EXPECT_EQ(color[1], 0.2f);
    EXPECT_EQ(color[2], 0.3f);
    EXPECT_EQ(color[3], 0.4f);
}

TEST(to_uint8, case1) {
    colors::RGBAf colorf{0.1f, 0.2f, 0.3f, 0.4f};
    colors::RGBA color = colors::to_uint8(colorf);
    EXPECT_EQ(color[0], uint8_t(colorf[0] * 255));
    EXPECT_EQ(color[1], uint8_t(colorf[1] * 255));
    EXPECT_EQ(color[2], uint8_t(colorf[2] * 255));
    EXPECT_EQ(color[3], uint8_t(colorf[3] * 255));
}

TEST(to_float, case1) {
    colors::RGBA color{8, 10, 50, 100};
    colors::RGBAf colorf = colors::to_float(color);
    EXPECT_EQ(colorf[0], float(color[0]) / 255);
    EXPECT_EQ(colorf[1], float(color[1]) / 255);
    EXPECT_EQ(colorf[2], float(color[2]) / 255);
    EXPECT_EQ(colorf[3], float(color[3]) / 255);
}

TEST(RGBA, field_names) {
    colors::RGBA color{8, 10, 50, 100};

    EXPECT_EQ(color.red, color[0]);
    EXPECT_EQ(color.green, color[1]);
    EXPECT_EQ(color.blue, color[2]);
    EXPECT_EQ(color.alpha, color[3]);
}

TEST(RGBAf, field_names) {
    colors::RGBAf color{0.1f, 0.2f, 0.3f, 0.5f};

    EXPECT_EQ(color.red, color[0]);
    EXPECT_EQ(color.green, color[1]);
    EXPECT_EQ(color.blue, color[2]);
    EXPECT_EQ(color.alpha, color[3]);
}

TEST(RGBA, seq_integration) {
    colors::RGBA color{8, 10, 50, 100};
    std::array<uint8_t, 4> color_array;
    seq_copy(color, color_array);

    EXPECT_EQ(color_array[0], color[0]);
    EXPECT_EQ(color_array[1], color[1]);
    EXPECT_EQ(color_array[2], color[2]);
    EXPECT_EQ(color_array[3], color[3]);
}

TEST(RGBAf, seq_integration) {
    colors::RGBAf color{0.1f, 0.2f, 0.3f, 0.4f};
    std::array<float, 4> color_array;
    seq_copy(color, color_array);

    EXPECT_EQ(color_array[0], color[0]);
    EXPECT_EQ(color_array[1], color[1]);
    EXPECT_EQ(color_array[2], color[2]);
    EXPECT_EQ(color_array[3], color[3]);
}

TEST(RGBA, make_rgba) {
    colors::RGBA c1{1, 2, 3, 4};
    colors::RGBA c2 = colors::make_rgba({1, 2, 3, 4});
    EXPECT_EQ(c1[0], c2[0]);
    EXPECT_EQ(c1[1], c2[1]);
    EXPECT_EQ(c1[2], c2[2]);
    EXPECT_EQ(c1[3], c2[3]);

}
