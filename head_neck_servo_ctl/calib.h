/*
 * calib.h
 *
 *  Created on: Jul 17, 2018
 *      Author: priori
 */

#ifndef CALIB_H_
#define CALIB_H_


void head_neck_calib_cam_capture(
		int cam_base_id, int cam_mobile_id,
		int camera_width, int camera_height,
		int board_n, int board_m, float board_cell_width, float board_cell_height,
		int nr_images, char *fn, char *folder);


#endif /* CALIB_H_ */
