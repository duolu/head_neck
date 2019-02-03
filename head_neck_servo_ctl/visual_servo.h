/*
 * visual_servo.h
 *
 *  Created on: Jun 26, 2018
 *      Author: priori
 */

#ifndef VISUAL_SERVO_H_
#define VISUAL_SERVO_H_

void single_cam_visual_servo_loop_color_track(int cam_id);
void single_cam_visual_servo_loop_face_track(int cam_id);


void dual_cam_visual_servo_loop_color_track(int cam_left_id, int cam_right_id);
void dual_cam_visual_servo_loop_face_track(int cam_left_id, int cam_right_id);



#endif /* VISUAL_SERVO_H_ */
