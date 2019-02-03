/*
 * sprotocol.h
 *
 *  Created on: May 10, 2018
 *      Author: duolu
 */

#ifndef SPROTOCOL_H_
#define SPROTOCOL_H_

// ---------------- serial protocol -------------------

#define MC_PROTO_HEADER_SIZE 4

#define SERIAL_STATE_INIT      0
#define SERIAL_STATE_MAGIC1    1
#define SERIAL_STATE_MAGIC2    2
#define SERIAL_STATE_PROTO     3

#define SERIAL_MAGIC_1 'G'
#define SERIAL_MAGIC_2 'V'


#define OPCODE_CTRL_PAN_TILT          	0x10

#define OPCODE_STATE_PAN_TILT          	0x80

#define OPCODE_RUNTIME_ERROR            0xF0

// CAUTION: we assume both the computer and the microcontroller are little-endian!!!
// Otherwise the sending procedure will not work!!!


struct h2l_header {

	uint8_t magic1;
	uint8_t magic2;
	uint8_t len;
	uint8_t opcode;
};

// message from computer to microcontroller (uplink)

// CAUTION: we only use the three Euler angles from the IMU

struct h2l_ctrl_pan_tilt_state_msg {

	struct h2l_header header;

	uint32_t timestamp;
	uint32_t ctrl_flag;

	int16_t pan_left_ctrl_pos;
	int16_t tilt_left_ctrl_pos;
	int16_t pan_right_ctrl_pos;
	int16_t tilt_right_ctrl_pos;
	int16_t pan_neck_ctrl_pos;
	int16_t tilt_neck_ctrl_pos;

	int16_t pan_left_pwm;
	int16_t tilt_left_pwm;
	int16_t pan_right_pwm;
	int16_t tilt_right_pwm;
	int16_t pan_neck_pwm;
	int16_t tilt_neck_pwm;

	int16_t pan_left_pos;
	int16_t tilt_left_pos;
	int16_t pan_right_pos;
	int16_t tilt_right_pos;
	int16_t pan_neck_pos;
	int16_t tilt_neck_pos;

	float euler_left_x;
	float euler_left_y;
	float euler_left_z;

//	float acc_left_x;
//	float acc_left_y;
//	float acc_left_z;
//
//	float gyro_left_x;
//	float gyro_left_y;
//	float gyro_left_z;

	float euler_right_x;
	float euler_right_y;
	float euler_right_z;

//	float acc_right_x;
//	float acc_right_y;
//	float acc_right_z;
//
//	float gyro_right_x;
//	float gyro_right_y;
//	float gyro_right_z;

    uint32_t temp1;
    uint32_t temp2;
    uint32_t temp3;
    uint32_t temp4;

    uint32_t checksum;

};

// message from computer to microcontroller (downlink)

struct h2l_state_pan_tilt_ctrl_msg {

	struct h2l_header header;

	uint32_t ctrl_flag;

	int16_t pan_left_ctrl_pos;
	int16_t tilt_left_ctrl_pos;
	int16_t pan_right_ctrl_pos;
	int16_t tilt_right_ctrl_pos;
	int16_t pan_neck_ctrl_pos;
	int16_t tilt_neck_ctrl_pos;

	int16_t pan_left_ctrl_pwm;
	int16_t tilt_left_ctrl_pwm;
	int16_t pan_right_ctrl_pwm;
	int16_t tilt_right_ctrl_pwm;
	int16_t pan_neck_ctrl_pwm;
	int16_t tilt_neck_ctrl_pwm;

    uint32_t checksum;
};


static inline void h2l_set_header(struct h2l_header *pheader, uint8_t len, uint8_t opcode) {

	pheader->magic1 = SERIAL_MAGIC_1;
	pheader->magic2 = SERIAL_MAGIC_2;
	pheader->len = len;
	pheader->opcode = opcode;

}

void *serial_send_thread(void *para);
void *serial_recv_thread(void *para);


#endif /* SPROTOCOL_H_ */
