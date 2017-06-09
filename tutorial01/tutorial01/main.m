//
//  main.m
//  tutorial01
//
//  Created by zjc on 16/6/21.
//  Copyright © 2016年 zjc. All rights reserved.
//

#import <Foundation/Foundation.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <libavutil/avutil.h>

#define FFP_VERSION_MODULE_NAME_LENGTH 13
#define IJKVERSION_GET_MAJOR(x)     ((x >> 16) & 0xFF)
#define IJKVERSION_GET_MINOR(x)     ((x >>  8) & 0xFF)
#define IJKVERSION_GET_MICRO(x)     ((x      ) & 0xFF)

static void ffp_show_version_str(const char *module, const char *version)
{
    av_log(NULL, AV_LOG_INFO, "%-*s: %s\n", FFP_VERSION_MODULE_NAME_LENGTH, module, version);
}

static void ffp_show_version_int(const char *module, unsigned version)
{
    av_log(NULL, AV_LOG_INFO, "%-*s: %u.%u.%u\n",
           FFP_VERSION_MODULE_NAME_LENGTH, module,
           (unsigned int)IJKVERSION_GET_MAJOR(version),
           (unsigned int)IJKVERSION_GET_MINOR(version),
           (unsigned int)IJKVERSION_GET_MICRO(version));
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
    FILE *pFile;
    char szFilename[32];
    int  y;
    
    // Open file
    // 字符串格式化命令，主要功能是把格式化的数据写入某个字符串中。sprintf 是个变参函数。
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;
    
    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    
    // Write pixel data
    
    /*
     size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream);
     注意：这个函数以二进制形式对文件进行操作，不局限于文本文件
     返回值：返回实际写入的数据块数目
     （1）buffer：是一个指针，对fwrite来说，是要获取数据的地址；
     （2）size：要写入内容的单字节数；
     （3）count:要进行写入size字节的数据项的个数；
     （4）stream:目标文件指针；
     （5）返回实际写入的数据项个数count。
     */
    
    /*
     
     */
    
    for(y=0; y<height; y++)
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
    
    // Close file
    fclose(pFile);
}

int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream;
    AVCodecContext  *pCodecCtx = NULL;
    AVCodec         *pCodec = NULL;
    AVFrame         *pFrame = NULL;
    AVFrame         *pFrameRGB = NULL;
    AVPacket        packet;
    int             frameFinished;
    int             numBytes;
    uint8_t         *buffer = NULL;
    
    AVDictionary    *optionsDict = NULL;
    struct SwsContext      *sws_ctx = NULL;
    
    // /zjcletv/Desktop/FFmpeg-Learning/FFmpeg-tutorial/40M.mp4
    //
    const char * fileName = "/zjcletv/Desktop/FFmpeg-Learning/FFmpeg-tutorial/40M.mp4";
    // http://123.126.32.41/yudong/2016/LiveHlsVerimatrix/stream-007/stream-vod-noenc.m3u8
//    const char * fileName = "http://123.126.32.41/yudong/2016/LiveHlsVerimatrix/stream-007/stream-vod-noenc.m3u8";
    
//    if(argc < 2) {
//        printf("Please provide a movie file\n");
//        return -1;
//    }
    // Register all formats and codecs
    av_register_all(); // import function
    
    avformat_network_init(); //
    avcodec_register_all();
    
    av_log(NULL, AV_LOG_INFO, "===== versions =====\n");
//    ffp_show_version_str("FFmpeg",         av_version_info()); // 后续的版本才有的函数
    ffp_show_version_int("libavutil",      avutil_version());
    ffp_show_version_int("libavcodec",     avcodec_version());
    ffp_show_version_int("libavformat",    avformat_version());
    ffp_show_version_int("libswscale",     swscale_version());
//    ffp_show_version_int("libswresample",  swresample_version());
    
    
    // Open video file
    if(avformat_open_input(&pFormatCtx, fileName, NULL, NULL)!=0)
        return -1; // Couldn't open file
    
    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        return -1; // Couldn't find stream information
    
    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, fileName, 0);
    
    // Find the first video stream
    /*
     *  这个Demo只取videostream
     */
    
    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    if(videoStream==-1)
        return -1; // Didn't find a video stream
    
    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;
    
    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }
    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
        return -1; // Could not open codec
    
    // Allocate video frame
    pFrame=av_frame_alloc();
    
    // Allocate an AVFrame structure
    pFrameRGB=av_frame_alloc();
    if(pFrameRGB==NULL)
        return -1;
    
    // Determine required buffer size and allocate buffer
    numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
                                pCodecCtx->height);
    buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    
    // 创建一个 SwsContext
     sws_ctx = sws_getContext(pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
     pCodecCtx->width,pCodecCtx->height,PIX_FMT_RGB24,
                   SWS_BILINEAR,
                   NULL,NULL,NULL
     );
    
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
                   pCodecCtx->width, pCodecCtx->height);
    
    // Read frames and save first five frames to disk
    i=0;
    while(av_read_frame(pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,
                                  &packet);
            
            // Did we get a video frame?
            if(frameFinished) {
                // Convert the image from its native format to RGB
                sws_scale
                (
                 sws_ctx,
                 (uint8_t const * const *)pFrame->data,
                 pFrame->linesize,
                 0,
                 pCodecCtx->height,
                 pFrameRGB->data,
                 pFrameRGB->linesize
                 );
                
                // Save the frame to disk
                if(++i<=100)
                    SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height,
                              i);
            }
        }
        
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }
    
    // Free the RGB image
    av_free(buffer);
    av_free(pFrameRGB);
    
    // Free the YUV frame
    av_free(pFrame);
    
    // Close the codec
    avcodec_close(pCodecCtx);
    
    // Close the video file
    avformat_close_input(&pFormatCtx);
    
    return 0;
}



