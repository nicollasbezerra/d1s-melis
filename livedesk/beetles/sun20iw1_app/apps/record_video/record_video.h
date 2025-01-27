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
#ifndef __RECORD_VIDEO_H__
#define __RECORD_VIDEO_H__

#include "beetles_app.h"
#define RECORD_VIDEO_FRM_WIN_ID (0x8022)

typedef struct tag_record_video_attr
{
    GUI_FONT           *font;
    H_LYR               layer;
    __s32               current_freq;
} record_video_attr_t;

typedef struct tag_record_video_para
{
    H_LYR   layer;
    GUI_FONT    *font;

    __s32   media_type;
    __s32   total_freq;
    __s32   focus_item;
    __s32   listbar_focus_id;
    __s32   listbar_old_focus_id;

    __s64   File_size;
    __u32   record_time;
    __s32   record_status;
    __s32   record_frequency;
    __s32   record_bandwidth;
    //__u32     record_signal_s;//signal strenth
    //__u32 record_signal_q;//signal quality

    __s32   record_time_id;
    __s32   refresh_time_id;

    __s32   focus_txt_color;
    __s32   unfocus_txt_color;
    H_LBAR  h_ListBar;
    HTHEME  h_item_freq_list_focus;
    HTHEME  h_item_freq_list_unfocus;
    HTHEME  h_item_start_icon;
    HTHEME  h_item_stop_icon;
    HTHEME  h_item_focus_bmp;
    HTHEME  h_item_unfocus_bmp;
    HTHEME  h_select_freq_top_bmp;

    unsigned char disk[4];
    unsigned char File_Path[128];
    record_video_attr_t *record_video_attr;
} record_video_para_t;


typedef struct tag_record_video_ctrl
{
    GUI_FONT *font;
    __s32   current_freq;
    H_WIN   lyr_setting;
    H_WIN   h_frm_record_video;
} record_video_ctrl_t;

H_WIN app_record_video_create(H_WIN parent, __s32 current_freq);
#endif
