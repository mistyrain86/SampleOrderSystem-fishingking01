#ifdef _DEBUG
#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(Sanity, AlwaysPass) {
    EXPECT_EQ(1 + 1, 2);
}
#endif
