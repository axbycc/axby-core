#include "axgl/texture.h"

#include <glad/mx_shim.h>

#include "gl_test_fixture.h"
#include "gtest/gtest.h"
#include "seq/seq.h"

using namespace axby;

TEST_F(GlTestFixture, round_trip_rgb_f) {
    std::vector<float> data{
        // clang-format off
        1,2,3, 4,5,6,
        6,5,4, 3,2,1
        // clang-format on
    };

    gl::TextureOptions opts =
        gl::TextureOptions().set_rgb().set_data_type<float>();
    gl::Texture texture(opts);

    texture.upload(2, 2, data);

    std::vector<float> downloaded_data(data.size(), 0.0f);
    texture.download(downloaded_data);

    EXPECT_TRUE(axby::seq_equals(downloaded_data, data));
}

TEST_F(GlTestFixture, round_trip_rg_f) {
    std::vector<float> data{
        // clang-format off
        1,2, 4,5, 6,5,
        4,3, 2,1, 9,9
        // clang-format on
    };

    gl::TextureOptions opts =
        gl::TextureOptions().set_rg().set_data_type<float>();
    gl::Texture texture(opts);

    texture.upload(3, 2, data);

    std::vector<float> downloaded_data(data.size(), 0.0f);
    texture.download(downloaded_data);

    EXPECT_TRUE(axby::seq_equals(downloaded_data, data));
}

TEST_F(GlTestFixture, round_trip_r_f) {
    std::vector<float> data{
        // clang-format off
        1, 2, 4, 5,
        6, 5, 4, 3,
        2, 1, 9, 9
        // clang-format on
    };

    gl::TextureOptions opts =
        gl::TextureOptions().set_r().set_data_type<float>();
    gl::Texture texture(opts);

    texture.upload(4, 3, data);

    std::vector<float> downloaded_data(data.size(), 0.0f);
    texture.download(downloaded_data);

    EXPECT_TRUE(axby::seq_equals(downloaded_data, data));
}

TEST_F(GlTestFixture, double_upload_round_trip) {
    gl::TextureOptions opts =
        gl::TextureOptions().set_r().set_data_type<float>();
    gl::Texture texture(opts);

    {
        std::vector<float> data{
            // clang-format off
        1, 2, 4, 5,
        6, 5, 4, 3,
        2, 1, 9, 9
            // clang-format on
        };
        texture.upload(4, 3, data);
        std::vector<float> downloaded_data(data.size(), 0.0f);
        texture.download(downloaded_data);
        EXPECT_TRUE(axby::seq_equals(downloaded_data, data));
    }
    {
        std::vector<float> data{
            // clang-format off
        0, 2, 4, 
        6, 0, 4, 
        2, 1, 0,
            // clang-format on
        };

        texture.upload(3, 3, data);

        std::vector<float> downloaded_data(data.size(), 0.0f);
        texture.download(downloaded_data);

        EXPECT_TRUE(axby::seq_equals(downloaded_data, data));
    }
}
