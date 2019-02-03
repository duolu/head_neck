/*
 * head_neck_ctl.h
 *
 *  Created on: Jun 14, 2018
 *      Author: priori
 */

#ifndef HEAD_NECK_CTL_H_
#define HEAD_NECK_CTL_H_

#include <stdint.h>
#include <time.h>


struct pan_tilt_ctrl {

	volatile int16_t pan_left;
	volatile int16_t tilt_left;
	volatile int16_t pan_right;
	volatile int16_t tilt_right;
	volatile int16_t pan_neck;
	volatile int16_t tilt_neck;

	volatile int button_0;
	volatile int button_1;
	volatile int button_2;
	volatile int button_3;

	volatile int js_axis_enable;
	volatile int js_button_enable;

	int js_left_pan_lock;
	int js_left_tilt_lock;
	int js_right_pan_lock;
	int js_right_tilt_lock;

	volatile int16_t pan_left_angle;
	volatile int16_t tilt_left_angle;
	volatile int16_t pan_right_angle;
	volatile int16_t tilt_right_angle;
	volatile int16_t pan_neck_angle;
	volatile int16_t tilt_neck_angle;

};


extern struct pan_tilt_ctrl pt_ctrl;

extern int stop_serial_tx;
extern int stop_serial_rx;
extern int stop_js;
extern int stop_main;

static inline uint64_t get_us() {

	struct timespec spec;

	clock_gettime(CLOCK_MONOTONIC, &spec);

	uint64_t s = spec.tv_sec;
	uint64_t us = spec.tv_nsec / 1000 + s * 1000 * 1000;

	return us;
}


#endif /* HEAD_NECK_CTL_H_ */
