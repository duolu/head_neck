
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "errno.h"

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include "head_neck_ctl.h"
#include "joystick.h"

#include <iostream>

using namespace std;



void process_js_event(struct js_event e) {

	uint8_t type = e.type & ~JS_EVENT_INIT;

	if (type == JS_EVENT_BUTTON && e.value == 1 && e.number == 8) {

		stop_main = 1;
		stop_js = 1;
		stop_serial_tx = 1;
		stop_serial_rx = 1;

		cout << "Exiting..." << endl;
	}

	if (type == JS_EVENT_BUTTON && e.value == 1 && e.number == 9) {

		pt_ctrl.js_button_enable = (pt_ctrl.js_button_enable > 0) ? 0 : 1;
		pt_ctrl.js_axis_enable = (pt_ctrl.js_axis_enable > 0) ? 0 : 1;

		cout << "Button toggled (" << pt_ctrl.js_button_enable << ")"  << endl;
	}


	if (type == JS_EVENT_BUTTON && e.value == 1 && pt_ctrl.js_button_enable) {

//		printf("Butten event: ts=%u, number=%u, value=%d\n", e.time, e.number,
//				e.value);

		switch (e.number) {

		case 0:
			pt_ctrl.button_0 = 1;
			break;
		case 1:
			pt_ctrl.button_1 = 1;
			break;
		case 2:
			pt_ctrl.button_2 = 1;
			break;
		case 3:
			pt_ctrl.button_3 = 1;
			break;
		case 4:
			pt_ctrl.js_left_pan_lock = (pt_ctrl.js_left_pan_lock > 0) ? 0 : 1;
			cout << "Left pan lock toggled (" << pt_ctrl.js_left_pan_lock << ")"  << endl;
			break;
		case 5:
			pt_ctrl.js_right_pan_lock = (pt_ctrl.js_right_pan_lock > 0) ? 0 : 1;
			cout << "Right pan lock toggled (" << pt_ctrl.js_right_pan_lock << ")"  << endl;
			break;
		case 6:
			pt_ctrl.js_left_tilt_lock = (pt_ctrl.js_left_tilt_lock > 0) ? 0 : 1;
			cout << "Left tilt lock toggled (" << pt_ctrl.js_left_tilt_lock << ")"  << endl;
			break;
		case 7:
			pt_ctrl.js_right_tilt_lock = (pt_ctrl.js_right_tilt_lock > 0) ? 0 : 1;
			cout << "Right tilt lock toggled (" << pt_ctrl.js_right_tilt_lock << ")"  << endl;
			break;
		default:

			break;
		}

	} else if (type == JS_EVENT_AXIS && pt_ctrl.js_axis_enable) {

//		printf("AXIS event: ts=%u, number=%u, value=%d\n", e.time, e.number,
//				e.value);

		switch (e.number) {

		case 0:
			if (pt_ctrl.js_left_pan_lock == 0)
				pt_ctrl.pan_left = js_to_servo_incremental(pt_ctrl.pan_left, e.value);
			//pt_ctrl.pan_left = js_to_servo(e.value);
			break;
		case 1:
			if (pt_ctrl.js_left_tilt_lock == 0)
				pt_ctrl.tilt_left = js_to_servo_incremental(pt_ctrl.tilt_left, e.value);
			//pt_ctrl.tilt_left = js_to_servo(e.value);
			break;
		case 2:
			if (pt_ctrl.js_right_pan_lock == 0)
				pt_ctrl.pan_right = js_to_servo_incremental(pt_ctrl.pan_right, e.value);
			//pt_ctrl.pan_right = js_to_servo(e.value);
			break;
		case 3:
			if (pt_ctrl.js_right_tilt_lock == 0)
				pt_ctrl.tilt_right = js_to_servo_incremental(pt_ctrl.tilt_right, e.value);
			//pt_ctrl.tilt_right = js_to_servo(e.value);
			break;
		case 4:
			if (e.value > 0 && pt_ctrl.pan_neck <= 2500) {
				pt_ctrl.pan_neck += 4;
			} else if (e.value < 0 && pt_ctrl.pan_neck >= 500) {
				pt_ctrl.pan_neck -= 4;
			}
			break;
		case 5:
			if (e.value > 0 && pt_ctrl.tilt_neck <= 2500) {
				pt_ctrl.tilt_neck += 4;
			} else if (e.value < 0 && pt_ctrl.tilt_neck >= 500) {
				pt_ctrl.tilt_neck -= 4;
			}
			break;
		default:

			break;
		}
	}

}

void *joystick_thread(void *para) {

	int ret = 0;

	int fd = open("/dev/input/js0", O_RDONLY, O_NONBLOCK);
	if (fd < 0) {
		printf("Cannot find a joystick attached.\n");
		exit(0);
	}

	printf("joystick thread started.\n");

	while (!stop_js) {

		struct js_event e;
		ret = read(fd, &e, sizeof(e));

		if (ret < 0) {

			printf("Unable to read joystick event\n");
			break;
		}

		process_js_event(e);

		usleep(20000);
	}

	printf("joystick thread exited.\n");

	return NULL;
}

