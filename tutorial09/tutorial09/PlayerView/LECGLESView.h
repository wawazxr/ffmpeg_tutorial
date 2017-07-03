//
//  LECGLESView.h
//  LECMediaPlayer
//
//  Created by zjc on 16/4/19.
//  Copyright © 2016年 zjc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "LECVoutOverlay.h"
#import "ijksdl_fourcc.h"

@interface LECGLESView : UIView

- (id) initWithFrame:(CGRect)frame;
- (void) display: (SDL_VoutOverlay *) overlay;

- (UIImage*) snapshot;

@property(nonatomic,strong) NSLock  *appActivityLock;
@property(nonatomic)        CGFloat  fps;
@property(nonatomic)        CGFloat  scaleFactor;

@end