//#define LOG_NDEBUG 0
#define LOG_TAG "sample_virvi2vo"
#include <utils/plat_log.h>

//#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/prctl.h>

//#include <linux/fb.h> 
//#include <sys/mman.h> 

#include "media/mm_comm_vi.h"
#include "media/mpi_vi.h"
#include "vo/hwdisplay.h"
#include "log/log_wrapper.h"

#include <ClockCompPortIndex.h>
#include <mpi_videoformat_conversion.h>
#include <confparser.h>
#include "sample_virvi2vo_config.h"
#include "sample_virvi2vo.h"
#include "mpi_isp.h"

static SampleVIRVI2VOContext *pSampleVIRVI2VOContext = NULL;

static int initSampleVIRVI2VOContext(SampleVIRVI2VOContext *pContext)
{
    memset(pContext, 0, sizeof(SampleVIRVI2VOContext));
    pContext->mUILayer = HLAY(2, 0);
    cdx_sem_init(&pContext->mSemExit, 0);
    return 0;
}

static int destroySampleVIRVI2VOContext(SampleVIRVI2VOContext *pContext)
{
    cdx_sem_deinit(&pContext->mSemExit);
    return 0;
}

static ERRORTYPE SampleVIRVI2VO_VOCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;
    SampleVIRVI2VOContext *pContext = (SampleVIRVI2VOContext*)cookie;
    if(MOD_ID_VOU == pChn->mModId)
    {
        alogd("VO callback: VO Layer[%d] chn[%d] event:%d", pChn->mDevId, pChn->mChnId, event);
        switch(event)
        {
            case MPP_EVENT_RELEASE_VIDEO_BUFFER:
            {
                VIDEO_FRAME_INFO_S *pFrameInfo = (VIDEO_FRAME_INFO_S*)pEventData;
                aloge("vo layer[%d] release frame id[0x%x]!", pChn->mDevId, pFrameInfo->mId);
                break;
            }
            case MPP_EVENT_SET_VIDEO_SIZE:
            {
                SIZE_S *pDisplaySize = (SIZE_S*)pEventData;
                alogd("vo layer[%d] report video display size[%dx%d]", pChn->mDevId, pDisplaySize->Width, pDisplaySize->Height);
                break;
            }
            case MPP_EVENT_RENDERING_START:
            {
                alogd("vo layer[%d] report rendering start", pChn->mDevId);
                break;
            }
            default:
            {
                //postEventFromNative(this, event, 0, 0, pEventData);
                aloge("fatal error! unknown event[0x%x] from channel[0x%x][0x%x][0x%x]!", event, pChn->mModId, pChn->mDevId, pChn->mChnId);
                ret = ERR_VO_ILLEGAL_PARAM;
                break;
            }
        }
    }
    else
    {
        aloge("fatal error! why modId[0x%x]?", pChn->mModId);
        ret = FAILURE;
    }
    return ret;
}

static ERRORTYPE SampleVIRVI2VO_CLOCKCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    alogw("clock channel[%d] has some event[0x%x]", pChn->mChnId, event);
    return SUCCESS;
}

static int ParseCmdLine(int argc, char **argv, SampleVIRVI2VOCmdLineParam *pCmdLinePara)
{
    alogd("sample_virvi2vo path:[%s], arg number is [%d]", argv[0], argc);
    int ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(SampleVIRVI2VOCmdLineParam));
    while(i < argc)
    {
        if(!strcmp(argv[i], "-path"))
        {
            if(++i >= argc)
            {
                aloge("fatal error! use -h to learn how to set parameter!!!");
                ret = -1;
                break;
            }
            if(strlen(argv[i]) >= MAX_FILE_PATH_SIZE)
            {
                aloge("fatal error! file path[%s] too long: [%d]>=[%d]!", argv[i], strlen(argv[i]), MAX_FILE_PATH_SIZE);
            }
            strncpy(pCmdLinePara->mConfigFilePath, argv[i], MAX_FILE_PATH_SIZE-1);
            pCmdLinePara->mConfigFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
        }
        else if(!strcmp(argv[i], "-h"))
        {
            printf("CmdLine param example:\n"
                "\t run -path /home/sample_virvi2vo.conf\n");
            ret = 1;
            break;
        }
        else
        {
            alogd("ignore invalid CmdLine param:[%s], type -h to get how to set parameter!", argv[i]);
        }
        i++;
    }
    return ret;
}

static void LoadVIPP2VOConfig(int idx, VIPP2VOConfig *pVIPP2VOConfig, CONFPARSER_S *pConfParser)
{
    if(0 == idx)
    {
        pVIPP2VOConfig->mDevNum        = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DEVICE_NUMBER, 0);
        pVIPP2VOConfig->mCaptureWidth  = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_CAPTURE_WIDTH, 0);
        pVIPP2VOConfig->mCaptureHeight = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_CAPTURE_HEIGHT, 0);
        pVIPP2VOConfig->mDisplayX   = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_X, 0);
        pVIPP2VOConfig->mDisplayY   = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_Y, 0);
        pVIPP2VOConfig->mDisplayWidth  = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_WIDTH, 0);
        pVIPP2VOConfig->mDisplayHeight = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_HEIGHT, 0);
        pVIPP2VOConfig->mLayerNum = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_LAYER_NUM, 0);
    }
    else if(1 == idx)
    {
        pVIPP2VOConfig->mDevNum        = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DEVICE_NUMBER2, 0);
        pVIPP2VOConfig->mCaptureWidth  = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_CAPTURE_WIDTH2, 0);
        pVIPP2VOConfig->mCaptureHeight = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_CAPTURE_HEIGHT2, 0);
        pVIPP2VOConfig->mDisplayX   = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_X2, 0);
        pVIPP2VOConfig->mDisplayY   = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_Y2, 0);
        pVIPP2VOConfig->mDisplayWidth  = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_WIDTH2, 0);
        pVIPP2VOConfig->mDisplayHeight = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_HEIGHT2, 0);
        pVIPP2VOConfig->mLayerNum = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_LAYER_NUM2, 0);
    }
    else if(2 == idx)
    {
        pVIPP2VOConfig->mDevNum        = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DEVICE_NUMBER3, 0);
        pVIPP2VOConfig->mCaptureWidth  = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_CAPTURE_WIDTH3, 0);
        pVIPP2VOConfig->mCaptureHeight = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_CAPTURE_HEIGHT3, 0);
        pVIPP2VOConfig->mDisplayX   = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_X3, 0);
        pVIPP2VOConfig->mDisplayY   = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_Y3, 0);
        pVIPP2VOConfig->mDisplayWidth  = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_WIDTH3, 0);
        pVIPP2VOConfig->mDisplayHeight = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_HEIGHT3, 0);
        pVIPP2VOConfig->mLayerNum = GetConfParaInt(pConfParser, SAMPLE_VIRVI2VO_KEY_LAYER_NUM3, 0);
    }
    else
    {
        aloge("fatal error! need add vipp[%d] config!", idx);
    }
}

static ERRORTYPE loadSampleVIRVI2VOConfig(SampleVIRVI2VOConfig *pConfig, const char *conf_path)
{
    int ret;
    char *ptr;
    pConfig->mVIPP2VOConfigArray[0].mDevNum = 0;
    pConfig->mVIPP2VOConfigArray[0].mCaptureWidth = 1920;
    pConfig->mVIPP2VOConfigArray[0].mCaptureHeight = 1080;
    pConfig->mVIPP2VOConfigArray[0].mDisplayX = 60;
    pConfig->mVIPP2VOConfigArray[0].mDisplayY = 0;
    pConfig->mVIPP2VOConfigArray[0].mDisplayWidth = 360;
    pConfig->mVIPP2VOConfigArray[0].mDisplayHeight = 640;
    pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    pConfig->mFrameRate = 60;
    pConfig->mTestDuration = 20;
    pConfig->mDispType = VO_INTF_LCD;
    pConfig->mDispSync = VO_OUTPUT_NTSC;
    pConfig->mIspWdrSetting = NORMAL_CFG;
    if(NULL == conf_path)
    {
        alogd("user not set config file. use default test parameter!");
        return SUCCESS;
    }
    CONFPARSER_S stConfParser;
    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0)
    {
        aloge("load conf fail");
        return FAILURE;
    }
    memset(pConfig, 0, sizeof(SampleVIRVI2VOConfig));
    int i;
    for(i=0;i<VIPP2VO_NUM;i++)
    {
        LoadVIPP2VOConfig(i, &pConfig->mVIPP2VOConfigArray[i], &stConfParser);
    }
    pConfig->mbAddUILayer = (BOOL)GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_ADD_UI_LAYER, 0);
    pConfig->mTestUILayer = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_UI_TEST_LAYER, 0);
    pConfig->mUIDisplayWidth = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_UI_DISPLAY_WIDTH, 0);
    pConfig->mUIDisplayHeight = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_UI_DISPLAY_HEIGHT, 0);

    char *pStrPixelFormat = (char*)GetConfParaString(&stConfParser, SAMPLE_VIRVI2VO_KEY_PIC_FORMAT, NULL);
    if(!strcmp(pStrPixelFormat, "yu12"))
    {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YUV_PLANAR_420;
    }
    else if(!strcmp(pStrPixelFormat, "yv12"))
    {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_PLANAR_420;
    }
    else if(!strcmp(pStrPixelFormat, "nv21"))
    {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }
    else if(!strcmp(pStrPixelFormat, "nv12"))
    {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    }
    else if(!strcmp(pStrPixelFormat, "nv21s"))
    {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_AW_NV21S;
    }
    else
    {
        aloge("fatal error! conf file pic_format is [%s]?", pStrPixelFormat);
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }
    char *pStrDispType = (char*)GetConfParaString(&stConfParser, SAMPLE_VIRVI2VO_KEY_DISP_TYPE, NULL);
    if (!strcmp(pStrDispType, "hdmi"))
    {
        pConfig->mDispType = VO_INTF_HDMI;
        if (pConfig->mVIPP2VOConfigArray[0].mDisplayWidth > 1920)
            pConfig->mDispSync = VO_OUTPUT_3840x2160_30;
        else if (pConfig->mVIPP2VOConfigArray[0].mDisplayWidth > 1280)
            pConfig->mDispSync = VO_OUTPUT_1080P30;
        else
            pConfig->mDispSync = VO_OUTPUT_720P60;
    }
    else if (!strcmp(pStrDispType, "lcd"))
    {
        pConfig->mDispType = VO_INTF_LCD;
        pConfig->mDispSync = VO_OUTPUT_NTSC;
    }
    else if (!strcmp(pStrDispType, "cvbs"))
    {
        pConfig->mDispType = VO_INTF_CVBS;
        pConfig->mDispSync = VO_OUTPUT_NTSC;
    }

    pConfig->mFrameRate = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_FRAME_RATE, 0);
    pConfig->mTestDuration = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_TEST_DURATION, 0);

    for(i=0;i<VIPP2VO_NUM;i++)
    {
        alogd("vipp[%d]: captureSize[%dx%d], displayArea[%d,%d,%dx%d]", 
            pConfig->mVIPP2VOConfigArray[i].mDevNum, pConfig->mVIPP2VOConfigArray[i].mCaptureWidth, pConfig->mVIPP2VOConfigArray[i].mCaptureHeight,
            pConfig->mVIPP2VOConfigArray[i].mDisplayX, pConfig->mVIPP2VOConfigArray[i].mDisplayY, pConfig->mVIPP2VOConfigArray[i].mDisplayWidth, pConfig->mVIPP2VOConfigArray[i].mDisplayHeight);
    }
    alogd("dispSync[%d], dispType[%d], frameRate[%d], testDuration[%d]", pConfig->mDispSync, pConfig->mDispType, pConfig->mFrameRate, pConfig->mTestDuration);

    destroyConfParser(&stConfParser);
    return SUCCESS;
}
typedef struct {
    int ae_mode;
    int ae_expOffset;
    int ae_exposure;
    int gain;
    int awb_mode;
    int awb_coltmp;
    int brightness;
    int contrast;
    int saturation;
    int hue;
    unsigned int maxexp;
    unsigned int minexp;
    int sharpness;
}Test_IspParam_S;

static Test_IspParam_S param = {0};
static void handle_exit()
{
    alogd("user want to exit!");
    #if 0
    FILE *isp_fp = NULL;
    isp_fp = fopen("./isp_config.txt","w");
    if(isp_fp == NULL)
    {
        printf("open isp config file error!\n");
    }
    else
    {
        fprintf(isp_fp, "ae_mode=%d\n", param.ae_mode);
        fprintf(isp_fp, "ae_expOffset=%d\n", param.ae_expOffset);
        fprintf(isp_fp, "ae_exposure=%d\n", param.ae_exposure);
        fprintf(isp_fp, "gain=%d\n", param.gain);
        fprintf(isp_fp, "awb_mode=%d\n", param.awb_mode);
        fprintf(isp_fp, "awb_coltmp=%d\n", param.awb_coltmp);
        fprintf(isp_fp, "brightness=%d\n", param.brightness);
        fprintf(isp_fp, "contrast=%d\n", param.contrast);
        fprintf(isp_fp, "saturation=%d\n", param.saturation);
        fprintf(isp_fp, "hue=%d\n", param.hue);
        fprintf(isp_fp, "maxexp=%d\n", (int)param.maxexp);
        fprintf(isp_fp, "minexp=%d\n", (int)param.minexp);
        fprintf(isp_fp, "sharpness=%d\n", param.sharpness);
        
        fclose(isp_fp);
    }
    #endif
    if(NULL != pSampleVIRVI2VOContext)
    {
        cdx_sem_up(&pSampleVIRVI2VOContext->mSemExit);
    }
}

static void print_menu(void)
{
    printf("\n---------------------\n");
    printf("Please input the num to config ISp \n");
    printf("\t num 1 is the AE_mode\n\t num 2 is the AE_expOffset\n\t num 3 is the AE_exposure\n\t num 4 is the gain\n\t num 5 is AWB_MODE\n\t num 6 is GetColorTemp\n\t num 7 is AE Max Exp\n\t num 8 is AE Min Exp\n\t num 9 is HUE\n\t num 10 is the Brightness\n\t num 11 is the contrast\n\t num 12 is sattuation\n\t num 13 is Sharpness\n\t num 14 is User Region\n\t num 99 is quit\n");
    printf("\n---------------------\n");

}

static ERRORTYPE SampleVirvi2Vo_MPPCallback(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    SampleVIRVI2VOContext *pContext = (SampleVIRVI2VOContext*)cookie;
    if(MOD_ID_VIU == pChn->mModId)
    {
        switch(event)
        {
            case MPP_EVENT_VI_TIMEOUT:
            {
                alogd("receive vi timeout. vipp:%d, chn:%d", pChn->mDevId, pChn->mChnId);
                break;
            }
            default:
            {
                aloge("fatal error! unknown event[0x%x] from channel[0x%x][0x%x][0x%x]!", event, pChn->mModId, pChn->mDevId, pChn->mChnId);
                break;
            }
        }
    }
    else
    {
        aloge("fatal error! unknown event[0x%x] from channel[0x%x][0x%x][0x%x]!", event, pChn->mModId, pChn->mDevId, pChn->mChnId);
    }
    return SUCCESS;
}

#define ISP_RUN 1
static ERRORTYPE CreateVIPP2VOLink(int idx, SampleVIRVI2VOContext *pContext)
{
    ERRORTYPE result = SUCCESS;
    VIPP2VOConfig *pConfig = &pContext->mConfigPara.mVIPP2VOConfigArray[idx];
    VIPP2VOLinkInfo *pLinkInfo = &pContext->mLinkInfoArray[idx];
    if(0 == pConfig->mCaptureWidth || 0 == pConfig->mCaptureHeight)
    {
        alogd("do not need create link for idx[%d]", idx);
        return result;
    }
    /* create vi channel */
    pLinkInfo->mVIDev = pConfig->mDevNum;
    pLinkInfo->mVIChn = 0;
    alogd("Vipp dev[%d] vir_chn[%d].", pLinkInfo->mVIDev, pLinkInfo->mVIChn);
    ERRORTYPE eRet = AW_MPI_VI_CreateVipp(pLinkInfo->mVIDev);
    if (eRet != SUCCESS)
    {
        aloge("fatal error! AW_MPI_VI CreateVipp:%d failed", pLinkInfo->mVIDev);
        result = FAILURE;
        goto _err0;
    }
    MPPCallbackInfo cbInfo;
    cbInfo.cookie = (void*)pContext;
    cbInfo.callback = (MPPCallbackFuncType)&SampleVirvi2Vo_MPPCallback;
    eRet = AW_MPI_VI_RegisterCallback(pLinkInfo->mVIDev, &cbInfo);
    if (eRet != SUCCESS)
    {
        aloge("fatal error! vipp[%d] RegisterCallback failed", pLinkInfo->mVIDev);
    }
    VI_ATTR_S stAttr;
    eRet = AW_MPI_VI_GetVippAttr(pLinkInfo->mVIDev, &stAttr);
    if (eRet != SUCCESS)
    {
        aloge("fatal error! AW_MPI_VI GetVippAttr failed");
    }
    memset(&stAttr, 0, sizeof(VI_ATTR_S));
    stAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    stAttr.memtype = V4L2_MEMORY_MMAP;
    stAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pContext->mConfigPara.mPicFormat);
    stAttr.format.field = V4L2_FIELD_NONE;
    stAttr.format.colorspace = V4L2_COLORSPACE_JPEG;
    stAttr.format.width = pConfig->mCaptureWidth;
    stAttr.format.height =pConfig->mCaptureHeight;
    stAttr.nbufs = 5;
    stAttr.nplanes = 2;
    stAttr.drop_frame_num = 0; // drop 2 second video data, default=0
    /* do not use current param, if set to 1, all this configuration will
     * not be used.
     */

#define TEST_WDR_FLIP  1
#ifdef TEST_WDR_FLIP
    stAttr.wdr_mode = 1;
#endif
    if(0 == idx)
    {
        stAttr.use_current_win = 0;
    }
    else
    {
        stAttr.use_current_win = 1;
    }
    stAttr.fps = pContext->mConfigPara.mFrameRate;
    eRet = AW_MPI_VI_SetVippAttr(pLinkInfo->mVIDev, &stAttr);
    if (eRet != SUCCESS)
    {
        aloge("fatal error! AW_MPI_VI SetVippAttr:%d failed", pLinkInfo->mVIDev);
    }
#if ISP_RUN
    /* open isp */
    AW_MPI_ISP_Run(pContext->mIspDev);
#endif
    eRet = AW_MPI_VI_CreateVirChn(pLinkInfo->mVIDev, pLinkInfo->mVIChn, NULL);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! createVirChn[%d] fail!", pLinkInfo->mVIChn);
    }
    eRet = AW_MPI_VI_EnableVipp(pLinkInfo->mVIDev);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! enableVipp fail!");
        result = FAILURE;
        goto _err1;
    }
#ifdef TEST_WDR_FLIP
	AW_MPI_VI_SetVippFlip(pLinkInfo->mVIDev, 1);
#endif
#if ISP_RUN
    //if enable CONFIG_ENABLE_SENSOR_FLIP_OPTION, flip operations must be after stream_on.
    AW_MPI_VI_SetVippMirror(pLinkInfo->mVIDev, 0);//0,1
    AW_MPI_VI_SetVippFlip(pLinkInfo->mVIDev, 0);//0,1
#endif
    /* enable vo layer */
    int hlay0 = 0;
    /*int hwDispChn = 0;
    while(hlay0 < VO_MAX_LAYER_NUM)
    {
        if(SUCCESS == AW_MPI_VO_EnableVideoLayer(hlay0))
        {
            break;
        }
        hwDispChn++;
        hlay0 = HLAY(hwDispChn, 0);
    }
    if(hlay0 >= VO_MAX_LAYER_NUM)
    {
        aloge("fatal error! enable video layer fail!");
    }*/
    hlay0 = pConfig->mLayerNum;
    if(SUCCESS != AW_MPI_VO_EnableVideoLayer(hlay0))
    {
        aloge("fatal error! enable video layer[%d] fail!", hlay0);
        hlay0 = MM_INVALID_LAYER;
        pLinkInfo->mVoLayer = hlay0;
        result = FAILURE;
        goto _err1_5;
    }
    pLinkInfo->mVoLayer = hlay0;
    AW_MPI_VO_GetVideoLayerAttr(pLinkInfo->mVoLayer, &pLinkInfo->mLayerAttr);
    pLinkInfo->mLayerAttr.stDispRect.X = pConfig->mDisplayX;
    pLinkInfo->mLayerAttr.stDispRect.Y = pConfig->mDisplayY;
    pLinkInfo->mLayerAttr.stDispRect.Width = pConfig->mDisplayWidth;
    pLinkInfo->mLayerAttr.stDispRect.Height = pConfig->mDisplayHeight;
    AW_MPI_VO_SetVideoLayerAttr(pLinkInfo->mVoLayer, &pLinkInfo->mLayerAttr);

    /* create vo channel and clock channel. 
    (because frame information has 'pts', there is no need clock channel now) 
    */
    BOOL bSuccessFlag = FALSE;
    pLinkInfo->mVOChn = 0;
    while(pLinkInfo->mVOChn < VO_MAX_CHN_NUM)
    {
        eRet = AW_MPI_VO_EnableChn(pLinkInfo->mVoLayer, pLinkInfo->mVOChn);
        if(SUCCESS == eRet)
        {
            bSuccessFlag = TRUE;
            alogd("create vo channel[%d] success!", pLinkInfo->mVOChn);
            break;
        }
        else if(ERR_VO_CHN_NOT_DISABLE == eRet)
        {
            alogd("vo channel[%d] is exist, find next!", pLinkInfo->mVOChn);
            pLinkInfo->mVOChn++;
        }
        else
        {
            aloge("fatal error! create vo channel[%d] ret[0x%x]!", pLinkInfo->mVOChn, eRet);
            break;
        }
    }
    if(FALSE == bSuccessFlag)
    {
        pLinkInfo->mVOChn = MM_INVALID_CHN;
        aloge("fatal error! create vo channel fail!");
        result = FAILURE;
        goto _err2;
    }
    cbInfo.cookie = (void*)pContext;
    cbInfo.callback = (MPPCallbackFuncType)&SampleVIRVI2VO_VOCallbackWrapper;
    AW_MPI_VO_RegisterCallback(pLinkInfo->mVoLayer, pLinkInfo->mVOChn, &cbInfo);
    AW_MPI_VO_SetChnDispBufNum(pLinkInfo->mVoLayer, pLinkInfo->mVOChn, 2);
    /* bind clock,vo, viChn
    (because frame information has 'pts', there is no need to bind clock channel now)
    */
    MPP_CHN_S VOChn = {MOD_ID_VOU, pLinkInfo->mVoLayer, pLinkInfo->mVOChn};
    MPP_CHN_S VIChn = {MOD_ID_VIU, pLinkInfo->mVIDev, pLinkInfo->mVIChn};
    AW_MPI_SYS_Bind(&VIChn, &VOChn);

    /* start vo, vi_channel. */
    eRet = AW_MPI_VI_EnableVirChn(pLinkInfo->mVIDev, pLinkInfo->mVIChn);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! enableVirChn fail!");
        result = FAILURE;
        goto _err3;
    }
    AW_MPI_VO_StartChn(pLinkInfo->mVoLayer, pLinkInfo->mVOChn);
    return result;

_err3:
    eRet = AW_MPI_SYS_UnBind(&VIChn, &VOChn);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! ViChn && VoChn SYS_UnBind fail!");
    }
    eRet = AW_MPI_VO_DisableChn(pLinkInfo->mVoLayer, pLinkInfo->mVOChn);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! Vo Disable Chn fail!");
    }
_err2:
    eRet = AW_MPI_VO_DisableVideoLayer(pLinkInfo->mVoLayer);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! VO DisableVideoLayer fail!");
    }
_err1_5:
    eRet = AW_MPI_VI_DestroyVirChn(pLinkInfo->mVIDev, pLinkInfo->mVIChn);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! VI DestoryVirChn fail!");
    }
#if ISP_RUN
    eRet = AW_MPI_ISP_Stop(pContext->mIspDev);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! ISP Stop fail!");
    }
#endif
_err1:
    eRet = AW_MPI_VI_DestoryVipp(pLinkInfo->mVIDev);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! VI DestoryVipp fail!");
    }
_err0:
    return result;

}

static ERRORTYPE DestroyVIPP2VOLink(int idx, SampleVIRVI2VOContext *pContext)
{
    ERRORTYPE eRet;
    VIPP2VOConfig *pConfig = &pContext->mConfigPara.mVIPP2VOConfigArray[idx];
    VIPP2VOLinkInfo *pLinkInfo = &pContext->mLinkInfoArray[idx];
    if(0 == pConfig->mCaptureWidth || 0 == pConfig->mCaptureHeight)
    {
        alogd("do not need destroy link for idx[%d]", idx);
        return SUCCESS;
    }
    /* stop vo channel, vi channel */
    eRet = AW_MPI_VO_StopChn(pLinkInfo->mVoLayer, pLinkInfo->mVOChn);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! VO StopChn fail!");
    }
    eRet = AW_MPI_VI_DisableVirChn(pLinkInfo->mVIDev, pLinkInfo->mVIChn);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! VI DisableVirChn fail!");
    }
    eRet = AW_MPI_VO_DisableChn(pLinkInfo->mVoLayer, pLinkInfo->mVOChn);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! VO DisableChn fail!");
    }
    pLinkInfo->mVOChn = MM_INVALID_CHN;
    /* disable vo layer */
    eRet = AW_MPI_VO_DisableVideoLayer(pLinkInfo->mVoLayer);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! VO DisableChn fail!");
    }
    pLinkInfo->mVoLayer = -1;
    //wait hwdisplay kernel driver processing frame buffer, must guarantee this! Then vdec can free frame buffer.
    usleep(50*1000);

    eRet = AW_MPI_VI_DestroyVirChn(pLinkInfo->mVIDev, pLinkInfo->mVIChn);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! VI DestoryVirChn fail!");
    }
#if ISP_RUN
    eRet = AW_MPI_ISP_Stop(pContext->mIspDev);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! VO DisableChn fail!");
    }

#endif
    eRet = AW_MPI_VI_DisableVipp(pLinkInfo->mVIDev);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! VO DisableChn fail!");
    }
    eRet = AW_MPI_VI_DestoryVipp(pLinkInfo->mVIDev);
    if(eRet != SUCCESS)
    {
        aloge("fatal error! VO DisableChn fail!");
    }
    pLinkInfo->mVIDev = MM_INVALID_DEV;
    pLinkInfo->mVIChn = MM_INVALID_CHN;
    return SUCCESS;
}

static int fillColorToRGBFrame(VIDEO_FRAME_INFO_S *pFrameInfo)
{
    if(pFrameInfo->VFrame.mPixelFormat != MM_PIXEL_FORMAT_RGB_8888)
    {
        aloge("fatal error! pixelFormat[0x%x] is not argb8888!", pFrameInfo->VFrame.mPixelFormat);
        return -1;
    }
    int nFrameBufLen = pFrameInfo->VFrame.mStride[0];
    //memset(pFrameInfo->VFrame.mpVirAddr[0], 0xFF, nFrameBufLen);

    int *add = (int *)pFrameInfo->VFrame.mpVirAddr[0];
    int i;
    for(i=0; i<nFrameBufLen/4; i++)
    { 
        if(i<nFrameBufLen/8)
        {
            *add = 0x5fff0000;
        }
        else
        {
            *add = 0x5f0000ff;
        }
        add++; 
    }
    return 0;
}

static int CreateTestUILayer(SampleVIRVI2VOContext *pContext)
{
    int result = 0;
    //test other ui layer
    ERRORTYPE ret;
    pContext->mTestUILayer = pContext->mConfigPara.mTestUILayer;
    ret = AW_MPI_VO_EnableVideoLayer(pContext->mTestUILayer);
    if(ret!=SUCCESS)
    {
        aloge("fatal error! enable video layer fail");
    }
    AW_MPI_VO_GetVideoLayerAttr(pContext->mTestUILayer, &pContext->mTestUILayerAttr);
    pContext->mTestUILayerAttr.stDispRect.X = 0;
    pContext->mTestUILayerAttr.stDispRect.Y = 0;
    pContext->mTestUILayerAttr.stDispRect.Width = pContext->mConfigPara.mUIDisplayWidth;
    pContext->mTestUILayerAttr.stDispRect.Height = pContext->mConfigPara.mUIDisplayHeight;
    AW_MPI_VO_SetVideoLayerAttr(pContext->mTestUILayer, &pContext->mTestUILayerAttr);

    //create bmp frame buffer.
    unsigned int nPhyAddr = 0;
    void *pVirtAddr = NULL;
    unsigned int nFrameBufLen = 0;
    PIXEL_FORMAT_E ePixelFormat = MM_PIXEL_FORMAT_RGB_8888;
    switch(ePixelFormat)
    {
        case MM_PIXEL_FORMAT_RGB_8888:
            nFrameBufLen = pContext->mConfigPara.mUIDisplayWidth*pContext->mConfigPara.mUIDisplayHeight*4;
            break;
        case MM_PIXEL_FORMAT_RGB_1555:
        case MM_PIXEL_FORMAT_RGB_565:
            nFrameBufLen = pContext->mConfigPara.mUIDisplayWidth*pContext->mConfigPara.mUIDisplayHeight*2;
            break;
        default:
            aloge("fatal error! unsupport pixel format:0x%x", ePixelFormat);
            break;
    }
    ret = AW_MPI_SYS_MmzAlloc_Cached(&nPhyAddr, &pVirtAddr, nFrameBufLen);
    if(ret!=SUCCESS)
    {
        aloge("fatal error! ion malloc fail:0x%x",ret);
        goto _err1;
    }
    pContext->mTestUIFrame.mId = 0x21;
    pContext->mTestUIFrame.VFrame.mWidth = pContext->mConfigPara.mUIDisplayWidth;
    pContext->mTestUIFrame.VFrame.mHeight = pContext->mConfigPara.mUIDisplayHeight;
    pContext->mTestUIFrame.VFrame.mField = VIDEO_FIELD_FRAME;
    pContext->mTestUIFrame.VFrame.mPixelFormat = ePixelFormat;
    pContext->mTestUIFrame.VFrame.mPhyAddr[0] = nPhyAddr;
    pContext->mTestUIFrame.VFrame.mpVirAddr[0] = pVirtAddr;
    pContext->mTestUIFrame.VFrame.mStride[0] = nFrameBufLen;
    pContext->mTestUIFrame.VFrame.mOffsetTop = 0;
    pContext->mTestUIFrame.VFrame.mOffsetBottom = pContext->mTestUIFrame.VFrame.mHeight;
    pContext->mTestUIFrame.VFrame.mOffsetLeft = 0;
    pContext->mTestUIFrame.VFrame.mOffsetRight = pContext->mTestUIFrame.VFrame.mWidth;

    //fill color in frame.
    fillColorToRGBFrame(&pContext->mTestUIFrame);
    //create vo channel
    bool bSuccessFlag = false;
    pContext->mTestVOChn = 0;
    while(pContext->mTestVOChn < VO_MAX_CHN_NUM)
    {
        ret = AW_MPI_VO_EnableChn(pContext->mTestUILayer, pContext->mTestVOChn);
        if(SUCCESS == ret)
        {
            bSuccessFlag = true;
            alogd("create vo channel[%d] success!", pContext->mTestVOChn);
            break;
        }
        else if(ERR_VO_CHN_NOT_DISABLE == ret)
        {
            alogd("vo channel[%d] is exist, find next!", pContext->mTestVOChn);
            pContext->mTestVOChn++;
        }
        else
        {
            aloge("fatal error! create vo channel[%d] ret[0x%x]!", pContext->mTestVOChn, ret);
            break;
        }
    }

    if(false == bSuccessFlag)
    {
        pContext->mTestVOChn = MM_INVALID_CHN;
        aloge("fatal error! create vo channel fail!");
        result = -1;
        goto _err1;
    }
    MPPCallbackInfo cbInfo;
    cbInfo.cookie = (void*)pContext;
    cbInfo.callback = (MPPCallbackFuncType)&SampleVIRVI2VO_VOCallbackWrapper;
    AW_MPI_VO_RegisterCallback(pContext->mTestUILayer, pContext->mTestVOChn, &cbInfo);
    AW_MPI_VO_SetChnDispBufNum(pContext->mTestUILayer, pContext->mTestVOChn, 0);

    ret = AW_MPI_VO_StartChn(pContext->mTestUILayer, pContext->mTestVOChn);
    if(ret != SUCCESS)
    {
        aloge("fatal error! vo start chn fail!");
    }
    ret = AW_MPI_VO_SendFrame(pContext->mTestUILayer, pContext->mTestVOChn, &pContext->mTestUIFrame, 0);
    if(ret != SUCCESS)
    {
        aloge("fatal error! send frame to vo fail!");
    }
    return result;

_err1:
    ret = AW_MPI_SYS_MmzFree(pContext->mTestUIFrame.VFrame.mPhyAddr[0], pContext->mTestUIFrame.VFrame.mpVirAddr[0]);
    if(ret != SUCCESS)
    {
        aloge("fatal error! free ion memory fail!");
    }
_err0:
    ret = AW_MPI_VO_DisableVideoLayer(pContext->mTestUILayer);
    if(ret != SUCCESS)
    {
        aloge("fatal error! disable video layer fail!");
    }
    
    return result;
    
}

static int DestroyTestUILayer(SampleVIRVI2VOContext *pContext)
{
    ERRORTYPE ret;
    ret = AW_MPI_VO_StopChn(pContext->mTestUILayer, pContext->mTestVOChn);
    if(ret != SUCCESS)
    {
        aloge("fatal error! vo stop chn fail!");
    }
    AW_MPI_VO_DisableChn(pContext->mTestUILayer, pContext->mTestVOChn);
    ret = AW_MPI_VO_DisableVideoLayer(pContext->mTestUILayer);
    if(ret != SUCCESS)
    {
        aloge("fatal error! disable video layer fail!");
    }
    //wait hwdisplay kernel driver processing frame buffer, must guarantee this! Then we can free frame buffer.
    usleep(50*1000);
    ret = AW_MPI_SYS_MmzFree(pContext->mTestUIFrame.VFrame.mPhyAddr[0], pContext->mTestUIFrame.VFrame.mpVirAddr[0]);
    if(ret != SUCCESS)
    {
        aloge("fatal error! free ion memory fail!");
    }
    return 0;
}

static void* SampleDetectInputThread(void* pThreadData)
{
    SampleVIRVI2VOContext* pContext = (SampleVIRVI2VOContext*) pThreadData;
    int ret;
    alogd("detect input thread is running!");
    int nOldCancelType = 0;
    ret = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &nOldCancelType);
    if(ret != 0)
    {
        aloge("fatal error! pthread setcancelstate fail[%d]", ret);
    }
    alogd("set cancel type:%d, old:%d", PTHREAD_CANCEL_ASYNCHRONOUS, nOldCancelType);
    prctl(PR_SET_NAME, (unsigned long)"sample_virvi2vo_DetectInput", 0, 0, 0);
    while(1)
    {
        //alogd("before getchar.");
        char ch = getchar();
        alogd("after getchar[%c][0x%x].", ch, ch);
        if ( ch == 'q' || ch == '\03' )
        {
            handle_exit();
            alogd("detect input thread is exit.");
            return 0;
        }
        else
        {
        }
        //alogd("before testcancel");
        pthread_testcancel();
        //alogd("after testcancel");
    }
    alogd("fatal error! detect input thread is exit2.");
    return (void*)0;
}

int main_sample_virvi2vo(int argc __attribute__((__unused__)), char *argv[] __attribute__((__unused__)))
{
    int result = 0;
    void *pValue = NULL;
    //int iIspDev = 0;
    //Test_IspParam_S param = {0};
    #if 0
    FILE *fp = fopen("isp_congfig.txt", "r");
    if (NULL != fp)
    {
        //遍历参数文件
        char text[1024];
        while (fgets(text, 1024, fp))
        {
            //解析参数
            char *chr = strchr(text, '=');
            if (NULL == chr)
                continue;
            
            *chr++ = '\0';
            if (0 == strcmp(text, "ae_mode"))
                param.ae_mode = atoi(chr);
            else if (0 == strcmp(text, "ae_expOffset"))
                param.ae_expOffset = atoi(chr);
            else if (0 == strcmp(text, "ae_exposure"))
                param.ae_exposure = atoi(chr);
            else if (0 == strcmp(text, "gain"))
                param.gain = atoi(chr);
            else if (0 == strcmp(text, "awb_mode"))
                param.awb_mode = atoi(chr);
            else if (0 == strcmp(text, "awb_coltmp"))
                param.awb_coltmp = atoi(chr);
            else if (0 == strcmp(text, "brightness"))
                param.brightness = atoi(chr);
            else if (0 == strcmp(text, "contrast"))
                param.contrast = atoi(chr);
            else if (0 == strcmp(text, "saturation"))
                param.saturation = atoi(chr);
            else if (0 == strcmp(text, "maxexp"))
                param.maxexp = atoi(chr);
            else if (0 == strcmp(text, "minexp"))
                param.minexp = atoi(chr);
            else if (0 == strcmp(text, "hue"))
                param.hue = atoi(chr);
            else if (0 == strcmp(text, "sharpness"))
                param.sharpness = atoi(chr);
        }   
        fclose(fp);
    }
    #endif

    SampleVIRVI2VOContext stContext;

    alogd("sample_virvi2vo running!\n");
    initSampleVIRVI2VOContext(&stContext);
    pSampleVIRVI2VOContext = &stContext;

    /* parse command line param */
    if(ParseCmdLine(argc, argv, &stContext.mCmdLinePara) != 0)
    {
        //aloge("fatal error! command line param is wrong, exit!");
        result = -1;
        goto _exit;
    }
    char *pConfigFilePath;
    if(strlen(stContext.mCmdLinePara.mConfigFilePath) > 0)
    {
        pConfigFilePath = stContext.mCmdLinePara.mConfigFilePath;
    }
    else
    {
        pConfigFilePath = NULL;
    }

    /* parse config file. */
#if 0
    //check folder existence
    char *pDirPath = "/mnt/E";
    struct stat sb;
    int nWaitTotalMs = 0;
    int nWaitItlMs = 100;
    while(1)
    {
        if (stat(pDirPath, &sb) == 0)
        {
            if(S_ISDIR(sb.st_mode))
            {
                break;
            }
        }
        usleep(nWaitItlMs*1000);
        nWaitTotalMs += nWaitItlMs;
    }
    alogd("[%s] is exist, wait [%d]ms\n", pDirPath, nWaitTotalMs);
#endif
    if(loadSampleVIRVI2VOConfig(&stContext.mConfigPara, pConfigFilePath) != SUCCESS)
    {
        aloge("fatal error! no config file or parse conf file fail");
        result = -1;
        goto _exit;
    }

    /* register process function for SIGINT, to exit program. */
    //if (signal(SIGINT, handle_exit) == SIG_ERR)
    //    perror("can't catch SIGSEGV");
    result = pthread_create(&stContext.mDetectInputThreadId, NULL, SampleDetectInputThread, &stContext);
    if (result != 0)
    {
        aloge("fatal error! pthread create fail[0x%x]", result);
        goto _exit;
    }
    alogd("create detect input thread:[0x%lx]", stContext.mDetectInputThreadId);
    alogd("    Please enter 'q' or Ctrl-C to quit top command.");

    /* init mpp system */
    memset(&stContext.mSysConf, 0, sizeof(MPP_SYS_CONF_S));
    stContext.mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stContext.mSysConf);
    result = AW_MPI_SYS_Init();
    if (result) {
        aloge("fatal error! AW_MPI_SYS_Init failed");
        goto sys_init_err;
    }

    stContext.mIspDev = 0;
    /* enable vo dev */
    stContext.mVoDev = 0;
    AW_MPI_VO_Enable(stContext.mVoDev);
    AW_MPI_VO_AddOutsideVideoLayer(stContext.mUILayer);
    AW_MPI_VO_CloseVideoLayer(stContext.mUILayer); /* close ui layer. */
    VO_PUB_ATTR_S spPubAttr;
    AW_MPI_VO_GetPubAttr(stContext.mVoDev, &spPubAttr);
    spPubAttr.enIntfType = stContext.mConfigPara.mDispType;
    spPubAttr.enIntfSync = stContext.mConfigPara.mDispSync;
    AW_MPI_VO_SetPubAttr(stContext.mVoDev, &spPubAttr);

    int i;
    for(i=0;i<VIPP2VO_NUM;i++)
    {
        CreateVIPP2VOLink(i, &stContext);
    }
#if ISP_RUN
    result = AW_MPI_ISP_SwitchIspConfig(stContext.mIspDev, stContext.mConfigPara.mIspWdrSetting);
    if(result != SUCCESS)
    {
        aloge("fatal error! isp switch wdr[%d] fail[%d]", stContext.mConfigPara.mIspWdrSetting, result);
    }
#endif
    if(stContext.mConfigPara.mbAddUILayer)
    {
        CreateTestUILayer(&stContext);
    }

#if 0
    char str[256]  = {0};
    char val[16] = {0};
    int num = 0, vl = 0;

    printf("\033[33m");
    printf("===========ISP test=========\n");
    printf("input <99> & ctrl+c to exit!\n");
    printf("input <1-13> to test\n");
    printf("============================\n");
    printf("\033[0m");                

    RECT_S get_test_rect;
    memset(&get_test_rect, 0, sizeof(RECT_S));

    RECT_S test_rect;
    memset(&test_rect, 0, sizeof(RECT_S));
    while (1)
    {
        print_menu();
        memset(str, 0, sizeof(str));
        memset(val,0,sizeof(val));
        fgets(str, 256, stdin);
        num = atoi(str);
        if (99 == num) {
            printf("break test.\n");
            printf("enter ctrl+c to exit this program.\n");
            break;
        }

        switch (num) {
            case 1://///////
                AW_MPI_ISP_AE_GetMode(stContext.mIspDev,&vl);
                printf("\n---------------AE set mode---the current AE mode = %d\n",vl);
                fgets(val,16,stdin);
                param.ae_mode = atoi(val);
                AW_MPI_ISP_AE_SetMode(stContext.mIspDev,param.ae_mode);//0 ,1---ok
                AW_MPI_ISP_AE_GetMode(stContext.mIspDev,&vl);
                printf("AE mode: after setting  value = %d\r\n", vl);
                break;
            case 2:
                AW_MPI_ISP_AE_GetExposureBias(stContext.mIspDev,&vl);
                printf("\n---------------AE set expOffset---the current AE expOffset bias = %d\n",vl);
                fgets(val,16,stdin);
                param.ae_expOffset = atoi(val);
                AW_MPI_ISP_AE_SetExposureBias(stContext.mIspDev,param.ae_expOffset);//0~8---ok
                AW_MPI_ISP_AE_GetExposureBias(stContext.mIspDev,&vl);
                printf("AE exposure bias: after setting  value = %d\r\n", vl);
                break;
            case 3://////
                AW_MPI_ISP_AE_GetExposure(stContext.mIspDev,&vl);
                printf("\n---------------AE set exposure---the current AE exposure = %d\n",vl);
                fgets(val,16,stdin);
                param.ae_exposure = atoi(val);
                AW_MPI_ISP_AE_SetExposure(stContext.mIspDev,param.ae_exposure);//
                AW_MPI_ISP_AE_GetExposure(stContext.mIspDev,&vl);//
                printf("AE exposure: after setting value = %d\r\n", vl);
                break;
            case 4://///////
                AW_MPI_ISP_AE_GetGain(stContext.mIspDev,&vl);
                printf("\n---------------AE set gain---the current AE gain  = %d\n",vl);
                fgets(val,16,stdin);
                param.gain = atoi(val);
                AW_MPI_ISP_AE_SetGain(stContext.mIspDev,param.gain );//256
                AW_MPI_ISP_AE_GetGain(stContext.mIspDev,&vl);//256
                printf("AE gain: after setting value = %d\r\n", vl);
                break;
            case 5:
                AW_MPI_ISP_AWB_GetMode(stContext.mIspDev,&vl);
                printf("\n---------------AWB  mode---the current AWB mode  = %d\n",vl);
                fgets(val,16,stdin);
                param.awb_mode = atoi(val);
                AW_MPI_ISP_AWB_SetMode(stContext.mIspDev,param.awb_mode);//0,1---ok
                AW_MPI_ISP_AWB_GetMode(stContext.mIspDev,&vl);//0,1
                printf("AWB mode: after setting value = %d\r\n", vl);
                break;
            case 6:
                AW_MPI_ISP_AWB_GetColorTemp(stContext.mIspDev,&vl);
                printf("\n---------------AWB  clolor temperature---the current AWB clolor temperature  = %d\n",vl);
                fgets(val,16,stdin);
                param.awb_coltmp = atoi(val);
                AW_MPI_ISP_AWB_SetColorTemp(stContext.mIspDev,param.awb_coltmp);//0,1,2,3,4,5,6,7, ---ok
                AW_MPI_ISP_AWB_GetColorTemp(stContext.mIspDev,&vl);//0,1,2,3,4,5,6,7,
                printf("AWB clolor temperature: after setting value = %d\r\n", vl);
                break;
#if 0
            case 7:
                AW_MPI_ISP_GetAeMinMaxExp(iIspDev, &param.minexp, &param.maxexp);
                printf("\n---------------AE Max Exp---the current AE Max Exp  = %d\n",param.maxexp);
                fgets(val,16,stdin);
                param.maxexp = atoi(val);
                AW_MPI_ISP_SetAeMinMaxExp(iIspDev, param.minexp, param.maxexp);//
                AW_MPI_ISP_GetAeMinMaxExp(iIspDev, &param.minexp, &param.maxexp);//
                printf("AeMaxExp: after setting MAx value = %d.\r\n", param.maxexp);
                break;
            case 8:
                AW_MPI_ISP_GetAeMinMaxExp(iIspDev, &param.minexp, &param.maxexp);
                printf("\n---------------AE Min Exp---the current AE Min Exp  = %d\n",param.minexp);
                fgets(val,16,stdin);
                param.minexp = atoi(val);
                AW_MPI_ISP_SetAeMinMaxExp(iIspDev, param.minexp, param.maxexp);//
                AW_MPI_ISP_GetAeMinMaxExp(iIspDev, &param.minexp, &param.maxexp);//
                printf("AeMinExp: after setting MIN value = %d.\r\n", param.minexp);
                break;
            case 9:
                AW_MPI_ISP_GetHue(iIspDev,&vl);
                printf("\n---------------HUE  VALUE---the current HUE  VALUE  = %d\n",vl);
                fgets(val,16,stdin);
                param.hue = atoi(val);
                AW_MPI_ISP_SetHue(iIspDev,param.hue);//
                AW_MPI_ISP_GetHue(iIspDev,&vl);//
                printf("HUE: after setting value = %d\r\n", vl);
                break;
#endif
            case 10:
                AW_MPI_ISP_GetBrightness(stContext.mIspDev,&vl);
                printf("\n---------------brightness  VALUE---the current brightness  value  = %d\n",vl);
                fgets(val,16,stdin);
                param.brightness = atoi(val);
                AW_MPI_ISP_SetBrightness(stContext.mIspDev,param.brightness);//0~256
                AW_MPI_ISP_GetBrightness(stContext.mIspDev,&vl);//0~256
                printf("brightness: after setting value = %d\r\n", vl);
                break;
            case 11:
                AW_MPI_ISP_GetContrast(stContext.mIspDev,&vl);
                printf("\n---------------contrast  VALUE---the current contrast  value  = %d\n",vl);
                fgets(val,16,stdin);
                param.contrast = atoi(val);
                AW_MPI_ISP_SetContrast(stContext.mIspDev,param.contrast);//0~256
                AW_MPI_ISP_GetContrast(stContext.mIspDev,&vl);//0~256
                printf("contrast: after setting value = %d\r\n", vl);
                break;
            case 12:
                AW_MPI_ISP_GetSaturation(stContext.mIspDev,&vl);
                printf("\n---------------saturation  VALUE---the current saturation  value  = %d\n",vl);
                fgets(val,16,stdin);
                param.saturation = atoi(val);
                AW_MPI_ISP_SetSaturation(stContext.mIspDev,param.saturation);//0~256
                AW_MPI_ISP_GetSaturation(stContext.mIspDev,&vl);//0~256
                printf("saturation: after setting value = %d\r\n", vl);
                break;
            case 13:
                AW_MPI_ISP_GetSharpness(stContext.mIspDev,&vl);
                printf("\n---------------sharpness  VALUE---the current sharpness  value  = %d\n",vl);
                fgets(val,16,stdin);
                param.sharpness = atoi(val);
                AW_MPI_ISP_SetSharpness(stContext.mIspDev,param.sharpness);//0~256
                AW_MPI_ISP_GetSharpness(stContext.mIspDev,&vl);//0~256
                printf("sharpness: after setting value = %d\r\n", vl);
                break;
            case 14:
                AW_MPI_VO_GetFrameDisplayRegion(stContext.mLinkInfoArray[0].mVoLayer, stContext.mLinkInfoArray[0].mVOChn,&get_test_rect);
                alogd("\n-get-X:%d,Y:%d,W:%d,H:%d-\n",get_test_rect.X,get_test_rect.Y,get_test_rect.Width,get_test_rect.Height);
                alogd("\n\n***Please enter the display region,[X,Y,W,H]****:\n");
                scanf("%d,%d,%d,%d",&test_rect.X,&test_rect.Y,&test_rect.Width,&test_rect.Height);
                alogd("\n\nX:%d  Y:%d  W:%d  H:%d\n\n",test_rect.X, test_rect.Y,test_rect.Width,test_rect.Height);
                AW_MPI_VO_SetFrameDisplayRegion(stContext.mLinkInfoArray[0].mVoLayer,stContext.mLinkInfoArray[0].mVOChn,&test_rect);
                break;
            default:
                printf("intput error.\r\n");
                break;
        }
    }

#endif

    if(stContext.mConfigPara.mTestDuration > 0)
    {
        cdx_sem_down_timedwait(&stContext.mSemExit, stContext.mConfigPara.mTestDuration*1000);
    }
    else
    {
        cdx_sem_down(&stContext.mSemExit);
    }

//    result = AW_MPI_ISP_SetSaveCTX(stContext.mIspDev);
//    if(result != 0)
//    {
//        aloge("fatal error! isp[%d] set save ctx wrong[%d]!", stContext.mIspDev, result);
//    }
//    else
//    {
//        aloge("isp[%d] setSaveCtx success!", stContext.mIspDev);
//    }

    for(i=0;i<VIPP2VO_NUM;i++)
    {
        DestroyVIPP2VOLink(i, &stContext);
    }
    if(stContext.mConfigPara.mbAddUILayer)
    {
        DestroyTestUILayer(&stContext);
    }

    AW_MPI_VO_CloseVideoLayer(stContext.mUILayer);
    AW_MPI_VO_RemoveOutsideVideoLayer(stContext.mUILayer);
    /* disalbe vo dev */
    AW_MPI_VO_Disable(stContext.mVoDev);
    stContext.mVoDev = -1;

    /* exit mpp system */
    AW_MPI_SYS_Exit();
sys_init_err:
    result = pthread_cancel(stContext.mDetectInputThreadId);
    alogd("post cancel signal to pthread[0x%lx] join, ret[0x%x]", stContext.mDetectInputThreadId, result);
    result = pthread_join(stContext.mDetectInputThreadId, &pValue);
    alogd("pthread[0x%lx] join, ret[0x%x], pValue[%p]", stContext.mDetectInputThreadId, result, pValue);
_exit:
    destroySampleVIRVI2VOContext(&stContext);
    if (result == 0) {
        alogd("sample_virvi2vo exit!\n");
    }
    return result;
}

FINSH_FUNCTION_EXPORT_ALIAS(main_sample_virvi2vo, __cmd_sample_virvi2vo, test mpp virvi2vo);
