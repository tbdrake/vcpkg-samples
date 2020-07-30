#include <gtest/gtest.h>

// main() added because tests are not discovered for some reason with gtest_main.dll
// https://github.com/google/googletest/issues/2157
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
