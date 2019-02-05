
#include <TimerOne.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <Servo.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

#include "serial.h"

// ----------------------------------- serial communication -------------------------


struct h2l_ctrl_pan_tilt_state_msg latest_state_msg;

struct h2l_state_pan_tilt_ctrl_msg latest_ctrl_msg;

uint32_t state_msg_checksum_computed;


uint32_t ctrl_msg_checksum;
uint32_t ctrl_msg_checksum_computed;

uint32_t mode = 0;

// ----------------------------------- sensor -------------------------

// Motor motion is in right hand rule.

// pan left potentiometer is A8, decreasing while the motor is moving forward
// tilt left potentiometer is A9, decreasing while the motor is moving forward
// pan left potentiometer is A10, decreasing while the motor is moving forward
// tilt right potentiometer is A11, decreasing while the motor is moving forward

// pan neck potentiometer is A12, increasing while the motor is moving forward
// tilt neck potentiometer is A13, increasing while the motor is moving forward

#define PAN_LEFT_POTENTIOMETER_PIN       8
#define TILT_LEFT_POTENTIOMETER_PIN      9
#define PAN_RIGHT_POTENTIOMETER_PIN      10
#define TILT_RIGHT_POTENTIOMETER_PIN     11
#define PAN_NECK_POTENTIOMETER_PIN       12
#define TILT_NECK_POTENTIOMETER_PIN      13

// most recent sensor reading from the potentiometer
int pan_left_pos = 0;
int tilt_left_pos = 0;
int pan_right_pos = 0;
int tilt_right_pos = 0;
int pan_neck_pos = 0;
int tilt_neck_pos = 0;

// The left IMU has address 0x28 (ADR pin float) 
// The right IMU has address 0x29 (ADR pin connected to 3.3V)

Adafruit_BNO055 imu_left = Adafruit_BNO055(55, BNO055_ADDRESS_A);
Adafruit_BNO055 imu_right = Adafruit_BNO055(56, BNO055_ADDRESS_B);


// most recent sensor reading from the IMU

imu::Vector<3> euler_left;
imu::Vector<3> acc_left;
imu::Vector<3> gyro_left;

imu::Vector<3> euler_right;
imu::Vector<3> acc_right;
imu::Vector<3> gyro_right;

// most recent sensor reading from the debug pin

#define DEBUG_PIN0 0
#define DEBUG_PIN1 1

int debug0;
int debug1;

void sense() {

        debug0 = analogRead(DEBUG_PIN0);
        debug1 = analogRead(DEBUG_PIN1);

        pan_left_pos = analogRead(PAN_LEFT_POTENTIOMETER_PIN);
        tilt_left_pos = analogRead(TILT_LEFT_POTENTIOMETER_PIN);
        pan_right_pos = analogRead(PAN_RIGHT_POTENTIOMETER_PIN);
        tilt_right_pos = analogRead(TILT_RIGHT_POTENTIOMETER_PIN);
        pan_neck_pos = analogRead(PAN_NECK_POTENTIOMETER_PIN);
        tilt_neck_pos = analogRead(TILT_NECK_POTENTIOMETER_PIN);

        euler_left = imu_left.getVector(Adafruit_BNO055::VECTOR_EULER);
        //acc_left = imu_left.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
        //gyro_left = imu_left.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);

        euler_right = imu_right.getVector(Adafruit_BNO055::VECTOR_EULER);
        //acc_right = imu_right.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
        //gyro_right = imu_right.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);


}

// ----------------------------------- actuator -------------------------

// Motor is driven by Adafruit Motor Shield 

// pan left motor is M2, forward directrion follows right hand rule (counter-clockwise)
// tilt left motor is M4, forward directrion follows right hand rule (counter-clockwise)
// pan right motor is M3, forward directrion follows right hand rule (counter-clockwise)
// tilt right motor is M1, forward directrion follows right hand rule (counter-clockwise)

// pan neck motor is D9 (PWM pin), forward directrion follows right hand rule (counter-clockwise)
// tilt neck motor is D10 (PWM pin), forward directrion follows right hand rule (counter-clockwise)

// Note that both the pan neck and the tilt neck has a gear which changes the servo rotation direction.

#define PAN_LEFT_AFMS_INDEX      2
#define TILT_LEFT_AFMS_INDEX     4
#define PAN_RIGHT_AFMS_INDEX     3
#define TILT_RIGHT_AFMS_INDEX    1

#define PAN_NECK_CRSERVO_PWM_PIN        9
#define TILT_NECK_CRSERVO_PWM_PIN       10

#define MOTOR_MAX_PWM  255

#define CRSERVO_MAX_PWM  500


Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

Adafruit_DCMotor *m_pl = AFMS.getMotor(PAN_LEFT_AFMS_INDEX);
Adafruit_DCMotor *m_tl = AFMS.getMotor(TILT_LEFT_AFMS_INDEX);
Adafruit_DCMotor *m_pr = AFMS.getMotor(PAN_RIGHT_AFMS_INDEX);
Adafruit_DCMotor *m_tr = AFMS.getMotor(TILT_RIGHT_AFMS_INDEX);

Servo pn; // pan neck (continuous servo)
Servo tn; // tilt neck (continuous servo)

// current actuator output (without saturation)
int pan_left_pwm = 0;
int tilt_left_pwm = 0;
int pan_right_pwm = 0;
int tilt_right_pwm = 0;
int pan_neck_pwm = 0;
int tilt_neck_pwm = 0;

int pn_stop = 1;
int tn_stop = 1;

void motor_actuate(Adafruit_DCMotor *m, int pwm) {

        if (pwm < 0) {

                pwm = -pwm;
                pwm = (pwm > MOTOR_MAX_PWM) ? MOTOR_MAX_PWM : pwm;
                m->setSpeed(pwm);
                m->run(BACKWARD);
                
        } else if (pwm > 0) {

                pwm = (pwm > MOTOR_MAX_PWM) ? MOTOR_MAX_PWM : pwm;
                m->setSpeed(pwm);
                m->run(FORWARD);
                
        } else { // pwm == 0

                m->setSpeed(0);
                m->run(FORWARD);
        }
}

void cr_servo_actuate(int pin, int pwm) {

        pwm = (pwm > CRSERVO_MAX_PWM) ? CRSERVO_MAX_PWM : pwm;
        pwm = (pwm < -CRSERVO_MAX_PWM) ? -CRSERVO_MAX_PWM : pwm;

        if (pin == 0 && pwm != 0) {

                pn_stop = 0;
                pn.writeMicroseconds(pwm + 1500);
                
        } else if (pin == 1 && pwm != 0) {

                tn_stop = 0;
                tn.writeMicroseconds(pwm + 1500);
                
        } else if (pin == 0 && pwm == 0 && pn_stop != 1) {

                pn_stop = 1;
                tn.writeMicroseconds(1500);
                
        } else if (pin == 1 && pwm == 0 && tn_stop != 1) {

                tn_stop = 1;
                tn.writeMicroseconds(1500);
        }
}

void actuate() {

        motor_actuate(m_pl, pan_left_pwm);
        motor_actuate(m_tl, tilt_left_pwm);
        motor_actuate(m_pr, pan_right_pwm);
        motor_actuate(m_tr, tilt_right_pwm);

        cr_servo_actuate(0, pan_neck_pwm);
        cr_servo_actuate(1, tilt_neck_pwm);
}



// ----------------------------------- controller -------------------------

// controller flag
long ctrl_flag = 0x00000000;


// desired position (close loop)
int pan_left_ctrl_pos = 512;
int tilt_left_ctrl_pos = 512;
int pan_right_ctrl_pos = 512;
int tilt_right_ctrl_pos = 512;
int pan_neck_ctrl_pos = 512;
int tilt_neck_ctrl_pos = 172;

// desired speed, i.e., PWM (open loop)
int pan_left_ctrl_pwm = 0;
int tilt_left_ctrl_pwm = 0;
int pan_right_ctrl_pwm = 0;
int tilt_right_ctrl_pwm = 0;
int pan_neck_ctrl_pwm = 0;
int tilt_neck_ctrl_pwm = 0;










int controller_iterate(int index) {

        // TODO: implement PID controller


        return 0;
}


long check_flag(long flag, int pos) {

        return (flag >> pos) & 0x00000001;
}

void control() {

        // left eye

        if (check_flag(ctrl_flag, 5) == 0) {

                pan_left_pwm = pan_left_ctrl_pwm;
                
        } else {

                pan_left_pwm = controller_iterate(5);
        }

        if (check_flag(ctrl_flag, 4) == 0) {

                tilt_left_pwm = tilt_left_ctrl_pwm;
                
        } else {

                tilt_left_pwm = controller_iterate(4);
        }


        // right eye

        if (check_flag(ctrl_flag, 3) == 0) {

                pan_right_pwm = pan_right_ctrl_pwm;
                
        } else {

                pan_right_pwm = controller_iterate(3);
        }

        if (check_flag(ctrl_flag, 2) == 0) {

                tilt_right_pwm = tilt_right_ctrl_pwm;
                
        } else {

                tilt_right_pwm = controller_iterate(2);
        }


        // neck

        if (check_flag(ctrl_flag, 1) == 0) {

                pan_neck_pwm = pan_neck_ctrl_pwm;
                
        } else {

                pan_neck_pwm = controller_iterate(1);
        }

        if (check_flag(ctrl_flag, 0) == 0) {

                tilt_neck_pwm = tilt_neck_ctrl_pwm;
                
        } else {

                tilt_neck_pwm = controller_iterate(0);
        }


}





// ----------------------------------- interrupt -------------------------

// loop frequency in millisecond
long loop_period = 10;  // 50 Hz


// Timer interrupt counter. It is reset when new loop period comes.
int timer_counter = 0;

// Timer flag for the controller loop
volatile int timer_loop_flag = 0;

// CAUTION: This function is called in interrupt!!! It must be short!!!
void timer_update() {

  timer_counter++;

  if (timer_counter >= loop_period) {
    timer_loop_flag = 1;
    timer_counter = 0;
  }

}

// ----------------------------------- initialization -------------------------


void setup() {
  
        Serial.begin(576000); 

        Timer1.initialize(1000); // NOTE: the period parameter is in us
        Timer1.attachInterrupt(timer_update);


        AFMS.begin();

        if (!imu_left.begin())
        {
                char *error = "IMU (left) error!";
                serial_send_error_msg(error, strlen(error));
                while (1);
        }
        if (!imu_right.begin())
        {
                char *error = "IMU (right) error!";
                serial_send_error_msg(error, strlen(error));
                while (1);
        }
        
        pn.attach(PAN_NECK_CRSERVO_PWM_PIN);
        tn.attach(TILT_NECK_CRSERVO_PWM_PIN);
        
}


// ----------------------------------- loop -------------------------

unsigned long loop_count = 0;

void loop() {

        if (timer_loop_flag > 0) {
        
                timer_loop_flag = 0;
                
                loop_count++;
                
                unsigned long timestamp = millis();

                sense();

                unsigned long ts1 = millis();

                control();

                unsigned long ts2 = millis();

                actuate();

                unsigned long ts3 = millis();
                
                serial_send_pan_tilt_state(timestamp, ts1, ts2, ts3);
                
                //Serial.println(timestamp);
        
        }
        
        if (Serial.available() > 0) {
                
                int c = Serial.read();
                serial_state_machine_proceed(c);
        }


}





