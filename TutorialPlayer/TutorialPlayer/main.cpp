//
//  main.cpp
//  TutorialPlayer
//
//  Created by zjc on 16/6/22.
//  Copyright © 2016年 zjc. All rights reserved.
//

#include <iostream>
#include <stdio.h>

extern "C" {
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

PacketQueue audioq;
int quit = 0;

void SaveFrame(AVFrame *pFrame, int width, int height, int index);
void audioCallback (void *userdata, Uint8 * stream, int len);
int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size);
int packet_queue_put(PacketQueue *q, AVPacket *pkt);
void packet_queue_init(PacketQueue *queue);
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block);

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    
    AVFormatContext *pFormatCtx = NULL;

    // 放在product
    const char *fileName = "/zjcletv/Desktop/FFmpeg-Learning/FFmpeg-Tutorial/TutorialPlayer/TutorialPlayer/40M.mp4";

    av_register_all();
    avformat_network_init();
    avcodec_register_all();
    
    int i_avformatInput = avformat_open_input(&pFormatCtx, fileName, NULL, NULL);
    if (i_avformatInput < 0) {
        return -1;
    }
    
    int i_avfindStream = avformat_find_stream_info(pFormatCtx, NULL);
    if (i_avfindStream < 0) {
        return -1;
    }
    
    av_dump_format(pFormatCtx, 0, fileName, 0);
    
    int videoStream = -1;
    int audioStream = -1;
    int i;
    
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
        }else if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audioStream = i;
        }
    }
    
    if (videoStream == -1 || audioStream == -1) {
        return -1;
    }

    AVCodecContext *pCodecCtx = NULL;
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    AVCodec *pCodec = NULL;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }
    
    int i_codecopen = avcodec_open2(pCodecCtx, pCodec, NULL);
    if (i_codecopen < 0) {
        return -1;
    }
    
    AVFrame *pFrame = NULL;
//    AVFrame *pFrameRGB = NULL;
    AVFrame *pFrameYUV = NULL;
    
    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
//    pFrameRGB = av_frame_alloc();
    if (pFrameYUV == NULL || pFrame == NULL) {
        return -1;
    }
    
////    PIX_FMT_RGB24
//    int numBytes;
//    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
//    uint8_t *buffer ;
//    buffer = (uint8_t *)av_malloc(sizeof(uint8_t) * numBytes);
//    
//    SwsContext  *sws_ctx = NULL;
//    sws_ctx = sws_getContext(pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
//                             pCodecCtx->width,pCodecCtx->height,PIX_FMT_RGB24,
//                             SWS_BILINEAR,
//                             NULL,NULL,NULL
//                             );
//    
//    // 填充 pFrameRGB
//    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
//                   pCodecCtx->width, pCodecCtx->height);
    
    int i_sdl = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    if (i_sdl > 0) {
        std::cout << "Could not initialize SDL - "  << SDL_GetError() << std::endl;
        exit(1);
    }
    
    // 使用SDL 打开电脑的音频设备
    SDL_AudioSpec   wanted_spec, spec;
    AVCodecContext *aCodecCtx = NULL;
    aCodecCtx = pFormatCtx->streams[audioStream]->codec;
    int count = SDL_GetNumAudioDevices(0); // 获取设备的音频数量
    if (count > 0) {
        std::cout << "there is a new" << std::endl;
    }
    
    // set audio setting form codec info
    wanted_spec.freq = aCodecCtx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = aCodecCtx->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = audioCallback;
    wanted_spec.userdata = aCodecCtx;
    
    
    SDL_AudioDeviceID   dev;
    dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (dev == 0) {
        std::cout<< "Failed to open audio" << std::endl;
    } else {
        if (wanted_spec.format != spec.format) {
            return -1;
        }
    }
    AVCodec         *aCodec = NULL;
    aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if (!aCodec) {
        return -1;
    }
    avcodec_open2(aCodecCtx, aCodec, NULL);
    
    packet_queue_init(&audioq);
    SDL_PauseAudioDevice(dev, 0);
    
    
    SDL_Window *screen = NULL;
    screen = SDL_CreateWindow("FFmpeg Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, pCodecCtx->width, pCodecCtx->height, SDL_WINDOW_OPENGL);
    if (!screen) {
        std::cout << "could not create window - exiting" << std::endl;
        exit(1);
    }
    
    SDL_Renderer *render = NULL;
    render = SDL_CreateRenderer(screen, 1, SDL_RENDERER_ACCELERATED);
    if (!render) {
        std::cout << "could not create render - exiting" <<std::endl;
        exit(1);
    }
    SDL_RenderClear(render);
    
    SDL_Texture *texture = NULL;
    texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);
    if (!texture) {
        std::cout << "could not create texture - exiting " << std::endl;
        exit(1);
    }
    
    SDL_Rect        rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = pCodecCtx->width;
    rect.h = pCodecCtx->height;
    
    
    SwsContext  *sws_ctx = NULL;
    // 创建一个转换 yuv420p 的 swscontext
    sws_ctx = sws_getContext(pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
                                 pCodecCtx->width,pCodecCtx->height,AV_PIX_FMT_YUV420P,
                                 SWS_BILINEAR,
                                 NULL,NULL,NULL
                                 );
    Uint8 *yPlane, *uPlane, *vPlane;
    size_t yPlaneSz, uvPlaneSz;
    yPlaneSz = pCodecCtx->width * pCodecCtx ->height;
    uvPlaneSz = pCodecCtx->width * pCodecCtx->height /4;
    
    yPlane = (Uint8 *)malloc(yPlaneSz);
    uPlane = (Uint8 *)malloc(uvPlaneSz);
    vPlane = (Uint8 *)malloc(uvPlaneSz);
    
    if (!yPlane || !uPlane || !vPlane) {
        fprintf(stderr, "Could not allocate pixel buffer -exiting\n");
        exit(1);
    }
    int uvPitch;
    uvPitch = pCodecCtx->width /2 ;
    
    i = 0;
    AVPacket        packet;
    int frameFinished;
    while (av_read_frame(pFormatCtx, &packet)>=0) {
        if (packet.stream_index == videoStream) { //
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet); // 解码
            if (frameFinished) {
//                AVPicture pict;
                pFrameYUV->data[0] = yPlane;
                pFrameYUV->data[1] = uPlane;
                pFrameYUV->data[2] = vPlane;
                pFrameYUV->linesize[0] = pCodecCtx->width;
                pFrameYUV->linesize[1] = uvPitch;
                pFrameYUV->linesize[2] = uvPitch;
                
                
                // 对 avFrame 进行变化得到
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

//                SDL_UpdateYUVTexture(texture, NULL, yPlane, pCodecCtx->width, uPlane, uvPitch, vPlane, uvPitch);
                SDL_UpdateYUVTexture(texture, &rect, pFrameYUV->data[0], pCodecCtx->width, pFrameYUV->data[1], uvPitch, pFrameYUV->data[2], uvPitch);
                SDL_RenderClear(render);
                SDL_RenderCopy(render, texture, NULL, &rect);
                SDL_RenderPresent(render);
                
                SDL_Delay(38);
                
                av_packet_unref(&packet); //  unref packet
//                if (++i <= 50) {
//                    //
////                    SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height,
////                              i);
//                }
                
            }
           
        }else if (packet.stream_index == audioStream){
            // 保存 音频的数据包
            packet_queue_put(&audioq,&packet);
        }else{
            // never
        }
        
//        av_free_packet(&packet);
//         av_packet_unref(&packet);
        SDL_Event event;
        SDL_PollEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
                quit = 1;
                SDL_DestroyTexture(texture);
                SDL_DestroyRenderer(render);
                SDL_DestroyWindow(screen);
                SDL_Quit();
                exit(1);
                break;
                
            default:
                break;
        }
        
        
    }
    
//    av_free(buffer);
    av_free(pFrameYUV);
    av_free(pFrame);
    free(yPlane);
    free(uPlane);
    free(vPlane);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    
    return 0;
}

// 测试转换成RGB24 并将文件保存在本地

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
    FILE *pFile;
    char szFilename[32];
    
    
    // Open file
    // 字符串格式化命令，主要功能是把格式化的数据写入某个字符串中。sprintf 是个变参函数。
    sprintf(szFilename, "frame%d.pg", iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;
    
    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    
    // Write pixel data
    
    // linesize[0] = 1272 / w * h = 424 *240
    int  y;
    for(y=0; y<height; y++)
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
    
    // Close file
    fclose(pFile);
}

void audioCallback (void *userdata, Uint8 * stream,
                                            int len) {
    AVCodecContext *aCodecCtx = (AVCodecContext *) userdata;
    int len1 = 0;
    int audio_size = 0;
    
    static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE *3) /2];
    static unsigned int  audio_buf_size = (MAX_AUDIO_FRAME_SIZE * 3) / 2;
    static unsigned int audio_buf_index = 0;
    
    while (len > 0) {
        if (audio_buf_index >= audio_buf_size) {
            //  开始解码
            audio_size = audio_decode_frame(aCodecCtx, audio_buf, audio_buf_size);
            if (audio_size < 0) {
                audio_buf_size = 1024;
                memset(audio_buf, 0, audio_buf_size);
            } else{
                audio_buf_size = audio_size;
            }
            audio_buf_index = 0;
        }
        
        len1 = audio_buf_size - audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }
        
        memcpy(stream, audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}

int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size){
    static AVPacket pkt;
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;
    static AVFrame frame;
    
    int len1, data_size = 0;
    uint8_t *out[] = {audio_buf};
    
    // resample
    uint64_t wanted_channel_layout = 0;
    wanted_channel_layout = aCodecCtx->channel_layout;
    SwrContext* t_audio_conv = swr_alloc_set_opts(NULL,
                                                  wanted_channel_layout,AV_SAMPLE_FMT_S16,aCodecCtx->sample_rate,
                                                  wanted_channel_layout,aCodecCtx->sample_fmt, aCodecCtx->sample_rate,
                                                  0,NULL);
    swr_init(t_audio_conv);
    
    for (; ; ) {
        while (audio_pkt_size > 0) {
            int got_frame = 0;
            len1 = avcodec_decode_audio4(aCodecCtx, &frame, &got_frame, &pkt);
            if (len1 < 0) {
                audio_pkt_size = 0;
                break;
            }
            audio_pkt_data += len1;
            audio_pkt_size -= len1;
            if (got_frame) {
                int size1 = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
                int len = swr_convert(t_audio_conv, out, buf_size/aCodecCtx->channels/size1, (const uint8_t **)frame.extended_data, frame.nb_samples);
                len = len * aCodecCtx->channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
                av_packet_unref(&pkt);
                swr_free(&t_audio_conv);
                return len;
            }
            if (data_size <= 0) {
                continue;
            }
            return data_size;
        }
        if (pkt.data) {
            av_packet_unref(&pkt);
        }
        if (quit) {
            return -1;
        }
        if (packet_queue_get(&audioq, &pkt, 1) < 0) {
            return -1;
        }
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
        
    }
//    return 1;
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt){
    
    AVPacketList *pkt1;
    if (av_packet_ref(pkt, pkt)) {
        return -1;
    }
    
    pkt1 = (AVPacketList *)av_malloc(sizeof(AVPacketList));
    if (!pkt1) {
        return -1;
    }
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
    
    SDL_LockMutex(q->mutex);
    if (!q->last_pkt) {
        q->first_pkt = pkt1;
    }else{
        q->last_pkt->next = pkt1;
    }
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    SDL_CondSignal(q->cond);
    
    SDL_UnlockMutex(q->mutex);
    return 0;
}

void packet_queue_init(PacketQueue *queue) {
    memset(queue, 0, sizeof(PacketQueue));
    queue->mutex = SDL_CreateMutex();
    queue->cond = SDL_CreateCond();
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    AVPacketList *pkt1;
    int ret;
    
    SDL_LockMutex(q->mutex);
    for (; ; ) {
        if (quit) {
            ret = -1;
            break;
        }
        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt) {
                q->last_pkt = NULL;
            }
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block){
            ret = 0;
            break;
        } else{
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    
    SDL_UnlockMutex(q->mutex);
    return ret;
    
}
