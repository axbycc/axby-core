#pragma once

#include "gtest/gtest.h"

void create_window();
void destroy_window();

class GlTestFixture : public ::testing::Test {
   protected:
    void SetUp() override; 
    void TearDown() override; 
};
