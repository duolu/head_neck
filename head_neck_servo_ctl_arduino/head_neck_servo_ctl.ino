

//----------------------------------- config ----------------------------


int ctrl_loop_period = 100; // in ms



//----------------------------------- servo ----------------------------

#include <Servo.h>

Servo pl; // pan left eye
Servo tl; // tilt left eye
Servo pr; // pan right eye
Servo tr; // titl right eye
Servo pn; // pan neck
Servo tn; // tilt neck, CAUTION: it is a continuous servo without position feedback

// These are desired values (in microseconds)
int pan_left = 1500;
int tilt_left = 1500;
int pan_right = 1500;
int tilt_right = 1500;
int pan_neck = 1500;
int tilt_neck = 1450;

// Since the tilt neck actuator is a continuous servo (just a motor actually),
// we implemented a close loop controller here.
int tilt_neck_pwm = 0;

// current value from the potentiometer
int pan_left_pos = 0;
int tilt_left_pos = 0;
int pan_right_pos = 0;
int tilt_right_pos = 0;
int pan_neck_pos = 0;
int tilt_neck_pos = 0;


void servo_setup() {

  // CAUTION: do not use timer1 PWM (i.e., pin 11 and 12)

  pl.attach(7);
  tl.attach(8);
  pr.attach(9);
  tr.attach(10);

  pn.attach(5);
  tn.attach(6);
  
  
}


void servo_actuate() {

  pl.writeMicroseconds(pan_left);
  tl.writeMicroseconds(tilt_left);


  // CAUTION: right tilt has a mirrored direction respect to the head!!!
  pr.writeMicroseconds(pan_right);
  tr.writeMicroseconds(2500 - tilt_right + 500);


  pn.writeMicroseconds(pan_neck);
  tn.writeMicroseconds(tilt_neck);

//  pl.write(pan_left);
//  tl.write(tilt_left);
//  
//  pr.write(pan_right);
//  tr.write(255 - tilt_right);
//
//  pl.write(pan_left);
//  tl.write(tilt_left);
  
}

void servo_sense() {

  pan_left_pos = analogRead(2);
  tilt_left_pos = analogRead(3);
  pan_right_pos = analogRead(4);
  tilt_right_pos = analogRead(5);
  pan_neck_pos = analogRead(0);
  tilt_neck_pos = analogRead(1);
}



// ------------------------------------- serial protocol -------------------------------



#define SERIAL_STATE_INIT      0
#define SERIAL_STATE_MAGIC1    1
#define SERIAL_STATE_MAGIC2    2
#define SERIAL_STATE_PROTO     3

#define SERIAL_MAGIC_1 'G'
#define SERIAL_MAGIC_2 'V'

// command sent from HLC to LLC

#define OPCODE_CTRL_PAN_TILT            0x10


// state report from LLC to HLC


#define OPCODE_STATE_PAN_TILT            0x80





// ------------ serial utility function -------

// CAUTION: this is busy waiting on write!!!

void serial_put_char(byte c) {

  while (Serial.write(c) <= 0)
    ;
}

void serial_send_header(byte len, byte opcode) {

  serial_put_char(SERIAL_MAGIC_1);
  serial_put_char(SERIAL_MAGIC_2);
  serial_put_char(len);
  serial_put_char(opcode);
  Serial.flush();
}

void serial_send_int(int i) {

  byte *cp = (byte *) &i;

  byte c0 = cp[0];
  byte c1 = cp[1];

  serial_put_char(c0);
  serial_put_char(c1);
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

  serial_put_char(c0);
  serial_put_char(c1);
  serial_put_char(c2);
  serial_put_char(c3);
  Serial.flush();
}

void serial_send_float(float f) {

  byte *cp = (byte *) &f;

  byte c0 = cp[0];
  byte c1 = cp[1];
  byte c2 = cp[2];
  byte c3 = cp[3];

  serial_put_char(c0);
  serial_put_char(c1);
  serial_put_char(c2);
  serial_put_char(c3);
  Serial.flush();
}

// CAUTION: this is busy waiting on read!!!

int serial_get_char() {

  while (Serial.available() <= 0)
    ;
  return (char) Serial.read();

}
int serial_get_int() {

  char bytearray[2];

  bytearray[0] = serial_get_char();
  bytearray[1] = serial_get_char();

  return *((int *) bytearray);
}

long serial_get_long() {

  char bytearray[4];

  bytearray[0] = serial_get_char();
  bytearray[1] = serial_get_char();
  bytearray[2] = serial_get_char();
  bytearray[3] = serial_get_char();

  return *((long *) bytearray);
}

float serial_get_float() {

  char bytearray[4];

  bytearray[0] = serial_get_char();
  bytearray[1] = serial_get_char();
  bytearray[2] = serial_get_char();
  bytearray[3] = serial_get_char();

  return *((float *) bytearray);
}


int serial_state = SERIAL_STATE_INIT;
int serial_length = 0;

// -----------    dispatch serial message according to the opcode --------------


void serial_parse_command(int opcode) {

  switch (opcode) {

    case OPCODE_CTRL_PAN_TILT: {

        pan_left = serial_get_int();
        tilt_left = serial_get_int();
        pan_right = serial_get_int();
        tilt_right = serial_get_int();
        pan_neck = serial_get_int();
        tilt_neck = serial_get_int();

        //Serial.println(pan_left);


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

void serial_send_pan_tilt_state(long timestamp) {

  serial_send_header(28, OPCODE_STATE_PAN_TILT);
  serial_send_long(timestamp);
  serial_send_int(pan_left);
  serial_send_int(tilt_left);
  serial_send_int(pan_right);
  serial_send_int(tilt_right);
  serial_send_int(pan_neck);
  serial_send_int(tilt_neck);
  serial_send_int(pan_left_pos);
  serial_send_int(tilt_left_pos);
  serial_send_int(pan_right_pos);
  serial_send_int(tilt_right_pos);
  serial_send_int(pan_neck_pos);
  serial_send_int(tilt_neck_pos);
  
}



// ----------------------------------- interrupt -------------------------

// Timer interrupt counter. It is reset when new loop period comes.
int timer_counter = 0;

// Timer flag for the controller loop
volatile int timer_inner_loop_flag = 0;

// CAUTION: This function is called in interrupt!!! Must make it short!!!
void timer_update() {

  timer_counter++;

  if (timer_counter >= ctrl_loop_period) {
    timer_inner_loop_flag = 1;
    timer_counter = 0;
  }

}

#include <TimerOne.h>

void setup() {

  Timer1.initialize(1000); // NOTE: period is in us
  Timer1.attachInterrupt(timer_update);


  Serial.begin(115200);

  servo_setup();

}

unsigned long loop_count = 0;

void loop() {

  if (timer_inner_loop_flag > 0) {

    timer_inner_loop_flag = 0;

    loop_count++;

    unsigned long timestamp = millis();

    servo_actuate();
    servo_sense();
    serial_send_pan_tilt_state(timestamp);

    //Serial.println(timestamp);

    
  }

  if (Serial.available() > 0) {
    int c = Serial.read();
    serial_state_machine_proceed(c);
  }

}
