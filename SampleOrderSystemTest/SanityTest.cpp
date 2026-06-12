#include <gtest/gtest.h>
#include <gmock/gmock.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(Sanity, AlwaysPass) {
    EXPECT_EQ(1 + 1, 2);
}
