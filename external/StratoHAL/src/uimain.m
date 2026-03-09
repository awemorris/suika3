/* -*- coding: utf-8; tab-width: 4; indent-tabs-mode: nil; -*- */

/*
 * StratoHAL
 * Main code for iOS
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

@import UIKit;

#import <AVFoundation/AVFoundation.h>
#import <GameController/GameController.h>

#import "stratohal/platform.h"
#import "stdfile.h"
#import "uimain.h"
#import "aunit.h"
#import "GameRenderer.h"
#import "GameViewControllerProtocol.h"

#import <sys/time.h>

// Window title.
char *window_title;

// Window size.
int screen_width;
int screen_height;

// View controller.
static ViewController *theViewController;

// Swipe mode.
static BOOL isContinuousSwipeEnabled;

// Forward declaration.
static void initGamepad(void);

//
// Main
//

int main2(int argc, char * argv[]) {
    setlocale(LC_NUMERIC, "C");
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

//
// AppDelegate
//

@interface AppDelegate ()
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application
didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    self.window.rootViewController = [ViewController new];
    [self.window makeKeyAndVisible];
    return YES;
}

@end

//
// ViewController
//

@interface ViewController ()
@end

@implementation ViewController
{
    // The GameView for AppKit
    GameView *_view;

    // The GameRender (common for AppKit and UIKit)
    GameRenderer *_renderer;

    // The screen information
    float _screenScale;
    CGSize _screenSize;
    CGPoint _screenOffset;

    // The video player objects and status.
    AVPlayer *_avPlayer;
    AVPlayerLayer *_avPlayerLayer;
    BOOL _isVideoPlaying;
}

- (void)loadView {
    self.view = [[GameView alloc] initWithFrame:[UIScreen mainScreen].bounds];
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAllButUpsideDown;
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];

    _view.frame = self.view.bounds;

    CGFloat scale = UIScreen.mainScreen.nativeScale;
    _view.contentScaleFactor = scale;
    _view.autoResizeDrawable = YES;
    _view.drawableSize = (CGSize){
        _view.bounds.size.width  * scale,
        _view.bounds.size.height * scale
    };
}

// Called when the view is loaded.
- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Initialize the file HAL.
    if(!init_file())
        exit(1);
    
    // Do a boot callback to acquire a screen configuration.
    if (!hal_callback_on_event_boot(&window_title, &screen_width, &screen_height))
        exit(1);
    
    // Initialize the sound HAL.
    if(!init_aunit())
        exit(1);
    
    // Create an MTKView.
    _view = (GameView *)self.view;
    _view.enableSetNeedsDisplay = YES;
    _view.device = MTLCreateSystemDefaultDevice();
    _view.clearColor = MTLClearColorMake(0.0, 0, 0, 1.0);
    _renderer = [[GameRenderer alloc] initWithMetalKitView:_view andController:self];
    if(!_renderer) {
        NSLog(@"Renderer initialization failed");
        return;
    }
    [_renderer mtkView:_view drawableSizeWillChange:_view.drawableSize];
    _view.delegate = _renderer;
    [self updateViewport:_view.frame.size];

    // Set multi-touch.
    self.view.multipleTouchEnabled = YES;

    // Do a start callback.
    if(!hal_callback_on_event_start())
        exit(1);

    // Start rendering.
    _view.backgroundColor = [UIColor blackColor];
    _view.preferredFramesPerSecond = 60;
    _view.enableSetNeedsDisplay = NO;
    _view.paused = NO;
    _view.delegate = _renderer;
    gameRendererStartFlag = TRUE;

    // Initialize the gamepad.
    initGamepad();
}

// Initialize the gamepad.
static void initGamepad(void)
{
    [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification
                                                      object:nil
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification *note) {
        GCController *controller = note.object;

        controller.extendedGamepad.dpad.up.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            if (pressed) {
                hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_UP);
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_DOWN);
            } else {
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_UP);
            }
        };
        controller.extendedGamepad.dpad.down.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            if (pressed) {
                hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_DOWN);
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_UP);
            } else {
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_DOWN);
            }
        };
        controller.extendedGamepad.dpad.left.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            if (pressed) {
                hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_LEFT);
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_RIGHT);
            } else {
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_LEFT);
            }
        };
        controller.extendedGamepad.dpad.right.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            if (pressed) {
                hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_RIGHT);
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_LEFT);
            } else {
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_RIGHT);
            }
        };
        controller.extendedGamepad.buttonA.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            if (pressed) 
                hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_A);
            else
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_A); 
        };
        controller.extendedGamepad.buttonB.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            if (pressed)
                hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_B);
            else
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_B);
        };
        controller.extendedGamepad.buttonX.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            if (pressed)
                hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_X);
            else
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_X);
        };
        controller.extendedGamepad.buttonY.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            if (pressed)
                hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_Y);
            else
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_Y);
        };
        controller.extendedGamepad.leftShoulder.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            if (pressed)
                hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_L);
            else
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_L);
        };
        controller.extendedGamepad.rightShoulder.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            if (pressed)
                hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_R);
            else
                hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_A);
        };
        controller.extendedGamepad.leftThumbstick.xAxis.valueChangedHandler = ^(GCControllerAxisInput *axis, float value) {
            hal_callback_on_event_analog_input(HAL_ANALOG_X1, (int)(value * 32767));
        };
        controller.extendedGamepad.leftThumbstick.yAxis.valueChangedHandler = ^(GCControllerAxisInput *axis, float value) {
            hal_callback_on_event_analog_input(HAL_ANALOG_Y1, (int)(value * 32767));
        };
        controller.extendedGamepad.rightThumbstick.xAxis.valueChangedHandler = ^(GCControllerAxisInput *axis, float value) {
            hal_callback_on_event_analog_input(HAL_ANALOG_X2, (int)(value * 32767));
        };
        controller.extendedGamepad.rightThumbstick.yAxis.valueChangedHandler = ^(GCControllerAxisInput *axis, float value) {
            hal_callback_on_event_analog_input(HAL_ANALOG_Y2, (int)(value * 32767));
        };
        controller.extendedGamepad.leftTrigger.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            hal_callback_on_event_analog_input(HAL_ANALOG_L, (int)(value * 32767));
        };
        controller.extendedGamepad.rightTrigger.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            hal_callback_on_event_analog_input(HAL_ANALOG_R, (int)(value * 32767));
        };
    }];
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:^{
        NSLog(@"Finished scanning for controllers.");
    }];
}

// Calculate the viewport size.
- (void)updateViewport:(CGSize)newViewSize {
    // If called before the view initialization.
    if (newViewSize.width == 0 || newViewSize.height == 0)
        return;

    // Get the aspect ratio of the game.
    float aspect = (float)screen_height / (float)screen_width;

    // 1. Use width-first.
    _screenSize.width = newViewSize.width;
    _screenSize.height = _screenSize.width * aspect;
    _screenScale = (float)screen_width / (float)_screenSize.width;
    
    // 2. If the height is not enough, use height-first.
    if(_screenSize.height > newViewSize.height) {
        _screenSize.height = newViewSize.height;
        _screenSize.width = _screenSize.height / aspect;
        _screenScale = (float)screen_height / (float)_screenSize.height;
    }
    
    // Calculate the scale factor and the margin for touch position.
    _view.left = (float)(newViewSize.width - _screenSize.width) / 2.0f;
    _view.top = (float)(newViewSize.height - _screenSize.height) / 2.0f;
    _view.scale = _screenScale;

    // Multiply the scale factor of MTKView.
    _screenScale *= (float)_view.layer.contentsScale;
    _screenSize.width *= _view.layer.contentsScale;
    _screenSize.height *= _view.layer.contentsScale;
    _screenOffset.x = _view.left * _view.layer.contentsScale;
    _screenOffset.y = _view.top * _view.layer.contentsScale;
}

// Called when the view appeared.
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self updateViewport:_view.frame.size];
}

// Get the screen scale.
- (float)screenScale {
    return _screenScale;
}

// Get the screen offset.
- (CGPoint)screenOffset {
    return _screenOffset;
}

// Get the screen size.
- (CGSize)screenSize {
    return _screenSize;
}

// Check whether we are playing back a video.
- (BOOL)isVideoPlaying {
    return _isVideoPlaying;
}

// Play a video.
- (void)playVideoWithPath:(NSString *)path skippable:(BOOL)isSkippable {
    // Create a player.
    AVPlayerItem *playerItem = [[AVPlayerItem alloc] initWithURL:[NSURL fileURLWithPath:path]];
    _avPlayer = [[AVPlayer alloc] initWithPlayerItem:playerItem];

    // Create a layer for the player.
    _avPlayerLayer = [AVPlayerLayer playerLayerWithPlayer:_avPlayer];
    [_avPlayerLayer setFrame:self.view.bounds];
    [self.view.layer addSublayer:_avPlayerLayer];

    // Set the finish notification.
    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(onPlayEnd:)
                                               name:AVPlayerItemDidPlayToEndTimeNotification
                                             object:playerItem];

    // Start playing.
    [_avPlayer play];

    _isVideoPlaying = YES;
}

// Called when a video playback is finished.
- (void)onPlayEnd:(NSNotification *)notification {
    [_avPlayer replaceCurrentItemWithPlayerItem:nil];
    _isVideoPlaying = NO;
}

// Stop a video playback.
- (void)stopVideo {
    if (_avPlayer != nil) {
        [_avPlayer replaceCurrentItemWithPlayerItem:nil];
        _isVideoPlaying = NO;
        _avPlayer = nil;
        _avPlayerLayer = nil;
    }
}

// Not used in the iOS app.
- (void)setWindowTitle:(NSString *)name {
}

// Not used in the iOS app.
- (void)enterFullScreen {
}

// Not used in the iOS app.
- (BOOL)isFullScreen { 
    return NO;
}

// Not used in the iOS app.
- (void)leaveFullScreen { 
}

// Not used in the iOS app.
- (CGPoint)windowPointToScreenPoint:(CGPoint)windowPoint { 
    return windowPoint;
}

@end

//
// GameView
//

@implementation GameView
{
    BOOL _isTouch;
    int _touchStartX;
    int _touchStartY;
    int _touchLastY;
}

+ (Class)layerClass {
    return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        // Init here.
    }
    return self;
}

// Called when touches began.
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    _isTouch = YES;

    UITouch *touch = [[event allTouches] anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    _touchStartX = (int)((touchLocation.x - self.left) * self.scale);
    _touchStartY = (int)((touchLocation.y - self.top) * self.scale);
    _touchLastY = _touchStartY;

    hal_callback_on_event_mouse_press(HAL_MOUSE_LEFT, _touchStartX, _touchStartY);
}

// Called when touched moved.
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [[event allTouches] anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    int touchX = (int)((touchLocation.x - self.left) * self.scale);
    int touchY = (int)((touchLocation.y - self.top) * self.scale);

	// Emulate a wheel down.
	const int FLICK_Y_DISTANCE = 30;
	int deltaY = touchY - _touchLastY;
	_touchLastY = touchY;
    if (isContinuousSwipeEnabled) {
        if (deltaY > 0 && deltaY < FLICK_Y_DISTANCE) {
            hal_callback_on_event_key_press(HAL_KEY_DOWN);
            hal_callback_on_event_key_release(HAL_KEY_DOWN);
            return;
        }
    }

	// Emulate a mouse move.
    hal_callback_on_event_mouse_move((int)touchX, (int)touchY);
}

// Called when touches ended.
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    _isTouch = NO;

    UITouch *touch = [[event allTouches] anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    int touchEndX = (int)((touchLocation.x - self.left) * self.scale);
    int touchEndY = (int)((touchLocation.y - self.top) * self.scale);

    // Detect a down/up swipe.
	const int FLICK_Y_DISTANCE = 50;
	int deltaY = touchEndY - _touchStartY;
	if (deltaY > FLICK_Y_DISTANCE) {
        hal_callback_on_event_touch_cancel();
        hal_callback_on_event_swipe_down();
        return;
	} else if (deltaY < -FLICK_Y_DISTANCE) {
        hal_callback_on_event_touch_cancel();
        hal_callback_on_event_swipe_up();
        return;
    }

	// Emulate a left click.
	const int FINGER_DISTANCE = 10;
    if ([[event allTouches] count] == 1 &&
        abs(touchEndX - _touchStartX) < FINGER_DISTANCE &&
	    abs(touchEndY - _touchStartY) < FINGER_DISTANCE) {
        hal_callback_on_event_touch_cancel();
        hal_callback_on_event_mouse_press(HAL_MOUSE_LEFT, touchEndX, touchEndY);
        hal_callback_on_event_mouse_release(HAL_MOUSE_LEFT, touchEndX, touchEndY);
        return;
    }

    // Emulate a right click.
    if ([[event allTouches] count] == 2 &&
        abs(touchEndX - _touchStartX) < FINGER_DISTANCE &&
        abs(touchEndY - _touchStartY) < FINGER_DISTANCE) {
        hal_callback_on_event_touch_cancel();
        hal_callback_on_event_mouse_press(HAL_MOUSE_RIGHT, touchEndX, touchEndY);
        hal_callback_on_event_mouse_release(HAL_MOUSE_RIGHT, touchEndX, touchEndY);
        return;
    }

    // Cancel the touch move.
    hal_callback_on_event_touch_cancel();
}

@end

//
// HAL
//

// Show an INFO log.
bool
hal_log_info(
        const char *s,
        ...)
{
    char buf[1024];
    va_list ap;
    
    va_start(ap, s);
    vsnprintf(buf, sizeof(buf), s, ap);
    va_end(ap);

    NSLog(@"%s", buf);
    
    return true;
}

// Show a WARN log.
bool
hal_log_warn(
        const char *s,
        ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, s);
    vsnprintf(buf, sizeof(buf), s, ap);
    va_end(ap);

    NSLog(@"%s", buf);

    return true;
}

// Show an ERROR log.
bool
hal_log_error(
        const char *s,
        ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, s);
    vsnprintf(buf, sizeof(buf), s, ap);
    va_end(ap);

    NSLog(@"%s", buf);

    return true;
}

// Show an out-of-memory error.
bool
hal_log_out_of_memory(void)
{
    hal_log_error("Out of memory.");

    return true;
}

// Make a save directory.
bool
hal_make_save_directory(void)
{
    @autoreleasepool {
        NSString *path = [NSString stringWithFormat:@"%@/%@/%s/sav",
                          NSHomeDirectory(),
                          @"/Library/Application Support",
                          window_title];
        NSFileManager *manager = [NSFileManager defaultManager];
        NSError *error;
        if(![manager createDirectoryAtPath:path
               withIntermediateDirectories:YES
                                attributes:nil
                                     error:&error]) {
            NSLog(@"createDirectoryAtPath error: %@", error);
            return false;
        }
        return true;
    }
}

// Get a real path for a file.
char *
hal_make_real_path(
        const char *fname)
{
    @autoreleasepool {
        // If a save file:
            if(strncmp(fname, "save/", 5) == 0) {
            // Return a "Application Support" path.
            NSString *path = [NSString stringWithFormat:@"%@/%@/%s/save/%s",
                              NSHomeDirectory(),
                              @"/Library/Application Support",
                              window_title,
                              fname];
            const char *cstr = [path UTF8String];
            return strdup(cstr);
        }

        // If an mp4 file:
        if(strncmp(fname, "video/", 4) == 0) {
            // Return an bundle resource path.
            *strstr(fname, ".") = '\0';
            NSString *basename = [NSString stringWithFormat:@"video/%s", fname];
            NSString *path = [[NSBundle mainBundle] pathForResource:basename ofType:@"mp4"];
            const char *cstr = [path UTF8String];
            return strdup(cstr);
        }

        // If the package:
        if(strcmp(fname, HAL_PACKAGE_FILE) == 0) {
            // Return a bundle resource path.
            NSString *path = [[NSBundle mainBundle] pathForResource:@"assets" ofType:@"pak"];
            const char *cstr = [path UTF8String];
            return strdup(cstr);
        }

        // We cannot load other files.
        return strdup("dummy");
    }
}

// Reset a lap timer.
void
hal_reset_lap_timer(
        uint64_t *origin)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    *origin = (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

// Get a timer lap
uint64_t
hal_get_lap_timer_millisec(
        uint64_t *origin)
{
    struct timeval tv;
    uint64_t now;

    gettimeofday(&tv, NULL);
    now = (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
    if (now < *origin) {
        hal_reset_lap_timer(origin);
        return 0;
    }
    return (uint64_t)(now - *origin);
}

// Play a video.
bool
hal_play_video(
        const char *fname,
        bool is_skippable)
{
    // Make a path.
    char *cpath = hal_make_real_path(fname);
    assert(cpath != NULL);
    NSString *path = [[NSString alloc] initWithUTF8String:cpath];
    free(cpath);
    path = [path stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLPathAllowedCharacterSet]];

    // Play a video.
    [theViewController playVideoWithPath:path skippable:is_skippable ? YES : NO];

    return true;
}

// Stop a video.
void
hal_stop_video(void)
{
    [theViewController stopVideo];
}

// Check if video is playing back.
bool
hal_is_video_playing(void)
{
    return [theViewController isVideoPlaying] ? true : false;
}

// Check if the full screen mode is supported.
bool
hal_is_full_screen_supported(void)
{
    return true;
}

// Check if operating in the full screen mode.
bool
hal_is_full_screen_mode(void)
{
    return [theViewController isFullScreen];
}

// Enter the full screen mode.
void
hal_enter_full_screen_mode(void)
{
    [theViewController enterFullScreen];
}

// Leave the full screen mode.
void
hal_leave_full_screen_mode(void)
{
    [theViewController leaveFullScreen];
}

// Get a system language.
const char *
hal_get_system_language(void)
{
    NSString *language = [[NSLocale preferredLanguages] objectAtIndex:0];
    if ([language hasPrefix:@"ja"])
        return "ja";
    if ([language hasPrefix:@"en"])
        return "en";
    if ([language hasPrefix:@"fr"])
        return "fr";
    if ([language hasPrefix:@"de"])
        return "de";
    if ([language hasPrefix:@"es"])
        return "es";
    if ([language hasPrefix:@"it"])
        return "it";
    if ([language hasPrefix:@"el"])
        return "el";
    if ([language hasPrefix:@"ru"])
        return "ru";
    if ([language hasPrefix:@"zh-Hans"])
        return "zh";
    if ([language hasPrefix:@"zh-Hant"])
        return "tw";
    return "other";
}

// Not used in macOS.
void
hal_set_continuous_swipe_enabled(
        bool is_enabled)
{
    isContinuousSwipeEnabled = is_enabled;
}
