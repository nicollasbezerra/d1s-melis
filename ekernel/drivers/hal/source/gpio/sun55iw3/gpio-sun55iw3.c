/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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

#include <stdio.h>
#include <string.h>

#include <hal_gpio.h>
#include "../gpio.h"

static const unsigned int sun55iw3p1_irq_bank_base[] =
{
    SUNXI_PIO_BANK_BASE(PB_BASE, 0),
    SUNXI_PIO_BANK_BASE(PC_BASE, 1),
    SUNXI_PIO_BANK_BASE(PD_BASE, 2),
    SUNXI_PIO_BANK_BASE(PE_BASE, 3),
    SUNXI_PIO_BANK_BASE(PF_BASE, 4),
    SUNXI_PIO_BANK_BASE(PG_BASE, 5),
};

static const unsigned int sun55iw3p1_bank_base[] =
{
    SUNXI_PIO_BANK_BASE(PB_BASE, 0),
    SUNXI_PIO_BANK_BASE(PC_BASE, 1),
    SUNXI_PIO_BANK_BASE(PD_BASE, 2),
    SUNXI_PIO_BANK_BASE(PE_BASE, 3),
    SUNXI_PIO_BANK_BASE(PF_BASE, 4),
    SUNXI_PIO_BANK_BASE(PG_BASE, 5),
};

static const int sun55iw3p1_bank_irq_num[] =
{
    SUNXI_IRQ_GPIOB,
    SUNXI_IRQ_GPIOC,
    SUNXI_IRQ_GPIOD,
    SUNXI_IRQ_GPIOE,
    SUNXI_IRQ_GPIOF,
    SUNXI_IRQ_GPIOG,
};

static struct gpio_desc sun55iw3p1_gpio_desc =
{
    .membase = SUNXI_GPIO_PBASE,
    .resource_size = SUNXI_GPIO_RES_SIZE,
    .virq_offset = 0,
    .irq_arry_size = ARRAY_SIZE(sun55iw3p1_bank_irq_num),
    .irq = (const uint32_t *)sun55iw3p1_bank_irq_num,
    .pin_base = PA_BASE,
    .banks = ARRAY_SIZE(sun55iw3p1_bank_base),
    .bank_base = (const uint32_t *)sun55iw3p1_bank_base,
    .irq_banks = ARRAY_SIZE(sun55iw3p1_irq_bank_base),
    .irq_bank_base = (const uint32_t *)sun55iw3p1_irq_bank_base,
};

static const struct gpio_desc *platform_gpio_desc[] =
{
    &sun55iw3p1_gpio_desc,
    NULL,
};

/*
 * Called by hal_gpio_init().
 */
const struct gpio_desc **gpio_get_platform_desc(void)
{
	return platform_gpio_desc;
}
