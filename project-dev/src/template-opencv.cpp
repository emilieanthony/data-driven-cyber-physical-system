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

// Yellow hsv values
const int hMinY = 15;
const int hMaxY = 25;
const int sMinY = 75;
const int sMaxY = 185;
const int vMinY = 147;
const int vMaxY = 255;

// Blue hsv values
const int hMinB = 118;
const int hMaxB = 179;
const int sMinB = 78;
const int sMaxB = 125;
const int vMinB = 35;
const int vMaxB = 63;

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    // Parse the command line parameters as we require the user to specify some mandatory information on startup.
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ||
         (0 == commandlineArguments.count("name")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ) {
        std::cerr << argv[0] << " attaches to a shared memory area containing an ARGB image." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OD4 session> --name=<name of shared memory area> [--verbose]" << std::endl;
        std::cerr << "         --cid:    CID of the OD4Session to send and receive messages" << std::endl;
        std::cerr << "         --name:   name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:  width of the frame" << std::endl;
        std::cerr << "         --height: height of the frame" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=253 --name=img --width=640 --height=480 --verbose" << std::endl;
    }
    else {
        // Extract the values from the command line parameters
        const std::string NAME{commandlineArguments["name"]};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};

        // Attach to the shared memory.
        std::unique_ptr<cluon::SharedMemory> sharedMemory{new cluon::SharedMemory{NAME}};
        if (sharedMemory && sharedMemory->valid()) {
            std::clog << argv[0] << ": Attached to shared memory '" << sharedMemory->name() << " (" << sharedMemory->size() << " bytes)." << std::endl;

            // Interface to a running OpenDaVINCI session where network messages are exchanged.
            // The instance od4 allows you to send and receive messages.
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            opendlv::proxy::GroundSteeringRequest gsr;
            std::mutex gsrMutex;
            auto onGroundSteeringRequest = [&gsr, &gsrMutex](cluon::data::Envelope &&env){
                // The envelope data structure provide further details, such as sampleTimePoint as shown in this test case:
                // https://github.com/chrberger/libcluon/blob/master/libcluon/testsuites/TestEnvelopeConverter.cpp#L31-L40
                std::lock_guard<std::mutex> lck(gsrMutex);
                gsr = cluon::extractMessage<opendlv::proxy::GroundSteeringRequest>(std::move(env));
                std::cout << "lambda: groundSteering = " << gsr.groundSteering() << std::endl;
            };

            od4.dataTrigger(opendlv::proxy::GroundSteeringRequest::ID(), onGroundSteeringRequest);

            // Endless loop; end the program by pressing Ctrl-C.
            while (od4.isRunning()) {
                // OpenCV data structure to hold an image.
                cv::Mat img;
                cv::Mat hsvImg;    // HSV Image
                cv::Mat threshImg;   // Thresh Image

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

                // TODO: Do something with the frame.                
                cv::cvtColor(img, hsvImg, CV_BGR2HSV);      // Convert Original Image to HSV Thresh Image
                cv::inRange(hsvImg, cv::Scalar(hMinY, sMinY, vMinY), cv::Scalar(hMaxY, sMaxY, vMaxY), threshImg);

                //width = 640
                //height = 480 / 3
                // cv::Mat crop = threshImg(cv::Range(80,280),cv::Range(150,330)); // Slicing to crop the image                

                cv::putText(img, //target image
                    output, //text
                    cv::Point(0, img.rows / 2), //top-left position
                    cv::FONT_HERSHEY_PLAIN,
                    1.0,
                    CV_RGB(255, 255, 255), //font color
                1);


                cv::GaussianBlur(threshImg, threshImg, cv::Size(3, 3), 0);   //Blur Effect
                cv::dilate(threshImg, threshImg, 0);        // Dilate Filter Effect
                cv::erode(threshImg, threshImg, 0);         // Erode Filter Effect

                std::vector<std::vector<cv::Point> > contours;  // mulitdimensional dynamic array
                std::vector<cv::Vec4i> hierarchy;
                cv::findContours(threshImg, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);

                // Draw the contours 
                //cv::Mat image_copy = threshImg.clone();
                //cv::drawContours(img, contours, -1, cv::Scalar(0, 255, 0), 2));
                


                // Example: Draw a red rectangle and display image.
                // cv::rectangle(img, cv::Point(50, 50), cv::Point(100, 100), cv::Scalar(0,0,255));
                //cv::putText(img, "Group15", cv::Point(50,50), cv::FONT_HERSHEY_SIMPLEX ,0.5, cv::Scalar(255,255,255)); // Draw the text

                // If you want to access the latest received ground steering, don't forget to lock the mutex:
                {
                    std::lock_guard<std::mutex> lck(gsrMutex);
                    std::cout << "main: groundSteering = " << gsr.groundSteering() << std::endl;
                }

                // Display image on your screen.
                if (VERBOSE) {
                    //cv::imshow(sharedMemory->name().c_str(), img);
                    cv::imshow("Original frame", img);     // show windows
                    // cv::imshow("threshImg", threshImg);
                    // cv::imshow("Cropped Img", crop);

                    cv::waitKey(1);
                }
            }
        }
        retCode = 0;
    }
    return retCode;
}

