#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>

TEST_CASE("Test PrimeChecker 1.")
{
    cv::Point2f blue;
    cv::Point2f yellow;

    blue.x = 100;
    blue.y = 12;

    yellow.x = 400;
    yellow.y = 15;

    REQUIRE(calculateSteeringWheelAngle(blue, yellow, 1234) == 0.0 );
}