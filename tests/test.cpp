#include "CppUTest/TestHarness.h"
#include "castle.h"

TEST_GROUP(FirstTestGroup)
{
};

TEST(FirstTestGroup, FirstTest){
    CHECK(1 == castle_init());
}
