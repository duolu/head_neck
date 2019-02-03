/*
 * This is the "driver" code running on Linux that interface with the head neck
 * hardware (the Arduino micro-controller) using a serial port.
 *
 * For the big pictures:
 *
 *                          +---------+
 *     joystick thread ---> | pt_ctrl | ---> serial tx thread (downlink, 50 Hz)
 *                          +---------+          |
 *                                               V
 *                                      +-----------------+
 *                                      | latest_ctrl_msg |
 *                                      +-----------------+
 *
 *                   +------------------+
 *     log file <--- | latest_state_msg | <--- serial rx thread (uplink, 50 Hz)
 *                   +------------------+
 *
 * Some explanations:
 *
 * 		* There is a protocol designed to periodically talk between the Linux
 * 		  machine and the micro-controller, see "sprotocol.h". We call the
 * 		  communication from the micro-controller to the Linux machine as
 * 		  "uplink", and the reverse direction as "downlink"
 *
 * 		* There are three threads, one for serial transmission, one for serial
 * 		  receiving, one for joystick events.
 *
 * 		* There is a global variable "pt_ctrl" holds the states. There are two
 * 		  global variables that hold the latest message.
 *
 * 		* The joystick thread reads joystick event from /dev/input/js0, and
 * 		  modify states in the global variable "pt_ctrl". The joystick can
 * 		  specify the position of each joint or speed of each motor. See the
 * 		  file "joystick.h" and "joystick.c" for details.
 *
 * 		* The serial transmission thread regularly wakes up and sends messages
 * 		  in the downlink. These messages are instances of the following struct
 * 		  "struct h2l_state_pan_tilt_ctrl_msg", which contains control commands
 * 		  such as the angular position of each joint (in close loop mode), or
 * 		  speed of each motor (in open loop mode). Each motor is independently
 * 		  controlled, i.e., the mode of each motor is independent from others.
 * 		  The control modes of the six motors are specified in the lower six
 * 		  bits of "ctrl_flag" field, both in the message and in the "pt_ctrl"
 * 		  global variable.
 *
 * 		* The serial transmission thread always send messages based on the
 * 		  current value of the "pt_ctrl" global variable, and it runs at a
 * 		  constant rate (50 Hz currently). Even if there is no joystick event,
 * 		  messages are sent in the downlink anyway.
 *
 * 		* The serial receiving thread block on the serial port /dev/ttyACM0,
 * 		  and read out messages sent in the uplink. The messages are instances
 * 		  of "struct h2l_ctrl_pan_tilt_state_msg", which contains state reports
 * 		  from the micro-controller, including the current position measured by
 * 		  the potentiometer, latest desired position of joints or PWM of motor,
 * 		  angular position measured by the IMUs, etc.
 *
 * 		* Messages in the uplink arrive regularly at a constant rate (also at
 * 		  50 Hz currently), and hence, the serial receiving thread wakes up
 * 		  regularly. It it does not receive any message for a while, it means
 * 		  the micro-controller is not running properly (rare cases). Currently,
 * 		  this error is not handled and the micro-controller is not restarted.
 *
 * 		* In rare cases, the micro-controller may send error messages, which is
 * 		  a type of message with a fixed header and a variable length string
 * 		  with human readable information about an error. This program running
 * 		  on the Linux machine shall print the error message and hang or abort.
 *
 *		* Messages in both the uplink and the downlink contains a fixed sized
 *		  (4 bytes) header, actual payload (whose size depends on the type of
 *		  the message), and a fixed sized (4 bytes) checksum. The checksum is
 *		  calculated as the sum of all 16-bit blocks of the actual payload,
 *		  treated as unsigned integers. The payload is always multiples of four
 *		  bytes.
 *
 * 		* See the file "sprotocol.h" and "sprotocol.c" for details on the
 * 		  protocol and the serial transmission and receiving threads. See the
 * 		  file "serial.h" and "serial.c" for detials on low level serial port
 * 		  interface.
 *
 * 		* The main thread initializes the system and creates other threads. See
 * 		  "head_neck_motor_ctl.cpp" for details.
 *
 * The log file contains state messages in uplink in CSV (comma separated
 * values) format, where each row is a message, and each column is a field.
 * The meaning of each column is as follows. Each log row is generated
 * immediately after a message is received in the uplink.
 *
 * 		* ts_rx: timestamp at receiving on the Linux machine in microsecond.
 * 		* ts_tx: timestamp at sending on the micro-controller in millisecond.
 * 		* checksum_ok: 1 if the checksum in the msg is correct; otherwise 0.
 * 		* ctrl_flag: controller flag in decimal number.
 *
 * 		* xxx_yyy_ctrl_pos: desired position (6 columns).
 * 		* xxx_yyy_pwm: actual PWM values of the motors (6 columns).
 * 		* xxx_yyy_pos: joint positions measured by potentiometers (6 columns).
 * 		* euler_yyy_xyz: angular positions in Euler angles measured by IMU
 * 		                 (6 columns).
 *
 * 		* 0: there is always a trailing 0 at the end of each row.
 *
 *
 */