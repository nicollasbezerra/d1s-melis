/*
 * ekernel/components/avframework/v4l2/platform/sunxi-vin/vin-mipi/protocol/protocol_reg.h
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


#ifndef __PROTOCOL_REG__H__
#define __PROTOCOL_REG__H__

#include "protocol.h"
#define MAX_MIPI_PTL  2

extern int ptcl_reg_map(unsigned int sel, unsigned long addr_base);
extern void ptcl_enable(unsigned int sel);
extern void ptcl_disable(unsigned int sel);
extern void ptcl_set_data_lane(unsigned int sel, unsigned char lane_num);
extern void ptcl_set_pl_bit_order(unsigned int sel, enum bit_order pl_bit_ord);
extern void ptcl_set_ph_bit_order(unsigned int sel, enum bit_order ph_bit_ord);
extern void ptcl_set_ph_byte_order(unsigned int sel,
				   enum byte_order ph_byte_order);
extern void ptcl_set_total_ch(unsigned int sel, unsigned char ch_num);
extern void ptcl_set_vc(unsigned int sel, unsigned char ch, unsigned char vc);
extern void ptcl_set_dt(unsigned int sel, unsigned char ch, enum pkt_fmt dt);
extern void ptcl_set_src_type(unsigned int sel, unsigned char ch,
			      enum source_type src_type);
extern void ptcl_set_line_sync(unsigned int sel, unsigned char ch,
			       enum line_sync ls_mode);
extern void ptcl_int_enable(unsigned int sel, unsigned char ch,
			    enum protocol_int int_flag);
extern void ptcl_int_disable(unsigned int sel, unsigned char ch,
			     enum protocol_int int_flag);
extern void ptcl_clear_int_status(unsigned int sel, unsigned char ch,
				  enum protocol_int int_flag);
extern unsigned char ptcl_get_data_lane(unsigned int sel);
extern enum bit_order ptcl_get_pl_bit_order(unsigned int sel);
extern enum bit_order ptcl_get_ph_bit_order(unsigned int sel);
extern enum byte_order ptcl_get_ph_byte_order(unsigned int sel);
extern unsigned char ptcl_get_total_ch(unsigned int sel);
extern unsigned char ptcl_get_vc(unsigned int sel, unsigned char ch);
extern enum pkt_fmt ptcl_get_dt(unsigned int sel, unsigned char ch);
extern enum source_type ptcl_get_src_type(unsigned int sel, unsigned char ch);
extern enum line_sync ptcl_get_line_sync(unsigned int sel, unsigned char ch);
extern unsigned char ptcl_get_int_status(unsigned int sel, unsigned char ch,
					 enum protocol_int int_flag);
extern void bsp_data_formats_enable(unsigned int sel, unsigned char type);
extern void bsp_data_formats_disable(unsigned int sel, unsigned char type);

#endif
