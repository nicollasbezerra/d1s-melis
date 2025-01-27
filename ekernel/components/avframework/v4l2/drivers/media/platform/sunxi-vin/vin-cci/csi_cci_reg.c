/*
 * ekernel/components/avframework/v4l2/platform/sunxi-vin/vin-cci/csi_cci_reg.c
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

/*
 ******************************************************************************
 *
 * csi_cci_reg.c
 *
 * Hawkview ISP - csi_cci_reg.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version         Author         Date          Description
 *
 *   3.0         Yang Feng    2015/12/02    ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#include "csi_cci_reg_i.h"
#include "csi_cci_reg.h"

#include "../utility/vin_io.h"

unsigned int fifo_last_pt[MAX_CSIC_CCI_NUM];
volatile void __iomem *csi_cci_base_addr[MAX_CSIC_CCI_NUM];

int csi_cci_set_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel >= MAX_CSIC_CCI_NUM)
		return -1;
	csi_cci_base_addr[sel] = (volatile void __iomem *)addr;
	return 0;
}

void csi_cci_enable(unsigned int sel)
{
	vin_reg_set(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
		    1 << CCI_CTRL_CCI_EN);
}

void csi_cci_disable(unsigned int sel)
{
	vin_reg_clr(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
		    1 << CCI_CTRL_CCI_EN);
}

void csi_cci_reset(unsigned int sel)
{
	vin_reg_set(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
		    1 << CCI_CTRL_SOFT_RESET);
	vin_reg_clr(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
		    1 << CCI_CTRL_SOFT_RESET);
}

void csi_cci_set_clk_div(unsigned int sel, unsigned char *div_coef)
{
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
			CCI_BUS_CTRL_CLK_M_MASK,
			div_coef[0] << CCI_BUS_CTRL_CLK_M);
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
			CCI_BUS_CTRL_CLK_N_MASK,
			div_coef[1] << CCI_BUS_CTRL_CLK_N);
}

/*interval unit in 40 scl cycles*/
void csi_cci_set_pkt_interval(unsigned int sel, unsigned char interval)
{
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_CFG_OFF,
			CCI_CFG_INTERVAL_MASK, interval << CCI_CFG_INTERVAL);
}

/*timeout unit in scl cycle*/
void csi_cci_set_ack_timeout(unsigned int sel, unsigned char to_val)
{
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_CFG_OFF,
			CCI_CFG_TIMEOUT_N_MASK, to_val << CCI_CFG_TIMEOUT_N);
}

/*trig delay unit in scl cycle*/
void csi_cci_set_trig_dly(unsigned int sel, unsigned int dly)
{
	if (dly == 0) {
		vin_reg_clr(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
			    1 << CCI_BUS_CTRL_DLY_TRIG);
	} else {
		vin_reg_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
			    1 << CCI_BUS_CTRL_DLY_TRIG);
		vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
				CCI_BUS_CTRL_DLY_CYC_MASK,
				dly << CCI_BUS_CTRL_DLY_CYC);
	}
}

void csi_cci_trans_start(unsigned int sel, enum cci_trans_mode trans_mode)
{
	fifo_last_pt[sel] = 0;
	switch (trans_mode) {
	case SINGLE:
		vin_reg_clr(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
			    1 << CCI_CTRL_REPEAT_TRAN);
		vin_reg_set(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
			    1 << CCI_CTRL_SINGLE_TRAN);
		break;
	case REPEAT:
		vin_reg_clr(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
			    1 << CCI_CTRL_SINGLE_TRAN);
		vin_reg_set(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
			    1 << CCI_CTRL_REPEAT_TRAN);
		break;
	default:
		vin_reg_clr(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
			    1 << CCI_CTRL_SINGLE_TRAN);
		vin_reg_clr(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
			    1 << CCI_CTRL_REPEAT_TRAN);
		break;
	}
}

unsigned int csi_cci_get_trans_done(unsigned int sel)
{
	unsigned int reg_val, single_tran;

	reg_val = vin_reg_readl(csi_cci_base_addr[sel] + CCI_CTRL_OFF);
	single_tran = (reg_val & (1 << CCI_CTRL_SINGLE_TRAN));
	if (single_tran == 0)
		return 0;
	else
		return 1;
}

void csi_cci_set_bus_fmt(unsigned int sel, struct cci_bus_fmt *bus_fmt)
{
	if (bus_fmt->rs_mode == 0)
		vin_reg_clr(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
			    1 << CCI_CTRL_RESTART_MODE);
	else
		vin_reg_set(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
			    1 << CCI_CTRL_RESTART_MODE);

	if (bus_fmt->rs_start == 0)
		vin_reg_clr(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
			    1 << CCI_CTRL_READ_TRAN_MODE);
	else
		vin_reg_set(csi_cci_base_addr[sel] + CCI_CTRL_OFF,
			    1 << CCI_CTRL_READ_TRAN_MODE);

	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_FMT_OFF,
			CCI_FMT_SLV_ID_MASK,
			bus_fmt->saddr_7bit << CCI_FMT_SLV_ID);
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_FMT_OFF, CCI_FMT_CMD_MASK,
			bus_fmt->wr_flag << CCI_FMT_CMD);
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_FMT_OFF,
			CCI_FMT_ADDR_BYTE_MASK,
			bus_fmt->addr_len << CCI_FMT_ADDR_BYTE);
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_FMT_OFF,
			CCI_FMT_DATA_BYTE_MASK,
			bus_fmt->data_len << CCI_FMT_DATA_BYTE);
}

void csi_cci_set_tx_buf_mode(unsigned int sel, struct cci_tx_buf *tx_buf_mode)
{
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_CFG_OFF,
			CCI_CFG_SRC_SEL_MASK,
			tx_buf_mode->buf_src << CCI_CFG_SRC_SEL);
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_CFG_OFF,
			CCI_CFG_PACKET_MODE_MASK,
			tx_buf_mode->pkt_mode << CCI_CFG_PACKET_MODE);
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_FMT_OFF,
			CCI_FMT_PACKET_CNT_MASK,
			tx_buf_mode->pkt_cnt << CCI_FMT_PACKET_CNT);
}

void csi_cci_fifo_pt_reset(unsigned int sel)
{
	fifo_last_pt[sel] = 0;
}

void csi_cci_fifo_pt_add(unsigned int sel, unsigned int byte_cnt)
{
	fifo_last_pt[sel] += byte_cnt;
}

void csi_cci_fifo_pt_sub(unsigned int sel, unsigned int byte_cnt)
{
	fifo_last_pt[sel] -= byte_cnt;
}

static void cci_wr_tx_buf(unsigned int sel, unsigned int byte_index,
			  unsigned char value)
{
	unsigned int index_remain, index_dw_align, tmp;

	index_remain = (byte_index) % 4;
	index_dw_align = (byte_index) / 4;

	tmp =
	    vin_reg_readl(csi_cci_base_addr[sel] + CCI_FIFO_ACC_OFF +
			  4 * index_dw_align);
	tmp &= ~(0xff << (index_remain << 3));
	tmp |= value << (index_remain << 3);
	vin_reg_writel(csi_cci_base_addr[sel] + CCI_FIFO_ACC_OFF +
		       4 * index_dw_align, tmp);
}

static void cci_rd_tx_buf(unsigned int sel, unsigned int byte_index,
			  unsigned char *value)
{
	unsigned int index_remain, index_dw_align, tmp;

	index_remain = (byte_index) % 4;
	index_dw_align = (byte_index) / 4;
	tmp =
	    vin_reg_readl(csi_cci_base_addr[sel] + CCI_FIFO_ACC_OFF +
			  4 * index_dw_align);
	*value = (tmp & (0xff << (index_remain << 3))) >> (index_remain << 3);
}

void csi_cci_wr_tx_buf(unsigned int sel, unsigned char *buf,
		       unsigned int byte_cnt)
{
	unsigned int i;

	for (i = 0; i < byte_cnt; i++, fifo_last_pt[sel]++)
		cci_wr_tx_buf(sel, fifo_last_pt[sel], buf[i]);
}

void csi_cci_rd_tx_buf(unsigned int sel, unsigned char *buf,
		       unsigned int byte_cnt)
{
	unsigned int i;

	for (i = 0; i < byte_cnt; i++, fifo_last_pt[sel]++)
		cci_rd_tx_buf(sel, fifo_last_pt[sel], &buf[i]);
}

void csi_cci_set_trig_mode(unsigned int sel, struct cci_tx_trig *tx_trig_mode)
{
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_CFG_OFF,
			CCI_CFG_TRIG_MODE_MASK,
			tx_trig_mode->trig_src << CCI_CFG_TRIG_MODE);
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_CFG_OFF,
			CCI_CFG_CSI_TRIG_MASK,
			tx_trig_mode->trig_con << CCI_CFG_CSI_TRIG);
}

void csi_cci_set_trig_line_cnt(unsigned int sel, unsigned int line_cnt)
{
	vin_reg_clr_set(csi_cci_base_addr[sel] + CCI_LC_TRIG_OFF,
			CCI_LC_TRIG_LN_CNT_MASK,
			line_cnt << CCI_LC_TRIG_LN_CNT);
}

void cci_int_enable(unsigned int sel, enum cci_int_sel interrupt)
{
	vin_reg_set(csi_cci_base_addr[sel] + CCI_INT_CTRL_OFF,
		    ((interrupt << 16) & 0xffff0000));
}

void cci_int_disable(unsigned int sel, enum cci_int_sel interrupt)
{
	vin_reg_clr(csi_cci_base_addr[sel] + CCI_INT_CTRL_OFF,
		    ((interrupt << 16) & 0xffff0000));
}

void CCI_INLINE cci_int_get_status(unsigned int sel,
					struct cci_int_status *status)
{
	unsigned int reg_val =
	    vin_reg_readl(csi_cci_base_addr[sel] + CCI_INT_CTRL_OFF);
	status->complete = (reg_val >> CCI_INT_CTRL_S_TRAN_COM_PD) & 0x1;
	status->error = (reg_val >> CCI_INT_CTRL_S_TRAN_ERR_PD) & 0x1;
}

void CCI_INLINE cci_int_clear_status(unsigned int sel,
					  enum cci_int_sel interrupt)
{
	vin_reg_clr(csi_cci_base_addr[sel] + CCI_INT_CTRL_OFF, 0xffff << 0);
	vin_reg_set(csi_cci_base_addr[sel] + CCI_INT_CTRL_OFF, interrupt << 0);
}

enum cci_bus_status CCI_INLINE cci_get_bus_status(unsigned int sel)
{
	unsigned int reg_val =
	    vin_reg_readl(csi_cci_base_addr[sel] + CCI_CTRL_OFF);
	return (reg_val >> CCI_CTRL_CCI_STA) & 0xff;
}

void cci_get_line_status(unsigned int sel, struct cci_line_status *status)
{
	unsigned int reg_val =
	    vin_reg_readl(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF);
	status->cci_sck = (reg_val >> CCI_BUS_CTRL_SCL_STA) & 0x1;
	status->cci_sda = (reg_val >> CCI_BUS_CTRL_SDA_STA) & 0x1;
}

void cci_pad_en(unsigned int sel)
{
	vin_reg_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SDA_PEN);
	vin_reg_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SCL_PEN);
}

void cci_stop(unsigned int sel)
{
	vin_reg_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SCL_MOE);
	vin_reg_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SDA_MOE);
	vin_reg_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SCL_MOV);
	csi_cci_udelay(5);
	vin_reg_clr(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SDA_MOV);
	csi_cci_udelay(5);
	vin_reg_clr(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SCL_MOE);
	vin_reg_clr(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SDA_MOE);
}

void cci_sck_cycles(unsigned int sel, unsigned int cycle_times)
{
	vin_reg_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SCL_MOE);
	vin_reg_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SDA_MOE);
	while (cycle_times) {
		vin_reg_set(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
			    1 << CCI_BUS_CTRL_SCL_MOV);
		csi_cci_udelay(5);
		vin_reg_clr(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
			    1 << CCI_BUS_CTRL_SCL_MOV);
		csi_cci_udelay(5);
		cycle_times--;
	}
	vin_reg_clr(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SCL_MOE);
	vin_reg_clr(csi_cci_base_addr[sel] + CCI_BUS_CTRL_OFF,
		    1 << CCI_BUS_CTRL_SDA_MOE);
}

void cci_print_info(unsigned int sel)
{
	unsigned int reg_val = 0, i;

	pr_info("Print cci regs:\n");
	for (i = 0; i < 32; i += 4) {
		reg_val = vin_reg_readl(csi_cci_base_addr[sel] + i);
		pr_info("0x%lx = 0x%x\n",
		       (unsigned long int)csi_cci_base_addr[sel] + i, reg_val);
	}

	for (i = 0; i < 12; i += 4) {
		reg_val = vin_reg_readl(csi_cci_base_addr[sel] + 0x100 + i);
		pr_info("0x%lx = 0x%x\n",
		       (unsigned long int)csi_cci_base_addr[sel] + 0x100 + i,
		       reg_val);
	}
}
