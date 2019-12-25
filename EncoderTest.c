/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : EncoderTest.c
 * Description : EncoderTest
 * History :
 *
 */

#include <cdx_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vencoder.h"
#include <sys/time.h>
#include <time.h>
#include <memoryAdapter.h>

#define DEMO_FILE_NAME_LEN 256
//#define USE_SVC
//#define USE_VIDEO_SIGNAL
//#define USE_ASPECT_RATIO
//#define USE_SUPER_FRAME

#define _ENCODER_TIME_
#ifdef _ENCODER_TIME_
//extern int gettimeofday(struct timeval *tv, struct timezone *tz);
static long long GetNowUs()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000000 + now.tv_usec;
}

long long time1=0;
long long time2=0;
long long time3=0;
#endif

typedef struct {
    char             intput_file[256];
    char             output_file[256];
    int              compare_flag;
    int              compare_result;

    unsigned int  encode_frame_num;
    unsigned int  encode_format;

    unsigned int src_size;
    unsigned int dst_size;

    unsigned int src_width;
    unsigned int src_height;
    unsigned int dst_width;
    unsigned int dst_height;

    int bit_rate;
    int frame_rate;
    int maxKeyFrame;
}encode_param_t;

typedef enum {
    INPUT,
    HELP,
    ENCODE_FRAME_NUM,
    ENCODE_FORMAT,
    OUTPUT,
    SRC_SIZE,
    DST_SIZE,
    COMPARE_FILE,
    INVALID
}ARGUMENT_T;

typedef struct {
    char Short[8];
    char Name[128];
    ARGUMENT_T argument;
    char Description[512];
}argument_t;

static const argument_t ArgumentMapping[] =
{
    { "-h",  "--help",    HELP,
        "Print this help" },
    { "-i",  "--input",   INPUT,
        "Input file path" },
    { "-n",  "--encode_frame_num",   ENCODE_FRAME_NUM,
        "After encoder n frames, encoder stop" },
    { "-f",  "--encode_format",  ENCODE_FORMAT,
        "0:h264 encoder, 1:jpeg_encoder" },
    { "-o",  "--output",  OUTPUT,
        "output file path" },
    { "-s",  "--srcsize",  SRC_SIZE,
        "src_size,can be 1080,720,480" },
    { "-d",  "--dstsize",  DST_SIZE,
        "dst_size,can be 1080,720,480" },
};

int yu12_nv12(unsigned int width, unsigned int height, unsigned char *addr_uv,
          unsigned char *addr_tmp_uv)
{
    unsigned int i, chroma_bytes;
    unsigned char *u_addr = NULL;
    unsigned char *v_addr = NULL;
    unsigned char *tmp_addr = NULL;

    chroma_bytes = width*height/4;

    u_addr = addr_uv;
    v_addr = addr_uv + chroma_bytes;
    tmp_addr = addr_tmp_uv;

    for(i=0; i<chroma_bytes; i++)
    {
        *(tmp_addr++) = *(u_addr++);
        *(tmp_addr++) = *(v_addr++);
    }

    memcpy(addr_uv, addr_tmp_uv, chroma_bytes*2);

    return 0;
}

ARGUMENT_T GetArgument(char *name)
{
    int i = 0;
    int num = sizeof(ArgumentMapping) / sizeof(argument_t);
    while(i < num)
    {
        if((0 == strcmp(ArgumentMapping[i].Name, name)) ||
            ((0 == strcmp(ArgumentMapping[i].Short, name)) &&
             (0 != strcmp(ArgumentMapping[i].Short, "--"))))
        {
            return ArgumentMapping[i].argument;
        }
        i++;
    }
    return INVALID;
}

static void PrintDemoUsage(void)
{
    int i = 0;
    int num = sizeof(ArgumentMapping) / sizeof(argument_t);
    printf("Usage:");
    while(i < num)
    {
        printf("%s %-32s %s", ArgumentMapping[i].Short, ArgumentMapping[i].Name,
                ArgumentMapping[i].Description);
        printf("\n");
        i++;
    }
}

void ParseArgument(encode_param_t *encode_param, char *argument, char *value)
{
    ARGUMENT_T arg;

    arg = GetArgument(argument);

    switch(arg)
    {
        case HELP:
            PrintDemoUsage();
            exit(-1);
        case INPUT:
            memset(encode_param->intput_file, 0, sizeof(encode_param->intput_file));
            sscanf(value, "%255s", encode_param->intput_file);
            logd(" get input file: %s ", encode_param->intput_file);
            break;
        case ENCODE_FRAME_NUM:
            sscanf(value, "%u", &encode_param->encode_frame_num);
            break;
        case ENCODE_FORMAT:
            sscanf(value, "%u", &encode_param->encode_format);
            break;
        case OUTPUT:
            memset(encode_param->output_file, 0, sizeof(encode_param->output_file));
            sscanf(value, "%255s", encode_param->output_file);
            logd(" get output file: %s ", encode_param->output_file);
            break;
        case SRC_SIZE:
            sscanf(value, "%u", &encode_param->src_size);
            logd(" get src_size: %dp ", encode_param->src_size);
            if(encode_param->src_size == 1080)
            {
                encode_param->src_width = 1920;
                encode_param->src_height = 1080;
            }
            else if(encode_param->src_size == 720)
            {
                encode_param->src_width = 1280;
                encode_param->src_height = 720;
            }
            else if(encode_param->src_size == 480)
            {
                encode_param->src_width = 640;
                encode_param->src_height = 480;
            }
            else
            {
                encode_param->src_width = 1280;
                encode_param->src_height = 720;
                logw("encoder demo only support the size 1080p,720p,480p, \
                 now use the default size 720p\n");
            }
            break;
        case DST_SIZE:
            sscanf(value, "%u", &encode_param->dst_size);
            logd(" get dst_size: %dp ", encode_param->dst_size);
            if(encode_param->dst_size == 1080)
            {
                encode_param->dst_width = 1920;
                encode_param->dst_height = 1080;
            }
            else if(encode_param->dst_size == 720)
            {
                encode_param->dst_width = 1280;
                encode_param->dst_height = 720;
            }
            else if(encode_param->dst_size == 480)
            {
                encode_param->dst_width = 640;
                encode_param->dst_height = 480;
            }
            else
            {
                encode_param->dst_width = 1280;
                encode_param->dst_height = 720;
                logw("encoder demo only support the size 1080p,720p,480p,\
                 now use the default size 720p\n");
            }
            break;
            case COMPARE_FILE:
            break;
        case INVALID:
        default:
            logd("unknowed argument :  %s", argument);
            break;
    }
}

void DemoHelpInfo(void)
{
    logd(" ==== CedarX2.0 encoder demo help start ===== ");
    logd(" -h or --help to show the demo usage");
    logd(" demo created by yangcaoyuan, allwinnertech/AL3 ");
    logd(" email: yangcaoyuan@allwinnertech.com ");
    logd(" ===== CedarX2.0 encoder demo help end ====== ");
}

int main(int argc, char** argv)
{
    VencBaseConfig baseConfig;
    VencAllocateBufferParam bufferParam;
    VideoEncoder* pVideoEnc = NULL;
    VencInputBuffer inputBuffer;
    VencOutputBuffer outputBuffer;
    VencHeaderData sps_pps_data;
    VencH264Param h264Param;
    VencH264FixQP fixQP;
    EXIFInfo exifinfo;
    VencCyclicIntraRefresh sIntraRefresh;
    unsigned char *uv_tmp_buffer = NULL;
#ifdef USE_SUPER_FRAME
    VencSuperFrameConfig     sSuperFrameCfg;
#endif
#ifdef USE_SVC
    VencH264SVCSkip         SVCSkip; // set SVC and skip_frame
#endif
#ifdef USE_ASPECT_RATIO
    VencH264AspectRatio        sAspectRatio;
#endif
#ifdef USE_VIDEO_SIGNAL
    VencH264VideoSignal        sVideoSignal;
#endif

    int result = 0;
    int i = 0;
    //long long pts = 0;

    FILE *in_file = NULL;
    FILE *out_file = NULL;

    char *input_path = NULL;
    char *output_path = NULL;

    //set the default encode param
    encode_param_t    encode_param;
    memset(&encode_param, 0, sizeof(encode_param));

    encode_param.src_width = 1280;
    encode_param.src_height = 720;
    encode_param.dst_width = 1280;
    encode_param.dst_height = 720;

    encode_param.bit_rate = 6*1024*1024;
    encode_param.frame_rate = 30;
    encode_param.maxKeyFrame = 30;

    encode_param.encode_format = VENC_CODEC_H264;
    encode_param.encode_frame_num = 200;

    strcpy((char*)encode_param.intput_file,        "/data/camera/720p-30zhen.yuv");
    strcpy((char*)encode_param.output_file,        "/data/camera/720p.264");

    //parse the config paramter
    if(argc >= 2)
    {
        for(i = 1; i < (int)argc; i += 2)
        {
            ParseArgument(&encode_param, argv[i], argv[i + 1]);
        }
    }
    else
    {
        printf(" we need more arguments ");
        PrintDemoUsage();
        return 0;
    }


    //intraRefresh
    sIntraRefresh.bEnable = 1;
    sIntraRefresh.nBlockNumber = 10;

    //fix qp mode
    fixQP.bEnable = 1;
    fixQP.nIQp = 20;
    fixQP.nPQp = 30;

    //* h264 param
    h264Param.bEntropyCodingCABAC = 1;
    h264Param.nBitrate = encode_param.bit_rate;
    h264Param.nFramerate = encode_param.frame_rate;
    h264Param.nCodingMode = VENC_FRAME_CODING;
    //h264Param.nCodingMode = VENC_FIELD_CODING;

    h264Param.nMaxKeyInterval = encode_param.maxKeyFrame;
    h264Param.sProfileLevel.nProfile = VENC_H264ProfileMain;
    h264Param.sProfileLevel.nLevel = VENC_H264Level31;
    h264Param.sQPRange.nMinqp = 10;
    h264Param.sQPRange.nMaxqp = 40;

    input_path = encode_param.intput_file;
    output_path = encode_param.output_file;

    in_file = fopen(input_path, "r");
    if(in_file == NULL)
    {
        loge("open in_file fail\n");
        return -1;
    }

    out_file = fopen(output_path, "wb");
    if(out_file == NULL)
    {
        loge("open out_file fail\n");
        fclose(in_file);
        return -1;
    }


    memset(&baseConfig, 0 ,sizeof(VencBaseConfig));
    memset(&bufferParam, 0 ,sizeof(VencAllocateBufferParam));

    baseConfig.memops = MemAdapterGetOpsS();
    if (baseConfig.memops == NULL)
    {
        printf("MemAdapterGetOpsS failed\n");
        goto out;
    }
    CdcMemOpen(baseConfig.memops);
    baseConfig.nInputWidth= encode_param.src_width;
    baseConfig.nInputHeight = encode_param.src_height;
    baseConfig.nStride = encode_param.src_width;

    baseConfig.nDstWidth = encode_param.dst_width;
    baseConfig.nDstHeight = encode_param.dst_height;
    //the format of yuv file is yuv420p,
    //but the old ic only support the yuv420sp,
    //so use the func yu12_nv12() to config all the format.
    baseConfig.eInputFormat = VENC_PIXEL_YUV420SP;

    bufferParam.nSizeY = baseConfig.nInputWidth*baseConfig.nInputHeight;
    bufferParam.nSizeC = baseConfig.nInputWidth*baseConfig.nInputHeight/2;
    bufferParam.nBufferNum = 4;

    pVideoEnc = VideoEncCreate(encode_param.encode_format);

    if(encode_param.encode_format == VENC_CODEC_H264)
    {
        int value;

        VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264Param, &h264Param);


        value = 0;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamIfilter, &value);

        value = 0; //degree
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamRotation, &value);

        value = 0;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamSetPSkip, &value);
        //value = 1;
        //VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264FastEnc, &value);
    }

    VideoEncInit(pVideoEnc, &baseConfig);

    if(encode_param.encode_format == VENC_CODEC_H264)
    {
        unsigned int head_num = 0;
        VideoEncGetParameter(pVideoEnc, VENC_IndexParamH264SPSPPS, &sps_pps_data);
        fwrite(sps_pps_data.pBuffer, 1, sps_pps_data.nLength, out_file);
        logd("sps_pps_data.nLength: %d", sps_pps_data.nLength);
        for(head_num=0; head_num<sps_pps_data.nLength; head_num++)
            logd("the sps_pps :%02x\n", *(sps_pps_data.pBuffer+head_num));
    }

    AllocInputBuffer(pVideoEnc, &bufferParam);

    if(baseConfig.eInputFormat == VENC_PIXEL_YUV420SP)
    {
        uv_tmp_buffer = (unsigned char*)malloc(baseConfig.nInputWidth*baseConfig.nInputHeight/2);
        if(uv_tmp_buffer == NULL)
        {
            loge("malloc uv_tmp_buffer fail\n");
            fclose(out_file);
            fclose(in_file);
            return -1;
        }
    }


    unsigned int testNumber = 0;

    while(testNumber < encode_param.encode_frame_num)
    {
        GetOneAllocInputBuffer(pVideoEnc, &inputBuffer);
        {
            unsigned int size1, size2;

            size1 = fread(inputBuffer.pAddrVirY, 1,
                    baseConfig.nInputWidth*baseConfig.nInputHeight, in_file);
            size2 = fread(inputBuffer.pAddrVirC, 1,
                      baseConfig.nInputWidth*baseConfig.nInputHeight/2, in_file);

            if((size1!= baseConfig.nInputWidth*baseConfig.nInputHeight)
             || (size2!= baseConfig.nInputWidth*baseConfig.nInputHeight/2))
            {
                fseek(in_file, 0L, SEEK_SET);
                size1 = fread(inputBuffer.pAddrVirY, 1,
                         baseConfig.nInputWidth*baseConfig.nInputHeight, in_file);
                size2 = fread(inputBuffer.pAddrVirC, 1,
                         baseConfig.nInputWidth*baseConfig.nInputHeight/2, in_file);
            }

            if(baseConfig.eInputFormat == VENC_PIXEL_YUV420SP)
            {
                yu12_nv12(baseConfig.nInputWidth, baseConfig.nInputHeight,
                     inputBuffer.pAddrVirC, uv_tmp_buffer);
            }
        }

        inputBuffer.bEnableCorp = 0;
        inputBuffer.sCropInfo.nLeft =  240;
        inputBuffer.sCropInfo.nTop  =  240;
        inputBuffer.sCropInfo.nWidth  =  240;
        inputBuffer.sCropInfo.nHeight =  240;

        FlushCacheAllocInputBuffer(pVideoEnc, &inputBuffer);

        //pts += 66000;
        //inputBuffer.nPts = pts;

        AddOneInputBuffer(pVideoEnc, &inputBuffer);
        time1 = GetNowUs();
        VideoEncodeOneFrame(pVideoEnc);
        time2 = GetNowUs();
        logv("encode frame %d use time is %lldus..\n",testNumber,(time2-time1));
        time3 += time2-time1;

        AlreadyUsedInputBuffer(pVideoEnc,&inputBuffer);
        ReturnOneAllocInputBuffer(pVideoEnc, &inputBuffer);

        result = GetOneBitstreamFrame(pVideoEnc, &outputBuffer);
        if(result == -1)
        {
            goto out;
        }

        fwrite(outputBuffer.pData0, 1, outputBuffer.nSize0, out_file);

        if(outputBuffer.nSize1)
        {
            fwrite(outputBuffer.pData1, 1, outputBuffer.nSize1, out_file);
        }

        FreeOneBitStreamFrame(pVideoEnc, &outputBuffer);

        if(h264Param.nCodingMode==VENC_FIELD_CODING && encode_param.encode_format==VENC_CODEC_H264)
        {
            GetOneBitstreamFrame(pVideoEnc, &outputBuffer);
            //logi("size: %d,%d", outputBuffer.nSize0,outputBuffer.nSize1);

            fwrite(outputBuffer.pData0, 1, outputBuffer.nSize0, out_file);

            if(outputBuffer.nSize1)
            {
                fwrite(outputBuffer.pData1, 1, outputBuffer.nSize1, out_file);
            }

            FreeOneBitStreamFrame(pVideoEnc, &outputBuffer);
        }

        testNumber++;
    }

    logd("the average encode time is %lldus...\n",time3/testNumber);
    if(pVideoEnc)
    {
        VideoEncDestroy(pVideoEnc);
    }
    pVideoEnc = NULL;
    printf("output file is saved:%s\n",encode_param.output_file);

out:
    if(out_file)
        fclose(out_file);
    if(in_file)
        fclose(in_file);
    if(uv_tmp_buffer)
        free(uv_tmp_buffer);
    if(baseConfig.memops)
    {
        CdcMemClose(baseConfig.memops);
    }

    return 0;
}
