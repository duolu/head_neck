/*
 * sprotocol.cpp
 *
 *  Created on: Jun 14, 2018
 *      Author: duolu
 */


#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "errno.h"

#include <unistd.h>
#include <sys/types.h>

#include "serial.h"
#include "sprotocol.h"
#include "head_neck_motor_ctl.h"




// ---------------- serial -------------------

static int h2l_send_pan_tilt_msg(int sfd) {

	int ret = 0;
	struct h2l_state_pan_tilt_ctrl_msg msg;

	h2l_set_header(&msg.header, sizeof(msg) - sizeof(msg.header),
			OPCODE_CTRL_PAN_TILT);

	msg.ctrl_flag = pt_ctrl.ctrl_flag;

	msg.pan_left_ctrl_pos = pt_ctrl.pan_left_ctrl_pos;
	msg.tilt_left_ctrl_pos = pt_ctrl.tilt_left_ctrl_pos;
	msg.pan_right_ctrl_pos = pt_ctrl.pan_right_ctrl_pos;
	msg.tilt_right_ctrl_pos = pt_ctrl.tilt_right_ctrl_pos;
	msg.pan_neck_ctrl_pos = pt_ctrl.pan_neck_ctrl_pos;
	msg.tilt_neck_ctrl_pos = pt_ctrl.tilt_neck_ctrl_pos;

	msg.pan_left_ctrl_pwm = pt_ctrl.pan_left_ctrl_pwm;
	msg.tilt_left_ctrl_pwm = pt_ctrl.tilt_left_ctrl_pwm;
	msg.pan_right_ctrl_pwm = pt_ctrl.pan_right_ctrl_pwm;
	msg.tilt_right_ctrl_pwm = pt_ctrl.tilt_right_ctrl_pwm;
	msg.pan_neck_ctrl_pwm = pt_ctrl.pan_neck_ctrl_pwm;
	msg.tilt_neck_ctrl_pwm = pt_ctrl.tilt_neck_ctrl_pwm;

	uint16_t *cp = (uint16_t *) &msg;
	int nr_blocks = sizeof(struct h2l_state_pan_tilt_ctrl_msg) / 2;

	uint32_t checksum = 0;

	for (int i = 2; i < nr_blocks - 2; i++) {

			checksum += ((uint32_t)cp[i] & 0x0000FFFF);;
	}
	msg.checksum = checksum;

	last_ctrl_msg = msg;


//	unsigned char *p = (unsigned char *) &msg;
//	for (int i = 0; i < sizeof(msg); i++) {
//
//		printf("%02X ", p[i] & 0x000000FF);
//
//		if (i % 8 == 7)
//			printf(" ");
//	}
//	printf("\n");


	// CAUTION: we assume both the computer and the microcontroller are little-endian!!!
	// Otherwise the sending procedure will not work!!!
	ret = serial_send_n_bytes(sfd, (char *) &msg, sizeof(msg));
	if (ret < 0) {

		printf("Unable to write to serial port, ret=%d, errno=%s.\n", ret, strerror(errno));
	}
	//serial_flush(sfd);

	return ret;
}



void *serial_send_thread(void *para) {

	// CAUTION: always sleep 1ms as an small interval
	struct timespec idle;
	idle.tv_sec = 0;
	idle.tv_nsec = 1000 * 1000;

	int sfd = *((int *) para);

	printf("serial sending thread started.\n");

	uint64_t init_ts = get_us();

	// serial loop
	for (uint32_t i = 0; !stop_serial_tx; i++) {

		h2l_send_pan_tilt_msg(sfd);


//		printf(
//				"[tx] pan_left=%d, tilt_left=%d, pan_right=%d, tilt_right=%d\n",
//				pt_ctrl.pan_left, pt_ctrl.tilt_left, pt_ctrl.pan_right,
//				pt_ctrl.tilt_right);

		uint64_t next_ts = init_ts + period * (i + 1);
		uint64_t ts = get_us();

//		printf("[serial] next_ts=%lu, ts=%lu\n", next_ts - init_ts, ts - init_ts);

		while (ts < next_ts) {

			nanosleep(&idle, NULL);
			ts = get_us();
		}

	}

	printf("serial sending thread exited.\n");

	return NULL;
}

static void print_msg_recv(struct h2l_ctrl_pan_tilt_state_msg *msg, int checksum_ok) {


	printf("ts: %10u, checksum_ok: %d, flag: %08X ",
			msg->timestamp, checksum_ok, msg->ctrl_flag);

	printf("pos: % 4d % 4d % 4d % 4d % 4d % 4d\t",
			msg->pan_left_pos, msg->tilt_left_pos,
			msg->pan_right_pos, msg->tilt_right_pos,
			msg->pan_neck_pos, msg->tilt_neck_pos);

	printf("pwm: % 4d % 4d % 4d % 4d % 4d % 4d\t",
			msg->pan_left_pwm, msg->tilt_left_pwm,
			msg->pan_right_pwm, msg->tilt_right_pwm,
			msg->pan_neck_pwm, msg->tilt_neck_pwm);

//	printf("checksum1: %08X, checksum2: %08X, checksum_ok: %d",
//			msg->temp1, msg->temp2, checksum_ok);

	printf("ts1 = %d, ts2 = %d, ts3 = %d, ts4 = %d",
			msg->temp1 - msg->timestamp, msg->temp2 - msg->timestamp,
			msg->temp3 - msg->timestamp, msg->temp4 - msg->timestamp);

	printf("\n");
}

static void log_msg(struct h2l_ctrl_pan_tilt_state_msg *msg, FILE *fd, int checksum_ok) {

	uint64_t ts = get_us();

	fprintf(fd, "%lu, %d, %d, %d, \t",
			ts, msg->timestamp, checksum_ok, msg->ctrl_flag);

	fprintf(fd, "%d, %d, %d, %d, %d, %d, \t",
			msg->pan_left_ctrl_pos, msg->tilt_left_ctrl_pos,
			msg->pan_right_ctrl_pos, msg->tilt_right_ctrl_pos,
			msg->pan_neck_ctrl_pos, msg->tilt_neck_ctrl_pos);

	fprintf(fd, "%d, %d, %d, %d, %d, %d, \t",
			msg->pan_left_pwm, msg->tilt_left_pwm,
			msg->pan_right_pwm, msg->tilt_right_pwm,
			msg->pan_neck_pwm, msg->tilt_neck_pwm);

	fprintf(fd, "%d, %d, %d, %d, %d, %d, \t",
			msg->pan_left_pos, msg->tilt_left_pos,
			msg->pan_right_pos, msg->tilt_right_pos,
			msg->pan_neck_pos, msg->tilt_neck_pos);

	fprintf(fd, "%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, \t",
			msg->euler_left_x, msg->euler_left_y, msg->euler_left_z,
			msg->euler_right_x, msg->euler_right_y, msg->euler_right_z);

	fprintf(fd, "0\n");
}

static int h2l_recv_process(int sfd, char serial_buff[]) {

	int ret;
	int i;

	int len = (int)serial_buff[2] & 0xFF;
	int opcode = (int)serial_buff[3] & 0xFF;



	switch (opcode) {

	case OPCODE_STATE_PAN_TILT: {

		ret = serial_recv_n_bytes(sfd, serial_buff + sizeof(struct h2l_header), sizeof(struct h2l_ctrl_pan_tilt_state_msg));

		struct h2l_ctrl_pan_tilt_state_msg *msg = (struct h2l_ctrl_pan_tilt_state_msg *) (serial_buff);
		msg->header.len = len;
		msg->header.opcode = opcode;

        uint16_t *cp = (uint16_t *)serial_buff;

        int nr_blocks = sizeof(struct h2l_ctrl_pan_tilt_state_msg) / 2;

        uint32_t checksum = 0;

        for (int i = 2; i < nr_blocks - 10; i++) {

        	checksum += ((uint32_t)cp[i] & 0x0000FFFF);
        }

        int checksum_ok = (checksum == msg->checksum);

		print_msg_recv(msg, checksum_ok);

		if (pt_ctrl.log_start > 0) {

			log_msg(msg, log_file, checksum_ok);
		}

		last_state_msg = *msg;

		break;
	}
	case OPCODE_RUNTIME_ERROR: {

		ret = serial_recv_n_bytes(sfd, serial_buff + sizeof(struct h2l_header), len);

		printf("Error: %s\n", serial_buff + sizeof(struct h2l_header));
		exit(-1);
	}
	default: break;
	}

	return 0;
}

void *serial_recv_thread(void *para) {

	int ret;
	int sfd = *((int *) para);
	char c;
	int comm_state = SERIAL_STATE_INIT;
	char serial_buff[128];

	printf("serial receiving thread started.\n");

	while (!stop_serial_rx) {

		// TODO: if nothing is received within a certain amount of time, stop
		// the receiving thread.

		// CAUTION: This is polling every 1 ms!!!
		// receive protocol header
		ret = serial_recv_byte(sfd, &c);
		if(ret < 0) {
			printf("Serial read error!!!\n");
			break;
		} else if(ret == 0) {

			printf("Nothing is received!!!\n");
			usleep(1000 * 1000);
			continue;
		}


		serial_buff[comm_state] = c;

		//printf("%02X ", (c & 0xFF));

		switch (comm_state) {
		case SERIAL_STATE_INIT: {
			if (c == SERIAL_MAGIC_1)
				comm_state = SERIAL_STATE_MAGIC1;
			else
				comm_state = SERIAL_STATE_INIT;
			break;
		}
		case SERIAL_STATE_MAGIC1: {
			if (c == SERIAL_MAGIC_2)
				comm_state = SERIAL_STATE_MAGIC2;
			else
				comm_state = SERIAL_STATE_INIT;
			break;
		}
		case SERIAL_STATE_MAGIC2: {
			comm_state = SERIAL_STATE_PROTO;
			break;
		}
		case SERIAL_STATE_PROTO: {

			h2l_recv_process(sfd, serial_buff);

			comm_state = SERIAL_STATE_INIT;
			break;
		}
		default: {
			comm_state = SERIAL_STATE_INIT;
			break;
		}

		}

	}

	printf("serial receiving thread exited.\n");

	return NULL;
}

