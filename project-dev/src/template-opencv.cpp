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

/*---------------- Global variables ---------------------*/

// Yellow hsv values
const int hMinY = 15;  // hue
const int hMaxY = 25;  // hue
const int sMinY = 75;  // saturation
const int sMaxY = 185; // saturation
const int vMinY = 147; // value
const int vMaxY = 255; // value

// Blue hsv values
const int hMinB = 100; // hue
const int hMaxB = 140; // hue
const int sMinB = 120; // saturation
const int sMaxB = 255; // saturation
const int vMinB = 40;  // value
const int vMaxB = 255; // value

// Car's position and thresholds
const int carPosition = 240;
const int middleLeft = 120;
const int middleRight = 360;

/*---------------- Function definitions ---------------------*/
cv::Point2f  drawContourWithCentroidPoint(cv::Mat inputImage,cv::Mat outputImage, int contourArea, cv::Scalar contourColor, cv::Scalar centroidColor);
double calculateSteeringWheelAngle(cv::Point2f blueCone, cv::Point2f yellowCone,int timestamp);
double calculateSteeringWheelAngleCounter(cv::Point2f blueCone, cv::Point2f yellowCone,int timestamp);

/*---------------- Main program ---------------------*/

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

            std::ofstream myFile("../test.csv");

            // print to csv
             myFile << "Timestamp " << ",";
            myFile << "GroundSteeringRequest " << ",";
            myFile << "Calculated Angle";
            myFile << "\n";

            // variables
            double NrOfCorrectAngle = 0;
            double NrOfCorrectAngleCounter = 0;
            double frames = 0;
            double calculatedAngle;
            cv::Point2f previousBluecone;
            cv::Point2f previousYellowCone;
            double previousCalculatedAngle;

            // Endless loop; end the program by pressing Ctrl-C.
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
            
                auto [_, ts] = sharedMemory->getTimeStamp();
                auto ms = static_cast<int64_t>(ts.seconds()) * static_cast<int64_t>(1000 * 1000) + static_cast<int64_t>(ts.microseconds());
                std::string output = "TS: " + std::to_string(ms) + "; GROUND STEERING: " + std::to_string(gsr.groundSteering());

                sharedMemory->unlock();
                cropedImg = img(cv::Range(310, 360), cv::Range(0, 640));
                cv::cvtColor(cropedImg, hsvImg, CV_BGR2HSV); // Convert Original Image to HSV Thresh Image

                cv::inRange(hsvImg, cv::Scalar(hMinB, sMinB, vMinB), cv::Scalar(hMaxB, sMaxB, vMaxB), blueThreshImg);
                cv::inRange(hsvImg, cv::Scalar(hMinY, sMinY, vMinY), cv::Scalar(hMaxY, sMaxY, vMaxY), yellowThreshImg);
                cv::Scalar blue = cv::Scalar(255,0,0);
                cv::Scalar red = cv::Scalar(0,0,255);
                cv::Scalar green = cv::Scalar(0,255,0);


                cv::Point2f blueCone = drawContourWithCentroidPoint(blueThreshImg,cropedImg,75,blue,red);
                cv::Point2f yellowCone = drawContourWithCentroidPoint(yellowThreshImg,cropedImg,75,green,red);

                // checking the direction
                if(previousBluecone.x > 0){
                    if(previousBluecone.x > blueCone.x){
                        // moving to the right counter clockwise
                        calculatedAngle = calculateSteeringWheelAngle(blueCone, yellowCone, ms);
                    } else {
                        // moving to the left clockwise
                        calculatedAngle = calculateSteeringWheelAngleCounter(blueCone,  yellowCone, ms);

                    }
                } 
                else if(previousYellowCone.x > 0){
                    if(previousYellowCone.x > yellowCone.x){
                         // moving to the right clockwise
                        calculatedAngle = calculateSteeringWheelAngleCounter(blueCone,  yellowCone, ms);
                    } else {
                        // moving to the left counter clockwise
                        calculatedAngle = calculateSteeringWheelAngle(blueCone, yellowCone, ms);
                     }
                } else {
                    // if we don't know the direction we just assume the steering wheel angle to be 0.
                    calculatedAngle = previousCalculatedAngle;
                }
                
                // assign the values to the variables
                previousBluecone = blueCone;
                previousYellowCone = yellowCone;
                previousCalculatedAngle = calculatedAngle;
                frames++;  

                // check if the calculated angle within 0.05 deviation
                if(calculatedAngle < gsr.groundSteering() + 0.05 && calculatedAngle > gsr.groundSteering() - 0.05 ){
                    NrOfCorrectAngle++;
                }

                // Write to file
                myFile << std::to_string(ms) << ",";
                myFile << std::to_string(gsr.groundSteering()) << ",";
                myFile << std::to_string(calculatedAngle);
                myFile << "\n";
                
                output.append(" CalAng: " + std::to_string(calculatedAngle));
                cv::putText(img,                        // target image
                            output,                     // text
                            cv::Point(0, img.rows / 2), // top-left position
                            cv::FONT_HERSHEY_PLAIN,
                            1.0,
                            CV_RGB(0, 0, 255),          // font color
                            1);
                // If you want to access the latest received ground steering, don't forget to lock the mutex:
                {
                    std::lock_guard<std::mutex> lck(gsrMutex);

                    std::cout << "Group 15; " << std::to_string(ms) << "; " << std::to_string(calculatedAngle) << std::endl;
                }

                // Display image on your screen.
                if (VERBOSE)
                {
                    cv::imshow(sharedMemory->name().c_str(), img);
                    cv::imshow("Cropped Image", cropedImg);
                    cv::waitKey(1);
                }
            }
            myFile.close();

            // Calculate percentage and print result to the console
            double percentage = NrOfCorrectAngle / frames;
            std::cout << "Percentage: " << std::to_string(percentage) << std::endl;
            std::cout << "Frames: " << std::to_string(frames) << std::endl;
            std::cout << "Nr Correct Angle: " << std::to_string(NrOfCorrectAngle) << std::endl;
        }
        retCode = 0;
    }
    return retCode;
}

/*---------------- Functions ---------------------*/
// This method returns the centre point of the cone
cv::Point2f drawContourWithCentroidPoint(cv::Mat inputImage, cv::Mat outputImage, int contourArea, cv::Scalar contourColor, cv::Scalar centroidColor)
{
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    // convert to gray scale
    // draw blue cones' contour
    cv::Mat img_channels[3];
    cv::split(inputImage, img_channels);
    cv::Mat img_gray = img_channels[0];
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
        }
    }
    return cone;
}


// Method to calculate the steering wheel angle. Works for clockwise (blue cones on the left). We are assuming that the fram is 480px wide
// and that the car's position is constant in the middle at 240px.

double calculateSteeringWheelAngle(cv::Point2f blueCone, cv::Point2f yellowCone,int timestamp){
    
    double angle = 0.0;

    if(blueCone.x < middleLeft || yellowCone.x > middleRight){
        // ---------------------------
        // |  B   |      |      |  Y  |
        // ---------------------------
        // Don't turn
        angle = 0.0;

    } else if (blueCone.x < carPosition && blueCone.x > middleLeft){
        // ---------------------------
        // |     |   B   |      |     |
        // ---------------------------   
        // Turn Right negative value
        angle = -0.1;

    } else if (blueCone.x > carPosition && blueCone.x < middleRight) {
        // ---------------------------
        // |     |     |   B  |     |
        // ---------------------------   
        // Turn sharp Right negative value
        angle = -0.2;

    } else if (blueCone.x < 480 && blueCone.x > middleRight) {
        // ---------------------------
        // |     |     |     |  B   |
        // ---------------------------   
        // Turn super sharp Right negative value
        angle = -0.25;
    } else if (yellowCone.x < middleRight && yellowCone.x > carPosition) {
        // ---------------------------
        // |     |     |   Y  |     |
        // ---------------------------   
        // Turn Left
        angle = 0.1;
    } else if (yellowCone.x < carPosition && yellowCone.x > middleLeft) {
        // ---------------------------
        // |     |  Y   |     |     |
        // ---------------------------   
        // Turn sharp Left postive value
        angle = 0.2;
    } else if (yellowCone.x > 5 && yellowCone.x < middleLeft) {
        // ---------------------------
        // | Y   |     |     |     |
        // ---------------------------   
        // Turn supersharp left positive value
        angle = 0.25;
    }
    return angle;
}

// Method to calculate the steering wheel angle. Works for counter clockwise (yellow cones on the left).
double calculateSteeringWheelAngleCounter(cv::Point2f blueCone, cv::Point2f yellowCone,int timestamp){

    double angle = 0.0;

    if(yellowCone.x < middleLeft || blueCone.x > middleRight){
        // ---------------------------
        // |  Y   |      |      |  B  |
        // ---------------------------
        // Don't turn
        angle = 0.0;

    } else if (yellowCone.x < carPosition && yellowCone.x > middleLeft){
        // ---------------------------
        // |     |   Y   |      |     |
        // ---------------------------   
        // Turn Right negative value
        angle = -0.1;

    } else if (yellowCone.x > carPosition && yellowCone.x < middleRight) {
        // ---------------------------
        // |     |     |  Y |     |
        // ---------------------------   
        // Turn sharp Right negative value
        angle = -0.2;
    }else if ( yellowCone.x > middleRight && yellowCone.x < 480) {
        // ---------------------------
        // |     |     |     |   Y  |
        // ---------------------------   
        // Turn supersharp Right negative value
        angle = -0.25;
    } else if (blueCone.x < middleLeft && blueCone.x > carPosition) {
        // ---------------------------
        // |     |     |   B  |     |
        // ---------------------------   
        // Turn Left
        angle = 0.1;
    } else if (blueCone.x < carPosition && blueCone.x > middleLeft) {
        // ---------------------------
        // |     |  B  |     |     |
        // ---------------------------   
        // Turn sharp Left value
        angle = 0.2;
    } else if (blueCone.x < middleLeft && blueCone.x > 5) {
        // ---------------------------
        // |  B   |     |     |     |
        // ---------------------------   
        // Turn supersharp Left value
        angle = 0.25;
    }

    return angle;
}

