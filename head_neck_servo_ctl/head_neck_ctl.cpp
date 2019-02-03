#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "errno.h"

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>

#include "serial.h"
#include "sprotocol.h"
#include "joystick.h"
#include "visual_servo.h"
#include "calib.h"
#include "head_neck_ctl.h"


// ----------- configuration -------------------

int period = 100 * 1000; // 100 ms


struct pan_tilt_ctrl pt_ctrl = {

		.pan_left = 1500,
		.tilt_left = 1500,
		.pan_right = 1500,
		.tilt_right = 1500,
		.pan_neck = 1500,
		.tilt_neck = 1450,

//		.js_axis_enable = 0,
//		.js_button_enable = 0,
//
//		.js_left_pan_lock = 0,
//		.js_left_tilt_lock = 0,
//		.js_right_pan_lock = 0,
//		.js_right_tilt_lock = 0,

		.button_0 = 0,
		.button_1 = 0,
		.button_2 = 0,
		.button_3 = 0,

};

int stop_serial_tx = 0;
int stop_serial_rx = 0;
int stop_js = 0;
int stop_main = 0;




// ----------------------- main -----------------------------

int main() {

	int ret = 0;

	int sfd = serial_init("/dev/ttyACM0", 115200);
	if (sfd < 0) {
		printf("Unable to open serial port!!!\n");
		exit(-1);
	}

	// CAUTION: When the serial port is opened, Arduino board is reset.
	// We must wait for a while before sending the first message so that the
	// Arduino board is correctly setup.
	sleep(1);

	pthread_t tx_serial;
	pthread_t rx_serial;
	pthread_t js;

	ret = pthread_create(&tx_serial, NULL, serial_send_thread, &sfd);
	if (ret != 0) {
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret);
		exit(EXIT_FAILURE);
	}
	ret = pthread_create(&rx_serial, NULL, serial_recv_thread, &sfd);
	if (ret != 0) {
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret);
		exit(EXIT_FAILURE);
	}

//	ret = pthread_create(&js, NULL, joystick_thread, &sfd);
//	if (ret != 0) {
//		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret);
//		exit(EXIT_FAILURE);
//	}


	//single_cam_visual_servo_loop_color_track(1);
	//single_cam_visual_servo_loop_face_track(1);

	dual_cam_visual_servo_loop_face_track(1, 2);


//	head_neck_calib_cam_capture(1, 2, 640, 480, 9, 6, 24.3, 24.2, 400,
//			"./calib/left_pan_raw(left_eye_to_base).csv", "./calib/left_pan");

	pthread_join(tx_serial, NULL);
	pthread_join(rx_serial, NULL);
	pthread_join(js, NULL);

	return 0;
}
