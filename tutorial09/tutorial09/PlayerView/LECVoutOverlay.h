//
//  LECVoutOverlay.h
//  LECMediaPlayer
//
//  Created by zjc on 16/4/20.
//  Copyright © 2016年 zjc. All rights reserved.
//

#ifndef LECVoutOverlay_h
#define LECVoutOverlay_h

#import <UIKit/UIKit.h>

#define LEC_NUM_DATA_POINTERS 8


typedef struct SDL_VoutOverlay SDL_VoutOverlay;
typedef struct SDL_VoutOverlay_Opaque SDL_VoutOverlay_Opaque;

struct SDL_VoutOverlay_Opaque {
    CVPixelBufferRef pixel_buffer;
    uint16_t pitches[LEC_NUM_DATA_POINTERS];
    uint8_t *pixels[LEC_NUM_DATA_POINTERS];
};

struct SDL_VoutOverlay {
    int w; /**< Read-only */
    int h; /**< Read-only */
    uint32_t format; /**< Read-only */
    int planes; /**< Read-only */
    uint16_t *pitches; /**< in bytes, Read-only */
    uint8_t **pixels; /**< Read-write */
    
    int is_private;
    
    int sar_num;
    int sar_den;
    SDL_VoutOverlay_Opaque  *opaque;
    
};

#endif /* LECVoutOverlay_h */
