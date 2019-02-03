/*
 * joystick.h
 *
 *  Created on: Jun 14, 2018
 *      Author: duolu
 */

#ifndef JOYSTICK_H_
#define JOYSTICK_H_

// ---------------- joystick event definition -------------------

#define JS_EVENT_BUTTON         0x01    // button pressed/released
#define JS_EVENT_AXIS           0x02    // joystick moved
#define JS_EVENT_INIT           0x80    // initial state of device

struct js_event {

	uint32_t time; 		// event timestamp in milliseconds
	int16_t value; 		// value, e.g., how hard a stick is pushed
	uint8_t type; 		// event type, e.g., button or axis
	uint8_t number; 	// axis/button number
};

static inline int16_t js_to_servo(int js_value) {

	return (int16_t) (js_value * 500 / 32768 + 1500);

}

static inline int16_t js_to_servo_incremental(int16_t ctl, int16_t value) {

	ctl += 5 * value / 32768;
	if (ctl <= 500)
		ctl = 500;
	if (ctl >= 2500)
		ctl = 2500;
	return ctl;
}

static inline int16_t js_to_motor_speed(int js_value) {

	// CAUTION: we restrict maximum motor PWM to 127 (255 is full)
	return (int16_t) (js_value * 127 / 32768);

}

static inline int16_t js_to_motor_pos_incremental(int16_t ctl, int16_t value) {

	ctl += 5 * value / 32768;
	if (ctl <= -1500)
		ctl = -1500;
	if (ctl >= 1500)
		ctl = 1500;
	return ctl;
}


void *joystick_thread(void *para);


#endif /* JOYSTICK_H_ */
