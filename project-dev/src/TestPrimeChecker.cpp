#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "PrimeChecker.hpp"

TEST_CASE("Test PrimeChecker 1.")
{
    PrimeChecker pc;
    REQUIRE(pc.isPrime(5));
}

TEST_CASE("Test PrimeChecker 2.")
{
    PrimeChecker pc;
    REQUIRE(!pc.isPrime(4));
}