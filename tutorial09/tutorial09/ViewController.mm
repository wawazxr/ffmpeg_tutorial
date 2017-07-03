//
//  ViewController.m
//  tutorial09
//
//  Created by zjc on 2017/6/30.
//  Copyright © 2017年 lecloud. All rights reserved.
//

#import "ViewController.h"
#include "player/player.h"
#import "LECGLESView.h"

@interface ViewController ()

@property (nonatomic, strong) LECGLESView * glesView;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    // 修改为.mm 文件，支持对cpp 函数的调用
    
    _glesView = [[LECGLESView alloc] initWithFrame:CGRectMake(0, 0, 300, 400)];
    [self.view addSubview:_glesView];
   
}

- (void) viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
     main_player(_glesView);
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
