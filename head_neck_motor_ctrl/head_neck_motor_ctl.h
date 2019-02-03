/*
 * head_neck_motor_ctl.h
 *
 *  Created on: Jun 14, 2018
 *      Author: duolu
 */





#ifndef HEAD_NECK_MOTOR_CTL_H_
#define HEAD_NECK_MOTOR_CTL_H_

#include <stdint.h>
#include <time.h>

// Forward declaration of the message structure in "sprotocol.h"
struct h2l_ctrl_pan_tilt_state_msg;
struct h2l_state_pan_tilt_ctrl_msg;


// A global structure that holds the current states.
struct pan_tilt_ctrl {

	volatile uint32_t mode;
	volatile uint32_t ctrl_flag;

	// open loop states (desired PWM values of motors)
	volatile int16_t pan_left_ctrl_pwm;
	volatile int16_t tilt_left_ctrl_pwm;
	volatile int16_t pan_right_ctrl_pwm;
	volatile int16_t tilt_right_ctrl_pwm;
	volatile int16_t pan_neck_ctrl_pwm;
	volatile int16_t tilt_neck_ctrl_pwm;

	// close loop states (desired position of joints)
	volatile int16_t pan_left_ctrl_pos;
	volatile int16_t tilt_left_ctrl_pos;
	volatile int16_t pan_right_ctrl_pos;
	volatile int16_t tilt_right_ctrl_pos;
	volatile int16_t pan_neck_ctrl_pos;
	volatile int16_t tilt_neck_ctrl_pos;

	// button states, not used
	volatile int button_0;
	volatile int button_1;
	volatile int button_2;
	volatile int button_3;

	// axis and button enable states
	// If these states are not set (i.e, set to non-zero values), pushing axes
	// or press buttons will not take any effect.
	volatile int js_axis_enable;
	volatile int js_button_enable;

	// axis lock
	// If an axis is locked, pushing the stick of that axis will not take any
	// effect. These are specially designed for manual calibration of the head
	// neck system, so that each time we only move one axis.
	int js_left_pan_lock;
	int js_left_tilt_lock;
	int js_right_pan_lock;
	int js_right_tilt_lock;

	// log start indication
	// If this variable is set, the log file will be open and state messages
	// received in the uplink will be logged.
	volatile int log_start;

};

// Declaration of global variables.
// They are all defined in "head_neck_motor_ctl.cpp".

extern struct pan_tilt_ctrl pt_ctrl;

extern struct h2l_ctrl_pan_tilt_state_msg last_state_msg;
extern struct h2l_state_pan_tilt_ctrl_msg last_ctrl_msg;

// Loop period for serial transmission in millisecond.
// For example, period = 20 means 50 Hz.
extern int period;

extern int stop_serial_tx;
extern int stop_serial_rx;
extern int stop_js;
extern int stop_main;

extern FILE *log_file;

// A convenient inline function to obtain current time (relative to the machine
// startup) in microsecond.
static inline uint64_t get_us() {

	struct timespec spec;

	clock_gettime(CLOCK_MONOTONIC, &spec);

	uint64_t s = spec.tv_sec;
	uint64_t us = spec.tv_nsec / 1000 + s * 1000 * 1000;

	return us;
}

// Clear all states in the global variable "pt_ctrl".
void clear_pt_ctrl_state();


#endif /* HEAD_NECK_MOTOR_CTL_H_ */
