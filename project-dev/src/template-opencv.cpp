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
#include <fstream>

// Include the GUI and image processing header files from OpenCV
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

const float INPUT_WIDTH = 640.0;
const float INPUT_HEIGHT = 640.0;
const float SCORE_THRESHOLD = 0.5;
const float NMS_THRESHOLD = 0.45;
const float CONFIDENCE_THRESHOLD = 0.45;

// Text parameters.
const float FONT_SCALE = 0.7;
const int FONT_FACE = FONT_HERSHEY_SIMPLEX;
const int THICKNESS = 1;

// Colors.
cv::Scalar BLACK = Scalar(0, 0, 0);
cv::Scalar BLUE = Scalar(255, 178, 50);
cv::Scalar YELLOW = Scalar(0, 255, 255);
cv::Scalar RED = Scalar(0, 0, 255);
std::vector<std::string> load_class_list()
{
    std::vector<std::string> class_list;
    std::ifstream ifs("config_files/classes.txt");
    std::string line;
    while (getline(ifs, line))
    {
        class_list.push_back(line);
    }
    return class_list;
}

// Draw the predicted bounding box.
void draw_label(cv::Mat &input_image, std::string label, int left, int top)
{
    // Display the label at the top of the bounding box.
    int baseLine;
    Size label_size = getTextSize(label, FONT_FACE, FONT_SCALE, THICKNESS, &baseLine);
    top = max(top, label_size.height);
    // Top left corner.
    Point tlc = Point(left, top);
    // Bottom right corner.
    Point brc = Point(left + label_size.width, top + label_size.height + baseLine);
    // Draw black rectangle.
    rectangle(input_image, tlc, brc, BLACK, FILLED);
    // Put the label on the black rectangle.
    putText(input_image, label, Point(left, top + label_size.height), FONT_FACE, FONT_SCALE, YELLOW, THICKNESS);
}

std::vector<cv::Mat> pre_process(cv::Mat &input_image, cv::dnn::Net &net)
{
    // Convert to blob.
    cv::Mat blob;
    blobFromImage(input_image, blob, 1. / 255., Size(INPUT_WIDTH, INPUT_HEIGHT), Scalar(), true, false);

    net.setInput(blob);

    // Forward propagate.
    vector<Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());

    return outputs;
}

cv::Mat post_process(cv::Mat &input_image, std::vector<cv::Mat> &outputs, const std::vector<std::string> &class_name)
{
    // Initialize vectors to hold respective outputs while unwrapping detections.
    vector<int> class_ids;
    vector<float> confidences;
    vector<Rect> boxes;

    // Resizing factor.
    float x_factor = input_image.cols / INPUT_WIDTH;
    float y_factor = input_image.rows / INPUT_HEIGHT;

    float *data = (float *)outputs[0].data;

    const int dimensions = 85;
    const int rows = 25200;
    // Iterate through 25200 detections.
    for (int i = 0; i < rows; ++i)
    {
        float confidence = data[4];
        // Discard bad detections and continue.
        if (confidence >= CONFIDENCE_THRESHOLD)
        {
            float *classes_scores = data + 5;
            // Create a 1x85 Mat and store class scores of 80 classes.
            Mat scores(1, class_name.size(), CV_32FC1, classes_scores);
            // Perform minMaxLoc and acquire index of best class score.
            Point class_id;
            double max_class_score;
            minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
            // Continue if the class score is above the threshold.
            if (max_class_score > SCORE_THRESHOLD)
            {
                // Store class ID and confidence in the pre-defined respective vectors.

                confidences.push_back(confidence);
                class_ids.push_back(class_id.x);

                // Center.
                float cx = data[0];
                float cy = data[1];
                // Box dimension.
                float w = data[2];
                float h = data[3];
                // Bounding box coordinates.
                int left = int((cx - 0.5 * w) * x_factor);
                int top = int((cy - 0.5 * h) * y_factor);
                int width = int(w * x_factor);
                int height = int(h * y_factor);
                // Store good detections in the boxes vector.
                boxes.push_back(Rect(left, top, width, height));
            }
        }
        // Jump to the next column.
        data += 85;
    }

    // Perform Non Maximum Suppression and draw predictions.
    vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, SCORE_THRESHOLD, NMS_THRESHOLD, indices);
    for (int i = 0; i < indices.size(); i++)
    {
        int idx = indices[i];
        Rect box = boxes[idx];

        int left = box.x;
        int top = box.y;
        int width = box.width;
        int height = box.height;
        // Draw bounding box.
        rectangle(input_image, std::Point(left, top), std::Point(left + width, top + height), BLUE, 3 * THICKNESS);

        // Get the label for the class name and its confidence.
        string label = format("%.2f", confidences[idx]);
        label = class_name[class_ids[idx]] + ":" + label;
        // Draw class labels.
        draw_label(input_image, label, left, top);
    }
    return input_image;
}

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
        std::cerr << "         -cuda: to use cuda" << std::endl;

        std::cerr << "Example: " << argv[0] << " --cid=253 --name=img --width=640 --height=480 --verbose -cuda" << std::endl;
    }
    else
    {
        // Extract the values from the command line parameters
        const std::string NAME{commandlineArguments["name"]};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};
        const bool CUDA{commandlineArguments["cuda"] == "1"};
        // get object's name
        std::vector<std::string> class_list = load_class_list();

        bool is_cuda = CUDA;
        std::clog << argv[0] << ": cuda value is: " << is_cuda << " ." << std::endl;

        cv::dnn::Net net;
        load_net(net, is_cuda);

        auto start = std::chrono::high_resolution_clock::now();
        int frame_count = 0;
        float fps = -1;
        int total_frames = 0;

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
            auto onGroundSteeringRequest = [&gsr, &gsrMutex](cluon::data::Envelope &&env) {
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
                    cv::Mat wrapped(HEIGHT, WIDTH, CV_8UC3, sharedMemory->data());
                    img = wrapped.clone();
                }
                // TODO: Here, you can add some code to check the sampleTimePoint when the current frame was captured.

                auto [_, ts] = sharedMemory->getTimeStamp();
                auto ms = static_cast<int64_t>(ts.seconds()) * static_cast<int64_t>(1000 * 1000) + static_cast<int64_t>(ts.microseconds());
                std::string output = "TS: " + std::to_string(ms) + "; GROUND STEERING: " + std::to_string(gsr.groundSteering());

                sharedMemory->unlock();
                cropedImg = img(cv::Range(240, 480), cv::Range(0, 640));

                // Load model.
                cv::dnn::Net net;
                net = cv::dnn::readNet("models/conedetector-opt12.onnx");

                std::vector<cv::Mat> detections;
                detections = pre_process(cropedImg, net);

                cropedImg = post_process(cropedImg, detections, class_list);

                if (frame_count >= 30)
                {

                    auto end = std::chrono::high_resolution_clock::now();
                    fps = frame_count * 1000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                    frame_count = 0;
                    start = std::chrono::high_resolution_clock::now();
                }

                if (fps > 0)
                {

                    std::ostringstream fps_label;
                    fps_label << std::fixed << std::setprecision(2);
                    fps_label << "FPS: " << fps;
                    std::string fps_label_str = fps_label.str();

                    cv::putText(cropedImg, fps_label_str.c_str(), cv::Point(10, 260), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                }

                // If you want to access the latest received ground steering, don't forget to lock the mutex:
                {
                    std::lock_guard<std::mutex> lck(gsrMutex);

                    std::cout << "main: groundSteering = " << gsr.groundSteering() << std::endl;
                }

                // Display image on your screen.
                if (VERBOSE)
                {
                    // cv::imshow(sharedMemory->name().c_str(), img);
                    cv::imshow("blueImgWithPoint", cropedImg);
                    //cv::imshow("YellowImgWithPoint", yellowResultImg);
                    cv::waitKey(1);
                }
            }
            myFile.close();
            // double percentage = NrOfCorrectAngle / frames;
            // double percentageCounter = NrOfCorrectAngleCounter / frames;

            // std::cout << "Percentage: " << std::to_string(percentage) << std::endl;
            // std::cout << "Percentage Counter: " << std::to_string(percentageCounter) << std::endl;

            // std::cout << "Frames: " << std::to_string(frames) << std::endl;
            // std::cout << "Nr Correct Angle: " << std::to_string(NrOfCorrectAngle) << std::endl;
            // std::cout << "Nr Correct Angle  Counter: " << std::to_string(NrOfCorrectAngleCounter) << std::endl;
        }
        retCode = 0;
    }

    return retCode;
}
