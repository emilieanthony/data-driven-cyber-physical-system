/*
 * Copyright (C) 2020  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Include the single-file, header-only middleware libcluon to create high-performance microservices
#include "cluon-complete.hpp"
// Include the OpenDLV Standard Message Set that contains messages that are usually exchanged for automotive or robotic applications
#include "opendlv-standard-message-set.hpp"

// Include the GUI and image processing header files from OpenCV
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>

// Yellow hsv values
const int hMinY = 15;
const int hMaxY = 25;
const int sMinY = 75;
const int sMaxY = 185;
const int vMinY = 147;
const int vMaxY = 255;

// Blue hsv values
const int hMinB = 100;
const int hMaxB = 140;
// S-values:
// minValue: 75 is too low because it will show the things with high reflections
// 150 will only show the clostest ones
const int sMinB = 120;
const int sMaxB = 255;
// V-values:
// 40-120 works
// 0 will take in reflctions
// 100 will only show the nearest cone 
const int vMinB = 40;
const int vMaxB = 255;
cv::Point2f  drawContourWithCentroidPoint(cv::Mat inputImage,cv::Mat outputImage, int contourArea, cv::Scalar contourColor, cv::Scalar centroidColor);
double calculateSteeringWheelAngle(cv::Point2f blueCone, cv::Point2f yellowCone,int timestamp);
double calculateSteeringWheelAngleCounter(cv::Point2f blueCone, cv::Point2f yellowCone,int timestamp);

int32_t main(int32_t argc, char **argv)
{
    int32_t retCode{1};
    // Parse the command line parameters as we require the user to specify some mandatory information on startup.
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ((0 == commandlineArguments.count("cid")) ||
        (0 == commandlineArguments.count("name")) ||
        (0 == commandlineArguments.count("width")) ||
        (0 == commandlineArguments.count("height")))
    {
        std::cerr << argv[0] << " attaches to a shared memory area containing an ARGB image." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OD4 session> --name=<name of shared memory area> [--verbose]" << std::endl;
        std::cerr << "         --cid:    CID of the OD4Session to send and receive messages" << std::endl;
        std::cerr << "         --name:   name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:  width of the frame" << std::endl;
        std::cerr << "         --height: height of the frame" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=253 --name=img --width=640 --height=480 --verbose" << std::endl;
    }
    else
    {
        // Extract the values from the command line parameters
        const std::string NAME{commandlineArguments["name"]};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};

        // Attach to the shared memory.
        std::unique_ptr<cluon::SharedMemory> sharedMemory{new cluon::SharedMemory{NAME}};
        if (sharedMemory && sharedMemory->valid())
        {
            std::clog << argv[0] << ": Attached to shared memory '" << sharedMemory->name() << " (" << sharedMemory->size() << " bytes)." << std::endl;

            // Interface to a running OpenDaVINCI session where network messages are exchanged.
            // The instance od4 allows you to send and receive messages.
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            opendlv::proxy::GroundSteeringRequest gsr;
            std::mutex gsrMutex;
            auto onGroundSteeringRequest = [&gsr, &gsrMutex](cluon::data::Envelope &&env)
            {
                // The envelope data structure provide further details, such as sampleTimePoint as shown in this test case:
                // https://github.com/chrberger/libcluon/blob/master/libcluon/testsuites/TestEnvelopeConverter.cpp#L31-L40
                std::lock_guard<std::mutex> lck(gsrMutex);
                gsr = cluon::extractMessage<opendlv::proxy::GroundSteeringRequest>(std::move(env));
                std::cout << "lambda: groundSteering = " << gsr.groundSteering() << std::endl;
            };

            od4.dataTrigger(opendlv::proxy::GroundSteeringRequest::ID(), onGroundSteeringRequest);

            // Endless loop; end the program by pressing Ctrl-C.

            std::ofstream myFile("test.csv");

            double NrOfCorrectAngle = 0;
            double NrOfCorrectAngleCounter = 0;
            double frames = 0;

            while (od4.isRunning())
            {
                // OpenCV data structure to hold an image.
                cv::Mat img;
                cv::Mat cropedImg;
                cv::Mat hsvImg;        // HSV Image
                cv::Mat blueThreshImg; //  blue Thresh Image
                cv::Mat yellowThreshImg;

                // Wait for a notification of a new frame.
                sharedMemory->wait();

                // Lock the shared memory.
                sharedMemory->lock();
                {
                    // Copy the pixels from the shared memory into our own data structure.
                    cv::Mat wrapped(HEIGHT, WIDTH, CV_8UC4, sharedMemory->data());
                    img = wrapped.clone();
                }
                // TODO: Here, you can add some code to check the sampleTimePoint when the current frame was captured.

                auto [_, ts] = sharedMemory->getTimeStamp();
                auto ms = static_cast<int64_t>(ts.seconds()) * static_cast<int64_t>(1000 * 1000) + static_cast<int64_t>(ts.microseconds());
                std::string output = "TS: " + std::to_string(ms) + "; GROUND STEERING: " + std::to_string(gsr.groundSteering());

                sharedMemory->unlock();
                cropedImg = img(cv::Range(310, 370), cv::Range(0, 640));
                cv::cvtColor(cropedImg, hsvImg, CV_BGR2HSV); // Convert Original Image to HSV Thresh Image

                // TODO: Do something with the frame.
                cv::inRange(hsvImg, cv::Scalar(hMinB, sMinB, vMinB), cv::Scalar(hMaxB, sMaxB, vMaxB), blueThreshImg);
                cv::inRange(hsvImg, cv::Scalar(hMinY, sMinY, vMinY), cv::Scalar(hMaxY, sMaxY, vMaxY), yellowThreshImg);
                cv::Scalar blue = cv::Scalar(255,0,0);
                cv::Scalar red = cv::Scalar(0,0,255);
                cv::Scalar green = cv::Scalar(0,255,0);


                cv::Point2f blueCordinates = drawContourWithCentroidPoint(blueThreshImg,cropedImg,75,blue,red);
                cv::Point2f yellowCordinates = drawContourWithCentroidPoint(yellowThreshImg,cropedImg,75,green,red);

                double calculatedAngle = calculateSteeringWheelAngle(blueCordinates, yellowCordinates, ms);
                double calculatedAngleCounter = calculateSteeringWheelAngleCounter(blueCordinates,  yellowCordinates, ms);

                frames++;  
                // Is the calculated angle within 0.05 deviation
                if(calculatedAngle < gsr.groundSteering() + 0.05 && calculatedAngle > gsr.groundSteering() - 0.05 ){
                    NrOfCorrectAngle++;
                }

                if(calculatedAngleCounter < gsr.groundSteering() + 0.05 && calculatedAngleCounter > gsr.groundSteering() - 0.05 ){
                    NrOfCorrectAngleCounter++;
                }

                // Calculate Percentage

                std::string cordinates; 

                // Write to file
                myFile << std::to_string(ms) << ",";
                myFile << std::to_string(gsr.groundSteering()) << ",";
                myFile << std::to_string(calculatedAngle);
                myFile << "\n";


                // for(int i = 0; i < blueCordinates.size(); i++){

                //     if(blueCordinates[i].x > 0){
                //         myFile << std::to_string(blueCordinates[i].x) << ",";

                //     }
                // }

                // for(int i = 0; i < yellowCordinates.size(); i++){

                //     if(yellowCordinates[i].x > 0){
                //         myFile << std::to_string(yellowCordinates[i].x) << ",";

                //     }
                // }
                


                output.append(" CalAng: " + std::to_string(calculatedAngle));
                std::cout << "Calculated angle: " << std::to_string(calculatedAngle) << std::endl; 

                cv::putText(img,                        // target image
                            output,                     // text
                            cv::Point(0, img.rows / 2), // top-left position
                            cv::FONT_HERSHEY_PLAIN,
                            1.0,
                            CV_RGB(0, 0, 255), // font color
                            1);
                cv::putText(img,                        // target image
                            cordinates,                     // text
                            cv::Point(10, 50), // top-left position
                            cv::FONT_HERSHEY_PLAIN,
                            1.0,
                            CV_RGB(0, 0, 255), // font color
                            1);


                // If you want to access the latest received ground steering, don't forget to lock the mutex:
                {
                    std::lock_guard<std::mutex> lck(gsrMutex);

                    std::cout << "main: groundSteering = " << gsr.groundSteering() << std::endl;
                }

                // Display image on your screen.
                if (VERBOSE)
                {
                    cv::imshow(sharedMemory->name().c_str(), img);
                    cv::imshow("blueImgWithPoint", cropedImg);
                    //cv::imshow("YellowImgWithPoint", yellowResultImg);
                    cv::waitKey(1);
                }
            }
            myFile.close();
            double percentage = NrOfCorrectAngle / frames;
            double percentageCounter = NrOfCorrectAngleCounter / frames;

            std::cout << "Percentage: " << std::to_string(percentage) << std::endl;
            std::cout << "Percentage Counter: " << std::to_string(percentageCounter) << std::endl;

            std::cout << "Frames: " << std::to_string(frames) << std::endl;
            std::cout << "Nr Correct Angle: " << std::to_string(NrOfCorrectAngle) << std::endl;
            std::cout << "Nr Correct Angle  Counter: " << std::to_string(NrOfCorrectAngleCounter) << std::endl;
        }
        retCode = 0;


    }

    return retCode;
}

cv::Point2f drawContourWithCentroidPoint(cv::Mat inputImage, cv::Mat outputImage, int contourArea, cv::Scalar contourColor, cv::Scalar centroidColor)
{
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    // convert to gray scale
    // draw blue cones' contour
    cv::Mat img_channels[3];
    cv::split(inputImage, img_channels);
    cv::Mat img_gray = img_channels[0];
    // cv::Mat canny_img;
    // cv::Canny(img_gray, canny_img, 50, 60);
    cv::findContours(img_gray, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    // get the moments
    std::vector<cv::Moments> mu(contours.size());
    std::vector<cv::Point2f> mc(mu.size());
    cv::Point2f cone;



    if (contours.size() > 0)
    {
        for (int i = 0; i < contours.size(); i++)
        {
            if (cv::contourArea(contours[i]) > contourArea)
            {
                mu[i] = cv::moments(contours[i], false);
            }
        }

        // get the centroid of figures.
        for (int i = 0; i < mu.size(); i++)
        {
            mc[i] = cv::Point2f(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
        }
        for (int i = 0; i < mu.size(); i++)
        {
            circle(outputImage, mc[i], 4, centroidColor, -1, 8, 0);

            if(mc[i].x > 0 ){
                // then we have a value
                cone = mc[i];
            }
            //std::cout << "x: " << mc[i].x << "y: " << mc[i].y << std::endl;
        }
    }
    cv::drawContours(outputImage, contours, -1, contourColor, 2);
    return cone;
}



// Method to calculate the steering wheel angle. Works for clockwise (blue cones on the left) about 50% correct. 
// When running this counter clockwise we get about 20 % 
double calculateSteeringWheelAngle(cv::Point2f blueCone, cv::Point2f yellowCone,int timestamp){
    // car position 0 480/2 240

    int carPosition = 240;
    int middleLeft = 120;
    int middleRight = 360;

    double angle = 0.0;

    // 120 240 360 480 

    // Right is negative
    // left is positive

    if(yellowCone.x > middleRight && blueCone.x < middleLeft){
        // ---------------------------
        // |  X   |      |      |  X  |
        // ---------------------------
        // Dont turn
        angle = 0.0;

    } else if (blueCone.x < carPosition && blueCone.x > middleLeft){
        // ---------------------------
        // |     |   x   |      |     |
        // ---------------------------   
        // Turn Right negative value
        angle = -0.1;

    } else if (blueCone.x > carPosition && blueCone.x < middleRight) {
        // ---------------------------
        // |     |     |   x  |     |
        // ---------------------------   
        // Turn sharp Right negative value
        angle = -0.2;

    } else if (blueCone.x > carPosition && blueCone.x > middleRight) {
        // ---------------------------
        // |     |     |     |  x   |
        // ---------------------------   
        // Turn super sharp Right negative value
        angle = -0.25;
    } else if (yellowCone.x < middleLeft && yellowCone.x > carPosition) {
        // ---------------------------
        // |     |     |   x  |     |
        // ---------------------------   
        // Turn Left
        angle = 0.1;
    } else if (yellowCone.x < carPosition && yellowCone.x > middleLeft) {
        // ---------------------------
        // |     |  x   |     |     |
        // ---------------------------   
        // Turn sharp Left negative value
        angle = 0.2;
    } else if (blueCone.x > carPosition && blueCone.x > middleRight) {
        // ---------------------------
        // |     |     |   x  |     |
        // ---------------------------   
        // Turn supersharp left negative value
        angle = -0.25;
    }

    return angle;
}

// Method to calculate the steering wheel angle. Works for counter clockwise (yellow cones on the left) about 50% correct. 
// When running this counter clockwise we get about 20 % 
double calculateSteeringWheelAngleCounter(cv::Point2f blueCone, cv::Point2f yellowCone,int timestamp){
    // car position 0 480/2 240

    int carPosition = 240;
    int middleLeft = 120;
    int middleRight = 360;

    double angle = 0.0;

    // 120 240 360 480 

    // Right is negative
    // left is positive

    if(blueCone.x > middleRight && yellowCone.x < middleLeft){
        // ---------------------------
        // |  X   |      |      |  X  |
        // ---------------------------
        // Dont turn
        angle = 0.0;

    } else if (yellowCone.x < carPosition && yellowCone.x > middleLeft){
        // ---------------------------
        // |     |   x   |      |     |
        // ---------------------------   
        // Turn Right negative value
        angle = -0.1;

    } else if (yellowCone.x > carPosition && yellowCone.x < middleRight) {
        // ---------------------------
        // |     |     |   x  |     |
        // ---------------------------   
        // Turn sharp Right negative value
        angle = -0.2;
    } else if (blueCone.x < middleLeft && blueCone.x > carPosition) {
        // ---------------------------
        // |     |     |   x  |     |
        // ---------------------------   
        // Turn Left
        angle = 0.1;
    } else if (blueCone.x < carPosition && blueCone.x > middleLeft) {
        // ---------------------------
        // |     |  x   |     |     |
        // ---------------------------   
        // Turn sharp Left negative value
        angle = 0.2;
    }

    return angle;
}

