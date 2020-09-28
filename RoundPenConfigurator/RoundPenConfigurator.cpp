#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <fstream>
#include <stdio.h>

#define CVUI_IMPLEMENTATION
#include "cvui.h"
#include "tinyfiledialogs.h"

using namespace std;
const int updateEveryXFrames = 20;

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
	cv::Mat hsv;
    cv::Mat window;

    char namesBuffer[2048];
    namesBuffer[0] = 0;
    uint16_t namesBufferLength = 0;
    char* markerNames[16];
	cv::Vec3b markerColors[16];
	cv::Vec3b windowColors[16];
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
	double h1 = 1900 * (frame.rows / (double)frame.cols);
	double w2 = 780 * (frame.cols / (double)frame.rows);
	if (h1 <= 780) {
		cv::resize(frame, frame, cv::Size(1900, h1));
	}
	else {
		cv::resize(frame, frame, cv::Size(w2, 780));
	}

	cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    frame.push_back(cv::Mat(200, frame.cols, frame.type(), cv::Scalar::all(0)));

    cvui::init("RoundPen Configurator");

    bool running = true;
	char errorMsg[128];
	char saveMsg[128];
	bool colorSet = false;
	errorMsg[0] = 0;
	saveMsg[0] = 0;
	char namesCursor = '|';
	uint32_t frameNr = 0;
	int markersToSave;
	ofstream outfile;
    while (running) {
		if (frameNr%updateEveryXFrames == 0) {
			if (namesCursor == 0) {
				namesCursor = '|';
			}
			else {
				namesCursor = 0;
			}
		}
        frame.copyTo(window);
        cvui::text(window, 10, window.rows - 190, "Click on a pixel in the window to define a new marker.");
        cvui::text(window, 10, window.rows - 170, "Controls: Left-Click = Select Color, Typing = Set Name, Enter = Next, CTRL+T = Save, Esc = Exit.");
        cvui::printf(window, 10, window.rows - 150, "Markers: %s%c", namesBuffer, namesCursor);
        cvui::text(window, 10, window.rows - 130, "Colors:");
        for (int i = 0; i < markersLength; i++) {
            cvui::printf(window, 69 + 30*i, window.rows - 130, "%d", i);
			int color = ((windowColors[i][0]) << 0) + ((windowColors[i][1]) << 8) + ((windowColors[i][2]) << 16);
            cvui::rect(window, 79 + 30*i, window.rows - 132, 16, 16, 0, color);
        }
		cvui::printf(window, 10, window.rows - 110, "Current color (HSV 360/100/100): %d %d %d", markerColors[markersLength-1][0]*2, markerColors[markersLength - 1][1]*100/256, markerColors[markersLength - 1][2]*100/256);
		cvui::text(window, 10, window.rows - 90, errorMsg, 0.4, 0xff0000);
		cvui::text(window, 10, window.rows - 10, saveMsg, 0.4, 0xff00);

        if (cvui::mouse(cvui::IS_DOWN)) {
			if (cvui::mouse().x >= 0 && cvui::mouse().x < hsv.cols && cvui::mouse().y >= 0 && cvui::mouse().y < hsv.rows) {
				markerColors[markersLength - 1] = hsv.at<cv::Vec3b>(cv::Point(cvui::mouse().x, cvui::mouse().y));
				windowColors[markersLength - 1] = window.at<cv::Vec3b>(cv::Point(cvui::mouse().x, cvui::mouse().y));
				colorSet = true;
				saveMsg[0] = 0;
			}
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
			if (markerNames[markersLength - 1] != namesBuffer + namesBufferLength) {
				if (colorSet) {
					errorMsg[0] = 0;
					// Add , to the end of the char string.
					namesBuffer[namesBufferLength] = ',';
					namesBufferLength++;
					// The next string has 0 at the start. Will be overwritten.
					namesBuffer[namesBufferLength] = 0;
					// Point to the start of the new name.
					markerNames[markersLength] = namesBuffer + namesBufferLength;
					// Color not set for new marker.
					colorSet = false;
					// Increase the names.
					markersLength++;
				}
				else {
					strcpy_s(errorMsg, "Marker Color not set\0");
				}
			}
			else {
				strcpy_s(errorMsg, "Marker Name not set\0");
			}
            break;
        case 8:
            if (markerNames[markersLength - 1] != namesBuffer + namesBufferLength) {
                namesBufferLength--;
                namesBuffer[namesBufferLength] = 0;
				saveMsg[0] = 0;
            }
            break;
        case 20:
			markersToSave = markersLength;
			if (markerNames[markersLength - 1] == namesBuffer + namesBufferLength || !colorSet) {
				markersToSave--;
			}
			if (markersToSave > 0) {
				outfile.open("markers.csv", ios::out | ios::trunc);

				char buf[10];
				for (int i = 0; i < markersToSave; i++) {
					if (i < markersLength-1) {
						*(markerNames[i + 1] - 1) = 0;
						outfile << markerNames[i] << ";" << static_cast<unsigned>(markerColors[i][0]) << ";" << static_cast<unsigned>(markerColors[i][1]) << ";" << static_cast<unsigned>(markerColors[i][2], buf, 10) << endl;
						*(markerNames[i + 1] - 1) = ',';
					}
					else {
						outfile << markerNames[i] << ";" << static_cast<unsigned>(markerColors[i][0]) << ";" << static_cast<unsigned>(markerColors[i][1]) << ";" << static_cast<unsigned>(markerColors[i][2], buf, 10) << endl;
					}
				}
				outfile.close();
				strcpy_s(saveMsg, "Configuration saved.\0");
			}
            break;
        default:
            if (k >= 'A' && k <= 'Z' || k >= 'a' && k <= 'z' || k >= '0' && k <= '9' || k == '_' || k == '-' || k == ' ') {
                namesBuffer[namesBufferLength] = k;
                namesBufferLength++;
                namesBuffer[namesBufferLength] = 0;
				saveMsg[0] = 0;
            }
            break;
        }
		frameNr++;
    }
}
