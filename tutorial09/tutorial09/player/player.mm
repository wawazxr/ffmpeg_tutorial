extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include "player.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

//#include <SDL.h>
//#include <SDL_thread.h>

#include <iostream>

#include "PacketQueue.h"
#include "Audio.h"
#include "Media.h"
#include "VideoDisplay.h"

// 修改为.mm 文件，增加对OC代码调用的支持
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

using namespace std;

bool quit = false;

//int main(int argv, char* argc[])
static int event_thread(void *data) {
    
    MediaState *media = (MediaState*)data;
    SDL_Event event;
//    while (true) // SDL event loop
//    {
//        SDL_WaitEvent(&event);
//        switch (event.type)
//        {
//            case FF_QUIT_EVENT:
//            case SDL_QUIT:
//                quit = 1;
//                SDL_Quit();
//                
//                return 0;
//                break;
//                
//            case FF_REFRESH_EVENT:
//                video_refresh_timer(&media);
//                break;
//                
//            default:
//                break;
//        }
//    }
    
    return 0;
}

int main_player(UIView * pView)
{
    int sdlRet;
	av_register_all();
    av_version_info();
    
//    sdlRet = SDL_Init(SDL_INIT_TIMER);

	sdlRet = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    if (sdlRet < 0){
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        fprintf(stderr, "(Did you set the DISPLAY variable?)\n");
    }
    
//    char* filename = "/zjcletv/Desktop/FFmpeg-Learning/FFmpeg-tutorial/40M.mp4";
//	MediaState media(filename);
        
    NSString * videoPath = [[NSBundle mainBundle] pathForResource:@"40M" ofType:@"mp4"];
    MediaState media((char *)[videoPath UTF8String],(__bridge void *)pView);

    if (media.openInput()){
		SDL_CreateThread(decode_thread, "decode_thread", &media); //创建解码线程，读取packet到队列中缓存
    }
	media.audio->audio_play(); // create audio thread

	media.video->video_play(&media); // create video thread

	AVStream *audio_stream = media.pFormatCtx->streams[media.audio->stream_index];
	AVStream *video_stream = media.pFormatCtx->streams[media.video->stream_index];

	double audio_duration = audio_stream->duration * av_q2d(audio_stream->time_base);
	double video_duration = video_stream->duration * av_q2d(video_stream->time_base);

	cout << "audio 时长" << audio_duration << endl;
	cout << "video 时长" << video_duration << endl;

//    SDL_CreateThread(event_thread, "evnet_thread", &media); //创建解码线程，读取packet到队列中缓存
    

	SDL_Event event;
	while (true) // SDL event loop
	{
		SDL_WaitEvent(&event);
		switch (event.type)
		{
		case FF_QUIT_EVENT:
		case SDL_QUIT:
			quit = 1;
			SDL_Quit();

			return 0;
			break;

		case FF_REFRESH_EVENT:
			video_refresh_timer(&media);
			break;

		default:
			break;
		}
	}
//	getchar();
	return 0;
}


