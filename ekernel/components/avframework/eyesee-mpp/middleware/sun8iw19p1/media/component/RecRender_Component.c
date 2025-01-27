/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/

//#include <CDX_LogNDebug.h>
//#define LOG_NDEBUG 0
#define LOG_TAG "RecRender_Component"
#include <utils/plat_log.h>

#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <errno.h>

#include <tmessage.h>
#include <tsemaphore.h>

//#include <type.h>
//#include <H264encLibApi.h>
#include <vencoder.h>
//#include <mp4_mux_lib.h>
//#include <cedarv_osal_linux.h>
//#include <type_camera.h>
#include <record_writer.h>
//#include <include_system/cedarx_avs_counter.h>
//#include <DataQuene.h>
#include <FsWriter.h>
#include "RecRenderSink.h"
#include "RecRender_cache.h"

#include <SystemBase.h>
//#include <aenc_sw_lib.h>
#include <cdx_list.h>

#include "RecRender_Component.h"
#include <EncodedStream.h>
#include <VencCompStream.h>
#include <AencCompStream.h>
#include <media_common.h>


//#define __SAVE_INPUT_BS__
#ifdef __SAVE_INPUT_BS__
static FILE *fp_bs;
#endif


static void* RecRender_ComponentThread(void* pThreadData);
extern ERRORTYPE RecRender_ReleaseBuffer(PARAM_IN RECRENDERDATATYPE *pRecRenderData, PARAM_IN RecSinkPacket* pRSPacket);

ERRORTYPE copy_MUX_GRP_ATTR_S(MUX_GRP_ATTR_S *pDst, MUX_GRP_ATTR_S *pSrc)
{
    memcpy(pDst, pSrc, sizeof(MUX_GRP_ATTR_S));
    return SUCCESS;
}

MEDIA_FILE_FORMAT_E map_MUXERMODES_to_MEDIA_FILE_FORMAT_E(MUXERMODES nMuxerMode)
{
    MEDIA_FILE_FORMAT_E dstFormat;
    switch(nMuxerMode)
    {
        case MUXER_MODE_MP4:
            dstFormat = MEDIA_FILE_FORMAT_MP4;
            break;
        case MUXER_MODE_MP3:
            dstFormat = MEDIA_FILE_FORMAT_MP3;
            break;
        case MUXER_MODE_AAC:
            dstFormat = MEDIA_FILE_FORMAT_AAC;
            break;
    	case MUXER_MODE_RAW:
            dstFormat = MEDIA_FILE_FORMAT_RAW;
            break;
    	case MUXER_MODE_TS:
            dstFormat = MEDIA_FILE_FORMAT_TS;
            break;
        default:
            dstFormat = MEDIA_FILE_FORMAT_UNKNOWN;
            break;
    }
    return dstFormat;
}

MUXERMODES map_MEDIA_FILE_FORMAT_E_to_MUXERMODES(MEDIA_FILE_FORMAT_E nMuxFileFormat)
{
    MUXERMODES dstFormat;
    switch(nMuxFileFormat)
    {
        case MEDIA_FILE_FORMAT_MP4:
            dstFormat = MUXER_MODE_MP4;
            break;
        case MEDIA_FILE_FORMAT_MP3:
            dstFormat = MUXER_MODE_MP3;
            break;
        case MEDIA_FILE_FORMAT_AAC:
            dstFormat = MUXER_MODE_AAC;
            break;
    	case MEDIA_FILE_FORMAT_RAW:
            dstFormat = MUXER_MODE_RAW;
            break;
    	case MEDIA_FILE_FORMAT_TS:
            dstFormat = MUXER_MODE_TS;
            break;
        default:
            dstFormat = MUXER_MODE_MP4;
            break;
    }
    return dstFormat;
}

ERRORTYPE config_MUX_CHN_ATTR_S_by_RecSink(MUX_CHN_ATTR_S *pDst, RecSink *pSrc)
{
    pDst->mMediaFileFormat = map_MUXERMODES_to_MEDIA_FILE_FORMAT_E(pSrc->nMuxerMode);
    pDst->mMaxFileDuration = pSrc->mMaxFileDuration;
    pDst->mFallocateLen = pSrc->nFallocateLen;
    pDst->mCallbackOutFlag = pSrc->nCallbackOutFlag;
    return SUCCESS;
}

ERRORTYPE configCdxOutputSinkInfo(CdxOutputSinkInfo *pDst, MUX_CHN_ATTR_S *pChnAttr, int nFd)
{
    pDst->mMuxerId = pChnAttr->mMuxerId;
    pDst->nMuxerMode = map_MEDIA_FILE_FORMAT_E_to_MUXERMODES(pChnAttr->mMediaFileFormat);
    pDst->nOutputFd = nFd;
    pDst->nFallocateLen = pChnAttr->mFallocateLen;
    pDst->nCallbackOutFlag = pChnAttr->mCallbackOutFlag;
    pDst->bBufFromCacheFlag = pChnAttr->bBufFromCacheFlag;
    return SUCCESS;
}

ERRORTYPE RecSinkCB_EventHandler(
     PARAM_IN COMP_HANDLETYPE hComponent,
     PARAM_IN void* pAppData,
     PARAM_IN COMP_EVENTTYPE eEvent,
     PARAM_IN unsigned int nData1,
     PARAM_IN unsigned int nData2,
     PARAM_IN void* pEventData){

    alogv("event[%d]");
    ERRORTYPE   eError = SUCCESS;
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE*)pAppData;
    //int data;
    switch (eEvent)
    {
        case COMP_EventError:
        {
            ERRORTYPE eError = (ERRORTYPE)nData1;
            if(ERR_MUX_SAMESTATE == eError)
            {
                aloge("eventError, RecSink sameState");
            }
            else if(ERR_MUX_INVALIDSTATE == eError)
            {
                aloge("eventError, RecSink invalidState");
            }
            else if(ERR_MUX_INCORRECT_STATE_OPERATION == eError)
            {
                aloge("eventError, RecSink incorrect state operation");
            }
            else if(ERR_MUX_INCORRECT_STATE_TRANSITION == eError)
            {
                aloge("eventError, RecSink incorrect state transition");
            }
            else if(ERR_MUX_NOT_PERM == eError)
            {
                aloge("meet eventUndefinedError, RecSink is turn to invalidState[%d]", nData2);
            }
            break;
        }
        case COMP_EventCmdComplete:
        {
            if (COMP_CommandStateSet == nData1)
            {
                alogv("eventCmdComplete, RecSink in state[%d]", (COMP_STATETYPE)nData2);
            }
            else if(SwitchFile == nData1)
            {
                ERRORTYPE switchRet = (ERRORTYPE)pEventData;
                BOOL bCacheFlag = pRecRenderData->cache_manager?TRUE:FALSE;
                alogd("recSink MuxerId[%d] cacheFlag[%d] switch file ret[0x%x] done", (int)nData2, bCacheFlag, switchRet);
                if(bCacheFlag && switchRet==SUCCESS)
                {
                    message_t msg;
                    msg.command = SwitchFileDone;
                    msg.para0 = nData2;
                    put_message(&pRecRenderData->cmd_queue, &msg);
                }
            }
            else if(SwitchFileNormal == nData1)
            {
                ERRORTYPE switchRet = (ERRORTYPE)pEventData;
                alogd("recSink MuxerId[%d] switch file normal ret[0x%x], begin to switch!", (int)nData2, switchRet);
            }
            break;
        }
        case COMP_EventRecordDone:
        {
            pRecRenderData->pCallbacks->EventHandler(pRecRenderData->hSelf, pRecRenderData->pAppData, COMP_EventRecordDone, nData1, 0, NULL);
            break;
        }
        case COMP_EventNeedNextFd:
        {
            pRecRenderData->pCallbacks->EventHandler(pRecRenderData->hSelf, pRecRenderData->pAppData, COMP_EventNeedNextFd, nData1, 0, NULL);
            break;
        }
        case COMP_EventWriteDiskError:
        {
            pRecRenderData->pCallbacks->EventHandler(pRecRenderData->hSelf, pRecRenderData->pAppData, COMP_EventWriteDiskError, nData1, 0, NULL);
            break;
        }
        default:
        {
            aloge("fatal error! unknown event[%d]", eEvent);
            eError = ERR_MUX_NOT_SUPPORT;
            break;
        }
    }

    return eError;
}

ERRORTYPE RecSinkCB_EmptyBufferDone(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN void* pAppData,
        PARAM_IN RecSinkPacket* pRSPacket)
{
    RECRENDERDATATYPE   *pRecRenderData = (RECRENDERDATATYPE*)pAppData;
    if(SOURCE_TYPE_COMPONENT == pRSPacket->mSourceType)
    {
        RecRender_ReleaseBuffer(pRecRenderData, pRSPacket);
    }
    else if(SOURCE_TYPE_CACHEMANAGER == pRSPacket->mSourceType)
    {
        pRecRenderData->cache_manager->ReleasePacket(pRecRenderData->cache_manager, pRSPacket->mId);
    }
    else
    {
        aloge("fatal error! wrong RSPacket type[%d]", pRSPacket->mSourceType);
        return ERR_MUX_NOT_SUPPORT;
    }
    return SUCCESS;
}

RecSinkCallbackType RecSinkCallback = {
        .EventHandler = RecSinkCB_EventHandler,
        .EmptyBufferDone = RecSinkCB_EmptyBufferDone,
};

static int CDXRecoder_WritePacket_CB(void *parent, CDXRecorderBsInfo *bs_info)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE*)parent;
    return pRecRenderData->pCallbacks->EventHandler(
                            pRecRenderData->hSelf,
                            pRecRenderData->pAppData,
                            COMP_EventBsframeAvailable,
                            0,
                            0,
                            bs_info);
}

ERRORTYPE setRecSinkPacketByVEncCompOutputBuffer(PARAM_IN RECRENDERDATATYPE *pRecRenderData, PARAM_OUT RecSinkPacket* pRSPacket, PARAM_IN EncodedStream* pEncodedStream)
{
    if(pRecRenderData->mnBasePts < 0)
    {
        pRecRenderData->mnBasePts = pEncodedStream->nTimeStamp;
    }

    int iSize0 = 0;
    int iSize1 = 0;
    if (pEncodedStream->nFilledLen > 0)
    {
        if (pEncodedStream->nFilledLen <= pEncodedStream->nBufferLen)
        {
            iSize0 = pEncodedStream->nFilledLen;
            iSize1 = 0;
        }
        else
        {
            iSize0 = pEncodedStream->nBufferLen;
            iSize1 = pEncodedStream->nFilledLen - pEncodedStream->nBufferLen;
        }
    }

    pRSPacket->mId = pEncodedStream->nID;
    pRSPacket->mStreamType = CODEC_TYPE_VIDEO;
    pRSPacket->mFlags = 0;
    pRSPacket->mFlags |= (pEncodedStream->nFlags & CEDARV_FLAG_KEYFRAME)? AVPACKET_FLAG_KEYFRAME : 0;
    pRSPacket->mPts = pEncodedStream->nTimeStamp - pRecRenderData->mnBasePts;
    if(iSize0 > 0)
    {
        pRSPacket->mpData0 = (char*)pEncodedStream->pBuffer;
    }
    else
    {
        pRSPacket->mpData0 = NULL;
    }
    pRSPacket->mSize0 = iSize0;
    if(iSize1 > 0)
    {
        pRSPacket->mpData1 = (char*)pEncodedStream->pBufferExtra;
    }
    else
    {
        pRSPacket->mpData1 = NULL;
    }
    pRSPacket->mSize1 = iSize1;
    pRSPacket->mCurrQp = pEncodedStream->video_frame_info.CurrQp;
    pRSPacket->mavQp = pEncodedStream->video_frame_info.avQp;
    pRSPacket->mnGopIndex = pEncodedStream->video_frame_info.nGopIndex;
    pRSPacket->mnFrameIndex = pEncodedStream->video_frame_info.nFrameIndex;
    pRSPacket->mnTotalIndex = pEncodedStream->video_frame_info.nTotalIndex;
    pRSPacket->mSourceType = SOURCE_TYPE_COMPONENT;
    pRSPacket->mRefCnt = 0;

    return SUCCESS;
}

ERRORTYPE setRecSinkPacketByAEncCompOutputBuffer(PARAM_IN RECRENDERDATATYPE *pRecRenderData, PARAM_OUT RecSinkPacket* pRSPacket, PARAM_IN EncodedStream* pEncodedStream)
{
    if(pRecRenderData->mnAudioBasePts < 0)
    {
        pRecRenderData->mnAudioBasePts = pEncodedStream->nTimeStamp;
    }
    pRSPacket->mId = pEncodedStream->nID;
    pRSPacket->mStreamType = CODEC_TYPE_AUDIO;
    pRSPacket->mFlags = 0;
    pRSPacket->mPts = pEncodedStream->nTimeStamp - pRecRenderData->mnAudioBasePts;
    pRSPacket->mpData0 = (char*)pEncodedStream->pBuffer;
    pRSPacket->mSize0 = pEncodedStream->nFilledLen;
    pRSPacket->mpData1 = NULL;
    pRSPacket->mSize1 = 0;
    pRSPacket->mCurrQp = 0;
    pRSPacket->mavQp = 0;
    pRSPacket->mnGopIndex = 0;
    pRSPacket->mnFrameIndex = 0;
    pRSPacket->mnTotalIndex = 0;
    pRSPacket->mSourceType = SOURCE_TYPE_COMPONENT;
    pRSPacket->mRefCnt = 0;
    return SUCCESS;
}

ERRORTYPE setRecSinkPacketByTEncCompOutputBuffer(PARAM_IN RECRENDERDATATYPE *pRecRenderData, PARAM_OUT RecSinkPacket* pRSPacket, PARAM_IN EncodedStream* pEncoderStream)
{
    if(pRecRenderData->mnTextBasePts < 0)
    {
        pRecRenderData->mnTextBasePts = pEncoderStream->nTimeStamp;
    }
    pRSPacket->mId = pEncoderStream->nID;
    pRSPacket->mStreamType = CODEC_TYPE_TEXT;
    pRSPacket->mFlags = 0;
    pRSPacket->mPts = pEncoderStream->nTimeStamp - pRecRenderData->mnTextBasePts;
    pRSPacket->mpData0 = (char*)pEncoderStream->pBuffer;
    pRSPacket->mSize0 = (int)pEncoderStream->nBufferLen;
    pRSPacket->mpData1 = NULL;
    pRSPacket->mSize1 = 0;
    pRSPacket->mCurrQp = 0;
    pRSPacket->mavQp = 0;
    pRSPacket->mnGopIndex = 0;
    pRSPacket->mnFrameIndex = 0;
    pRSPacket->mnTotalIndex = 0;
    pRSPacket->mSourceType = SOURCE_TYPE_COMPONENT;
    pRSPacket->mRefCnt = 0;
    return SUCCESS;
}

ERRORTYPE RecRender_GetVideoBuffer(
        PARAM_IN RECRENDERDATATYPE *pRecRenderData,
        PARAM_OUT RecSinkPacket* pRSPacket)
{
    ERRORTYPE eError;
    pthread_mutex_lock(&pRecRenderData->mVideoInputFrameListMutex);
    if (!list_empty(&pRecRenderData->mVideoInputFrameReadyList))
    {
        ENCODER_NODE_T *pEntry = list_first_entry(&pRecRenderData->mVideoInputFrameReadyList, ENCODER_NODE_T, mList);
        setRecSinkPacketByVEncCompOutputBuffer(pRecRenderData, pRSPacket, &pEntry->stEncodedStream);
        pEntry->mUsedRefCnt = 1;
        list_move_tail(&pEntry->mList, &pRecRenderData->mVideoInputFrameUsedList);
        eError = SUCCESS;
    }
    else
    {
        eError = ERR_MUX_NOMEM;
    }
    pthread_mutex_unlock(&pRecRenderData->mVideoInputFrameListMutex);
    return eError;
}

ERRORTYPE RecRender_GetAudioBuffer(
        PARAM_IN RECRENDERDATATYPE *pRecRenderData,
        PARAM_INOUT RecSinkPacket* pRSPacket)

{
    ERRORTYPE eError;
    pthread_mutex_lock(&pRecRenderData->mAudioInputFrameListMutex);
    if (!list_empty(&pRecRenderData->mAudioInputFrameReadyList))
    {
        ENCODER_NODE_T *pEntry = list_first_entry(&pRecRenderData->mAudioInputFrameReadyList, ENCODER_NODE_T, mList);
        setRecSinkPacketByAEncCompOutputBuffer(pRecRenderData, pRSPacket, &pEntry->stEncodedStream);
        pEntry->mUsedRefCnt = 1;
        list_move_tail(&pEntry->mList, &pRecRenderData->mAudioInputFrameUsedList);
        eError = SUCCESS;
    }
    else
    {
        eError = ERR_MUX_NOMEM;
    }
    pthread_mutex_unlock(&pRecRenderData->mAudioInputFrameListMutex);
    return eError;
}
ERRORTYPE RecRender_GetTextBuffer(
        PARAM_IN RECRENDERDATATYPE *pRecRenderData,
        PARAM_INOUT RecSinkPacket* pRSPacket)
{
    ERRORTYPE eError;
    pthread_mutex_lock(&pRecRenderData->mTextInputFrameListMutex);
    if(!list_empty(&pRecRenderData->mTextInputFrameReadyList))
    {
        ENCODER_NODE_T *pEntry = list_first_entry(&pRecRenderData->mTextInputFrameReadyList, ENCODER_NODE_T, mList);
        setRecSinkPacketByTEncCompOutputBuffer(pRecRenderData, pRSPacket, &pEntry->stEncodedStream);
        pEntry->mUsedRefCnt = 1;
        list_move_tail(&pEntry->mList, &pRecRenderData->mTextInputFrameUsedList);
        eError = SUCCESS;
    }
    else
    {
        eError = ERR_MUX_NOMEM;
    }
    pthread_mutex_unlock(&pRecRenderData->mTextInputFrameListMutex);
    return eError;
}

ERRORTYPE RecRender_RefBuffer(
        PARAM_IN RECRENDERDATATYPE *pRecRenderData,
        PARAM_IN RecSinkPacket* pRSPacket)
{
    ERRORTYPE eError = SUCCESS;
    if(CODEC_TYPE_VIDEO == pRSPacket->mStreamType)
    {
        pthread_mutex_lock(&pRecRenderData->mVideoInputFrameListMutex);
        ENCODER_NODE_T *pEntry;
        BOOL bFindFlag = FALSE;
        list_for_each_entry(pEntry, &pRecRenderData->mVideoInputFrameUsedList, mList)
        {
            if(pEntry->stEncodedStream.nID == pRSPacket->mId)
            {
                bFindFlag = TRUE;
                break;
            }
        }
        if (bFindFlag)
        {
            pEntry->mUsedRefCnt++;
        }
        else
        {
            aloge("fatal error! not find AFrmId[%d] in used list.", pRSPacket->mId);
            eError = ERR_MUX_UNEXIST;
        }
        pthread_mutex_unlock(&pRecRenderData->mVideoInputFrameListMutex);
    }
    else if(CODEC_TYPE_AUDIO == pRSPacket->mStreamType)
    {
        pthread_mutex_lock(&pRecRenderData->mAudioInputFrameListMutex);
        ENCODER_NODE_T *pEntry;
        BOOL bFindFlag = FALSE;
        list_for_each_entry(pEntry, &pRecRenderData->mAudioInputFrameUsedList, mList)
        {
            if(pEntry->stEncodedStream.nID == pRSPacket->mId)
            {
                bFindFlag = TRUE;
                break;
            }
        }
        if(bFindFlag)
        {
            pEntry->mUsedRefCnt++;
        }
        else
        {
            aloge("fatal error! not find AFrmId[%d] in used list.", pRSPacket->mId);
            eError = ERR_MUX_UNEXIST;
        }
        pthread_mutex_unlock(&pRecRenderData->mAudioInputFrameListMutex);
    }
    else if(CODEC_TYPE_TEXT == pRSPacket->mStreamType)
    {
        pthread_mutex_lock(&pRecRenderData->mTextInputFrameListMutex);
        ENCODER_NODE_T *pEntry;
        BOOL bFindFlag = FALSE;
        list_for_each_entry(pEntry, &pRecRenderData->mTextInputFrameUsedList, mList)
        {
            if(pEntry->stEncodedStream.nID == pRSPacket->mId)
            {
                bFindFlag = TRUE;
                break;
            }
        }
        if(bFindFlag)
        {
            pEntry->mUsedRefCnt++;
        }
        else
        {
            aloge("fatal error! not find TFrmId[%d] in used list.", pRSPacket->mId);
            eError = ERR_MUX_UNEXIST;
        }
        pthread_mutex_unlock(&pRecRenderData->mTextInputFrameListMutex);
    }
    else
    {
        aloge("fatal error! invalid streamType[%d]", pRSPacket->mStreamType);
        eError = ERR_MUX_UNEXIST;
    }
    return eError;
}
ERRORTYPE RecRender_ReleaseBuffer(
        PARAM_IN RECRENDERDATATYPE *pRecRenderData,
        PARAM_IN RecSinkPacket* pRSPacket)
{
    ERRORTYPE eRet = SUCCESS;
    if (CODEC_TYPE_VIDEO == pRSPacket->mStreamType)
    {
        pthread_mutex_lock(&pRecRenderData->mVideoInputFrameListMutex);
        ENCODER_NODE_T *pEntry;
        BOOL bFindFlag = FALSE;
        list_for_each_entry(pEntry, &pRecRenderData->mVideoInputFrameUsedList, mList)
        {
            if(pEntry->stEncodedStream.nID == pRSPacket->mId)
            {
                bFindFlag = TRUE;
                break;
            }
        }
        if(bFindFlag)
        {
            pEntry->mUsedRefCnt--;
            if (pEntry->mUsedRefCnt == 0)
            {
                list_del(&pEntry->mList);
                //list_move_tail(&pEntry->mList, &pRecRenderData->mVideoInputFrameIdleList);
                pthread_mutex_unlock(&pRecRenderData->mVideoInputFrameListMutex);
                MM_COMPONENTTYPE *pInPortTunnelComp = (MM_COMPONENTTYPE*)(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].hTunnel);
                COMP_BUFFERHEADERTYPE obh;
                obh.nOutputPortIndex   = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].nTunnelPortIndex;
                obh.nInputPortIndex    = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].nPortIndex;
                obh.pOutputPortPrivate = (void*)pEntry;
                eRet = pInPortTunnelComp->FillThisBuffer(pInPortTunnelComp, &obh);
                if(eRet != SUCCESS)
                {
                    aloge("fatal error! fill this buffer fail[0x%x], video frame id=[%d], check code!", eRet, pEntry->stEncodedStream.nID);
                    pthread_mutex_lock(&pRecRenderData->mVideoInputFrameListMutex);
                    list_add_tail(&pEntry->mList, &pRecRenderData->mVideoInputFrameUsedList);
                    //list_move_tail(&pEntry->mList, &pRecRenderData->mVideoInputFrameUsedList);
                    pthread_mutex_unlock(&pRecRenderData->mVideoInputFrameListMutex);
                }
                else
                {
                    pthread_mutex_lock(&pRecRenderData->mVideoInputFrameListMutex);
                    list_add_tail(&pEntry->mList, &pRecRenderData->mVideoInputFrameIdleList);
                    pthread_mutex_unlock(&pRecRenderData->mVideoInputFrameListMutex);
                }
            }
            else
            {
                if(pEntry->mUsedRefCnt < 0)
                {
                    aloge("fatal error! usedRefCnt[%d]<0, check code!", pEntry->mUsedRefCnt);
                    eRet = ERR_MUX_NOT_PERM;
                }
                pthread_mutex_unlock(&pRecRenderData->mVideoInputFrameListMutex);
            }
        }
        else
        {
            aloge("fatal error! not find VFrmId[%d] in used list.", pRSPacket->mId);
            eRet = ERR_MUX_UNEXIST;
            pthread_mutex_unlock(&pRecRenderData->mVideoInputFrameListMutex);
        }
    }
    else if(CODEC_TYPE_AUDIO == pRSPacket->mStreamType)
    {
        pthread_mutex_lock(&pRecRenderData->mAudioInputFrameListMutex);
        ENCODER_NODE_T *pEntry;
        BOOL bFindFlag = FALSE;
        list_for_each_entry(pEntry, &pRecRenderData->mAudioInputFrameUsedList, mList)
        {
            if(pEntry->stEncodedStream.nID == pRSPacket->mId)
            {
                bFindFlag = TRUE;
                break;
            }
        }

        if(bFindFlag)
        {
            pEntry->mUsedRefCnt--;
            if(pEntry->mUsedRefCnt == 0)
            {
                list_del(&pEntry->mList);
                //list_move_tail(&pEntry->mList, &pRecRenderData->mAudioInputFrameIdleList);
                pthread_mutex_unlock(&pRecRenderData->mAudioInputFrameListMutex);
                MM_COMPONENTTYPE *pInPortTunnelComp = (MM_COMPONENTTYPE*)(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].hTunnel);
                COMP_BUFFERHEADERTYPE obh;
                obh.nOutputPortIndex   = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].nTunnelPortIndex;
                obh.nInputPortIndex    = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].nPortIndex;
                obh.pOutputPortPrivate = (void*)pEntry;
                eRet = pInPortTunnelComp->FillThisBuffer(pInPortTunnelComp, &obh);
                if (eRet != SUCCESS)
                {
                    aloge("fatal error! fill this buffer fail[0x%x], audio frame id=[%d], check code!", eRet, pEntry->stEncodedStream.nID);
                    pthread_mutex_lock(&pRecRenderData->mAudioInputFrameListMutex);
                    list_add_tail(&pEntry->mList, &pRecRenderData->mAudioInputFrameUsedList);
                    //list_move_tail(&pEntry->mList, &pRecRenderData->mAudioInputFrameUsedList);
                    pthread_mutex_unlock(&pRecRenderData->mAudioInputFrameListMutex);
                }
                else
                {
                    pthread_mutex_lock(&pRecRenderData->mAudioInputFrameListMutex);
                    list_add_tail(&pEntry->mList, &pRecRenderData->mAudioInputFrameIdleList);
                    pthread_mutex_unlock(&pRecRenderData->mAudioInputFrameListMutex);
                }
            }
            else
            {
                if(pEntry->mUsedRefCnt < 0)
                {
                    aloge("fatal error! usedRefCnt[%d]<0, check code!", pEntry->mUsedRefCnt);
                    eRet = ERR_MUX_NOT_PERM;
                }
                pthread_mutex_unlock(&pRecRenderData->mAudioInputFrameListMutex);
            }
        }
        else
        {
            aloge("fatal error! not find AFrmId[%d] in used list.", pRSPacket->mId);
            eRet = ERR_MUX_UNEXIST;
            pthread_mutex_unlock(&pRecRenderData->mAudioInputFrameListMutex);
        }
    }
    else if(CODEC_TYPE_TEXT == pRSPacket->mStreamType)
    {
        pthread_mutex_lock(&pRecRenderData->mTextInputFrameListMutex);
        ENCODER_NODE_T *pEntry;
        BOOL bFindFlag = FALSE;
        list_for_each_entry(pEntry, &pRecRenderData->mTextInputFrameUsedList, mList)
        {
            if(pEntry->stEncodedStream.nID == pRSPacket->mId)
            {
                bFindFlag = TRUE;
                break;
            }
        }
        if(bFindFlag)
        {
            pEntry->mUsedRefCnt--;
            if(pEntry->mUsedRefCnt == 0)
            {
                list_del(&pEntry->mList);
                //list_move_tail(&pEntry->mList, &pRecRenderData->mTextInputFrameIdleList);
                pthread_mutex_unlock(&pRecRenderData->mTextInputFrameListMutex);

                MM_COMPONENTTYPE *pInPortTunnelComp = (MM_COMPONENTTYPE*)(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].hTunnel);
                COMP_BUFFERHEADERTYPE obh;
                obh.nOutputPortIndex   = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].nTunnelPortIndex;
                obh.nInputPortIndex    = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].nPortIndex;
                obh.pOutputPortPrivate = (void*)pEntry;
                eRet = pInPortTunnelComp->FillThisBuffer(pInPortTunnelComp, &obh);
                if(eRet != SUCCESS)
                {
                    aloge("fatal error! fill this buffer fail[0x%x], text frame id=[%d], check code!", eRet, pEntry->stEncodedStream.nID);
                    pthread_mutex_lock(&pRecRenderData->mTextInputFrameListMutex);
                    list_add_tail(&pEntry->mList, &pRecRenderData->mTextInputFrameUsedList);
                    //list_move_tail(&pEntry->mList, &pRecRenderData->mTextInputFrameUsedList);
                    pthread_mutex_unlock(&pRecRenderData->mTextInputFrameListMutex);
                }
                else
                {
                    pthread_mutex_lock(&pRecRenderData->mTextInputFrameListMutex);
                    list_add_tail(&pEntry->mList, &pRecRenderData->mTextInputFrameIdleList);
                    pthread_mutex_unlock(&pRecRenderData->mTextInputFrameListMutex);
                }
            }
            else
            {
                if(pEntry->mUsedRefCnt < 0)
                {
                    aloge("fatal error! usedRefCnt[%d]<0, check code!", pEntry->mUsedRefCnt);
                    eRet = ERR_MUX_NOT_PERM;
                }
                pthread_mutex_unlock(&pRecRenderData->mTextInputFrameListMutex);
            }
        }
        else
        {
            aloge("fatal error! not find TFrmId[%d] in used list.", pRSPacket->mId);
            eRet = ERR_MUX_UNEXIST;
            pthread_mutex_unlock(&pRecRenderData->mTextInputFrameListMutex);
        }
    }
    else
    {
        aloge("fatal error! invalid streamType[%d]", pRSPacket->mStreamType);
        eRet = ERR_MUX_NOT_PERM;
    }
    return eRet;
}

/*******************************************************************************
Function name: RecRender_AddOutputSinkInfo
Description:
    1. add sinkInfo to recorder_render.
    2. only allow call this function in state COMP_StateLoaded,OMX_StateExecuting.
        In future, we will change if necessary.
    3. in this function, we default think the parameter is right, so we donot do exception restore process,
        donot check mSinkInfoArray[] valid.
    4. ref CDXRecorder_AddOutputSinkInfo()
Parameters:

Return:

Time: 2014/6/25
*******************************************************************************/
ERRORTYPE RecRender_AddOutputSinkInfo(RECRENDERDATATYPE *pRecRenderData, int nChnId, MUX_CHN_ATTR_S *pChnAttr, int nFd)
{
    int eRet = SUCCESS;
    CdxOutputSinkInfo SinkInfo;
    configCdxOutputSinkInfo(&SinkInfo, pChnAttr, nFd);
    alogd("(of:%d, fd:%d, callback_out_flag:%d, cache_flag:%d)", SinkInfo.nMuxerMode, SinkInfo.nOutputFd, SinkInfo.nCallbackOutFlag, SinkInfo.bBufFromCacheFlag);

    //int i = 0;
    //find if the same output_format sinkInfo exist
    //not find sink_type, add a new one
    pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
    if(list_empty(&pRecRenderData->mIdleSinkInfoList))
    {
        alogw("Low probability! sinkInfo is not enough, increase one!");
        RecSink *pNode = (RecSink*)malloc(sizeof(RecSink));
        if(pNode)
        {
            memset(pNode, 0, sizeof(RecSink));
            list_add_tail(&pNode->mList, &pRecRenderData->mIdleSinkInfoList);
            pRecRenderData->mSinkInfoTotalNum++;
        }
        else
        {
            aloge("fatal error! malloc fail[%s]!", strerror(errno));
            eRet = ERR_MUX_ILLEGAL_PARAM;
            pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
            return eRet;
        }
    }
    RecSink *pEntry = list_first_entry(&pRecRenderData->mIdleSinkInfoList, RecSink, mList);
    RecSinkInit(pEntry);
    pEntry->SetCallbacks(pEntry, &RecSinkCallback, (void*)pRecRenderData);
    pEntry->SetRecordMode(pEntry, pRecRenderData->recorder_mode);
    pEntry->SetMediaInf(pEntry, &pRecRenderData->media_inf);
    pEntry->SetVencExtraData(pEntry, &pRecRenderData->venc_extradata_info);
    pEntry->SetMaxFileDuration(pEntry, pChnAttr->mMaxFileDuration);
    //pEntry->SetImpactFileDuration(pEntry, pRecRenderData->impact_bftime+pRecRenderData->impact_aftime);
    pEntry->SetFileDurationPolicy(pEntry, pRecRenderData->mRecordFileDurationPolicy);
    pEntry->SetMaxFileSize(pEntry, pChnAttr->mMaxFileSizeBytes);
    pEntry->SetFsWriteMode(pEntry, pChnAttr->mFsWriteMode);
    pEntry->SetFsSimpleCacheSize(pEntry, pChnAttr->mSimpleCacheSize);
    pEntry->SetCallbackWriter(pEntry, &pRecRenderData->callback_writer);
    pEntry->ConfigByCdxSink(pEntry, &SinkInfo);
    list_move_tail(&pEntry->mList, &pRecRenderData->mValidSinkInfoList);
    pRecRenderData->mSinkInfoValidNum++;
    MuxChanAttrNode *pChanAttrNode = (MuxChanAttrNode*)malloc(sizeof(MuxChanAttrNode));
    memset(pChanAttrNode, 0, sizeof(MuxChanAttrNode));
    pChanAttrNode->mChnId = nChnId;
    pChanAttrNode->mChnAttr = *pChnAttr;
    list_add_tail(&pChanAttrNode->mList, &pRecRenderData->mChnAttrList);
    //write all used packets to cacheMuxer.
    if(pEntry->bBufFromCacheFlag)
    {
        pRecRenderData->cache_manager->LockPacketList((COMP_HANDLETYPE)pRecRenderData->cache_manager);
        struct list_head *pUsingPacketList = NULL;  //RecSinkPacket
        pRecRenderData->cache_manager->GetUsingPacketList((COMP_HANDLETYPE)pRecRenderData->cache_manager, &pUsingPacketList);
        RecSinkPacket *pCacheRSPacket;
        int nUsingCnt = 0;
        list_for_each_entry(pCacheRSPacket, pUsingPacketList, mList)
        {
            pCacheRSPacket->mRefCnt++;
            if(pEntry->EmptyThisBuffer(pEntry, pCacheRSPacket) == SUCCESS)
            {
            }
            else
            {
                aloge("fatal error! RecSink empty this buffer fail!");
                pCacheRSPacket->mRefCnt--;
                if(pCacheRSPacket->mRefCnt<=0)
                {
                    aloge("fatal error! used packet refCnt=[%d], check code!", pCacheRSPacket->mRefCnt);
                }
            }
            nUsingCnt++;
        }
        alogd("send [%d]cacheUsingPackets to muxerId[%d]", nUsingCnt, pEntry->mMuxerId);
        pRecRenderData->cache_manager->UnlockPacketList((COMP_HANDLETYPE)pRecRenderData->cache_manager);
    }
    pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
    return eRet;
}

/*******************************************************************************
Function name: RecRender_RemoveOutputSinkInfo
Description:
    1. remove sinkInfo from recorder_render.
    2. only allow call this function in state OMX_StateExecuting, and will call RecRender_MuxerClose() to really close muxer.
        In future, we will change if necessary.
    3. in this function, we default think the parameter is right, so we donot do exception restore process,
        donot check mSinkInfoArray[] valid.
    4. ref CDXRecorder_RemoveOutputSinkInfo()
Parameters:

Return:

Time: 2014/7/12
*******************************************************************************/
static ERRORTYPE RecRender_RemoveOutputSinkInfo(RECRENDERDATATYPE *pRecRenderData, int nChnId)
{
    //int omx_ret = SUCCESS;
    //(1)update cdx_rec_ctx->mSinkInfoArray[]
    int i = 0;
    int nFindFlag = 0;
    RecSink *pSinkInfo;
    int nMuxerId = -1;
    //find if the same output_format sinkInfo exist
    pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
    MuxChanAttrNode *pMuxChnEntry;
    list_for_each_entry(pMuxChnEntry, &pRecRenderData->mChnAttrList, mList)
    {
        if(pMuxChnEntry->mChnId == nChnId)
        {
            if(0 == nFindFlag)
            {
                nFindFlag = 1;
                nMuxerId = pMuxChnEntry->mChnAttr.mMuxerId;
                //break;
            }
            else
            {
                aloge("fatal error! more than one mux channel[%d]?", pMuxChnEntry->mChnId);
            }
        }
    }
    if(0==nFindFlag)
    {
        aloge("fatal error! not find an exist muxChn[%d]", nChnId);
        pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
        return ERR_MUX_ILLEGAL_PARAM;
    }
    alogd("(muxChn:%d, muxerId:%d)", nChnId, nMuxerId);
    nFindFlag = 0;
    list_for_each_entry(pSinkInfo, &pRecRenderData->mValidSinkInfoList, mList)
    {
        if(pSinkInfo->mMuxerId == nMuxerId)
        {
            alogd("find an exist Array[%d], muxerId[%d]:\n"
                "nMuxerMode[%d], fd[%d], fallocateLen[%d], callbackOutFlag[%d]",
                i, pSinkInfo->mMuxerId, pSinkInfo->nMuxerMode, pSinkInfo->nOutputFd, pSinkInfo->nFallocateLen, pSinkInfo->nCallbackOutFlag);
            nFindFlag = 1;
            break;
        }
        i++;
    }
    if(0 == nFindFlag)
    {
        list_for_each_entry(pSinkInfo, &pRecRenderData->mSwitchingSinkInfoList, mList)
        {
            if(pSinkInfo->mMuxerId == nMuxerId)
            {
                alogd("find an exist Array[%d], muxerId[%d], during switching file:\n"
                    "nMuxerMode[%d], fd[%d], fallocateLen[%d], callbackOutFlag[%d]",
                    i, pSinkInfo->mMuxerId, pSinkInfo->nMuxerMode, pSinkInfo->nOutputFd, pSinkInfo->nFallocateLen, pSinkInfo->nCallbackOutFlag);
                nFindFlag = 1;
                break;
            }
            i++;
        }
    }
    if(0==nFindFlag)
    {
        aloge("fatal error! not find an exist muxerId[%d]", nMuxerId);
        pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
        return ERR_MUX_ILLEGAL_PARAM;
    }
    else //find an exist sink_type
    {
        pSinkInfo->Reset(pSinkInfo);
        RecSinkDestroy(pSinkInfo);
        list_move_tail(&pSinkInfo->mList, &pRecRenderData->mIdleSinkInfoList);
        pRecRenderData->mSinkInfoValidNum--;
        MuxChanAttrNode *pEntry, *pTmp;
        nFindFlag = 0;
        list_for_each_entry_safe(pEntry, pTmp, &pRecRenderData->mChnAttrList, mList)
        {
            if(pEntry->mChnId == nChnId)
            {
                list_del(&pEntry->mList);
                free(pEntry);
                nFindFlag = 1;
                break;
            }
        }
        if(0 == nFindFlag)
        {
            aloge("fatal error! why chnAttrNode of muxerId[%d] is not exist?", nMuxerId);
        }
        pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
        return SUCCESS;
    }
}

MuxChanAttrNode* RecRender_FindMuxChanAttrNodeByMuxerId(RECRENDERDATATYPE *pRecRenderData, int muxerId)
{
    if(!list_empty(&pRecRenderData->mChnAttrList))
    {
        MuxChanAttrNode *pEntry;
        list_for_each_entry(pEntry, &pRecRenderData->mChnAttrList, mList)
        {
            if(pEntry->mChnAttr.mMuxerId == muxerId)
            {
                return pEntry;
            }
        }
    }
    return NULL;
}

ERRORTYPE RecRenderGetPortDefinition(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_INOUT COMP_PARAM_PORTDEFINITIONTYPE *pPortDef)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = SUCCESS;

    unsigned int portIdx = pPortDef->nPortIndex;

    if (portIdx < pRecRenderData->sPortParam.nPorts)
    {
        memcpy(pPortDef, &pRecRenderData->sInPortDef[portIdx], sizeof(COMP_PARAM_PORTDEFINITIONTYPE));
    }
    else
        eError = ERR_MUX_ILLEGAL_PARAM;
    return eError;
}

ERRORTYPE RecRenderSetPortDefinition(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN COMP_PARAM_PORTDEFINITIONTYPE *pPortDef)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = SUCCESS;

    unsigned int portIndex = pPortDef->nPortIndex;
    if (portIndex < pRecRenderData->sPortParam.nPorts)
    {
        memcpy(&pRecRenderData->sInPortDef[portIndex], pPortDef, sizeof(COMP_PARAM_PORTDEFINITIONTYPE));
    }
    else
        eError = ERR_MUX_ILLEGAL_PARAM;
    return eError;
}

ERRORTYPE RecRenderGetCompBufferSupplier(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_INOUT COMP_PARAM_BUFFERSUPPLIERTYPE *pPortBufSupplier)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = SUCCESS;

    //find nPortIndex
    BOOL bFindFlag = FALSE;
    int i;
    for(i=0; i<MAX_REC_RENDER_PORTS; i++)
    {
        if(pRecRenderData->sPortBufSupplier[i].nPortIndex == pPortBufSupplier->nPortIndex)
        {
            bFindFlag = TRUE;
            memcpy(pPortBufSupplier, &pRecRenderData->sPortBufSupplier[i], sizeof(COMP_PARAM_BUFFERSUPPLIERTYPE));
            break;
        }
    }
    if(bFindFlag)
    {
        eError = SUCCESS;
    }
    else
    {
        eError = ERR_MUX_ILLEGAL_PARAM;
    }
    return eError;
}

ERRORTYPE RecRenderSetCompBufferSupplier(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN COMP_PARAM_BUFFERSUPPLIERTYPE *pPortBufSupplier)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = SUCCESS;

    //find nPortIndex
    BOOL bFindFlag = FALSE;
    int i;
    for(i=0; i<MAX_REC_RENDER_PORTS; i++)
    {
        if(pRecRenderData->sPortBufSupplier[i].nPortIndex == pPortBufSupplier->nPortIndex)
        {
            bFindFlag = TRUE;
            memcpy(&pRecRenderData->sPortBufSupplier[i], pPortBufSupplier, sizeof(COMP_PARAM_BUFFERSUPPLIERTYPE));
            break;
        }
    }
    if(bFindFlag)
    {
        eError = SUCCESS;
    }
    else
    {
        eError = ERR_MUX_ILLEGAL_PARAM;
    }
    return eError;
}

ERRORTYPE RecRenderGetPortParam(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_OUT COMP_PORT_PARAM_TYPE *pPortParam)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = SUCCESS;

    memcpy(pPortParam, &pRecRenderData->sPortParam, sizeof(COMP_PORT_PARAM_TYPE));
    return eError;
}

ERRORTYPE RecRenderGetMPPChannelInfo(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_OUT MPP_CHN_S *pChn)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = SUCCESS;
    copy_MPP_CHN_S(pChn, &pRecRenderData->mMppChnInfo);
    return eError;
}

ERRORTYPE RecRenderSetMPPChannelInfo(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN MPP_CHN_S *pChn)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    copy_MPP_CHN_S(&pRecRenderData->mMppChnInfo, pChn);
    return SUCCESS;
}

ERRORTYPE RecRenderGetTunnelInfo(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_INOUT COMP_INTERNAL_TUNNELINFOTYPE *pTunnelInfo)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = ERR_MUX_UNEXIST;
    int i;
    for(i=0; i<MAX_REC_RENDER_PORTS; i++)
    {
        if(pRecRenderData->sInPortTunnelInfo[i].nPortIndex == pTunnelInfo->nPortIndex)
        {
            memcpy(pTunnelInfo, &pRecRenderData->sInPortTunnelInfo[i], sizeof(COMP_INTERNAL_TUNNELINFOTYPE));
            eError = SUCCESS;
            break;
        }
    }
    return eError;
}

ERRORTYPE RecRenderGetGroupAttr(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_OUT MUX_GRP_ATTR_S *pGroupAttr)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = SUCCESS;

    copy_MUX_GRP_ATTR_S(pGroupAttr, &pRecRenderData->mGroupAttr);
    return eError;
}

ERRORTYPE RecRenderSetGroupAttr(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN MUX_GRP_ATTR_S *pGroupAttr)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = SUCCESS;

    if(pRecRenderData->state!=COMP_StateIdle && pRecRenderData->state!=COMP_StateLoaded)
    {
        aloge("fatal error! cannot set groupAttr in wrong state[0x%x]", pRecRenderData->state);
        return ERR_MUX_INCORRECT_STATE_OPERATION;
    }

    copy_MUX_GRP_ATTR_S(&pRecRenderData->mGroupAttr, pGroupAttr);

    //config recorder_mode
    pRecRenderData->recorder_mode = RECORDER_MODE_NONE;
    if(pGroupAttr->mVideoEncodeType != PT_MAX)
    {
        pRecRenderData->recorder_mode |= RECORDER_MODE_VIDEO;
    }
    if(pGroupAttr->mAudioEncodeType != PT_MAX)
    {
        pRecRenderData->recorder_mode |= RECORDER_MODE_AUDIO;
    }
    if(PT_TEXT == pGroupAttr->mTextEncodeType)
    {
        pRecRenderData->recorder_mode |= RECORDER_MODE_TEXT;
    }

    //config media_inf
    if(pGroupAttr->mVideoEncodeType != PT_MAX)
    {
        pRecRenderData->media_inf.nHeight = pGroupAttr->mHeight;
        pRecRenderData->media_inf.nWidth = pGroupAttr->mWidth;
        pRecRenderData->media_inf.uVideoFrmRate = pGroupAttr->mVideoFrmRate;
        pRecRenderData->media_inf.create_time = pGroupAttr->mCreateTime;
        pRecRenderData->media_inf.maxKeyInterval = pGroupAttr->mMaxKeyInterval;
        pRecRenderData->media_inf.mVideoEncodeType = map_PAYLOAD_TYPE_E_to_VENC_CODEC_TYPE(pGroupAttr->mVideoEncodeType);
    }
    if(pGroupAttr->mAudioEncodeType != PT_MAX)
    {
        pRecRenderData->media_inf.channels = pGroupAttr->mChannels;
        pRecRenderData->media_inf.bits_per_sample = pGroupAttr->mBitsPerSample;
        pRecRenderData->media_inf.frame_size = pGroupAttr->mSamplesPerFrame;
        pRecRenderData->media_inf.sample_rate = pGroupAttr->mSampleRate;
        pRecRenderData->media_inf.audio_encode_type = map_PAYLOAD_TYPE_E_to_AUDIO_ENCODER_TYPE(pGroupAttr->mAudioEncodeType);
    }

    if(PT_TEXT == pGroupAttr->mTextEncodeType)
    {
        pRecRenderData->media_inf.text_encode_type = 0;
        pRecRenderData->media_inf.geo_available = 1;
    }
    
    return eError;
}

ERRORTYPE RecRenderGetChnAttr(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_INOUT MuxChnAttr *pChnAttr)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = SUCCESS;

    MuxChanAttrNode *pEntry;
    if(!list_empty(&pRecRenderData->mChnAttrList))
    {
        eError = ERR_MUX_UNEXIST;
        list_for_each_entry(pEntry, &pRecRenderData->mChnAttrList, mList)
        {
            if(pEntry->mChnId == pChnAttr->nChnId)
            {
                if(pChnAttr->pChnAttr)
                {
                    *pChnAttr->pChnAttr = pEntry->mChnAttr;
                }
                eError = SUCCESS;
                break;
            }
        }
    }
    else
    {
        eError = ERR_MUX_UNEXIST;
    }
    return eError;
}

ERRORTYPE RecRenderSetChnAttr(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN MuxChnAttr *pChnAttr)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = SUCCESS;

    MuxChanAttrNode *pEntry = NULL;
    if(!list_empty(&pRecRenderData->mChnAttrList))
    {
        eError = ERR_MUX_UNEXIST;
        list_for_each_entry(pEntry, &pRecRenderData->mChnAttrList, mList)
        {
            if(pEntry->mChnId == pChnAttr->nChnId)
            {
                pEntry->mChnAttr = *pChnAttr->pChnAttr;
                //pEntry->SetMaxFileDuration(pEntry, pRecRenderData->max_file_duration);
                eError = SUCCESS;
                break;
            }
        }
    }
    else
    {
        eError = ERR_MUX_UNEXIST;
    }

    RecSink *pRecSinkEntry;
    if(!list_empty(&pRecRenderData->mValidSinkInfoList))
    {
        eError = ERR_MUX_UNEXIST;
        list_for_each_entry(pRecSinkEntry, &pRecRenderData->mValidSinkInfoList, mList)
        {
            if(pRecSinkEntry->mMuxerId == pEntry->mChnAttr.mMuxerId)
            {
                if (pRecSinkEntry->mMaxFileDuration != pEntry->mChnAttr.mMaxFileDuration)
                {
                    pRecSinkEntry->SetMaxFileDuration(pRecSinkEntry, pEntry->mChnAttr.mMaxFileDuration);
                }
                eError = SUCCESS;
                break;
            }
        }
    }
    else
    {
        eError = ERR_MUX_UNEXIST;
    }
    return eError;
}

/*****************************************************************************/
ERRORTYPE RecRenderSendCommand(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN COMP_COMMANDTYPE Cmd,
        PARAM_IN unsigned int nParam1,
        PARAM_IN void* pCmdData)
{
    RECRENDERDATATYPE *pRecRenderData;
    CompInternalMsgType eCmd;
    ERRORTYPE eError = SUCCESS;
    message_t msg;
    void*   pMsgData = NULL;
    int     nMsgDataSize = 0;
    memset(&msg, 0, sizeof(message_t));

    alogv("RecRenderSendCommand: %d", Cmd);

    pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);

    if (pRecRenderData->state == COMP_StateInvalid)
    {
        alogd("stateInvalid deny command[0x%x]", Cmd);
        eError = ERR_MUX_INVALIDSTATE;
        goto OMX_CONF_CMD_BAIL;
    }

    switch (Cmd)
    {
        case COMP_CommandStateSet:
            eCmd = SetState;
            break;

        case COMP_CommandFlush:
            eCmd = Flush;
            break;

        case COMP_CommandVendorAddChn:
        {
            alogv("call COMP CommandVendorAddChn in state[%d]", pRecRenderData->state);
            if(pRecRenderData->state != COMP_StateExecuting && pRecRenderData->state != COMP_StateIdle)
            {
                aloge("fatal error! why call COMP CommandVendorAddChn in invalid state[%d]", pRecRenderData->state);
                eError = ERR_MUX_INCORRECT_STATE_OPERATION;
                goto OMX_CONF_CMD_BAIL;
            }
            eCmd = VendorAddOutputSinkInfo;
            if(NULL==pCmdData)
            {
                alogw("MUX_CHN_ATTR_S == NULL");
                eError = ERR_MUX_ILLEGAL_PARAM;
                goto OMX_CONF_CMD_BAIL;
            }
            pMsgData = pCmdData;
            nMsgDataSize = sizeof(MuxChnAttr);
            break;
        }
        case COMP_CommandVendorRemoveChn:
        {
            alogd("call COMP CommandVendorRemoveChn in state[%d], remove muxerId[%d]", pRecRenderData->state, nParam1);
            if(pRecRenderData->state != COMP_StateExecuting && pRecRenderData->state != COMP_StateIdle)
            {
                aloge("fatal error! why call COMP CommandVendorRemoveChn in invalid state[%d]", pRecRenderData->state);
                eError = ERR_MUX_ILLEGAL_PARAM;
                goto OMX_CONF_CMD_BAIL;
            }
            eCmd = VendorRemoveOutputSinkInfo;
            break;
        }
        case COMP_CommandSwitchFile:
        {
            alogd("OMX CommandSwitchFile nMuxerId=%d", nParam1);
            eCmd = SwitchFile;
            break;
        }
        case COMP_CommandSwitchFileNormal:
        {
            alogd("OMX CommandSwitchFile normal nMuxerId=%d", nParam1);
            eCmd = SwitchFileNormal;
            break;
        }

        default:
            aloge("fatal error! unknown command[0x%x]", Cmd);
            eCmd = -1;
            break;
    }

    msg.command = eCmd;
    msg.para0 = nParam1;
    msg.mpData = pMsgData;
    msg.mDataSize = nMsgDataSize;
    putMessageWithData(&pRecRenderData->cmd_queue, &msg);
OMX_CONF_CMD_BAIL:
    return eError;
}

/*****************************************************************************/
ERRORTYPE RecRenderGetState(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_OUT COMP_STATETYPE* pState) {
    RECRENDERDATATYPE *pRecRenderData;
    ERRORTYPE eError = SUCCESS;

    pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    //pthread_mutex_lock(&pRecRenderData->mStateMutex);
    *pState = pRecRenderData->state;
    //pthread_mutex_unlock(&pRecRenderData->mStateMutex);

    //OMX_CONF_CMD_BAIL:
    return eError;
}

/*****************************************************************************/
ERRORTYPE RecRenderSetCallbacks(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN COMP_CALLBACKTYPE* pCallbacks,
        PARAM_IN void* pAppData)
{
    RECRENDERDATATYPE *pRecRenderData;
    ERRORTYPE eError = SUCCESS;

    pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);

    pRecRenderData->pCallbacks = pCallbacks;
    pRecRenderData->pAppData = pAppData;

    //OMX_CONF_CMD_BAIL:
    return eError;
}


ERRORTYPE RecRenderSetSwitchPolicy(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN RecordFileDurationPolicy *pPolicy
        )
{
    RECRENDERDATATYPE *pRecRenderData;
    ERRORTYPE eError = SUCCESS;

    pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);

    pRecRenderData->mRecordFileDurationPolicy = *pPolicy;

    return eError;
}

ERRORTYPE RecRenderGetSwitchPolicy(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN RecordFileDurationPolicy *pPolicy
        )
{
    RECRENDERDATATYPE *pRecRenderData;
    ERRORTYPE eError = SUCCESS;

    pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);

    *pPolicy = pRecRenderData->mRecordFileDurationPolicy;

    return eError;
}

ERRORTYPE RecRenderSetShutDownType(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN ShutDownType *pShutDownType
        )
{
    RECRENDERDATATYPE *pRecRenderData;
    ERRORTYPE eError = SUCCESS;

    pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);

    pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);

    RecSink *pEntry = NULL;
    list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
    {
        if(pEntry->mMuxerId == pShutDownType->mMuxerId)
        {
            pEntry->SetShutDownNow(pEntry, pShutDownType->mbShutDownNowFlag);
            alogd("the MuxerId[%d], ShutDownNow[%d]", pEntry->mMuxerId, pShutDownType->mbShutDownNowFlag);
        }
        else if(-1 == pShutDownType->mMuxerId)
        {
            pEntry->SetShutDownNow(pEntry, pShutDownType->mbShutDownNowFlag);
            alogd("the MuxerId[%d], ShutDownNow[%d]", pEntry->mMuxerId, pShutDownType->mbShutDownNowFlag);
        }
    }

    pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);

    return eError;
}

/*****************************************************************************/
ERRORTYPE RecRenderGetConfig(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN COMP_INDEXTYPE nIndex,
        PARAM_INOUT void* pComponentConfigStructure)
{
    RECRENDERDATATYPE *pRecRenderData;
    ERRORTYPE eError = SUCCESS;
    //COMP_INDEXTYPE portIdx;

    pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);

    switch (nIndex)
    {
        case COMP_IndexParamPortDefinition:
        {
            eError = RecRenderGetPortDefinition(hComponent, (COMP_PARAM_PORTDEFINITIONTYPE*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexParamCompBufferSupplier:
        {
            eError = RecRenderGetCompBufferSupplier(hComponent, (COMP_PARAM_BUFFERSUPPLIERTYPE*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexParamOtherInit:
        {
            eError = RecRenderGetPortParam(hComponent, (COMP_PORT_PARAM_TYPE*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexVendorMPPChannelInfo:
        {
            eError = RecRenderGetMPPChannelInfo(hComponent, (MPP_CHN_S*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexVendorTunnelInfo:
        {
            eError = RecRenderGetTunnelInfo(hComponent, (COMP_INTERNAL_TUNNELINFOTYPE*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexVendorMuxGroupAttr:
        {
            eError = RecRenderGetGroupAttr(hComponent, (MUX_GRP_ATTR_S*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexVendorMuxChnAttr:
        {
            eError = RecRenderGetChnAttr(hComponent, (MuxChnAttr*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexVendorMuxGetDuration:
            *(int64_t*)pComponentConfigStructure = pRecRenderData->duration;
            break;
        case COMP_IndexVendorMuxCacheState:
            if(pRecRenderData->cache_manager)
            {
                if(TRUE == pRecRenderData->cache_manager->IsReady(pRecRenderData->cache_manager))
                {
                    eError = pRecRenderData->cache_manager->QueryCacheState(pRecRenderData->cache_manager, (CacheState*)pComponentConfigStructure);
                    if(eError!=SUCCESS)
                    {
                        eError = ERR_MUX_SYS_NOTREADY;
                    }
                }
                else
                {
                    eError = ERR_MUX_SYS_NOTREADY;
                }
            }
            else
            {
                eError = ERR_MUX_NOT_SUPPORT;
            }
            break;
        case COMP_IndexVendorMuxSwitchPolicy:
        {
            eError = RecRenderGetSwitchPolicy(hComponent, (RecordFileDurationPolicy *)pComponentConfigStructure);
            break;
        }
        default:
            aloge("fatal error! unknown index[0x%x]", nIndex);
            break;
    }

    if (pRecRenderData->state == COMP_StateInvalid)
        eError = ERR_MUX_INCORRECT_STATE_OPERATION;

    //OMX_CONF_CMD_BAIL:
    return eError;
}

ERRORTYPE RecRenderSetConfig(
        PARAM_IN COMP_HANDLETYPE hComponent,
        PARAM_IN COMP_INDEXTYPE nIndex,
        PARAM_IN void* pComponentConfigStructure)
{
    RECRENDERDATATYPE *pRecRenderData = NULL;
    ERRORTYPE eError = SUCCESS;

    pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);

    switch (nIndex)
    {
        case COMP_IndexParamPortDefinition:
        {
            eError = RecRenderSetPortDefinition(hComponent, (COMP_PARAM_PORTDEFINITIONTYPE*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexParamCompBufferSupplier:
        {
            eError = RecRenderSetCompBufferSupplier(hComponent, (COMP_PARAM_BUFFERSUPPLIERTYPE*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexVendorMPPChannelInfo:
        {
            eError = RecRenderSetMPPChannelInfo(hComponent, (MPP_CHN_S*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexVendorMuxGroupAttr:
        {
            eError = RecRenderSetGroupAttr(hComponent, (MUX_GRP_ATTR_S*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexVendorMuxChnAttr:
        {
            eError = RecRenderSetChnAttr(hComponent, (MuxChnAttr*)pComponentConfigStructure);
            break;
        }
        case COMP_IndexVendorMuxSwitchFd:
        {
            if (pRecRenderData->state != COMP_StateExecuting)
            {
                alogw("omx IndexConfigVendorSwitchFd state[%d] is not valid", pRecRenderData->state);
            }
            CdxFdT *pCdxFd = (CdxFdT*)pComponentConfigStructure;
            pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
            BOOL    bFindFlag = FALSE;
            int     i = 0;
            RecSink*    pEntry;
            list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
            {
                if(pEntry->mMuxerId == pCdxFd->mMuxerId)
                {
                    i++;
                }
            }
            list_for_each_entry(pEntry, &pRecRenderData->mSwitchingSinkInfoList, mList)
            {
                if(pEntry->mMuxerId == pCdxFd->mMuxerId)
                {
                    i++;
                }
            }
            if(i!=1)
            {
                aloge("fatal error! switch muxerId[%d], [%d]times", pEntry->mMuxerId, i);
            }

            list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
            {
                if(pEntry->mMuxerId == pCdxFd->mMuxerId)
                {
                    alogd("switch fd[%d] for muxerId[%d]", pCdxFd->mFd, pEntry->mMuxerId);
                    // can reset FD, when encode
                    pEntry->SwitchFd(pEntry, pCdxFd->mFd, pCdxFd->mnFallocateLen, pCdxFd->mIsImpact);
                    bFindFlag = TRUE;
                    break;
                }
            }
            list_for_each_entry(pEntry, &pRecRenderData->mSwitchingSinkInfoList, mList)
            {
                if(bFindFlag)
                {
                    break;
                }
                if(pEntry->mMuxerId == pCdxFd->mMuxerId)
                {
                    alogd("not switch fd[%d] again for muxerId[%d] during switching file", pCdxFd->mFd, pEntry->mMuxerId);
                    // can reset FD, when encode
                    //pEntry->SwitchFd(pEntry, pCdxFd->mFd, (char*)pCdxFd->mPath, pCdxFd->mnFallocateLen, pCdxFd->mIsImpact);
                    bFindFlag = TRUE;
                    break;
                }
            }
            pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
            break;
        }
        case COMP_IndexVendorSdcardState:
        {
            if (pRecRenderData->state != COMP_StateExecuting)
            {
                alogw("COMP IndexConfigVendorSetSdcardState state[%d] is not valid, sdcardState[%d]", pRecRenderData->state, *(int*)pComponentConfigStructure);
            }
            pRecRenderData->mbSdCardState = (BOOL)*(int*)pComponentConfigStructure;
            pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
            RecSink*    pEntry;
            list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
            {
                pEntry->SetSdcardState(pEntry, pRecRenderData->mbSdCardState);
            }
            pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
            break;
        }
        case COMP_IndexVendorsetMuxCacheDuration:
        {
            int64_t *duration = (int64_t*)pComponentConfigStructure;
            pRecRenderData->mCacheTime = *duration;
            if (pRecRenderData->mCacheTime > 0 && pRecRenderData->cache_manager == NULL)
            {
                if(NULL == pRecRenderData->cache_manager)
                {
                    pRecRenderData->cache_manager = RsPacketCacheManagerConstruct(pRecRenderData->mCacheTime);
                }
                else
                {
                    aloge("fatal error! cache_manager already exist in state[%d]", pRecRenderData->state);
                }
            }
            break;
        }
        case COMP_IndexVendorExtraData:
        {
            VencHeaderData *pH264SpsPps = (VencHeaderData*)pComponentConfigStructure;
            if(pH264SpsPps->nLength > 0)
            {
                pRecRenderData->venc_extradata_info.pBuffer = (unsigned char *)malloc(pH264SpsPps->nLength);
                if(pRecRenderData->venc_extradata_info.pBuffer == NULL)
                {
                    aloge("pRecRenderData->venc_extradata_info.data == NULL");
                    break;
                }
                memcpy(pRecRenderData->venc_extradata_info.pBuffer, pH264SpsPps->pBuffer, pH264SpsPps->nLength);
                pRecRenderData->venc_extradata_info.nLength = pH264SpsPps->nLength;
            }
            else
            {
                aloge("set video extra data error");
            }

            /* 
            //don't need these codes, because recRender use pointer of pRecRenderData->venc_extradata_info to set to recSink.
            //It is OK to update pRecRenderData->venc_extradata_info only.
            pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
            int i = 0;
            RecSink *pEntry;
            list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
            {
                pEntry->SetVencExtraData(pEntry, &pRecRenderData->venc_extradata_info);
                i++;
            }
            pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
            */
                                    
            break;
        }
//        case OMX_IndexVendorSetRecRenderCallbackInfo:
//            pRecRenderData->callback_writer = (void*)ComponentParameterStructure;
//            break;

//        case COMP_IndexVendorFsWriteMode:
//            pRecRenderData->mFsWriteMode = *(FSWRITEMODE*)pComponentConfigStructure;
//            break;
//        case COMP_IndexVendorFsSimpleCacheSize:
//            pRecRenderData->mSimpleCacheSize = *(int*)pComponentConfigStructure;
//            break;
        case COMP_IndexVendorMuxSwitchPolicy:
        {
            eError = RecRenderSetSwitchPolicy(hComponent, (RecordFileDurationPolicy *)pComponentConfigStructure);
            break;
        }
        case COMP_IndexVendorMuxShutDownType:
        {
            eError = RecRenderSetShutDownType(hComponent, (ShutDownType *)pComponentConfigStructure);
            break;
        }
        default:
        {
            aloge("fatal error! unknown nIndex[0x%x] in state[%d]", nIndex, pRecRenderData->state);
            eError = ERR_MUX_ILLEGAL_PARAM;
            break;
        }
    }

    return eError;
}

ERRORTYPE RecRenderComponentTunnelRequest(
    PARAM_IN  COMP_HANDLETYPE hComponent,
    PARAM_IN  unsigned int nPort,
    PARAM_IN  COMP_HANDLETYPE hTunneledComp,
    PARAM_IN  unsigned int nTunneledPort,
    PARAM_INOUT  COMP_TUNNELSETUPTYPE* pTunnelSetup)
{
    ERRORTYPE eError = SUCCESS;
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    if (pRecRenderData->state == COMP_StateExecuting)
    {
        alogw("Be careful! tunnel request may be some danger in StateExecuting");
    }
    else if (pRecRenderData->state != COMP_StateIdle)
    {
        aloge("fatal error! tunnel request can't be in state[0x%x]!", pRecRenderData->state);
        eError = ERR_MUX_INCORRECT_STATE_OPERATION;
        goto COMP_CMD_FAIL;
    }
    COMP_PARAM_PORTDEFINITIONTYPE    *pPortDef;
    COMP_INTERNAL_TUNNELINFOTYPE     *pPortTunnelInfo;
    COMP_PARAM_BUFFERSUPPLIERTYPE    *pPortBufSupplier;
    BOOL bFindFlag;
    int i;
    bFindFlag = FALSE;
    for(i=0; i<MAX_REC_RENDER_PORTS; i++)
    {
        if(pRecRenderData->sInPortDef[i].nPortIndex == nPort)
        {
            pPortDef = &pRecRenderData->sInPortDef[i];
            bFindFlag = TRUE;
            break;
        }
    }
    if(FALSE == bFindFlag)
    {
        aloge("fatal error! portIndex[%d] wrong!", nPort);
        eError = ERR_MUX_ILLEGAL_PARAM;
        goto COMP_CMD_FAIL;
    }

    bFindFlag = FALSE;
    for(i=0; i<MAX_REC_RENDER_PORTS; i++)
    {
        if(pRecRenderData->sInPortTunnelInfo[i].nPortIndex == nPort)
        {
            pPortTunnelInfo = &pRecRenderData->sInPortTunnelInfo[i];
            bFindFlag = TRUE;
            break;
        }
    }
    if(FALSE == bFindFlag)
    {
        aloge("fatal error! portIndex[%d] wrong!", nPort);
        eError = ERR_MUX_ILLEGAL_PARAM;
        goto COMP_CMD_FAIL;
    }

    bFindFlag = FALSE;
    for(i=0; i<MAX_REC_RENDER_PORTS; i++)
    {
        if(pRecRenderData->sPortBufSupplier[i].nPortIndex == nPort)
        {
            pPortBufSupplier = &pRecRenderData->sPortBufSupplier[i];
            bFindFlag = TRUE;
            break;
        }
    }
    if(FALSE == bFindFlag)
    {
        aloge("fatal error! portIndex[%d] wrong!", nPort);
        eError = ERR_MUX_ILLEGAL_PARAM;
        goto COMP_CMD_FAIL;
    }

    pPortTunnelInfo->nPortIndex = nPort;
    pPortTunnelInfo->hTunnel = hTunneledComp;
    pPortTunnelInfo->nTunnelPortIndex = nTunneledPort;
    pPortTunnelInfo->eTunnelType = (pPortDef->eDomain == COMP_PortDomainOther) ? TUNNEL_TYPE_CLOCK : TUNNEL_TYPE_COMMON;
    if(NULL==hTunneledComp && 0==nTunneledPort && NULL==pTunnelSetup)
    {
        alogd("omx_core cancel setup tunnel on port[%d]", nPort);
        eError = SUCCESS;
        goto COMP_CMD_FAIL;
    }
    if(pPortDef->eDir == COMP_DirOutput)
    {
        pTunnelSetup->nTunnelFlags = 0;
        pTunnelSetup->eSupplier = pPortBufSupplier->eBufferSupplier;
    }
    else
    {
        //Check the data compatibility between the ports using one or more GetParameter calls.
        //B checks if its input port is compatible with the output port of component A.
        COMP_PARAM_PORTDEFINITIONTYPE out_port_def;
        out_port_def.nPortIndex = nTunneledPort;
        ((MM_COMPONENTTYPE*)hTunneledComp)->GetConfig(hTunneledComp, COMP_IndexParamPortDefinition, &out_port_def);
        if(out_port_def.eDir != COMP_DirOutput)
        {
            aloge("fatal error! tunnel port index[%d] direction is not output!", nTunneledPort);
            eError = ERR_MUX_ILLEGAL_PARAM;
            goto COMP_CMD_FAIL;
        }
        pPortDef->format = out_port_def.format;

        //The component B informs component A about the final result of negotiation.
        if(pTunnelSetup->eSupplier != pPortBufSupplier->eBufferSupplier)
        {
            alogw("Low probability! use input portIndex[%d] buffer supplier[%d] as final!", nPort, pPortBufSupplier->eBufferSupplier);
            pTunnelSetup->eSupplier = pPortBufSupplier->eBufferSupplier;
        }
        COMP_PARAM_BUFFERSUPPLIERTYPE oSupplier;
        oSupplier.nPortIndex = nTunneledPort;
        ((MM_COMPONENTTYPE*)hTunneledComp)->GetConfig(hTunneledComp, COMP_IndexParamCompBufferSupplier, &oSupplier);
        oSupplier.eBufferSupplier = pTunnelSetup->eSupplier;
        ((MM_COMPONENTTYPE*)hTunneledComp)->SetConfig(hTunneledComp, COMP_IndexParamCompBufferSupplier, &oSupplier);
    }

COMP_CMD_FAIL:
    return eError;
}

//ERRORTYPE AudioRecRequstBuffer(
//        PARAM_IN  COMP_HANDLETYPE hComponent,
//        PARAM_IN  unsigned int nPortIndex,
//        PARAM_IN  COMP_BUFFERHEADERTYPE* pBuffer) {
//
//    RECRENDERDATATYPE *pRecRenderData = NULL;
//  ERRORTYPE eError = SUCCESS;
//
//  pRecRenderData
//          = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
//
//  put_data(&pRecRenderData->Audio_buffer_quene, pBuffer);
//
//ERROR:
//  return eError;
//}
//
//ERRORTYPE AudioRecReleaseBuffer(
//        PARAM_IN  COMP_HANDLETYPE hComponent,
//        PARAM_IN  unsigned int nPortIndex,
//        PARAM_IN  COMP_BUFFERHEADERTYPE* pBuffer) {
//
//    RECRENDERDATATYPE *pRecRenderData = NULL;
//  ERRORTYPE eError = SUCCESS;
//
//  pRecRenderData
//          = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
//
//ERROR:
//  return eError;
//
//}

ERRORTYPE RecRenderEmptyThisBuffer(
            PARAM_IN  COMP_HANDLETYPE hComponent,
            PARAM_IN  COMP_BUFFERHEADERTYPE* pBuffer)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);
    ERRORTYPE eError = SUCCESS;

    pthread_mutex_lock(&pRecRenderData->mStateMutex);

    // Check state
    if (  (pRecRenderData->state != COMP_StateIdle)
       && (pRecRenderData->state != COMP_StateExecuting)
       && (pRecRenderData->state != COMP_StatePause)
       )
    {
        alogd("send buffer in invalid state[0x%x]!", pRecRenderData->state);
        pthread_mutex_unlock(&pRecRenderData->mStateMutex);
        return ERR_MUX_INCORRECT_STATE_OPERATION;
    }

#ifdef __SAVE_INPUT_BS__
    EncodedStream *pOutFrame = (EncodedStream*)pBuffer->pOutputPortPrivate;
    if (pOutFrame->media_type == CDX_PacketVideo)
    {
        if (ftell(fp_bs) == 0)
        {// Write SPS/PPS info
            alogd("===== Write SPS/PPS Info");
            fwrite(pRecRenderData->venc_extradata_info.pBuffer, 1, pRecRenderData->venc_extradata_info.nLength, fp_bs);
        }
                
        fwrite(pOutFrame->pBuffer, 1, pOutFrame->nBufferLen, fp_bs);
        fwrite(pOutFrame->pBufferExtra, 1, pOutFrame->nBufferExtraLen, fp_bs);
        alogv("nBufferLen = %d nBufferExtraLen = %d KeyFrame = %d", pOutFrame->nBufferLen, pOutFrame->nBufferExtraLen, (pOutFrame->nFlags & CEDARV_FLAG_KEYFRAME) != 0);
    }
#endif

    // Notes: RecRender_Component don't support non-tunnel mode for now
    if (pBuffer->nInputPortIndex == pRecRenderData->sInPortDef[RECR_PORT_INDEX_VIDEO].nPortIndex)
    {// VENC input tunnel mode
        EncodedStream *pOutFrame = (EncodedStream*)pBuffer->pOutputPortPrivate;
        if (pOutFrame->nFilledLen == 0)
        {
            alogw("Video Pkt[ID=%d Pts=%lld] FilledLen[%d] not Match, BufferLen=%d BufferExtraLen=%d"
                ,pOutFrame->nID, pOutFrame->nTimeStamp, pOutFrame->nFilledLen, pOutFrame->nBufferLen, pOutFrame->nBufferExtraLen);
            //pOutFrame->nFilledLen = pOutFrame->nBufferLen + pOutFrame->nBufferExtraLen;
        }

        pthread_mutex_lock(&pRecRenderData->mVideoInputFrameListMutex);

        // Ensure there is Idle node
        if (list_empty(&pRecRenderData->mVideoInputFrameIdleList))
        {
            alogv("Low probability! RecRender idle frame is empty! Total Num = %d", pRecRenderData->mVideoInputFrameNum);
            ENCODER_NODE_T *pNode = (ENCODER_NODE_T*)malloc(sizeof(ENCODER_NODE_T));
            if (NULL == pNode)
            {
                pthread_mutex_unlock(&pRecRenderData->mVideoInputFrameListMutex);
                pthread_mutex_unlock(&pRecRenderData->mStateMutex);
                aloge("fatal error! malloc fail!");
                eError = ERR_MUX_NOMEM;
                return eError;
            }
            memset(pNode, 0, sizeof(ENCODER_NODE_T));
            list_add_tail(&pNode->mList, &pRecRenderData->mVideoInputFrameIdleList);
            pRecRenderData->mVideoInputFrameNum++;
        }

        // Move the node from IdelList to ReadyList
        ENCODER_NODE_T *pFirstNode = list_first_entry(&pRecRenderData->mVideoInputFrameIdleList, ENCODER_NODE_T, mList);
        memcpy(&pFirstNode->stEncodedStream, pOutFrame, sizeof(EncodedStream));
        pFirstNode->mUsedRefCnt = 0;  // clear ref count when moving it to ReadyList
        list_move_tail(&pFirstNode->mList, &pRecRenderData->mVideoInputFrameReadyList);

        // Send Input data valid message
        if (pRecRenderData->mNoInputFrameFlag)
        {
            pRecRenderData->mNoInputFrameFlag = 0;
            message_t msg;
            msg.command = RecRenderComp_InputFrameAvailable;
            put_message(&pRecRenderData->cmd_queue, &msg);
        }
        pthread_mutex_unlock(&pRecRenderData->mVideoInputFrameListMutex);
    }
    else if(pBuffer->nInputPortIndex == pRecRenderData->sInPortDef[RECR_PORT_INDEX_AUDIO].nPortIndex)
    {// AENC input tunnel mode
        EncodedStream *pOutFrame = (EncodedStream*)pBuffer->pOutputPortPrivate;
        if (pOutFrame->nFilledLen == 0)
        {
            alogw("Audio Pkt[ID=%d Pts=%lld] FilledLen[%d] not Match, BufferLen=%d BufferExtraLen=%d"
                ,pOutFrame->nID, pOutFrame->nTimeStamp, pOutFrame->nFilledLen, pOutFrame->nBufferLen, pOutFrame->nBufferExtraLen);
            //pOutFrame->nFilledLen = pOutFrame->nBufferLen + pOutFrame->nBufferExtraLen;
        }

        pthread_mutex_lock(&pRecRenderData->mAudioInputFrameListMutex);

        // Ensure there is Idle node
        if (list_empty(&pRecRenderData->mAudioInputFrameIdleList))
        {
            alogw("Low probability! RecRender idle frame is empty! Total Num = %d", pRecRenderData->mAudioInputFrameNum);
            ENCODER_NODE_T *pNode = (ENCODER_NODE_T*)malloc(sizeof(ENCODER_NODE_T));
            if (NULL == pNode)
            {
                pthread_mutex_unlock(&pRecRenderData->mAudioInputFrameListMutex);
                pthread_mutex_unlock(&pRecRenderData->mStateMutex);
                aloge("fatal error! malloc fail!");
                eError = ERR_MUX_NOMEM;
                return eError;
            }
            memset(pNode, 0, sizeof(ENCODER_NODE_T));
            list_add_tail(&pNode->mList, &pRecRenderData->mAudioInputFrameIdleList);
            pRecRenderData->mAudioInputFrameNum++;
        }

        // Move the node from IdelList to ReadyList
        ENCODER_NODE_T *pFirstNode = list_first_entry(&pRecRenderData->mAudioInputFrameIdleList, ENCODER_NODE_T, mList);
        memcpy(&pFirstNode->stEncodedStream, pOutFrame, sizeof(EncodedStream));
        pFirstNode->mUsedRefCnt = 0;  // clear ref count when moving it to ReadyList
        list_move_tail(&pFirstNode->mList, &pRecRenderData->mAudioInputFrameReadyList);

        if (pRecRenderData->mNoInputFrameFlag)
        {
            pRecRenderData->mNoInputFrameFlag = 0;
            message_t msg;
            msg.command = RecRenderComp_InputFrameAvailable;
            put_message(&pRecRenderData->cmd_queue, &msg);
        }
        pthread_mutex_unlock(&pRecRenderData->mAudioInputFrameListMutex);
    }
    else if(pBuffer->nInputPortIndex == pRecRenderData->sInPortDef[RECR_PORT_INDEX_TEXT].nPortIndex)
    {
        EncodedStream    *pOutFrame = (EncodedStream*)pBuffer->pOutputPortPrivate;
        if (pOutFrame->nFilledLen == 0)
        {
            alogw("text Pkt[ID=%d Pts=%lld] FilledLen[%d] not Match, BufferLen=%d BufferExtraLen=%d"
                ,pOutFrame->nID, pOutFrame->nTimeStamp, pOutFrame->nFilledLen, pOutFrame->nBufferLen, pOutFrame->nBufferExtraLen);
            //pOutFrame->nFilledLen = pOutFrame->nBufferLen + pOutFrame->nBufferExtraLen;
        }

        pthread_mutex_lock(&pRecRenderData->mTextInputFrameListMutex);

        if (list_empty(&pRecRenderData->mTextInputFrameIdleList))
        {
            alogw("Low probability! RecRender idle frame is empty! Total Num = %d", pRecRenderData->mAudioInputFrameNum);
            ENCODER_NODE_T *pNode = (ENCODER_NODE_T*)malloc(sizeof(ENCODER_NODE_T));
            if (NULL == pNode)
            {
                pthread_mutex_unlock(&pRecRenderData->mTextInputFrameListMutex);
                pthread_mutex_unlock(&pRecRenderData->mStateMutex);
                aloge("fatal error! malloc fail!");
                eError = ERR_MUX_NOMEM;
                return eError;
            }
            memset(pNode, 0, sizeof(ENCODER_NODE_T));
            list_add_tail(&pNode->mList, &pRecRenderData->mTextInputFrameIdleList);
            pRecRenderData->mTextInputFrameNum++;
        }
        
        // Move the node from IdelList to ReadyList
        ENCODER_NODE_T *pFirstNode = list_first_entry(&pRecRenderData->mTextInputFrameIdleList, ENCODER_NODE_T, mList);
        memcpy(&pFirstNode->stEncodedStream, pOutFrame, sizeof(EncodedStream));
        pFirstNode->mUsedRefCnt = 0;  // clear ref count when moving it to ReadyList 
        
        list_move_tail(&pFirstNode->mList, &pRecRenderData->mTextInputFrameReadyList);
        if(pRecRenderData->mNoInputFrameFlag)
        {
            pRecRenderData->mNoInputFrameFlag = 0;
            message_t msg;
            msg.command = RecRenderComp_InputFrameAvailable;
            put_message(&pRecRenderData->cmd_queue, &msg);
        }
        pthread_mutex_unlock(&pRecRenderData->mTextInputFrameListMutex);
    }
    else
    {
        aloge("fatal error! inputPortIndex[%d] match nothing!", pBuffer->nInputPortIndex);
    }
    pthread_mutex_unlock(&pRecRenderData->mStateMutex);
    return eError;
}

static ERRORTYPE RecRenderFlushCacheData(RECRENDERDATATYPE *pRecRenderData)
{
    ERRORTYPE ret = SUCCESS;
    int nPacketCount;
    int nCounter = 0;
    RecSinkPacket   *pCacheRSPacket;
    RecSink *pEntry;

    if (pRecRenderData->cache_manager == NULL) return SUCCESS;
    if (!pRecRenderData->mbSdCardState) return SUCCESS;

    nPacketCount = pRecRenderData->cache_manager->GetReadyPacketCount((COMP_HANDLETYPE)pRecRenderData->cache_manager);
    alogd("cacheManager left [%d]packets wait to flush", nPacketCount);
    pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
    while (1)
    {
        //if has callback muxer, must call its emptyThisBuffer() to send packet now.
        if (pRecRenderData->cache_manager->GetPacket((COMP_HANDLETYPE)pRecRenderData->cache_manager, &pCacheRSPacket) != SUCCESS)
        {
            break;
        }
        nCounter++;
#if 0
        //send frame to all RecSinks
        list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
        {
            if(!pEntry->nCallbackOutFlag)
            {
                pRecRenderData->cache_manager->RefPacket(pRecRenderData->cache_manager, pCacheRSPacket->mId);
                if(pEntry->EmptyThisBuffer(pEntry, pCacheRSPacket) == SUCCESS)
                {
                }
                else
                {
                    aloge("fatal error! RecSink muxerId[%d] empty this buffer fail!", pEntry->mMuxerId);
                    pRecRenderData->cache_manager->ReleasePacket(pRecRenderData->cache_manager, pCacheRSPacket->mId);
                }
            }
        }
        list_for_each_entry(pEntry, &pRecRenderData->mSwitchingSinkInfoList, mList)
        {
            if(!pEntry->nCallbackOutFlag)
            {
                pRecRenderData->cache_manager->RefPacket(pRecRenderData->cache_manager, pCacheRSPacket->mId);
                if(pEntry->EmptyThisBuffer(pEntry, pCacheRSPacket) == SUCCESS)
                {
                }
                else
                {
                    aloge("fatal error! RecSink muxerId[%d] empty this buffer fail!", pEntry->mMuxerId);
                    pRecRenderData->cache_manager->ReleasePacket(pRecRenderData->cache_manager, pCacheRSPacket->mId);
                }
            }
        }
        pRecRenderData->cache_manager->ReleasePacket(pRecRenderData->cache_manager, pCacheRSPacket->mId);
#endif
    }
    pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
    if(nPacketCount != nCounter)
    {
        aloge("fatal error! cacheManger packet count exception! [%d]!=[%d]", nPacketCount, nCounter);
    }
    return ret;
}

ERRORTYPE RecRenderComponentDeInit(PARAM_IN COMP_HANDLETYPE hComponent)
{
    RECRENDERDATATYPE *pRecRenderData;
    ERRORTYPE eError = SUCCESS;
    CompInternalMsgType eCmd = Stop;
    message_t msg;
    //int ret;
    int i;
    RecSink  *pSinkInfo = NULL;
    RecSink  *pTmpEntry = NULL;
    //ERRORTYPE omx_ret;
    memset(&msg, 0, sizeof(message_t));
    pRecRenderData = (RECRENDERDATATYPE *) (((MM_COMPONENTTYPE*) hComponent)->pComponentPrivate);

    pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
    i=0;
    list_for_each_entry_safe(pSinkInfo, pTmpEntry, &pRecRenderData->mValidSinkInfoList, mList)
    {
        pSinkInfo->Reset(pSinkInfo);
        RecSinkDestroy(pSinkInfo);
        list_move_tail(&pSinkInfo->mList, &pRecRenderData->mIdleSinkInfoList);
        pRecRenderData->mSinkInfoValidNum--;
        i++;
    }
    list_for_each_entry_safe(pSinkInfo, pTmpEntry, &pRecRenderData->mSwitchingSinkInfoList, mList)
    {
        pSinkInfo->Reset(pSinkInfo);
        RecSinkDestroy(pSinkInfo);
        list_move_tail(&pSinkInfo->mList, &pRecRenderData->mIdleSinkInfoList);
        pRecRenderData->mSinkInfoValidNum--;
        i++;
    }
    if(pRecRenderData->mSinkInfoValidNum != 0)
    {
        aloge("fatal error! valid sink info[%d]!=0", pRecRenderData->mSinkInfoValidNum);
    }
    pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);   // ???这个Mux没有销毁？

    // should already release all video frame
    int iIdleVideoFrameCnt = 0;
    struct list_head *pList;
    list_for_each(pList, &pRecRenderData->mVideoInputFrameIdleList)
    {
        iIdleVideoFrameCnt++;
    }
    if (iIdleVideoFrameCnt < pRecRenderData->mVideoInputFrameNum)
    {
        aloge("fatal error! inputFrames[%d]<[%d] must return all before!", iIdleVideoFrameCnt, pRecRenderData->mVideoInputFrameNum);
    }
    if (!list_empty(&pRecRenderData->mVideoInputFrameReadyList))
    {
        aloge("fatal error! why readyInputFrame is not empty?");
    }
    if (!list_empty(&pRecRenderData->mVideoInputFrameUsedList))
    {
        aloge("fatal error! why usedInputFrame is not empty?");
    }
    if (!list_empty(&pRecRenderData->mVideoInputFrameIdleList))
    {
        ENCODER_NODE_T *pEntry, *pTmp;
        list_for_each_entry_safe(pEntry, pTmp, &pRecRenderData->mVideoInputFrameIdleList, mList)
        {
            list_del(&pEntry->mList);
            free(pEntry);
        }
    }
    if(pRecRenderData->mVideoInputFrameNum != MUX_FIFO_LEVEL)
    {
        alogw("Low probability! RecRender idle frame Total Num: %d -> %d", MUX_FIFO_LEVEL, pRecRenderData->mVideoInputFrameNum);
    }
    // should already release all audio frame
    int iIdleAudioFrameCnt = 0;
    list_for_each(pList, &pRecRenderData->mAudioInputFrameIdleList)
    {
        iIdleAudioFrameCnt++;
    }
    if (iIdleAudioFrameCnt < pRecRenderData->mAudioInputFrameNum)
    {
        aloge("fatal error! inputFrames[%d]<[%d] must return all before!", iIdleAudioFrameCnt, pRecRenderData->mAudioInputFrameNum);
    }
    if (!list_empty(&pRecRenderData->mAudioInputFrameReadyList))
    {
        aloge("fatal error! why readyInputFrame is not empty?");
    }
    if (!list_empty(&pRecRenderData->mAudioInputFrameUsedList))
    {
        aloge("fatal error! why usedInputFrame is not empty?");
    }
    if (!list_empty(&pRecRenderData->mAudioInputFrameIdleList))
    {
        ENCODER_NODE_T *pEntry, *pTmp;
        list_for_each_entry_safe(pEntry, pTmp, &pRecRenderData->mAudioInputFrameIdleList, mList)
        {
            list_del(&pEntry->mList);
            free(pEntry);
        }
    }

    // should already release all text frame
    int iIdleTextFrameCnt = 0;
    list_for_each(pList, &pRecRenderData->mTextInputFrameIdleList)
    {
        iIdleTextFrameCnt++;
    }
    if (iIdleTextFrameCnt < pRecRenderData->mTextInputFrameNum)
    {
        aloge("fatal error! inputFrames[%d]<[%d] must return all before!", iIdleTextFrameCnt, pRecRenderData->mTextInputFrameNum);
    }
    if (!list_empty(&pRecRenderData->mTextInputFrameReadyList))
    {
        aloge("fatal error! why readyInputFrame is not empty?");
    }
    if (!list_empty(&pRecRenderData->mTextInputFrameUsedList))
    {
        aloge("fatal error! why usedInputFrame is not empty?");
    }
    if (!list_empty(&pRecRenderData->mTextInputFrameIdleList))
    {
        ENCODER_NODE_T *pEntry, *pTmp;
        list_for_each_entry_safe(pEntry, pTmp, &pRecRenderData->mTextInputFrameIdleList, mList)
        {
            list_del(&pEntry->mList);
            free(pEntry);
        }
    }

    msg.command = eCmd;
    put_message(&pRecRenderData->cmd_queue, &msg);
    //cdx_sem_up(&pRecRenderData->cdx_sem_wait_message);

    alogv("wait recorder render component exit!...");
    // Wait for thread to exit so we can get the status into "error"
    pthread_join(pRecRenderData->thread_id, (void*) &eError);

    //cdx_sem_deinit(&pRecRenderData->cdx_sem_wait_message);
    message_destroy(&pRecRenderData->cmd_queue);
    //data_quene_destroy(&pRecRenderData->Audio_buffer_quene);

    if (pRecRenderData->cache_manager != NULL)
    {
        RsPacketCacheManagerDestruct(pRecRenderData->cache_manager);
        pRecRenderData->cache_manager = NULL;
    }

#ifdef __SAVE_INPUT_BS__
    fclose(fp_bs);
#endif

    if(pRecRenderData->venc_extradata_info.pBuffer)
    {
        free(pRecRenderData->venc_extradata_info.pBuffer);
        pRecRenderData->venc_extradata_info.pBuffer = NULL;
    }

    pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
    int nodeNum = 0;
    if(!list_empty(&pRecRenderData->mIdleSinkInfoList))
    {
        RecSink *pEntry, *pTmp;
        list_for_each_entry_safe(pEntry, pTmp, &pRecRenderData->mIdleSinkInfoList, mList)
        {
            list_del(&pEntry->mList);
            free(pEntry);
            nodeNum++;
        }
    }
    if(nodeNum!=pRecRenderData->mSinkInfoTotalNum)
    {
        aloge("fatal error! frame node number is not match[%d][%d]", nodeNum, pRecRenderData->mSinkInfoTotalNum);
    }
    pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);

    if(!list_empty(&pRecRenderData->mChnAttrList))
    {
        MuxChanAttrNode *pEntry, *pTmp;
        list_for_each_entry_safe(pEntry, pTmp, &pRecRenderData->mChnAttrList, mList)
        {
            list_del(&pEntry->mList);
            free(pEntry);
        }
    }
    pthread_mutex_destroy(&pRecRenderData->mVideoInputFrameListMutex);
    pthread_mutex_destroy(&pRecRenderData->mAudioInputFrameListMutex);
    pthread_mutex_destroy(&pRecRenderData->mTextInputFrameListMutex);
    pthread_mutex_destroy(&pRecRenderData->mSinkInfoListMutex);
    pthread_mutex_destroy(&pRecRenderData->mStateMutex);

    if(pRecRenderData)
    {
        free(pRecRenderData);
        pRecRenderData = NULL;
    }
    //alogd("[%s]sync_1", strrchr(__FILE__, '/')+1);
    //sync();
    //alogd("[%s]sync_2", strrchr(__FILE__, '/')+1);
    alogd("recorder render component exited!");

    return eError;
}

/*****************************************************************************/
ERRORTYPE RecRenderComponentInit(PARAM_IN COMP_HANDLETYPE hComponent)
{
    MM_COMPONENTTYPE *pComp;
    RECRENDERDATATYPE *pRecRenderData;
    ERRORTYPE eError = SUCCESS;
    unsigned int err;
    int i;
    pComp = (MM_COMPONENTTYPE *) hComponent;

    // Create private data
    pRecRenderData = (RECRENDERDATATYPE *) malloc(sizeof(RECRENDERDATATYPE));
    memset(pRecRenderData, 0x0, sizeof(RECRENDERDATATYPE));

    pComp->pComponentPrivate = (void*) pRecRenderData;
    pRecRenderData->state = COMP_StateLoaded;
    pRecRenderData->hSelf = hComponent;
    pRecRenderData->mbSdCardState = TRUE;
    pRecRenderData->mnBasePts = -1;
    pRecRenderData->mnAudioBasePts = -1;
    pRecRenderData->mnTextBasePts = -1;
    pRecRenderData->duration = 0;
    pRecRenderData->duration_audio = 0;
    pRecRenderData->duration_text = 0;
    pRecRenderData->mRecordFileDurationPolicy = RecordFileDurationPolicy_AverageDuration;//
    err = pthread_mutex_init(&pRecRenderData->mStateMutex, NULL);
    if(err!=0)
    {
        aloge("pthread mutex init fail!");
        eError = ERR_MUX_NOMEM;
        goto EXIT;
    }
    INIT_LIST_HEAD(&pRecRenderData->mChnAttrList);
    INIT_LIST_HEAD(&pRecRenderData->mValidSinkInfoList);
    INIT_LIST_HEAD(&pRecRenderData->mSwitchingSinkInfoList);
    INIT_LIST_HEAD(&pRecRenderData->mIdleSinkInfoList);
    for(i=0;i<SINKINFO_SIZE;i++)
    {
        RecSink *pNode = (RecSink*)malloc(sizeof(RecSink));
        if(NULL == pNode)
        {
            aloge("fatal error! malloc fail[%s]!", strerror(errno));
            break;
        }
        memset(pNode, 0, sizeof(RecSink));
        list_add_tail(&pNode->mList, &pRecRenderData->mIdleSinkInfoList);
        pRecRenderData->mSinkInfoTotalNum++;
    }
    err = pthread_mutex_init(&pRecRenderData->mSinkInfoListMutex, NULL);
    if(err!=0)
    {
        aloge("pthread mutex init fail!");
        eError = ERR_MUX_NOMEM;
        goto EXIT;
    }

    // Init video output frame buffer
    INIT_LIST_HEAD(&pRecRenderData->mVideoInputFrameIdleList);
    INIT_LIST_HEAD(&pRecRenderData->mVideoInputFrameReadyList);
    INIT_LIST_HEAD(&pRecRenderData->mVideoInputFrameUsedList);
    for (int i = 0; i < MUX_FIFO_LEVEL; i++)
    {
        ENCODER_NODE_T *pNode = (ENCODER_NODE_T*)malloc(sizeof(ENCODER_NODE_T));
        if (NULL == pNode)
        {
            aloge("fatal error! malloc fail!");
            eError = ERR_MUX_NOMEM;
            goto EXIT;
        }
        memset(pNode, 0, sizeof(ENCODER_NODE_T));
        list_add_tail(&pNode->mList, &pRecRenderData->mVideoInputFrameIdleList);
        pRecRenderData->mVideoInputFrameNum++;
    }

    err = pthread_mutex_init(&pRecRenderData->mVideoInputFrameListMutex, NULL);
    if (err!=0)
    {
        aloge("pthread mutex init fail!");
        eError = ERR_MUX_NOMEM;
        goto EXIT;
    }

    // Init audio output frame buffer
    INIT_LIST_HEAD(&pRecRenderData->mAudioInputFrameIdleList);
    INIT_LIST_HEAD(&pRecRenderData->mAudioInputFrameReadyList);
    INIT_LIST_HEAD(&pRecRenderData->mAudioInputFrameUsedList);
    for (int i = 0; i < MUX_FIFO_LEVEL; i++)
    {
        ENCODER_NODE_T *pNode = (ENCODER_NODE_T*)malloc(sizeof(ENCODER_NODE_T));
        if (NULL == pNode)
        {
            aloge("fatal error! malloc fail!");
            eError = ERR_MUX_NOMEM;
            goto EXIT;
        }
        memset(pNode, 0, sizeof(ENCODER_NODE_T));
        list_add_tail(&pNode->mList, &pRecRenderData->mAudioInputFrameIdleList);
        pRecRenderData->mAudioInputFrameNum++;
    }

    err = pthread_mutex_init(&pRecRenderData->mAudioInputFrameListMutex, NULL);
    if(err!=0)
    {
        aloge("pthread mutex init fail!");
        eError = ERR_MUX_NOMEM;
        goto EXIT;
    }

    // Init text output frame buffer
    INIT_LIST_HEAD(&pRecRenderData->mTextInputFrameIdleList);
    INIT_LIST_HEAD(&pRecRenderData->mTextInputFrameReadyList);
    INIT_LIST_HEAD(&pRecRenderData->mTextInputFrameUsedList);
    for (int i = 0; i < MUX_FIFO_LEVEL; i++)
    {
        ENCODER_NODE_T *pNode = (ENCODER_NODE_T*)malloc(sizeof(ENCODER_NODE_T));
        if (NULL == pNode)
        {
            aloge("fatal error! malloc fail!");
            eError = ERR_MUX_NOMEM;
            goto EXIT;
        }
        memset(pNode, 0, sizeof(ENCODER_NODE_T));
        list_add_tail(&pNode->mList, &pRecRenderData->mTextInputFrameIdleList);
        pRecRenderData->mTextInputFrameNum++;
    }

    err = pthread_mutex_init(&pRecRenderData->mTextInputFrameListMutex, NULL);
    if(err!=0)
    {
        aloge("pthread mutex init fail!");
        eError = ERR_MUX_NOMEM;
        goto EXIT;
    }

    pRecRenderData->mNoInputFrameFlag = 0;
    // Fill in function pointers
    pComp->SetCallbacks = RecRenderSetCallbacks;
    pComp->SendCommand  = RecRenderSendCommand;
    pComp->GetConfig = RecRenderGetConfig;  //StubGetConfig
    pComp->SetConfig = RecRenderSetConfig;  //StubSetConfig
    pComp->GetState = RecRenderGetState;
    pComp->ComponentTunnelRequest = RecRenderComponentTunnelRequest;
    pComp->EmptyThisBuffer = RecRenderEmptyThisBuffer;
    pComp->ComponentDeInit = RecRenderComponentDeInit;

    // Initialize component data structures to default values
    pRecRenderData->sPortParam.nPorts = 0;
    pRecRenderData->sPortParam.nStartPortNumber = 0x0;

    pRecRenderData->sPortBufSupplier[RECR_PORT_INDEX_VIDEO].nPortIndex = 0x0;
    pRecRenderData->sPortBufSupplier[RECR_PORT_INDEX_VIDEO].eBufferSupplier = COMP_BufferSupplyOutput;
    pRecRenderData->sPortBufSupplier[RECR_PORT_INDEX_AUDIO].nPortIndex = 0x1;
    pRecRenderData->sPortBufSupplier[RECR_PORT_INDEX_AUDIO].eBufferSupplier = COMP_BufferSupplyOutput;
    pRecRenderData->sPortBufSupplier[RECR_PORT_INDEX_TEXT].nPortIndex = 0x2;
    pRecRenderData->sPortBufSupplier[RECR_PORT_INDEX_TEXT].eBufferSupplier = COMP_BufferSupplyOutput;

    pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].nPortIndex = 0x0;
    pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].eTunnelType = TUNNEL_TYPE_COMMON;
    pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].nPortIndex = 0x1;
    pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].eTunnelType = TUNNEL_TYPE_COMMON;
    pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].nPortIndex = 0x2;
    pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].eTunnelType = TUNNEL_TYPE_COMMON;

    pRecRenderData->sInPortDef[RECR_PORT_INDEX_VIDEO].nPortIndex = pRecRenderData->sPortParam.nPorts;
    pRecRenderData->sInPortDef[RECR_PORT_INDEX_VIDEO].eDir = COMP_DirInput;
    pRecRenderData->sInPortDef[RECR_PORT_INDEX_VIDEO].bEnabled = TRUE;
    pRecRenderData->sInPortDef[RECR_PORT_INDEX_VIDEO].eDomain = COMP_PortDomainVideo;
    pRecRenderData->sPortParam.nPorts++;

    pRecRenderData->sInPortDef[RECR_PORT_INDEX_AUDIO].nPortIndex = pRecRenderData->sPortParam.nPorts;
    pRecRenderData->sInPortDef[RECR_PORT_INDEX_AUDIO].eDir = COMP_DirInput;
    pRecRenderData->sInPortDef[RECR_PORT_INDEX_AUDIO].bEnabled = TRUE;
    pRecRenderData->sInPortDef[RECR_PORT_INDEX_AUDIO].eDomain = COMP_PortDomainAudio;
    pRecRenderData->sPortParam.nPorts++;

    pRecRenderData->sInPortDef[RECR_PORT_INDEX_TEXT].nPortIndex = pRecRenderData->sPortParam.nPorts;
    pRecRenderData->sInPortDef[RECR_PORT_INDEX_TEXT].eDir = COMP_DirInput;
    pRecRenderData->sInPortDef[RECR_PORT_INDEX_TEXT].bEnabled = TRUE;
    pRecRenderData->sInPortDef[RECR_PORT_INDEX_TEXT].eDomain = COMP_PortDomainText;
    pRecRenderData->sPortParam.nPorts++;

    if(message_create(&pRecRenderData->cmd_queue) < 0)
    {
        aloge("message error!");
        eError = ERR_MUX_NOMEM;
        goto EXIT;
    }
#ifdef __SAVE_INPUT_BS__
    fp_bs = fopen("/data/input_video.bin", "wb");
#endif
    pRecRenderData->callback_writer.parent = (void*)pRecRenderData;
    pRecRenderData->callback_writer.writer = CDXRecoder_WritePacket_CB;
    // Create the component thread
    err = pthread_create(&pRecRenderData->thread_id, NULL, RecRender_ComponentThread, pRecRenderData);
    if (err)
    {
        eError = ERR_MUX_NOMEM;
        goto EXIT;
    }
    alogd("create recRender comp threadId[0x%lx]", pRecRenderData->thread_id);
    EXIT: return eError;
}
/*
void adjustAvsCounter(void* pThreadData)
{
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE*) pThreadData;
    int         mediaTimediff;
    OMX_TICKS       now_avs_counter;

    // avs_counter_get_time_us(&now_avs_counter);
    pRecRenderData->avs_counter->get_time(pRecRenderData->avs_counter, &now_avs_counter);
    alogv("now_avs_counter: %lld", now_avs_counter);

    mediaTimediff = (int)(pRecRenderData->duration - pRecRenderData->duration_audio);
    if(mediaTimediff > 100 || mediaTimediff < -100)
    {
#if 0
        int adjust_ratio;
        adjust_ratio = mediaTimediff / (AVS_ADJUST_PERIOD_MS/100);
        adjust_ratio = adjust_ratio >  1 ? 1 : adjust_ratio;
        adjust_ratio = adjust_ratio < -1 ? -1 : adjust_ratio;

        if(adjust_ratio){
            avs_counter_adjust(adjust_ratio);
        }
#else
        int adjust_ratio;
        adjust_ratio = mediaTimediff / (AVS_ADJUST_PERIOD_MS/100);
        adjust_ratio = adjust_ratio >  2 ? 2 : adjust_ratio;
        adjust_ratio = adjust_ratio < -2 ? -2 : adjust_ratio;

        // avs_counter_adjust_abs(adjust_ratio);
        pRecRenderData->avs_counter->adjust(pRecRenderData->avs_counter, adjust_ratio);
#endif
        alogd("----adjust ratio:%d, video:%lld audio:%lld diff:%d diff-percent:%d ----",
                adjust_ratio, pRecRenderData->duration, pRecRenderData->duration_audio,
                mediaTimediff, mediaTimediff / (AVS_ADJUST_PERIOD_MS/100));
    }
    else
    {
        //avs_counter_adjust_abs(0);
        pRecRenderData->avs_counter->adjust(pRecRenderData->avs_counter, 0);
        alogd("----adjust ratio: 0, video: %lld(ms), auido: %lld(ms), diff: %lld(ms)",
            pRecRenderData->duration, pRecRenderData->duration_audio, pRecRenderData->duration - pRecRenderData->duration_audio);
    }
}
*/
/*****************************************************************************/
static void* RecRender_ComponentThread(void* pThreadData)
{
    //int ret = SUCCESS;
    //int track_index = -1;
    //int track_process_num = 0;
    int buffer_fail_flags = 0;
    unsigned int cmddata;
    CompInternalMsgType cmd;
    RECRENDERDATATYPE *pRecRenderData = (RECRENDERDATATYPE*) pThreadData;
    message_t cmd_msg;
    //COMP_BUFFERHEADERTYPE omx_buffer_header;
    //COMP_BUFFERHEADERTYPE * pAudioHeader;
    //VencOutputBuffer data_ctrl;
    //MM_COMPONENTTYPE *pTunnelComp;
    //COMP_INTERNAL_TUNNELINFOTYPE *pTunnelInfo;
    //COMP_BUFFERHEADERTYPE *p_omx_buffer;
    //AVPacket packet;
    //int64_t origin_pts/*, prev_pts[MAX_TRACK_COUNT],base_pts[MAX_TRACK_COUNT]*/;    //prev_pts[],base_pts[] are deprecated.
    RecSinkPacket   RSPacket;
    RecSinkPacket   *pCacheRSPacket;
    ERRORTYPE eRet;
    alogv("Recorder Render ComponentThread start run...");
    prctl(PR_SET_NAME, (unsigned long)"MuxerComp", 0, 0, 0);

    while (1)
    {
PROCESS_MESSAGE:
        if(get_message(&pRecRenderData->cmd_queue, &cmd_msg) == 0)
        {
            cmd = cmd_msg.command;
            cmddata = (unsigned int)cmd_msg.para0;

            alogv("RecRender ComponentThread get_message cmd:%d", cmd);

            // State transition command
            if (cmd == SetState)
            {
                pthread_mutex_lock(&pRecRenderData->mStateMutex);
                // If the parameter states a transition to the same state
                // raise a same state transition error.
                if (pRecRenderData->state == (COMP_STATETYPE) (cmddata))
                    pRecRenderData->pCallbacks->EventHandler(
                            pRecRenderData->hSelf,
                            pRecRenderData->pAppData,
                            COMP_EventError,
                            ERR_MUX_SAMESTATE,
                            0,
                            NULL);
                else
                {
                    // transitions/callbacks made based on state transition table
                    // cmddata contains the target state
                    switch ((COMP_STATETYPE) (cmddata))
                    {
                        case COMP_StateInvalid:
                        {
                            pRecRenderData->state = COMP_StateInvalid;
                            pRecRenderData->pCallbacks->EventHandler(
                                    pRecRenderData->hSelf,
                                    pRecRenderData->pAppData,
                                    COMP_EventError,
                                    ERR_MUX_INVALIDSTATE,
                                    0,
                                    NULL);
                            pRecRenderData->pCallbacks->EventHandler(
                                    pRecRenderData->hSelf,
                                    pRecRenderData->pAppData,
                                    COMP_EventCmdComplete,
                                    COMP_CommandStateSet,
                                    pRecRenderData->state,
                                    NULL);
                            break;
                        }
                        case COMP_StateLoaded:
                        {
                            if (pRecRenderData->state != COMP_StateIdle)
                            {
                                aloge("fatal error! curState[0x%x] is wrong", pRecRenderData->state);
                                pRecRenderData->pCallbacks->EventHandler(
                                        pRecRenderData->hSelf,
                                        pRecRenderData->pAppData,
                                        COMP_EventError,
                                        ERR_MUX_INCORRECT_STATE_TRANSITION,
                                        0,
                                        NULL);
                            }
                            alogv("RecRender set state LOADED");
                            RecRenderFlushCacheData(pRecRenderData);
                            //stop all RecSinks,
                            RecSink *pRecSinkEntry;
                            list_for_each_entry(pRecSinkEntry, &pRecRenderData->mValidSinkInfoList, mList)
                            {
                                pRecSinkEntry->Reset(pRecSinkEntry);
                            }
                            list_for_each_entry(pRecSinkEntry, &pRecRenderData->mSwitchingSinkInfoList, mList)
                            {
                                pRecSinkEntry->Reset(pRecSinkEntry);
                            }

                            //release all frames.
                            alogd("release all frames to VEnc, AEnc and TEnc, when state[0x%x]->[0x%x]", pRecRenderData->state, COMP_StateLoaded);
                            pthread_mutex_lock(&pRecRenderData->mVideoInputFrameListMutex);
                            ENCODER_NODE_T *pVEntry, *pVTmp;
                            if (!list_empty(&pRecRenderData->mVideoInputFrameUsedList))
                            {
                                aloge("fatal error:Used video frame should all released before!");

                                list_for_each_entry_safe(pVEntry, pVTmp, &pRecRenderData->mVideoInputFrameUsedList, mList)
                                {
                                    aloge("RefCnt[%d]", pVEntry->mUsedRefCnt);
                                    list_move_tail(&pVEntry->mList, &pRecRenderData->mVideoInputFrameIdleList);

                                    MM_COMPONENTTYPE *pInPortTunnelComp = (MM_COMPONENTTYPE*)(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].hTunnel);
                                    COMP_BUFFERHEADERTYPE obh;
                                    obh.nOutputPortIndex   = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].nTunnelPortIndex;
                                    obh.nInputPortIndex    = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].nPortIndex;
                                    obh.pOutputPortPrivate = (void*)pVEntry;
                                    eRet = pInPortTunnelComp->FillThisBuffer(pInPortTunnelComp, &obh);
                                    if(eRet != SUCCESS)
                                    {
                                        aloge("fatal error! fill this buffer fail[0x%x], video frame id=[%d], discard it!", eRet, pVEntry->stEncodedStream.nID);
                                    }
                                }
                            }
                            if (!list_empty(&pRecRenderData->mVideoInputFrameReadyList))
                            {
                                alogd("Ready video frame should all release before!");

                                list_for_each_entry_safe(pVEntry, pVTmp, &pRecRenderData->mVideoInputFrameReadyList, mList)
                                {
                                    alogw("RefCnt[%d]", pVEntry->mUsedRefCnt);
                                    list_move_tail(&pVEntry->mList, &pRecRenderData->mVideoInputFrameIdleList);

                                    MM_COMPONENTTYPE *pInPortTunnelComp = (MM_COMPONENTTYPE*)(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].hTunnel);
                                    COMP_BUFFERHEADERTYPE obh;
                                    obh.nOutputPortIndex   = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].nTunnelPortIndex;
                                    obh.nInputPortIndex    = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].nPortIndex;
                                    obh.pOutputPortPrivate = (void*)pVEntry;
                                    eRet = pInPortTunnelComp->FillThisBuffer(pInPortTunnelComp, &obh);
                                    if (eRet != SUCCESS)
                                    {
                                        aloge("fatal error! fill this buffer fail[0x%x], video frame id=[%d], discard it!", eRet, pVEntry->stEncodedStream.nID);
                                    }
                                }
                            }

                            int iIdleVideoFrameCnt = 0;
                            list_for_each_entry(pVEntry, &pRecRenderData->mVideoInputFrameIdleList, mList)
                            {
                                iIdleVideoFrameCnt++;
                            }
                            if (iIdleVideoFrameCnt != pRecRenderData->mVideoInputFrameNum)
                            {
                                aloge("fatal error! video input frames [%d]<[%d] must return all before", iIdleVideoFrameCnt, pRecRenderData->mVideoInputFrameNum);
                            }
                            else
                            {
                                alogd("Release all video input frame [%d]", pRecRenderData->mVideoInputFrameNum);
                            }
                            pthread_mutex_unlock(&pRecRenderData->mVideoInputFrameListMutex);

                            pthread_mutex_lock(&pRecRenderData->mAudioInputFrameListMutex);
                            ENCODER_NODE_T *pAEntry, *pATmp;
                            if (!list_empty(&pRecRenderData->mAudioInputFrameUsedList))
                            {
                                aloge("fatal error! used audio frame should all released before!");

                                list_for_each_entry_safe(pAEntry, pATmp, &pRecRenderData->mAudioInputFrameUsedList, mList)
                                {
                                    aloge("RefCnt[%d]", pAEntry->mUsedRefCnt);
                                    list_move_tail(&pAEntry->mList, &pRecRenderData->mAudioInputFrameIdleList);
                                    MM_COMPONENTTYPE *pInPortTunnelComp = (MM_COMPONENTTYPE*)(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].hTunnel);
                                    COMP_BUFFERHEADERTYPE obh;
                                    obh.nOutputPortIndex   = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].nTunnelPortIndex;
                                    obh.nInputPortIndex    = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].nPortIndex;
                                    obh.pOutputPortPrivate = (void*)pAEntry;
                                    eRet = pInPortTunnelComp->FillThisBuffer(pInPortTunnelComp, &obh);
                                    if(eRet != SUCCESS)
                                    {
                                        aloge("fatal error! fill this buffer fail[0x%x], audio frame id=[%d], discard it!", eRet, pAEntry->stEncodedStream.nID);
                                    }
                                }
                            }
                            if (!list_empty(&pRecRenderData->mAudioInputFrameReadyList))
                            {
                                alogd("Ready audio frame should all released before!");

                                list_for_each_entry_safe(pAEntry, pATmp, &pRecRenderData->mAudioInputFrameReadyList, mList)
                                {
                                    alogw("RefCnt[%d]", pAEntry->mUsedRefCnt);
                                    list_move_tail(&pAEntry->mList, &pRecRenderData->mAudioInputFrameIdleList);
                                    MM_COMPONENTTYPE *pInPortTunnelComp = (MM_COMPONENTTYPE*)(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].hTunnel);
                                    COMP_BUFFERHEADERTYPE obh;
                                    obh.nOutputPortIndex   = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].nTunnelPortIndex;
                                    obh.nInputPortIndex    = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].nPortIndex;
                                    obh.pOutputPortPrivate = (void*)pAEntry;
                                    eRet = pInPortTunnelComp->FillThisBuffer(pInPortTunnelComp, &obh);
                                    if (eRet != SUCCESS)
                                    {
                                        aloge("fatal error! fill this buffer fail[0x%x], audio frame id=[%d], discard it!", eRet, pAEntry->stEncodedStream.nID);
                                    }
                                }
                            }

                            int iIdleAudioFrameCnt = 0;
                            list_for_each_entry(pAEntry, &pRecRenderData->mAudioInputFrameIdleList, mList)
                            {
                                iIdleAudioFrameCnt++;
                            }
                            if (iIdleAudioFrameCnt != pRecRenderData->mAudioInputFrameNum)
                            {
                                aloge("fatal error! audio input frames [%d]<[%d] must return all before", iIdleAudioFrameCnt, pRecRenderData->mAudioInputFrameNum);
                            }
                            else
                            {
                                alogd("Release all audio input frame [%d]", pRecRenderData->mAudioInputFrameNum);
                            }
                            pthread_mutex_unlock(&pRecRenderData->mAudioInputFrameListMutex);

                            pthread_mutex_lock(&pRecRenderData->mTextInputFrameListMutex);
                            ENCODER_NODE_T *pTEntry, *pTTmp;
                            if (!list_empty(&pRecRenderData->mTextInputFrameUsedList))
                            {
                                aloge("fatal error! used text frame should all released before!");

                                list_for_each_entry_safe(pTEntry, pTTmp, &pRecRenderData->mTextInputFrameUsedList, mList)
                                {
                                    aloge("RefCnt[%d]", pTEntry->mUsedRefCnt);
                                    list_move_tail(&pTEntry->mList, &pRecRenderData->mTextInputFrameIdleList);
                                    MM_COMPONENTTYPE *pInPortTunnelComp = (MM_COMPONENTTYPE*)(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].hTunnel);
                                    COMP_BUFFERHEADERTYPE obh;
                                    obh.nOutputPortIndex   = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].nTunnelPortIndex;
                                    obh.nInputPortIndex    = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].nPortIndex;
                                    obh.pOutputPortPrivate = (void*)pTEntry;
                                    eRet = pInPortTunnelComp->FillThisBuffer(pInPortTunnelComp, &obh);
                                    if(eRet != SUCCESS)
                                    {
                                        aloge("fatal error! fill this buffer fail[0x%x], text frame id=[%d], discard it!", eRet, pTEntry->stEncodedStream.nID);
                                    }
                                }
                            }
                            if (!list_empty(&pRecRenderData->mTextInputFrameReadyList))
                            {
                                alogd("Ready text frame should all released before!");

                                list_for_each_entry_safe(pTEntry, pTTmp, &pRecRenderData->mTextInputFrameReadyList, mList)
                                {
                                    alogw("RefCnt[%d]", pTEntry->mUsedRefCnt);
                                    list_move_tail(&pTEntry->mList, &pRecRenderData->mTextInputFrameIdleList);
                                    MM_COMPONENTTYPE *pInPortTunnelComp = (MM_COMPONENTTYPE*)(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].hTunnel);
                                    COMP_BUFFERHEADERTYPE obh;
                                    obh.nOutputPortIndex   = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].nTunnelPortIndex;
                                    obh.nInputPortIndex    = pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].nPortIndex;
                                    obh.pOutputPortPrivate = (void*)pTEntry;
                                    eRet = pInPortTunnelComp->FillThisBuffer(pInPortTunnelComp, &obh);
                                    if(eRet != SUCCESS)
                                    {
                                        aloge("fatal error! fill this buffer fail[0x%x], text frame id=[%d], discard it!", eRet, pTEntry->stEncodedStream.nID);
                                    }
                                }
                            }

                            int iIdleTextFrameCnt = 0;
                            list_for_each_entry(pTEntry, &pRecRenderData->mTextInputFrameIdleList, mList)
                            {
                                iIdleTextFrameCnt++;
                            }
                            if (iIdleTextFrameCnt != pRecRenderData->mTextInputFrameNum)
                            {
                                aloge("fatal error! text input frames [%d]<[%d] must return all before", iIdleTextFrameCnt, pRecRenderData->mTextInputFrameNum);
                            }
                            else
                            {
                                alogd("Release all text input frame [%d]", pRecRenderData->mTextInputFrameNum);
                            }

                            pthread_mutex_unlock(&pRecRenderData->mTextInputFrameListMutex);

                            pRecRenderData->mnBasePts = -1;
                            pRecRenderData->mnAudioBasePts = -1;
                            pRecRenderData->mnTextBasePts = -1;
                            pRecRenderData->duration = 0;
                            pRecRenderData->duration_audio = 0;
                            pRecRenderData->duration_text = 0;

                            pRecRenderData->state = COMP_StateLoaded;
                            pRecRenderData->pCallbacks->EventHandler(
                                    pRecRenderData->hSelf,
                                    pRecRenderData->pAppData,
                                    COMP_EventCmdComplete,
                                    COMP_CommandStateSet,
                                    pRecRenderData->state,
                                    NULL);
                            alogv("RecRender set state LOADED ok");
                            break;
                        }
                        case COMP_StateIdle:
                        {
                            if (pRecRenderData->state == COMP_StateInvalid)
                            {
                                aloge("InvalidState[%d]->[%d]", pRecRenderData->state, COMP_StateIdle);
                                pRecRenderData->pCallbacks->EventHandler(
                                        pRecRenderData->hSelf,
                                        pRecRenderData->pAppData,
                                        COMP_EventError,
                                        ERR_MUX_INCORRECT_STATE_OPERATION,
                                        0,
                                        NULL);
                            }
                            else
                            {
                                alogv("recRender turn to stateIdle, [%d]->[%d]", pRecRenderData->state, COMP_StateIdle);
                                pRecRenderData->state = COMP_StateIdle;
                                pRecRenderData->pCallbacks->EventHandler(
                                        pRecRenderData->hSelf,
                                        pRecRenderData->pAppData,
                                        COMP_EventCmdComplete,
                                        COMP_CommandStateSet,
                                        pRecRenderData->state,
                                        NULL);
                            }
                            break;
                        }
                        case COMP_StateExecuting:
                        {
                            // Transition can only happen from pause or idle state
                            if (pRecRenderData->state == COMP_StateIdle || pRecRenderData->state == COMP_StatePause)
                            {
                                // Return buffers if currently in pause
                                if(pRecRenderData->state == COMP_StateIdle)
                                {
                                    pRecRenderData->tracks_count = 0;

                                    if (pRecRenderData->recorder_mode & RECORDER_MODE_VIDEO)
                                    {
                                        pRecRenderData->tracks_count++;
                                        pRecRenderData->track_exist |= 1<<CODEC_TYPE_VIDEO;
                                        if(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_VIDEO].hTunnel == NULL)
                                        {
                                            aloge("fatal error! hnd_venc_comp is null");
                                            pRecRenderData->state = COMP_StateInvalid;
                                            pRecRenderData->pCallbacks->EventHandler(
                                                    pRecRenderData->hSelf,
                                                    pRecRenderData->pAppData,
                                                    COMP_EventError,
                                                    ERR_MUX_INVALIDSTATE,
                                                    0,
                                                    NULL);
                                            //goto EXIT;
                                        }
                                    }
                                    if (pRecRenderData->recorder_mode & RECORDER_MODE_AUDIO)
                                    {
                                        pRecRenderData->tracks_count++;
                                        pRecRenderData->track_exist |= 1<<CODEC_TYPE_AUDIO;
                                        if(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_AUDIO].hTunnel == NULL)
                                        {
                                            aloge("fatal error! hnd_aenc_comp is null");
                                            pRecRenderData->state = COMP_StateInvalid;
                                            pRecRenderData->pCallbacks->EventHandler(
                                                    pRecRenderData->hSelf,
                                                    pRecRenderData->pAppData,
                                                    COMP_EventError,
                                                    ERR_MUX_INVALIDSTATE,
                                                    0,
                                                    NULL);
                                            //goto EXIT;
                                        }
                                    }
                                    if (pRecRenderData->recorder_mode & RECORDER_MODE_TEXT)
                                    {
                                        pRecRenderData->tracks_count++;
                                        pRecRenderData->track_exist |= 1<<CODEC_TYPE_TEXT;
                                        if(pRecRenderData->sInPortTunnelInfo[RECR_PORT_INDEX_TEXT].hTunnel == NULL)
                                        {
                                            aloge("fatal error! hnd_tenc_comp is null");
                                            pRecRenderData->state = COMP_StateInvalid;
                                            pRecRenderData->pCallbacks->EventHandler(
                                                    pRecRenderData->hSelf,
                                                    pRecRenderData->pAppData,
                                                    COMP_EventError,
                                                    ERR_MUX_INVALIDSTATE,
                                                    0,
                                                    NULL);
                                            //goto EXIT;
                                        }
                                    }
                                    //check if has fd sink.
                                    int i = 0;
                                    int nFdFlag = 0;
                                    RecSink *pEntry;
                                    list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
                                    {
                                        //find appropriate MuxChanAttrNode
                                        MuxChanAttrNode *pChanAttrNode = RecRender_FindMuxChanAttrNodeByMuxerId(pRecRenderData, pEntry->mMuxerId);
                                        pEntry->SetRecordMode(pEntry, pRecRenderData->recorder_mode);
                                        pEntry->SetMediaInf(pEntry, &pRecRenderData->media_inf);
                                        pEntry->SetVencExtraData(pEntry, &pRecRenderData->venc_extradata_info);
                                        pEntry->SetMaxFileDuration(pEntry, pChanAttrNode->mChnAttr.mMaxFileDuration);
                                        //pEntry->SetImpactFileDuration(pEntry, pRecRenderData->impact_bftime+pRecRenderData->impact_aftime);
                                        pEntry->SetFileDurationPolicy(pEntry, pRecRenderData->mRecordFileDurationPolicy);
                                        pEntry->SetMaxFileSize(pEntry, pChanAttrNode->mChnAttr.mMaxFileSizeBytes);
                                        pEntry->SetFsWriteMode(pEntry, pChanAttrNode->mChnAttr.mFsWriteMode);
                                        pEntry->SetFsSimpleCacheSize(pEntry, pChanAttrNode->mChnAttr.mSimpleCacheSize);
                                        pEntry->SetCallbackWriter(pEntry, &pRecRenderData->callback_writer);
                                        if(pEntry->nOutputFd >= 0)
                                        {
                                            nFdFlag++;
                                        }
                                        i++;
                                    }
                                    if(!list_empty(&pRecRenderData->mSwitchingSinkInfoList))
                                    {
                                        aloge("fatal error! switchingSinkList is not empty! check code!");
                                    }
                                    if(nFdFlag)
                                    {
                                        alogd("[%d] RecSinks when turn to OMX_StateExecuting", nFdFlag);
                                    }
                                }
                                else if (pRecRenderData->state == COMP_StatePause)
                                {
                                }
                                pRecRenderData->state = COMP_StateExecuting;
                                pRecRenderData->pCallbacks->EventHandler(
                                        pRecRenderData->hSelf,
                                        pRecRenderData->pAppData,
                                        COMP_EventCmdComplete,
                                        COMP_CommandStateSet,
                                        pRecRenderData->state,
                                        NULL);
                            }
                            else
                            {
                                aloge("InvalidState[%d]->[%d]", pRecRenderData->state, COMP_StateExecuting);
                                pRecRenderData->pCallbacks->EventHandler(
                                        pRecRenderData->hSelf,
                                        pRecRenderData->pAppData,
                                        COMP_EventError,
                                        ERR_MUX_INCORRECT_STATE_TRANSITION,
                                        0,
                                        NULL);
                            }
                            break;
                        }
                        case COMP_StatePause:
                        {
                            // Transition can only happen from executing state
                            if (pRecRenderData->state == COMP_StateExecuting)
                            {
                                pRecRenderData->state = COMP_StatePause;
                                pRecRenderData->pCallbacks->EventHandler(
                                        pRecRenderData->hSelf,
                                        pRecRenderData->pAppData,
                                        COMP_EventCmdComplete,
                                        COMP_CommandStateSet,
                                        pRecRenderData->state,
                                        NULL);
                            }
                            else
                            {
                                aloge("InvalidState[%d]->[%d]", pRecRenderData->state, COMP_StatePause);
                                pRecRenderData->pCallbacks->EventHandler(
                                        pRecRenderData->hSelf,
                                        pRecRenderData->pAppData,
                                        COMP_EventError,
                                        ERR_MUX_INCORRECT_STATE_TRANSITION,
                                        0,
                                        NULL);
                            }
                            break;
                        }
                        default:
                        {
                            aloge("fatal error! InvalidState[%d]->[%d]", pRecRenderData->state, (COMP_STATETYPE) (cmddata));
                            break;
                        }
                    }
                }
                pthread_mutex_unlock(&pRecRenderData->mStateMutex);
            }
            else if (cmd == StopPort)
            {
            }
            else if (cmd == Stop)
            {
                // Kill thread
                goto EXIT;
            }
            else if(VendorAddOutputSinkInfo == cmd)
            {
                //must free message_t->data.
                ERRORTYPE eRet = SUCCESS;
                int nFd = cmd_msg.para0;
                MUX_CHN_ATTR_S *pChnAttr = ((MuxChnAttr*)cmd_msg.mpData)->pChnAttr;
                int nChnId = ((MuxChnAttr*)cmd_msg.mpData)->nChnId;

                if (!(pRecRenderData->recorder_mode & RECORDER_MODE_AUDIO))
                {
                    if ((pChnAttr->mMediaFileFormat == MEDIA_FILE_FORMAT_MP3) || \
                        (pChnAttr->mMediaFileFormat == MEDIA_FILE_FORMAT_AAC) )
                    {
                        aloge("grp attr not set to audio, but now set chn to audio out");
                        eRet = FAILURE;
                    }
                }

                if (eRet != FAILURE)
                {
                    eRet = RecRender_AddOutputSinkInfo(pRecRenderData, nChnId, pChnAttr, nFd);
                    if (eRet != SUCCESS)
                    {
                        aloge("fatal error! why RecRender AddOutputSinkInfo()[0x%x] fail?", eRet);
                    }
                }
                free(cmd_msg.mpData);
                cmd_msg.mpData = NULL;
                cmd_msg.mDataSize = 0;
                pRecRenderData->pCallbacks->EventHandler(
                        pRecRenderData->hSelf,
                        pRecRenderData->pAppData,
                        COMP_EventCmdComplete,
                        COMP_CommandVendorAddChn,
                        eRet,
                        NULL);
            }
            else if(VendorRemoveOutputSinkInfo == cmd)
            {
                ERRORTYPE eRet = SUCCESS;
                //must free message_t->data.
                eRet = RecRender_RemoveOutputSinkInfo(pRecRenderData, (int)cmddata);
                if(eRet != SUCCESS)
                {
                    aloge("fatal error! why RecRender_RemoveOutputSinkInfo()[0x%x] fail?", eRet);
                }
                pRecRenderData->pCallbacks->EventHandler(
                        pRecRenderData->hSelf,
                        pRecRenderData->pAppData,
                        COMP_EventCmdComplete,
                        COMP_CommandVendorRemoveChn,
                        0,
                        NULL);
            }
            else if (SwitchFile == cmd)
            {
                //alogd("<F:%s, L:%d>Switch file rec_file=%d", pRecRenderData->rec_file);
                int nMuxerId = (int)cmddata;
                alogd("Switch file at muxerId[%d]", nMuxerId);
                pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
                int i=0;
                RecSink *pEntry, *pTmp;
                list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
                {
                    if(pEntry->mMuxerId == nMuxerId)
                    {
                        i++;
                    }
                }
                list_for_each_entry(pEntry, &pRecRenderData->mSwitchingSinkInfoList, mList)
                {
                    if(pEntry->mMuxerId == nMuxerId)
                    {
                        alogw("Switch file at muxerId[%d] during switching, ignore it!", nMuxerId);
                        i++;
                    }
                }
                if(i!=1)
                {
                    aloge("fatal error! Switch file at muxerId[%d],[%d]times", nMuxerId, i);
                }

                list_for_each_entry_safe(pEntry, pTmp, &pRecRenderData->mValidSinkInfoList, mList)
                {
                    if(pEntry->mMuxerId == nMuxerId)
                    {
                        if(pEntry->nCallbackOutFlag == FALSE)
                        {
                            ERRORTYPE switchRet;
                            BOOL bCacheFlag = pRecRenderData->cache_manager?TRUE:FALSE;
                            switchRet = pEntry->SendCmdSwitchFile(pEntry, bCacheFlag);
                            if(SUCCESS == switchRet)
                            {
                                //if has cache manager, we should put recSink to switching list, wait recSink switch done, then resend rsPacket.
                                //if no cache manager, we think already switch done, so we do nothing after SendCmdSwitchFile().
                                if(bCacheFlag)
                                {
                                    list_move_tail(&pEntry->mList, &pRecRenderData->mSwitchingSinkInfoList);
                                }
                            }
                        }
                        else
                        {
                            aloge("fatal error! Switch file in callback mode muxerId[%d]", nMuxerId);
                        }
                        break;
                    }
                }
                pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
            }
            else if(SwitchFileDone == cmd)
            {
                int nMuxerId = (int)cmddata;
                BOOL bFindFlag = FALSE;
                BOOL bCacheFlag = pRecRenderData->cache_manager?TRUE:FALSE;
                if(bCacheFlag!=TRUE)
                {
                    aloge("fatal error! no cache manager but send SwitchFileDone, muxerId[%d], why?", nMuxerId);
                    goto EndProcessMessage;
                }
                int i=0;
                RecSink *pEntry;
                pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
                list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
                {
                    if(pEntry->mMuxerId == nMuxerId)
                    {
                        i++;
                    }
                }
                list_for_each_entry(pEntry, &pRecRenderData->mSwitchingSinkInfoList, mList)
                {
                    if(pEntry->mMuxerId == nMuxerId)
                    {
                        i++;
                    }
                }
                if(i!=1)
                {
                    aloge("fatal error! switch file done at muxerId[%d],[%d]times", nMuxerId, i);
                }

                list_for_each_entry(pEntry, &pRecRenderData->mSwitchingSinkInfoList, mList)
                {
                    if(pEntry->mMuxerId == nMuxerId)
                    {
                        bFindFlag = TRUE;
                        break;
                    }
                }
                if(bFindFlag!=TRUE)
                {
                    aloge("fatal error! not find muxerId[%d] when switch file done", nMuxerId);
                    pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
                    goto EndProcessMessage;
                }
                list_move_tail(&pEntry->mList, &pRecRenderData->mValidSinkInfoList);

                int cnt = 0;
                int sendCnt = 0;
                RecSinkPacket *pPacketEntry;
                //reSend all using_packet_list to recSink.
                //It's OK that not call mutex_lock, because usingPacketList's elements can not increase or decrease here.
                //I know eventHandler() may call ReleasePacket() to modify RecSinkPacket->mRefCnt, but mRefCnt impossible equal to 0!
                //pthread_mutex_lock(&pRecRenderData->cache_manager->mPacketListLock);
                int64_t nVideoStartPts = -1;
                BOOL bSendFlag;
                list_for_each_entry(pPacketEntry, &pRecRenderData->cache_manager->mUsingPacketList, mList)
                {
                    if(CODEC_TYPE_VIDEO == pPacketEntry->mStreamType && pPacketEntry->mFlags&AVPACKET_FLAG_KEYFRAME)
                    {
                        nVideoStartPts = pPacketEntry->mPts;
                        break;
                    }
                }
                list_for_each_entry(pPacketEntry, &pRecRenderData->cache_manager->mUsingPacketList, mList)
                {
                    if(CODEC_TYPE_AUDIO == pPacketEntry->mStreamType)
                    {
                        if(-1==nVideoStartPts || pPacketEntry->mPts>=nVideoStartPts)
                        {
                            bSendFlag = TRUE;
                        }
                        else
                        {
                            bSendFlag = FALSE;
                        }
                     }
                     else
                     {
                        bSendFlag = TRUE;
                    }
                    if(bSendFlag)
                    {
                        pRecRenderData->cache_manager->RefPacket(pRecRenderData->cache_manager, pPacketEntry->mId);
                        if(pEntry->EmptyThisBuffer(pEntry, pPacketEntry) == SUCCESS)
                        {
                        }
                        else
                        {
                            aloge("fatal error! RecSink muxerId[%d] empty this buffer fail!", pEntry->mMuxerId);
                            pRecRenderData->cache_manager->ReleasePacket(pRecRenderData->cache_manager, pPacketEntry->mId);
                        }
                        sendCnt++;
                     }
                     cnt++;
                }
                alogd("switch file done! Resend [%d]of[%d]usingPackets to muxerId[%d]!", sendCnt, cnt, pEntry->mMuxerId);
                if(pRecRenderData->cache_manager->GetReadyPacketCount(pRecRenderData->cache_manager)>0)
                {
                    aloge("fatal error! cache ReadyList impossible has element now!");
                }
                //pthread_mutex_unlock(&pRecRenderData->cache_manager->mPacketListLock);
                pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
            }
            else if (SwitchFileNormal == cmd)
            {
                int nMuxerId = (int)cmddata;
                alogd("Switch file normal at muxerId[%d]", nMuxerId);
                pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
                int i=0;
                RecSink *pEntry, *pTmp;
                list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
                {
                    if(pEntry->mMuxerId == nMuxerId)
                    {
                        i++;
                    }
                }
                list_for_each_entry(pEntry, &pRecRenderData->mSwitchingSinkInfoList, mList)
                {
                    if(pEntry->mMuxerId == nMuxerId)
                    {
                        alogw("Switch file normal at muxerId[%d] during switching, ignore it!", nMuxerId);
                        i++;
                    }
                }
                if(i!=1)
                {
                    aloge("fatal error! Switch file at muxerId[%d],[%d]times", nMuxerId, i);
                }

                list_for_each_entry_safe(pEntry, pTmp, &pRecRenderData->mValidSinkInfoList, mList)
                {
                    if(pEntry->mMuxerId == nMuxerId)
                    {
                        if(pEntry->nCallbackOutFlag == FALSE)
                        {
                            ERRORTYPE switchRet;
                            switchRet = pEntry->SendCmdSwitchFileNormal(pEntry);
                            if(SUCCESS == switchRet)
                            {
                                //in switch file normal, do nothing after SendCmdSwitchFileNormal().
                            }
                        }
                        else
                        {
                            aloge("fatal error! Switch file normal in callback mode muxerId[%d]", nMuxerId);
                        }
                        break;
                    }
                }
                pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
            }
            else if (RecRenderComp_InputFrameAvailable == cmd)
            {
                alogv("inputFrameAvailable");
                //pRecRenderData->mNoInputFrameFlag = 0;
            }
EndProcessMessage:
            //precede to process message
            goto PROCESS_MESSAGE;
        }

        if (pRecRenderData->state == COMP_StateExecuting)
        {
            buffer_fail_flags = 0;
            eRet = FAILURE;
            if(pRecRenderData->recorder_mode & RECORDER_MODE_VIDEO)
            {
                eRet = RecRender_GetVideoBuffer(pRecRenderData, &RSPacket);
                if(eRet!=SUCCESS)
                {
                    buffer_fail_flags |= 1<<CODEC_TYPE_VIDEO;
                }
                else
                {
                    pRecRenderData->duration = RSPacket.mPts;
                }
            }
            if(eRet!=SUCCESS)
            {
                if(pRecRenderData->recorder_mode & RECORDER_MODE_AUDIO)
                {
                    eRet = RecRender_GetAudioBuffer(pRecRenderData, &RSPacket);
                    if(eRet!=SUCCESS)
                    {
                        buffer_fail_flags |= 1<<CODEC_TYPE_AUDIO;
                    }
                    else
                    {
                        pRecRenderData->duration_audio = RSPacket.mPts;
                    }
                }
            }
            if(eRet!=SUCCESS)
            {
                if(pRecRenderData->recorder_mode & RECORDER_MODE_TEXT)
                {
                    eRet = RecRender_GetTextBuffer(pRecRenderData, &RSPacket);
                    if(eRet!=SUCCESS)
                    {
                        buffer_fail_flags |= 1<<CODEC_TYPE_TEXT;
                    }
                    else
                    {
                        pRecRenderData->duration_text = RSPacket.mPts;
                    }
                }
            }
            if (buffer_fail_flags == pRecRenderData->track_exist)
            {
                RECORDER_MODE curRecorderMode = pRecRenderData->recorder_mode;
                int curFailFlags = 0;
                if(curRecorderMode & RECORDER_MODE_VIDEO)
                {
                    pthread_mutex_lock(&pRecRenderData->mVideoInputFrameListMutex);
                    if (list_empty(&pRecRenderData->mVideoInputFrameReadyList))
                    {
                        curFailFlags |= 1<<CODEC_TYPE_VIDEO;
                    }
                }
                if(curRecorderMode & RECORDER_MODE_AUDIO)
                {
                    pthread_mutex_lock(&pRecRenderData->mAudioInputFrameListMutex);
                    if (list_empty(&pRecRenderData->mAudioInputFrameReadyList))
                    {
                        curFailFlags |= 1<<CODEC_TYPE_AUDIO;
                    }
                }
                if(curRecorderMode & RECORDER_MODE_TEXT)
                {
                    pthread_mutex_lock(&pRecRenderData->mTextInputFrameListMutex);
                    if (list_empty(&pRecRenderData->mTextInputFrameReadyList))
                    {
                        curFailFlags |= 1<<CODEC_TYPE_TEXT;
                    }
                }
                int bNeedWaitMessage = 0;
                if(curFailFlags == pRecRenderData->track_exist)
                {
                    pRecRenderData->mNoInputFrameFlag = 1;
                    bNeedWaitMessage = 1;
                }
                else
                {
                    alogd("Be careful! Low impossible. curFailFlags[0x%x]!=[0x%x]", curFailFlags, buffer_fail_flags);
                    bNeedWaitMessage = 0;
                }
                if(curRecorderMode & RECORDER_MODE_VIDEO)
                {
                    pthread_mutex_unlock(&pRecRenderData->mVideoInputFrameListMutex);
                }
                if(curRecorderMode & RECORDER_MODE_AUDIO)
                {
                    pthread_mutex_unlock(&pRecRenderData->mAudioInputFrameListMutex);
                }
                if(curRecorderMode & RECORDER_MODE_TEXT)
                {
                    pthread_mutex_unlock(&pRecRenderData->mTextInputFrameListMutex);
                }
                if(bNeedWaitMessage)
                {
                    TMessage_WaitQueueNotEmpty(&pRecRenderData->cmd_queue, 0);
                    //alogd("request nothing from all track, wait done");
                }
                goto PROCESS_MESSAGE;
            }

            // send frame to cache_manager
            if(pRecRenderData->cache_manager)
            {
                if (pRecRenderData->cache_manager->PushPacket((COMP_HANDLETYPE)pRecRenderData->cache_manager, &RSPacket) != SUCCESS)
                {
                    aloge("fatal error! push packet to cache manager fail, impossible!");
                }
                pRecRenderData->cache_manager->ControlCacheLevel(pRecRenderData->cache_manager);
            }

            //send frame to all RecSinks
            pthread_mutex_lock(&pRecRenderData->mSinkInfoListMutex);
            BOOL bCacheMuxerFlag = FALSE;
            RecSink *pEntry;
            list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
            {
                if(!pEntry->bBufFromCacheFlag)
                {
                    RecRender_RefBuffer(pRecRenderData, &RSPacket); //Notes: Must ref buf before send it to Sink
                    if(pEntry->EmptyThisBuffer(pEntry, &RSPacket) == SUCCESS)
                    {
                    }
                    else
                    {
                        aloge("fatal error! RecSink empty this buffer fail!");
                        RecRender_ReleaseBuffer(pRecRenderData, &RSPacket);
                    }
                }
                else
                {
                    bCacheMuxerFlag = TRUE;
                }
            }
            RecRender_ReleaseBuffer(pRecRenderData, &RSPacket);
            //write all ready packets of cacheManger to all cacheRecSinks.
            if(bCacheMuxerFlag)
            {
                int nCacheReadyPacketNum = 0;
                while (pRecRenderData->cache_manager)
                {
                    if (SUCCESS != pRecRenderData->cache_manager->GetPacket((COMP_HANDLETYPE)pRecRenderData->cache_manager, &pCacheRSPacket))
                    {
                        break;
                    }
                    nCacheReadyPacketNum++;
                    list_for_each_entry(pEntry, &pRecRenderData->mValidSinkInfoList, mList)
                    {
                        if(pEntry->bBufFromCacheFlag)
                        {
                            pRecRenderData->cache_manager->RefPacket(pRecRenderData->cache_manager, pCacheRSPacket->mId);
                            if(pEntry->EmptyThisBuffer(pEntry, pCacheRSPacket) == SUCCESS)
                            {
                            }
                            else
                            {
                                aloge("fatal error! RecSink empty this buffer fail!");
                                pRecRenderData->cache_manager->ReleasePacket(pRecRenderData->cache_manager, pCacheRSPacket->mId);
                            }
                        }
                    }
                }
                if(nCacheReadyPacketNum > 1)
                {
                    //alogd("send [%d]cacheReadyPackets", nCacheReadyPacketNum);
                }
            }
            pthread_mutex_unlock(&pRecRenderData->mSinkInfoListMutex);
        }
        else
        {
            //cdx_sem_down(&pRecRenderData->cdx_sem_wait_message);
            TMessage_WaitQueueNotEmpty(&pRecRenderData->cmd_queue, 0);
        }
    }
EXIT:
    alogv("Recorder Render ComponentThread stopped");
    return (void*) SUCCESS;
}

