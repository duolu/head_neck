/*
 * joystick.cpp
 *
 *  Created on: Jun 14, 2018
 *      Author: duolu
 */

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "errno.h"
#include "time.h"

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include "joystick.h"

#include "head_neck_motor_ctl.h"




static void process_js_event(struct js_event e) {

	uint8_t type = e.type & ~JS_EVENT_INIT;

	// button 8, the left button in the middle region
	if (type == JS_EVENT_BUTTON && e.value == 1 && e.number == 8) {

		stop_main = 1;
		stop_js = 1;
		stop_serial_tx = 1;
		stop_serial_rx = 1;

		printf("Exiting...\n");
	}

	// button 9, the right button in the middle region
	if (type == JS_EVENT_BUTTON && e.value == 1 && e.number == 9) {

		pt_ctrl.js_button_enable = (pt_ctrl.js_button_enable > 0) ? 0 : 1;
		pt_ctrl.js_axis_enable = (pt_ctrl.js_axis_enable > 0) ? 0 : 1;

		printf("Button enable toggled (%d)\n", pt_ctrl.js_button_enable);
	}


	if (type == JS_EVENT_BUTTON && e.value == 1 && pt_ctrl.js_button_enable) {

//		printf("Butten event: ts=%u, number=%u, value=%d\n", e.time, e.number,
//				e.value);

		switch (e.number) {

		// upper button in the right region
		case 0:
			pt_ctrl.button_0 = 1;

			// switch to open loop
			pt_ctrl.ctrl_flag = 0x00000000;
			pt_ctrl.mode = 0;

			break;
		// right button in the right region
		case 1:
			pt_ctrl.button_1 = 1;

			// clear state
			clear_pt_ctrl_state();

			printf("All states reset\n");

			break;
		// lower button in the right region
		case 2:
			pt_ctrl.button_2 = 1;

			// TODO: switch to close loop
			pt_ctrl.ctrl_flag = 0x0000003F;
			pt_ctrl.mode = 1;

			break;
		// left button in the right region
		case 3:
			pt_ctrl.button_3 = 1;

			if (pt_ctrl.log_start == 0) {

				char fn_buff[128];
				char suffix[64];
				time_t t = time(NULL);
				struct tm tm = *localtime(&t);

				snprintf(suffix, sizeof(suffix), "%04d_%02d_%02d___%02d_%02d_%02d",
						tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
						tm.tm_sec);
				snprintf(fn_buff, sizeof(fn_buff), "./log/log_%s.txt", suffix);
				log_file = fopen(fn_buff, "w+");

				pt_ctrl.log_start = 1;

				printf("log started: %s\n", fn_buff);

			} else {

				pt_ctrl.log_start = 0;
				usleep(100000);
				fclose(log_file);

				printf("log stopped\n");
			}


			break;


		// upper left button in the front side
		case 4:
			pt_ctrl.js_left_pan_lock = (pt_ctrl.js_left_pan_lock > 0) ? 0 : 1;
			printf("Left pan lock toggled (%d)\n", pt_ctrl.js_left_pan_lock);
			break;
		// upper right button in the front side
		case 5:
			pt_ctrl.js_right_pan_lock = (pt_ctrl.js_right_pan_lock > 0) ? 0 : 1;
			printf("Right pan lock toggled (%d)\n", pt_ctrl.js_right_pan_lock);
			break;
		// lower left button in the front side
		case 6:
			pt_ctrl.js_left_tilt_lock = (pt_ctrl.js_left_tilt_lock > 0) ? 0 : 1;
			printf("Left tilt lock toggled (%d)\n", pt_ctrl.js_left_tilt_lock);
			break;
		// lower right button in the front side
		case 7:
			pt_ctrl.js_right_tilt_lock = (pt_ctrl.js_right_tilt_lock > 0) ? 0 : 1;
			printf("Right tilt lock toggled (%d)\n", pt_ctrl.js_right_tilt_lock);
			break;
		default:

			break;
		}

	} else if (type == JS_EVENT_AXIS && pt_ctrl.js_axis_enable) {

//		printf("AXIS event: ts=%u, number=%u, value=%d\n", e.time, e.number,
//				e.value);

		switch (e.number) {

		// left stick horizontal movement -> left eye pan movement
		case 0:
			if (pt_ctrl.js_left_pan_lock == 0) {
				//pt_ctrl.pan_left_ctrl_pos = js_to_motor_pos_incremental(pt_ctrl.pan_left_ctrl_pos, e.value);
				pt_ctrl.pan_left_ctrl_pwm = js_to_motor_speed(e.value);
			}
			break;
		// left stick vertical movement -> left eye tilt movement
		case 1:
			if (pt_ctrl.js_left_tilt_lock == 0) {
				//pt_ctrl.tilt_left_ctrl_pos = js_to_motor_pos_incremental(pt_ctrl.tilt_left_ctrl_pos, e.value);
				pt_ctrl.tilt_left_ctrl_pwm = js_to_motor_speed(e.value);
			}
			break;
		// right stick horizontal movement -> right eye pan movement
		case 2:
			if (pt_ctrl.js_right_pan_lock == 0) {
				//pt_ctrl.pan_right_ctrl_pos = js_to_motor_pos_incremental(pt_ctrl.pan_right_ctrl_pos, e.value);
				pt_ctrl.pan_right_ctrl_pwm = js_to_motor_speed(e.value);
			}
			break;
		// right stick vertical movement -> right eye tilt movement
		case 3:
			if (pt_ctrl.js_right_tilt_lock == 0) {
				//pt_ctrl.tilt_right_ctrl_pos = js_to_motor_pos_incremental(pt_ctrl.tilt_right_ctrl_pos, e.value);
				pt_ctrl.tilt_right_ctrl_pwm = js_to_motor_speed(e.value);
			}
			break;
		// arrow button on the left region, horizontal movement -> neck pan movement
		case 4:
			if (e.value > 0) {
				//pt_ctrl.pan_neck_ctrl_pos += 4;

				pt_ctrl.pan_neck_ctrl_pwm += 20;
			} else if (e.value < 0) {
				//pt_ctrl.pan_neck_ctrl_pos -= 4;

				pt_ctrl.pan_neck_ctrl_pwm -= 20;
			}
			break;
		// arrow button on the left region, vertical movement -> neck tilt movement
		case 5:
			if (e.value > 0) {
				//pt_ctrl.tilt_neck_ctrl_pos += 4;

				pt_ctrl.tilt_neck_ctrl_pwm += 20;

			} else if (e.value < 0) {
				//pt_ctrl.tilt_neck_ctrl_pos -= 4;


				pt_ctrl.tilt_neck_ctrl_pwm -= 20;
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

