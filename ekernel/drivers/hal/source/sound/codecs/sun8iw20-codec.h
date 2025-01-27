/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _SUN8IW20_CODEC_H
#define _SUN8IW20_CODEC_H

#include "sound/common/snd_sunxi_rxsync.h"

#define SUNXI_CODEC_BASE_ADDR	(0x02030000ul)
#define SUNXI_CODEC_IRQ                41
#define SUNXI_CODEC_JACK_DET   GPIOE(9)

#define SUNXI_DAC_DPC		0x00
#define SUNXI_DAC_VOL_CTL	0x04
#define SUNXI_DAC_FIFOC		0x10
#define SUNXI_DAC_FIFOS		0x14
#define SUNXI_DAC_TXDATA	0X20
#define SUNXI_DAC_CNT		0x24
#define SUNXI_DAC_DG		0x28

#define	SUNXI_ADC_FIFOC		0x30
#define	SUNXI_ADC_VOL_CTL1	0x34
#define SUNXI_ADC_FIFOS		0x38
#define SUNXI_ADC_RXDATA	0x40
#define SUNXI_ADC_CNT		0x44
#define SUNXI_ADC_DG		0x4C
#define SUNXI_ADC_DIG_CTL	0x50

#define SUNXI_DAC_DAP_CTL	0xF0
#define SUNXI_ADC_DAP_CTL	0xF8

#define SUNXI_DAC_DRC_HHPFC	0x100
#define SUNXI_DAC_DRC_LHPFC	0x104
#define SUNXI_DAC_DRC_CTRL	0x108
#define SUNXI_DAC_DRC_LPFHAT	0x10C
#define SUNXI_DAC_DRC_LPFLAT	0x110
#define SUNXI_DAC_DRC_RPFHAT	0x114
#define SUNXI_DAC_DRC_RPFLAT	0x118
#define SUNXI_DAC_DRC_LPFHRT	0x11C
#define SUNXI_DAC_DRC_LPFLRT	0x120
#define SUNXI_DAC_DRC_RPFHRT	0x124
#define SUNXI_DAC_DRC_RPFLRT	0x128
#define SUNXI_DAC_DRC_LRMSHAT	0x12C
#define SUNXI_DAC_DRC_LRMSLAT	0x130
#define SUNXI_DAC_DRC_RRMSHAT	0x134
#define SUNXI_DAC_DRC_RRMSLAT	0x138
#define SUNXI_DAC_DRC_HCT	0x13C
#define SUNXI_DAC_DRC_LCT	0x140
#define SUNXI_DAC_DRC_HKC	0x144
#define SUNXI_DAC_DRC_LKC	0x148
#define SUNXI_DAC_DRC_HOPC	0x14C
#define SUNXI_DAC_DRC_LOPC	0x150
#define SUNXI_DAC_DRC_HLT	0x154
#define SUNXI_DAC_DRC_LLT	0x158
#define SUNXI_DAC_DRC_HKI	0x15C
#define SUNXI_DAC_DRC_LKI	0x160
#define SUNXI_DAC_DRC_HOPL	0x164
#define SUNXI_DAC_DRC_LOPL	0x168
#define SUNXI_DAC_DRC_HET	0x16C
#define SUNXI_DAC_DRC_LET	0x170
#define SUNXI_DAC_DRC_HKE	0x174
#define SUNXI_DAC_DRC_LKE	0x178
#define SUNXI_DAC_DRC_HOPE	0x17C
#define SUNXI_DAC_DRC_LOPE	0x180
#define SUNXI_DAC_DRC_HKN	0x184
#define SUNXI_DAC_DRC_LKN	0x188
#define SUNXI_DAC_DRC_SFHAT	0x18C
#define SUNXI_DAC_DRC_SFLAT	0x190
#define SUNXI_DAC_DRC_SFHRT	0x194
#define SUNXI_DAC_DRC_SFLRT	0x198
#define SUNXI_DAC_DRC_MXGHS	0x19C
#define SUNXI_DAC_DRC_MXGLS	0x1A0
#define SUNXI_DAC_DRC_MNGHS	0x1A4
#define SUNXI_DAC_DRC_MNGLS	0x1A8
#define SUNXI_DAC_DRC_EPSHC	0x1AC
#define SUNXI_DAC_DRC_EPSLC	0x1B0
#define SUNXI_DAC_DRC_OPT	0x1B4
#define SUNXI_DAC_DRC_HPFHGAIN	0x1B8
#define SUNXI_DAC_DRC_HPFLGAIN	0x1BC

#define SUNXI_ADC_DRC_HHPFC	0x200
#define SUNXI_ADC_DRC_LHPFC	0x204
#define SUNXI_ADC_DRC_CTRL	0x208
#define SUNXI_ADC_DRC_LPFHAT	0x20C
#define SUNXI_ADC_DRC_LPFLAT	0x210
#define SUNXI_ADC_DRC_RPFHAT	0x214
#define SUNXI_ADC_DRC_RPFLAT	0x218
#define SUNXI_ADC_DRC_LPFHRT	0x21C
#define SUNXI_ADC_DRC_LPFLRT	0x220
#define SUNXI_ADC_DRC_RPFHRT	0x224
#define SUNXI_ADC_DRC_RPFLRT	0x228
#define SUNXI_ADC_DRC_LRMSHAT	0x22C
#define SUNXI_ADC_DRC_LRMSLAT	0x230
#define SUNXI_ADC_DRC_HCT	0x23C
#define SUNXI_ADC_DRC_LCT	0x240
#define SUNXI_ADC_DRC_HKC	0x244
#define SUNXI_ADC_DRC_LKC	0x248
#define SUNXI_ADC_DRC_HOPC	0x24C
#define SUNXI_ADC_DRC_LOPC	0x250
#define SUNXI_ADC_DRC_HLT	0x254
#define SUNXI_ADC_DRC_LLT	0x258
#define SUNXI_ADC_DRC_HKI	0x25C
#define SUNXI_ADC_DRC_LKI	0x260
#define SUNXI_ADC_DRC_HOPL	0x264
#define SUNXI_ADC_DRC_LOPL	0x268
#define SUNXI_ADC_DRC_HET	0x26C
#define SUNXI_ADC_DRC_LET	0x270
#define SUNXI_ADC_DRC_HKE	0x274
#define SUNXI_ADC_DRC_LKE	0x278
#define SUNXI_ADC_DRC_HOPE	0x27C
#define SUNXI_ADC_DRC_LOPE	0x280
#define SUNXI_ADC_DRC_HKN	0x284
#define SUNXI_ADC_DRC_LKN	0x288
#define SUNXI_ADC_DRC_SFHAT	0x28C
#define SUNXI_ADC_DRC_SFLAT	0x290
#define SUNXI_ADC_DRC_SFHRT	0x294
#define SUNXI_ADC_DRC_SFLRT	0x298
#define SUNXI_ADC_DRC_MXGHS	0x29C
#define SUNXI_ADC_DRC_MXGLS	0x2A0
#define SUNXI_ADC_DRC_MNGHS	0x2A4
#define SUNXI_ADC_DRC_MNGLS	0x2A8
#define SUNXI_ADC_DRC_EPSHC	0x2AC
#define SUNXI_ADC_DRC_EPSLC	0x2B0
#define SUNXI_ADC_DRC_OPT	0x2B4
#define SUNXI_ADC_DRC_HPFHGAIN	0x2B8
#define SUNXI_ADC_DRC_HPFLGAIN	0x2BC

#define SUNXI_AC_VERSION	0x2C0
/* Analog register */
#define SUNXI_ADCL_REG		0x300
#define SUNXI_ADCR_REG		0x304
#define SUNXI_DAC_REG		0x310
#define SUNXI_MICBIAS_REG	0x318
#define SUNXI_RAMP_REG		0x31C
#define SUNXI_BIAS_REG		0x320
#define SUNXI_HEADPHONE_REG	0x324
#define SUNXI_HMIC_CTRL		0x328
#define SUNXI_HMIC_STS		0x32c

/* Analog register base - Digital register base */
/*SUNXI_PR_CFG is to tear the acreg and dcreg, it is of no real meaning*/
#define SUNXI_PR_CFG		(0x300)
#define SUNXI_ADC1_ANA_CTL	(SUNXI_PR_CFG + 0x00)
#define SUNXI_ADC2_ANA_CTL	(SUNXI_PR_CFG + 0x04)
#define SUNXI_ADC3_ANA_CTL	(SUNXI_PR_CFG + 0x08)
#define SUNXI_DAC_ANA_CTL	(SUNXI_PR_CFG + 0x10)
#define SUNXI_MICBIAS_ANA_CTL	(SUNXI_PR_CFG + 0x18)
#define SUNXI_RAMP_ANA_CTL	(SUNXI_PR_CFG + 0x1c)
#define SUNXI_BIAS_ANA_CTL	(SUNXI_PR_CFG + 0x20)
#define SUNXI_HP_ANA_CTL	(SUNXI_PR_CFG + 0x40)
#define SUNXI_POWER_ANA_CTL	(SUNXI_PR_CFG + 0x48)

/* SUNXI_DAC_DPC:0x00 */
#define EN_DAC			31
#define MODQU			25
#define DWA_EN			24
#define HPF_EN			18
#define DVOL			12
#define DAC_HUB_EN		0

/* SUNXI_DAC_VOL_CTL:0x04 */
#define DAC_VOL_SEL		16
#define DAC_VOL_L		8
#define DAC_VOL_R		0

/* SUNXI_DAC_FIFOC:0x10 */
#define DAC_FS			29
#define FIR_VER			28
#define SEND_LASAT		26
#define FIFO_MODE		24
#define DAC_DRQ_CLR_CNT		21
#define TX_TRIG_LEVEL		8
#define DAC_MONO_EN		6
#define TX_SAMPLE_BITS		5
#define DAC_DRQ_EN		4
#define DAC_IRQ_EN		3
#define FIFO_UNDERRUN_IRQ_EN	2
#define FIFO_OVERRUN_IRQ_EN	1
#define FIFO_FLUSH		0

/* SUNXI_DAC_FIFOS:0x14 */
#define	TX_EMPTY		23
#define	DAC_TXE_CNT		8
#define	DAC_TXE_INT		3
#define	DAC_TXU_INT		2
#define	DAC_TXO_INT		1

/* SUNXI_DAC_DG:0x28 */
#define	DAC_MODU_SEL		11
#define	DAC_PATTERN_SEL		9
#define	DAC_CODEC_CLK_SEL	8
#define	DAC_SWP			6
#define	ADDA_LOOP_MODE		0

/* SUNXI_ADC_FIFOC:0x30 */
#define ADC_FS			29
#define EN_AD			28
#define ADCFDT			26
#define ADCDFEN			25
#define RX_FIFO_MODE		24
#define RX_SYNC_EN_STA		21
#define RX_SYNC_EN		20
#define RX_SAMPLE_BITS		16
#define RX_FIFO_TRG_LEVEL	4
#define ADC_DRQ_EN		3
#define ADC_IRQ_EN		2
#define ADC_OVERRUN_IRQ_EN	1
#define ADC_FIFO_FLUSH		0

/* SUNXI_ADC_VOL_CTL1:0x34 */
#define ADC3_VOL		16
#define ADC2_VOL		8
#define ADC1_VOL		0

/* SUNXI_ADC_FIFOS:0x38 */
#define	RXA			23
#define	ADC_RXA_CNT		8
#define	ADC_RXA_INT		3
#define	ADC_RXO_INT		1

/* SUNXI_ADC_DG:0x4C */
#define	AD_SWP			24

/* SUNXI_ADC_DIG_CTL:0x50 */
#define	ADC3_VOL_EN	17
#define	ADC1_2_VOL_EN	16
#define	ADC3_CHANNEL_EN	2
#define	ADC2_CHANNEL_EN	1
#define	ADC1_CHANNEL_EN	0
#define	ADC_CHANNEL_EN	0

/* SUNXI_DAC_DAP_CTL:0xf0 */
#define	DDAP_EN			31
#define	DDAP_DRC_EN		29
#define	DDAP_HPF_EN		28

/* SUNXI_ADC_DAP_CTL:0xf8 */
#define	ADC_DAP0_EN		31
#define	ADC_DRC0_EN		29
#define	ADC_HPF0_EN		28
#define	ADC_DAP1_EN		27
#define	ADC_DRC1_EN		25
#define	ADC_HPF1_EN		24

/* SUNXI_DAC_DRC_HHPFC : 0x100*/
#define DAC_HHPF_CONF		0

/* SUNXI_DAC_DRC_LHPFC : 0x104*/
#define DAC_LHPF_CONF		0

/* SUNXI_DAC_DRC_CTRL : 0x108*/
#define DAC_DRC_DELAY_OUT_STATE	15
#define DAC_DRC_SIGNAL_DELAY	8
#define DAC_DRC_DELAY_BUF_EN	7
#define DAC_DRC_GAIN_MAX_EN	6
#define DAC_DRC_GAIN_MIN_EN	5
#define DAC_DRC_NOISE_DET_EN	4
#define DAC_DRC_SIGNAL_SEL	3
#define DAC_DRC_DELAY_EN	2
#define DAC_DRC_LT_EN		1
#define DAC_DRC_ET_EN		0

/* SUNXI_ADC_DRC_HHPFC : 0x200*/
#define ADC_HHPF_CONF		0

/* SUNXI_ADC_DRC_LHPFC : 0x204*/
#define ADC_LHPF_CONF		0

/* SUNXI_ADC_DRC_CTRL : 0x208*/
#define ADC_DRC_DELAY_OUT_STATE	15
#define ADC_DRC_SIGNAL_DELAY	8
#define ADC_DRC_DELAY_BUF_EN	7
#define ADC_DRC_GAIN_MAX_EN	6
#define ADC_DRC_GAIN_MIN_EN	5
#define ADC_DRC_NOISE_DET_EN	4
#define ADC_DRC_SIGNAL_SEL	3
#define ADC_DRC_DELAY_EN	2
#define ADC_DRC_LT_EN		1
#define ADC_DRC_ER_EN		0

/* SUNXI_ADC1_ANA_CTL:0x300 */
/* SUNXI_ADC2_ANA_CTL:0x304 */
/* SUNXI_ADC3_ANA_CTL:0x308 */
#define	ADC_EN			(31)
#define MIC_PGA_EN		(30)
#define ADC_DITHER_CTL		(29)
#define MIC_SIN_EN		(28)
#define FMINL_EN		(27)
#define FMINR_EN		(27)
#define FMINL_GAIN		(26)
#define FMINR_GAIN		(26)
#define LINEINL_EN		(23)
#define LINEINR_EN		(23)
#define LINEINL_GAIN		(22)
#define LINEINR_GAIN		(22)
#define ADC_PGA_CTL_RCM		(18)
#define ADC_PGA_IN_VCM_CTL	(16)
#define IOPADC			(14)
#define ADC_PGA_GAIN_CTL	(8)
#define ADC_IOPAAF		(6)
#define ADC_IOPSDM1		(4)
#define ADC_IOPSDM2		(2)
#define ADC_IOPMIC		(0)

/* SUNXI_DAC_ANA_CTL: SUNXI_PR_CFG + 0x10 */
#define CURRENT_TEST_SELECT	23
#define	VRA2_IOPVRS		20
#define	ILINEOUTAMPS		18
#define IOPDACS			16
#define DACLEN			15
#define DACREN			14
#define LINEOUTL_EN		13
#define DACLMUTE		12
#define LINEOUTR_EN		11
#define DACRMUTE		10
#define LINEOUTLDIFFEN		6
#define LINEOUTRDIFFEN		5
#define LINEOUT_VOL		0

/* SUNXI_MICBIAS_ANA_CTL: SUNXI_PR_CFG + 0x18 */
#define MICADCEN		20
#define MMICBIASEN		7
#define	MBIASSEL		5
#define	MMICBIAS_CHOP_EN	4
#define MMICBIAS_CHOP_CLK_SEL	2

/* SUNXI_RAMP_ANA_CTL: SUNXI_PR_CFG + 0x1c */
#define RMCEN			1
#define RDEN			0

/* SUNXI_BIAS_ANA_CTL: SUNXI_PR_CFG + 0x20 */
#define AC_BIASDATA		0

/* SUNXI_HP_ANA_CTL: SUNXI_PR_CFG + 0x40 */
#define HPFB_BUF_EN		31
#define HP_GAIN			28
#define HP_DRVEN		21
#define HP_DRVOUTEN		20
#define RSWITCH			19
#define RAMPEN			18
#define HPFB_IN_EN		17
#define RAMP_FINAL_CTL		16
#define RAMP_OUT_EN		15

/* SUNXI_POWER_ANA_CTL: SUNXI_PR_CFG + 0x48 */
#define HPLDO_EN		30
#define BG_TRIM			0

#define CODEC_REG_LABEL(constant)	{#constant, constant, 0}
#define CODEC_REG_LABEL_END		{NULL, 0, 0}

/* SUNXI_CODEC_DAP_ENABLE: Whether to use the adc/dac drc/hpf function */
#define SUNXI_CODEC_DAP_ENABLE

struct sunxi_codec_param {
	int16_t gpio_spk;
	int16_t gpio_spk_power;//add
	int16_t pa_msleep_time;
	uint8_t pa_level;
	uint8_t digital_vol;
	uint8_t lineout_vol;
	uint8_t mic1gain;
	uint8_t mic2gain;
	uint8_t mic3gain;
	uint8_t lineingain;
	uint8_t adcgain;
	uint8_t adcdrc_cfg;
	uint8_t adchpf_cfg;
	uint8_t dacdrc_cfg;
	uint8_t dachpf_cfg;
	uint8_t pb_audio_route;
	uint8_t cap_audio_route;
	uint8_t adc1_flag;
	uint8_t adc2_flag;
	uint8_t adc3_flag;
	bool rx_sync_en;
	bool rx_sync_ctl;
	int16_t rx_sync_id;
	rx_sync_domain_t rx_sync_domain;
	bool hp_detect_used;
};

struct sunxi_codec_info {
	void *codec_base_addr;
	bool hub_mode;//add to rtos
	hal_clk_t busclk;
	hal_clk_t pllclk;
	hal_clk_t pllclk1;
	hal_clk_t pllclkx4;
	hal_clk_t moduleclk;
	hal_clk_t moduleclk1;

	uint32_t irq;
	struct sunxi_codec_param param;

	int chip_ver;
};

#endif /* __SUN8IW20_CODEC_H */
