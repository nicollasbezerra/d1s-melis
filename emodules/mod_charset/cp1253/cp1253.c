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
#include "mod_charset_i.h"
#include <libc.h>

static const unsigned short cp1253_2uni[128] =
{
    /* 0x80 */
    0x20ac, 0xfffd, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0xfffd, 0x2030, 0xfffd, 0x2039, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
    /* 0x90 */
    0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0xfffd, 0x2122, 0xfffd, 0x203a, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
    /* 0xa0 */
    0x00a0, 0x0385, 0x0386, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0xfffd, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x2015,
    /* 0xb0 */
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x0384, 0x00b5, 0x00b6, 0x00b7,
    0x0388, 0x0389, 0x038a, 0x00bb, 0x038c, 0x00bd, 0x038e, 0x038f,
    /* 0xc0 */
    0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
    0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f,
    /* 0xd0 */
    0x03a0, 0x03a1, 0xfffd, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7,
    0x03a8, 0x03a9, 0x03aa, 0x03ab, 0x03ac, 0x03ad, 0x03ae, 0x03af,
    /* 0xe0 */
    0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
    /* 0xf0 */
    0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7,
    0x03c8, 0x03c9, 0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce, 0xfffd,
};

static int cp1253_mbtowc(__u16 *pwc, const unsigned char *s, int n)
{
    unsigned char c = *s;

    if (s == NULL || n <= 0)
    {
        return -1;
    }

    if (c < 0x80)
    {
        *pwc = (__u16) c;
        return 1;
    }
    else
    {
        unsigned short wc = cp1253_2uni[c - 0x80];

        if (wc != 0xfffd)
        {
            *pwc = (__u16) wc;
            return 1;
        }
    }

    return -1;
}

static const unsigned char cp1253_page00[32] =
{
    0xa0, 0x00, 0x00, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, /* 0xa0-0xa7 */
    0xa8, 0xa9, 0x00, 0xab, 0xac, 0xad, 0xae, 0x00, /* 0xa8-0xaf */
    0xb0, 0xb1, 0xb2, 0xb3, 0x00, 0xb5, 0xb6, 0xb7, /* 0xb0-0xb7 */
    0x00, 0x00, 0x00, 0xbb, 0x00, 0xbd, 0x00, 0x00, /* 0xb8-0xbf */
};
static const unsigned char cp1253_page03[80] =
{
    0x00, 0x00, 0x00, 0x00, 0xb4, 0xa1, 0xa2, 0x00, /* 0x80-0x87 */
    0xb8, 0xb9, 0xba, 0x00, 0xbc, 0x00, 0xbe, 0xbf, /* 0x88-0x8f */
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, /* 0x90-0x97 */
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, /* 0x98-0x9f */
    0xd0, 0xd1, 0x00, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, /* 0xa0-0xa7 */
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, /* 0xa8-0xaf */
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, /* 0xb0-0xb7 */
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, /* 0xb8-0xbf */
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, /* 0xc0-0xc7 */
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0x00, /* 0xc8-0xcf */
};
static const unsigned char cp1253_page20[48] =
{
    0x00, 0x00, 0x00, 0x96, 0x97, 0xaf, 0x00, 0x00, /* 0x10-0x17 */
    0x91, 0x92, 0x82, 0x00, 0x93, 0x94, 0x84, 0x00, /* 0x18-0x1f */
    0x86, 0x87, 0x95, 0x00, 0x00, 0x00, 0x85, 0x00, /* 0x20-0x27 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x28-0x2f */
    0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x30-0x37 */
    0x00, 0x8b, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x38-0x3f */
};

static int cp1253_wctomb(unsigned char *r, __u16 wc, int n)
{
    unsigned char c = 0;

    if (r == NULL || n == 0)
    {
        return -1;
    }

    if (wc < 0x0080)
    {
        *r = wc;
        return 1;
    }
    else if (wc >= 0x00a0 && wc < 0x00c0)
    {
        c = cp1253_page00[wc - 0x00a0];
    }
    else if (wc == 0x0192)
    {
        c = 0x83;
    }
    else if (wc >= 0x0380 && wc < 0x03d0)
    {
        c = cp1253_page03[wc - 0x0380];
    }
    else if (wc >= 0x2010 && wc < 0x2040)
    {
        c = cp1253_page20[wc - 0x2010];
    }
    else if (wc == 0x20ac)
    {
        c = 0x80;
    }
    else if (wc == 0x2122)
    {
        c = 0x99;
    }

    if (c != 0)
    {
        *r = c;
        return 1;
    }

    return -1;
}


__s32 CP1253_uni2char(const __u16 uni, __u8 *out, __u32 boundlen)
{
    __s32 n;
    n = cp1253_wctomb(out, uni, boundlen);

    if (n <= 0)
    {
        *out = '?';
        return -1;
    }
    else
    {
        return n;
    }
}


__s32 CP1253_char2uni(const __u8 *rawstring, __u32 boundlen, __u16 *uni)
{
    __s32 n;
    n = cp1253_mbtowc(uni, rawstring, boundlen);

    if (n <= 0)
    {
        *uni = 0x003f;  /* ? */
        return -1;
    }
    else
    {
        return n;
    }
}


__u32 CP1253_get_charset2upper_table(void *buf, __u32 size)
{
    return 0;
}


__u32 CP1253_get_charset2lower_table(void *buf, __u32 size)
{
    return 0;
}
