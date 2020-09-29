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
const int configHeight = 200;

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
		cv::resize(frame, frame, cv::Size(1900, (int) h1));
	}
	else {
		cv::resize(frame, frame, cv::Size((int) w2, 780));
	}

	cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    frame.push_back(cv::Mat(200, frame.cols, frame.type(), cv::Scalar::all(0)));

    cvui::init("RoundPen Configurator");

	// Variable to stop application.
    bool running = true;

	// Error message to print.
	char errorMsg[128];
	errorMsg[0] = 0;

	// Save message to print.
	char saveMsg[128];
	saveMsg[0] = 0;

	// Variable to check if user have set a color for current marker.
	bool colorSet = false;

	// UI Padding for config.
	int padding;

	// Cursor blinking. Will change to '' after x frames and back after 2x frames.
	char cursor = '|';

	// Counting frames for cursor blinking.
	uint32_t frameNr = 0;

	// Saving related variables.
	// Just here so it doesnt need to be created multiple times.
	int markersToSave;
	char markerColorStringBuffer[10];
	ofstream outfile;

    while (running) {
		if (frameNr%updateEveryXFrames == 0) {
			if (cursor == 0) {
				cursor = '|';
			}
			else {
				cursor = 0;
			}
		}
        frame.copyTo(window);
		padding = 10;
        cvui::text(window, 10, window.rows - configHeight + padding, "Click on a pixel in the window to define a new marker.");
		padding += 20;
        cvui::text(window, 10, window.rows - configHeight + padding, "Controls: Left-Click = Select Color, Typing = Set Name, Enter = Next, CTRL+T = Save, Esc = Exit.");
		padding += 20;
        cvui::printf(window, 10, window.rows - configHeight + padding, "Markers: %s%c", namesBuffer, cursor);
		padding += 20;
        cvui::text(window, 10, window.rows - configHeight + padding, "Colors:");
        for (int i = 0; i < markersLength; i++) {
            cvui::printf(window, 69 + 30*i, window.rows - configHeight + padding, "%d", i);
			int color = ((windowColors[i][0]) << 0) + ((windowColors[i][1]) << 8) + ((windowColors[i][2]) << 16);
            cvui::rect(window, 79 + 30*i, window.rows - configHeight + padding - 2, 16, 16, 0, color);
        }
		padding += 20;
		cvui::printf(window, 10, window.rows - configHeight + padding, "Current color (HSV 360/100/100): %d %d %d", markerColors[markersLength-1][0]*2, markerColors[markersLength - 1][1]*100/256, markerColors[markersLength - 1][2]*100/256);
		padding += 20;
		cvui::text(window, 10, window.rows - configHeight + padding, errorMsg, 0.4, 0xff0000);
		padding += 20;
		cvui::text(window, 10, window.rows - 10, saveMsg, 0.4, 0xff00);

        if (cvui::mouse(cvui::IS_DOWN)) {
			cv::Point pos(cvui::mouse().x, cvui::mouse().y);
			if (pos.x >= 0 && pos.x < hsv.cols && pos.y >= 0 && pos.y < hsv.rows) {
				markerColors[markersLength - 1] = hsv.at<cv::Vec3b>(pos);
				windowColors[markersLength - 1] = window.at<cv::Vec3b>(pos);

				if (pos.x > 15 && pos.y > 15) {
					int color = ((windowColors[markersLength - 1][0]) << 0) + ((windowColors[markersLength - 1][1]) << 8) + ((windowColors[markersLength - 1][2]) << 16);
					cv::Point pos2(pos.x - 15, pos.y - 15);
					cv::circle(window, pos2, 15, cv::Scalar(255, 255, 255), -1);
					cv::circle(window, pos2, 15, cv::Scalar(0, 0, 0), 1);
					cvui::rect(window, pos2.x-10, pos2.y-10, 20, 20, 0x000000, color);
				}
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

				for (int i = 0; i < markersToSave; i++) {
					if (i < markersLength-1) {
						*(markerNames[i + 1] - 1) = 0;
						outfile << markerNames[i] << ";" << static_cast<unsigned>(markerColors[i][0]) << ";" << static_cast<unsigned>(markerColors[i][1]) << ";" << static_cast<unsigned>(markerColors[i][2], markerColorStringBuffer, 10) << endl;
						*(markerNames[i + 1] - 1) = ',';
					}
					else {
						outfile << markerNames[i] << ";" << static_cast<unsigned>(markerColors[i][0]) << ";" << static_cast<unsigned>(markerColors[i][1]) << ";" << static_cast<unsigned>(markerColors[i][2], markerColorStringBuffer, 10) << endl;
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
