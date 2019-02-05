

// ------------ serial utility function -------

// CAUTION: this is busy waiting on sending!!!

void serial_send_char(byte c) {

  while (Serial.write(c) <= 0)
    ;
}

void serial_send_header(byte len, byte opcode) {

  serial_send_char(SERIAL_MAGIC_1);
  serial_send_char(SERIAL_MAGIC_2);
  serial_send_char(len);
  serial_send_char(opcode);
  Serial.flush();
}

void serial_send_int(int i) {

  byte *cp = (byte *) &i;

  byte c0 = cp[0];
  byte c1 = cp[1];

  serial_send_char(c0);
  serial_send_char(c1);
  Serial.flush();
}

void serial_send_long(long l) {

  byte *cp = (byte *) &l;

  //  byte c0 = (byte)(l & 0x000000FF);
  //  byte c1 = (byte)((l & 0x0000FF00) >> 8);
  //  byte c2 = (byte)((l & 0x00FF0000) >> 16);
  //  byte c3 = (byte)((l & 0xFF000000) >> 24);

  byte c0 = cp[0];
  byte c1 = cp[1];
  byte c2 = cp[2];
  byte c3 = cp[3];

  serial_send_char(c0);
  serial_send_char(c1);
  serial_send_char(c2);
  serial_send_char(c3);
  Serial.flush();
}

void serial_send_float(float f) {

  byte *cp = (byte *) &f;

  byte c0 = cp[0];
  byte c1 = cp[1];
  byte c2 = cp[2];
  byte c3 = cp[3];

  serial_send_char(c0);
  serial_send_char(c1);
  serial_send_char(c2);
  serial_send_char(c3);
  Serial.flush();
}



// CAUTION: this is busy waiting on receiving!!!

int serial_recv_char() {

  while (Serial.available() <= 0)
    ;
  return (char) Serial.read();

}
int serial_recv_int() {

  char bytearray[2];

  bytearray[0] = serial_recv_char();
  bytearray[1] = serial_recv_char();

  return *((int *) bytearray);
}

long serial_recv_long() {

  char bytearray[4];

  bytearray[0] = serial_recv_char();
  bytearray[1] = serial_recv_char();
  bytearray[2] = serial_recv_char();
  bytearray[3] = serial_recv_char();

  return *((long *) bytearray);
}

float serial_recv_float() {

  char bytearray[4];

  bytearray[0] = serial_recv_char();
  bytearray[1] = serial_recv_char();
  bytearray[2] = serial_recv_char();
  bytearray[3] = serial_recv_char();

  return *((float *) bytearray);
}

// -----------    dispatch serial message according to the opcode --------------


void recv_ctrl_msg_in_struct() {


        byte *cp = (byte *) &latest_ctrl_msg;

        int m = sizeof(struct h2l_header);
        int n = sizeof(struct h2l_state_pan_tilt_ctrl_msg) - m;

        for (int i = 0; i < n; i++) {

                cp[i + m] = serial_recv_char();
        }

        // restore the message header
        latest_ctrl_msg.header.magic1 = SERIAL_MAGIC_1;
        latest_ctrl_msg.header.magic2 = SERIAL_MAGIC_2;
        latest_ctrl_msg.header.len = n;
        latest_ctrl_msg.header.opcode = OPCODE_CTRL_PAN_TILT;

        ctrl_msg_checksum = latest_ctrl_msg.checksum;
}

int check_ctrl_msg() {

        // CAUTION: The message must be multiples of 16-bit block
        uint16_t *cp = (uint16_t *) &latest_ctrl_msg;

        int nr_blocks = sizeof(struct h2l_state_pan_tilt_ctrl_msg) / 2;

        ctrl_msg_checksum_computed = 0;

        for (int i = 2; i < nr_blocks - 2; i++) {

                ctrl_msg_checksum_computed += ((uint32_t)cp[i] & 0x0000FFFF);
        }


        if (ctrl_msg_checksum_computed == ctrl_msg_checksum)

                return 0;
        else
                return 1;
}

int update_ctrl_msg() {

        ctrl_flag = latest_ctrl_msg.ctrl_flag;
        
        pan_left_ctrl_pos = latest_ctrl_msg.pan_left_ctrl_pos;
        tilt_left_ctrl_pos = latest_ctrl_msg.tilt_left_ctrl_pos;
        pan_right_ctrl_pos = latest_ctrl_msg.pan_right_ctrl_pos;
        tilt_right_ctrl_pos = latest_ctrl_msg.tilt_right_ctrl_pos;
        pan_neck_ctrl_pos = latest_ctrl_msg.pan_neck_ctrl_pos;
        tilt_neck_ctrl_pos = latest_ctrl_msg.tilt_neck_ctrl_pos;
        
        pan_left_ctrl_pwm = latest_ctrl_msg.pan_left_ctrl_pwm;
        tilt_left_ctrl_pwm = latest_ctrl_msg.tilt_left_ctrl_pwm;
        pan_right_ctrl_pwm = latest_ctrl_msg.pan_right_ctrl_pwm;
        tilt_right_ctrl_pwm = latest_ctrl_msg.tilt_right_ctrl_pwm;
        pan_neck_ctrl_pwm = latest_ctrl_msg.pan_neck_ctrl_pwm;
        tilt_neck_ctrl_pwm = latest_ctrl_msg.tilt_neck_ctrl_pwm;

        
}

void serial_parse_command(int opcode) {

  switch (opcode) {

    case OPCODE_CTRL_PAN_TILT: {

        //recv_ctrl_msg();

        recv_ctrl_msg_in_struct();
        
        if (check_ctrl_msg() == 0) {
                
                update_ctrl_msg();
        }
        


        break;
      }

//    case OPCODE_XXX: {
//
//        // add new function here
//        // use these comments as template
//        break;
//      }
    default: {

        break;
      }

  }

}

// serial state machine

int serial_state = SERIAL_STATE_INIT;
int serial_length = 0;


void serial_state_machine_proceed(int c) {

  //  Serial.print(c, HEX);
  //  Serial.print(' ');

  switch (serial_state) {
    case SERIAL_STATE_INIT: {
        if (c == SERIAL_MAGIC_1)
          serial_state = SERIAL_STATE_MAGIC1;
        else
          serial_state = SERIAL_STATE_INIT;
        break;
      }
    case SERIAL_STATE_MAGIC1: {
        if (c == SERIAL_MAGIC_2)
          serial_state = SERIAL_STATE_MAGIC2;
        else
          serial_state = SERIAL_STATE_INIT;
        break;
      }
    case SERIAL_STATE_MAGIC2: {
        serial_length = c;
        serial_state = SERIAL_STATE_PROTO;
        break;
      }
    case SERIAL_STATE_PROTO: {

        // opcode = c
        serial_parse_command(c);

        serial_state = SERIAL_STATE_INIT;
        break;
      }
    default: {
        serial_state = SERIAL_STATE_INIT;
        break;
      }

  }

}



// -----------    send state back --------------

void serial_send_pan_tilt_state(long timestamp, long ts1, long ts2, long ts3) {

        // fill the message
        
        h2l_set_header(
                &latest_state_msg.header, 
                sizeof(struct h2l_ctrl_pan_tilt_state_msg) - 4, 
                OPCODE_STATE_PAN_TILT);

        latest_state_msg.timestamp = timestamp;
        latest_state_msg.ctrl_flag = ctrl_flag;

        latest_state_msg.pan_left_ctrl_pos = pan_left_ctrl_pos;
        latest_state_msg.tilt_left_ctrl_pos = tilt_left_ctrl_pos;
        latest_state_msg.pan_right_ctrl_pos = pan_right_ctrl_pos;
        latest_state_msg.tilt_right_ctrl_pos = tilt_right_ctrl_pos;
        latest_state_msg.pan_neck_ctrl_pos = pan_neck_ctrl_pos;
        latest_state_msg.tilt_neck_ctrl_pos = tilt_neck_ctrl_pos;
        
        latest_state_msg.pan_left_pwm = pan_left_pwm;
        latest_state_msg.tilt_left_pwm = tilt_left_pwm;
        latest_state_msg.pan_right_pwm = pan_right_pwm;
        latest_state_msg.tilt_right_pwm = tilt_right_pwm;
        latest_state_msg.pan_neck_pwm = pan_neck_pwm;
        latest_state_msg.tilt_neck_pwm = tilt_neck_pwm;
        
        latest_state_msg.pan_left_pos = pan_left_pos;
        latest_state_msg.tilt_left_pos = tilt_left_pos;
        latest_state_msg.pan_right_pos = pan_right_pos;
        latest_state_msg.tilt_right_pos = tilt_right_pos;
        latest_state_msg.pan_neck_pos = pan_neck_pos;
        latest_state_msg.tilt_neck_pos = tilt_neck_pos;
        
        latest_state_msg.euler_left_x = euler_left.x();
        latest_state_msg.euler_left_y = euler_left.y();
        latest_state_msg.euler_left_z = euler_left.z();
//        latest_state_msg.acc_left_x = acc_left.x();
//        latest_state_msg.acc_left_y = acc_left.y();
//        latest_state_msg.acc_left_z = acc_left.z();
//        latest_state_msg.gyro_left_x = gyro_left.x();
//        latest_state_msg.gyro_left_y = gyro_left.y();
//        latest_state_msg.gyro_left_z = gyro_left.z();
        
        latest_state_msg.euler_right_x = euler_right.x();
        latest_state_msg.euler_right_y = euler_right.y();
        latest_state_msg.euler_right_z = euler_right.z();
//        latest_state_msg.acc_right_x = acc_right.x();
//        latest_state_msg.acc_right_y = acc_right.y();
//        latest_state_msg.acc_right_z = acc_right.z();
//        latest_state_msg.gyro_right_x = gyro_right.x();
//        latest_state_msg.gyro_right_y = gyro_right.y();
//        latest_state_msg.gyro_right_z = gyro_right.z();

        latest_state_msg.temp1 = ts1;
        latest_state_msg.temp2 = ts2;
        latest_state_msg.temp3 = ts3;
        latest_state_msg.temp4 = 0;

         // compute checksum
        
        uint16_t *cp = (uint16_t *) &latest_state_msg;

        int nr_blocks = sizeof(struct h2l_ctrl_pan_tilt_state_msg) / 2;

        state_msg_checksum_computed = 0;

        for (int i = 2; i < nr_blocks - 10; i++) {

                state_msg_checksum_computed += ((uint32_t)cp[i] & 0x0000FFFF);
        }

        latest_state_msg.checksum = state_msg_checksum_computed;


        // send the message
        char *p = (char *) &latest_state_msg;
        for (int i = 0; i < sizeof(struct h2l_ctrl_pan_tilt_state_msg); i++) {

                serial_send_char(p[i]);

                if (i == 76) {

                        Serial.flush();
                        latest_state_msg.temp4 = millis();
                }
        }
        Serial.flush();
}

void serial_send_error_msg(char *msg, int len) {

        serial_send_header(len, OPCODE_RUNTIME_ERROR);
        
        for (int i = 0; i < len; i++) {

                serial_send_char(msg[i]);
        }
        Serial.flush();
}











