#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <stdio.h>

#define CVUI_IMPLEMENTATION
#include "cvui.h"
#include "tinyfiledialogs.h"

using namespace std;

int main()
{
    char const* lFilterPatterns[4] = { "*.avi", "*.mp4", "*.mkv", "*.mov" };
    char const* selection = tinyfd_openFileDialog( // there is also a wchar_t version
        "Select file", // title
        "%homepath%\\Videos\\", // optional initial directory
        4, // number of filter patterns
        lFilterPatterns,
        NULL, // optional filter description
        0 // forbid multiple selections
    );

    cv::VideoCapture cap;
    cv::Mat frame;
    cv::Mat window;

    char namesBuffer[2048];
    namesBuffer[0] = 0;
    uint16_t namesBufferLength = 0;
    char* markerNames[16];
    cv::Vec3b markerColors[16];
    markerNames[0] = namesBuffer;
    uint8_t markersLength = 1;

    cap.open(selection);
    if (!cap.isOpened()) {
        cerr << "Error opening video" << endl;
        cerr << "Call the command with a valid video file as first parameter" << endl;
        return -1;
    }

    if (!cap.read(frame)) {
        cerr << "Error reading first frame" << endl;
        return -1;
    }

    frame.push_back(cv::Mat(200, frame.cols, frame.type(), cv::Scalar::all(0)));

    cvui::init("RoundPen Configurator");

    bool running = true;
    while (running) {
        frame.copyTo(window);
        cvui::text(window, 10, window.rows - 190, "Click on a pixel in the window to define a new marker.");
        cvui::text(window, 10, window.rows - 170, "Controls: Left-Click = Select color, Typing = Setting name, Enter = next marker, CTRL+S = Save, Esc = Exit.");
        cvui::printf(window, 10, window.rows - 150, "Markers: %s", namesBuffer);
        cvui::text(window, 10, window.rows - 130, "Colors:");
        for (int i = 0; i < markersLength; i++) {
            cvui::printf(window, 40 + 10*i, window.rows - 130, "%d", i);
            cvui::rect(window, 44, window.rows - 130, 6, 6, 0, markerColors[i].);
        }


        if (cvui::mouse(cvui::IS_DOWN)) {
            markerColors[markersLength - 1] = frame.at<cv::Vec3b>(cv::Point(cvui::mouse().x, cvui::mouse().y));
        }

        // This function must be called *AFTER* all UI components. It does
        // all the behind the scenes magic to handle mouse clicks, etc.
        cvui::update();

        // Show everything on the screen
        cv::imshow("RoundPen Configurator", window);

        // Check if ESC key was pressed
        char k = cv::waitKey(20);
        switch (k) {
        case 27:
            running = false;
            break;
        case 13:
            // Add , to the end of the char string.
            namesBuffer[namesBufferLength] = ',';
            namesBufferLength++;
            // The next string has 0 at the start. Will be overwritten.
            namesBuffer[namesBufferLength] = 0;
            // Point to the start of the new name.
            markerNames[markersLength] = namesBuffer + namesBufferLength;
            // Increase the names.
            markersLength++;
            break;
        case 8:
            if (markerNames[markersLength - 1] != namesBuffer + namesBufferLength) {
                namesBufferLength--;
                namesBuffer[namesBufferLength] = 0;
            }
            break;
        case 19:

            break;
        default:
            if (k >= 'A' && k <= 'Z' || k >= 'a' && k <= 'z' || k >= '0' && k <= '9' || k == '_' || k == '-' || k == ' ') {
                namesBuffer[namesBufferLength] = k;
                namesBufferLength++;
                namesBuffer[namesBufferLength] = 0;
            }
            break;
        }
    }
}
