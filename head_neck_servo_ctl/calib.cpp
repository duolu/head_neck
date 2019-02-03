/*
 * calib.cpp
 *
 *  Created on: Jul 17, 2018
 *      Author: priori
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <iostream>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/objdetect/objdetect.hpp"

#include "head_neck_ctl.h"

using namespace cv;
using namespace std;

// CAUTION: assume only one axis is rotating and all others are kept still!!!
void head_neck_calib_cam_capture(
		int cam_base_id, int cam_mobile_id,
		int camera_width, int camera_height,
		int board_n, int board_m, float board_cell_width, float board_cell_height,
		int nr_images, char *fn, char *folder) {


	VideoCapture cam_base(cam_base_id);
	VideoCapture cam_mobile(cam_mobile_id);

	if (!cam_base.isOpened() || !cam_mobile.isOpened()) {
		cerr << "Error opening camera" << endl;
	}

	cam_base.set(CV_CAP_PROP_FRAME_WIDTH, camera_width);
	cam_base.set(CV_CAP_PROP_FRAME_HEIGHT, camera_height);

	cam_mobile.set(CV_CAP_PROP_FRAME_WIDTH, camera_width);
	cam_mobile.set(CV_CAP_PROP_FRAME_HEIGHT, camera_height);


	namedWindow("base", CV_WINDOW_AUTOSIZE);
	namedWindow("mobile", CV_WINDOW_AUTOSIZE);

	char fn_buf[128];

	vector<struct pan_tilt_ctrl> joint_states;
	vector<vector<Point3f> > object_points;
	vector<vector<Point2f> > points_base;
	vector<vector<Point2f> > points_mobile;

	int board_size = board_n * board_m;
	Size board_sz = Size(board_n, board_m);

	for (int i = 0; i < nr_images && !stop_main; ) {


		Mat frame_base;
		Mat frame_mobile;


		cam_base.grab();
		cam_mobile.grab();
		cam_base.retrieve(frame_base);
		cam_mobile.retrieve(frame_mobile);


		if (pt_ctrl.button_0) {

			pt_ctrl.button_0 = 0;


			vector<Point2f> corners_base;
			vector<Point2f> corners_mobile;

			bool found_base = findChessboardCorners(frame_base, board_sz, corners_base);
			bool found_mobile = findChessboardCorners(frame_mobile, board_sz, corners_mobile);

			if (found_base && found_mobile) {

				points_base.push_back(corners_base);
				points_mobile.push_back(corners_mobile);

				vector<Point3f> object_point;

				for (int j = 0; j < board_size; j++) {

					float x = board_cell_width * (j / board_n);
					float y = board_cell_height * (j % board_n);
					float z = 0.0;
					Point3f p(x, y, z);
					object_point.push_back(p);

				}

				object_points.push_back(object_point);

				joint_states.push_back(pt_ctrl);

				cout << "image " << i << " obtained: ";
				cout << "(";
				cout << pt_ctrl.pan_left_angle << ", " << pt_ctrl.tilt_left_angle << ", ";
				cout << pt_ctrl.pan_right_angle << ", " << pt_ctrl.tilt_right_angle << ", ";
				cout << pt_ctrl.pan_neck_angle << ", " << pt_ctrl.tilt_neck_angle;
				cout << ")";

				cout << "\t";

				cout << "(";
				cout << pt_ctrl.pan_left << ", " << pt_ctrl.tilt_left << ", ";
				cout << pt_ctrl.pan_right << ", " << pt_ctrl.tilt_right << ", ";
				cout << pt_ctrl.pan_neck << ", " << pt_ctrl.tilt_neck;
				cout << ")";


				//cout << "2D points: " << corners_base.size();
				//cout << "(" << corners_base[0].x << ", " << corners_base[0].y << ")";

				cout << endl;

				stringstream ss_base;
				ss_base << folder << "/" << i << "_base.png";
				string fn_base = ss_base.str();
				imwrite(fn_base.c_str(), frame_base);

				stringstream ss_mobile;
				ss_mobile << folder << "/" << i << "_mobile.png";
				string fn_mobile = ss_mobile.str();
				imwrite(fn_mobile.c_str(), frame_mobile);

				i++;

			}

			drawChessboardCorners(frame_base, board_sz, corners_base, found_base);
			drawChessboardCorners(frame_mobile, board_sz, corners_mobile, found_mobile);

		}

		imshow("base", frame_base);
		imshow("mobile", frame_mobile);


		if (waitKey(30) == 27) {
			cout << "esc key is pressed by user" << endl;
			break;
		}

	}

	if (stop_main)
		return;

	cout << "dumping to files...";

	// CAUTION: this is a CSV file
	ofstream f(fn);
	for (int i = 0; i < nr_images; i++) {

		struct pan_tilt_ctrl state = joint_states[i];
		vector<Point3f> object_point = object_points[i];
		vector<Point2f> corners_base = points_base[i];
		vector<Point2f> corners_mobile = points_mobile[i];

		f << i << ", ";
		f << state.pan_left_angle << ", " << state.tilt_left_angle  << ", ";
		f << state.pan_right_angle << ", " << state.tilt_right_angle  << ", ";
		f << state.pan_neck_angle << ", " << state.tilt_neck_angle  << ", ";
		f << "\t\t";

		int nr_points = object_point.size();

		for (int j = 0; j < nr_points; j++) {

			Point3f p = object_point[j];

			f << p.x << ", " << p.y << ", " << p.z << ", ";

		}
		f << "\t\t";
		for (int j = 0; j < nr_points; j++) {

			Point2f p = corners_base[j];

			f << p.x << ", " << p.y << ", ";

		}
		f << "\t\t";
		for (int j = 0; j < nr_points; j++) {

			Point2f p = corners_mobile[j];

			f << p.x << ", " << p.y << ", ";

		}

		// add a trailing column of 0.0
		f << 0.0;
		f << endl;
	}
	f.close();
	cout << "done" << endl;

	destroyWindow("base");
	destroyWindow("mobile");

	cout << "capture finished." << endl;
}





































