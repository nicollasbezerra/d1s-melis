/*
 * ekernel/components/avframework/v4l2/platform/sunxi-vin/modules/flash/flash.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __FLASH_H__
#define __FLASH_H__

#include <v4l2-subdev.h>
#include <v4l2-dev.h>
#include <v4l2-ctrls.h>

#include "../../utility/vin_os.h"

#include "../../utility/vin_supply.h"
#include "../sensor/camera_cfg.h"

typedef enum sunxi_flash_ctrl {
	SW_CTRL_FLASH_OFF = 0x100,
	SW_CTRL_FLASH_ON = 0x101,
	SW_CTRL_TORCH_ON = 0x102,

	CAM_CTRL_FLASH_OFF = 0x200,
	CAM_CTRL_FLASH_ON = 0x201,
	CAM_CTRL_TORCH_ON = 0x202,

	EXT_SYNC_FLASH_OFF = 0x300,
	EXT_SYNC_FLASH_ON = 0x301,
	EXT_SYNC_TORCH_ON = 0x302,

} __flash_ctrl_t;

typedef enum sunxi_flash_driver_ic_type {
	FLASH_RELATING,
	FLASH_EN_INDEPEND,
	FLASH_POWER,
} __flash_driver_ic_type;

typedef enum sunxi_flash_sync {
	NONE,
	LED_SINGLE,
	LED_CONTIN,
	LED_ALTERNATE,
	XENON_PULSE,
} __flash_sync_t;

struct flash_dev_info {
	unsigned int dev_if;	/*0-io type 1-i2c type*/
	unsigned int en_pol;	/*polarity*/
	unsigned int fl_mode_pol;	/*polarity*/

	unsigned int light_src;	/*0x01-LEDX1 0x02-LEDX2 0x10-XENON*/
	unsigned int light_temperature;	/*in K*/

	unsigned int flash_intensity;
	unsigned int flash_level;	/*in lux*/
	unsigned int torch_intensity;
	unsigned int torch_level;	/*in lux*/

	unsigned int timeout_counter;	/*in us*/

	unsigned int status;	/*0-led_off/1-flash_on/2-torch_on/*/
	enum sunxi_flash_driver_ic_type flash_driver_ic;
	enum v4l2_flash_led_mode flash_mode;
	enum sunxi_flash_sync flash_sync;
};

struct flash_dev {
	int use_cnt;
	struct v4l2_subdev subdev;
	struct platform_device *pdev;
	unsigned int id;
	struct flash_dev_info fl_info;
	struct v4l2_ctrl_handler handler;
	struct list_head flash_list;
};

struct v4l2_subdev *sunxi_flash_get_subdev(int id);
int sunxi_flash_platform_register(void);
void sunxi_flash_platform_unregister(void);
int sunxi_flash_check_to_start(struct v4l2_subdev *sd,
			       enum sunxi_flash_ctrl ctrl);
int sunxi_flash_stop(struct v4l2_subdev *sd);
int io_set_flash_ctrl(struct v4l2_subdev *sd, enum sunxi_flash_ctrl ctrl);

#endif	/* __FLASH_H__ */
