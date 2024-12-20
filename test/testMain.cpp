/// @file      testMain.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0
/// @details  This is the main file for test running.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

/// @cond NO_DOC

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

/// @endcond
