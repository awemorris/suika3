/* -*- coding: utf-8; tab-width: 4; indent-tabs-mode: nil; -*- */

/*
 * StratoHAL
 * Main code for macOS
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

@import AppKit;
@import MetalKit;

#import <AVFoundation/AVFoundation.h>
#import <GameController/GameController.h>

#import <stratohal/platform.h>
#import "stdfile.h"
#import "nsmain.h"
#import "aunit.h"
#import "GameRenderer.h"
#import "GameViewControllerProtocol.h"

// Is a bundled app?
bool is_bundled = true;

// Window title.
char *window_title;

// Window size.
int screen_width;
int screen_height;

// View controller.
static ViewController *theViewController;

// Log window.
static NSWindow *theLogWindow;
static NSTextView *theLogTextView;

// Log file.
static char *logFilePath;

// Release mode.
static bool releaseMode;

// Forward declaration.
static void checkBundleResource(int argc, const char *argv[]);
static void initGamepad(void);
static void openLogWindow(void);
static void putTextToLogWindow(const char *text);
static FILE *openLog(void);
static void showLogAtExit(void);

//
// main
//

#if defined(USE_MAIN2)
#define main main2
#endif

int main(int argc, const char *argv[]) {
    // Use "." for numeric points.
    setlocale(LC_NUMERIC, "C");

    // Check if we have valid game data as a resource.
    checkBundleResource(argc, argv);

    // Run.
    [NSApplication sharedApplication];
    [NSApp setDelegate:[[AppDelegate alloc] init]];
    [NSApp run];

    showLogAtExit();

    return 0;
}

static void checkBundleResource(int argc, const char *argv[])
{
    if (argc > 2) {
        if (strcmp(argv[1], ".") == 0) {
            is_bundled = false;
            return;
        }
    }

    NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
    if (![bundlePath hasSuffix:@".app"]) {
        is_bundled = false;
        return;
    }

    NSString *filePath = [NSString stringWithFormat:@"%@/Contents/Resources/%s", bundlePath, HAL_PACKAGE_FILE];
    if ([[NSFileManager defaultManager] fileExistsAtPath:filePath]) {
        // Test the resouce file size.
        NSError *attributesError = nil;
        NSDictionary *fileAttributes = [[NSFileManager defaultManager] attributesOfItemAtPath:filePath error:&attributesError];
        NSNumber *fileSizeNumber = [fileAttributes objectForKey:NSFileSize];
        long long fileSize = [fileSizeNumber longLongValue];
        if (fileSize > 100) {
            // Valid resource found. We will use a resource file.
            releaseMode = true;
        } else {
            // Valid resource not found. We will use files in the directory where the app bundle exists.
            releaseMode = false;
        }
    }
}

//
// AppDelegate
//

@interface AppDelegate ()
@property (strong) NSWindow *window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Make a window.
    self.window = [[NSWindow alloc]
                          initWithContentRect:NSMakeRect(0, 0, 640, 480)
                                    styleMask:(NSWindowStyleMaskTitled |
                                               NSWindowStyleMaskClosable |
                                               NSWindowStyleMaskResizable)
                                      backing:NSBackingStoreBuffered
                                        defer:NO];

    // Make a view controller.
    [self.window setContentViewController:[[ViewController alloc] init]];
    [self.window makeKeyAndOrderFront:nil];

    // Make a menu bar.
    NSMenu *menubar = [[NSMenu alloc] init];
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];

    // Make a menu item.
    NSMenu *appMenu = [[NSMenu alloc] init];
    NSString *appName = [[NSProcessInfo processInfo] processName];
    NSMenuItem *quitMenuItem = [[NSMenuItem alloc]
                                       initWithTitle:[@"Quit " stringByAppendingString:appName]
                                              action:@selector(terminate:)
                                       keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];

    // Make this non-bundled app front.
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
    ProcessSerialNumber psn = {0, kCurrentProcess};
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
    return YES;
}

@end

//
// ViewController
//  - Instanciated in Main.storyboard
//

@interface ViewController ()
@end

@implementation ViewController
{
    // The GameView (a subclass of MTKView)
    GameView *_view;

    // The GameRender (common for AppKit and UIKit)
    GameRenderer *_renderer;
    
    // The screen information
    float _screenScale;
    NSSize _screenSize;
    NSPoint _screenOffset;
    
    // The view frame (saved before entering a full screen mode)
    NSRect _savedViewFrame;
    
    // The temporary window frame size (for resizing)
    NSSize _resizeFrame;
    
    // The full screen status
    BOOL _isFullScreen;
    
    // The modifier key status
    BOOL _isShiftPressed;
    BOOL _isControlPressed;
    BOOL _isCommandPressed;
    
    // The video player objects and status
    AVPlayer *_avPlayer;
    AVPlayerLayer *_avPlayerLayer;
    BOOL _isVideoPlaying;
}

// For when we don't have XIB or Storyboard.
- (void)loadView {
    // Init for a non-bundled app.
    _view = [[GameView alloc] initWithFrame:NSMakeRect(0, 0, 640, 480)];
    self.view = _view;
}

// Called when the view is loaded.
- (void)viewDidLoad
{
    [super viewDidLoad];

    // Initialize the file HAL.
    if(!init_file()) {
        NSLog(@"File initialization failed.");
        [NSApp terminate:nil];
        exit(1);
    }

    // Do a boot callback to acquire a window configuration.
    if (!hal_callback_on_event_boot(&window_title, &screen_width, &screen_height)) {
        NSLog(@"Startup file failed.");
        showLogAtExit();
        [NSApp terminate:nil];
        return;
    }

    // Initialize the sound HAL.
    if(!init_aunit()) {
        NSLog(@"Audio initialization failed.");
        [NSApp terminate:nil];
        return;
    }

    // Create an MTKView.
    _view = (GameView *)self.view;
    _view.enableSetNeedsDisplay = YES;
    _view.device = MTLCreateSystemDefaultDevice();
    _view.clearColor = MTLClearColorMake(0.0, 0, 0, 1.0);
    _renderer = [[GameRenderer alloc] initWithMetalKitView:_view andController:self];
    if(!_renderer) {
        NSLog(@"Renderer initialization failed.");
        [NSApp terminate:nil];
        return;
    }
    [_renderer mtkView:_view drawableSizeWillChange:_view.drawableSize];
    _view.delegate = _renderer;
    [self updateViewport:_view.frame.size];

    // Setup a rendering timer.
    [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                     target:self
                                   selector:@selector(timerFired:)
                                   userInfo:nil
                                    repeats:YES];

    // Do a start callback.
    if(!hal_callback_on_event_start()) {
        NSLog(@"on_start() failed.");
        showLogAtExit();
        [NSApp terminate:nil];
        return;
    }
    gameRendererStartFlag = true;

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

// Called when the view is layouted.
- (void)viewDidLayout {
    [super viewDidLayout];
    
    self.view.window.delegate = self;
}

// Called when the view appeared.
- (void)viewDidAppear {
    self.view.window.delegate = self;
    
    // Set the window position and size.
    NSRect screenRect = [[NSScreen mainScreen] visibleFrame];
    NSRect contentRect = NSMakeRect(screenRect.origin.x + (screenRect.size.width - screen_width) / 2,
                                    screenRect.origin.y + (screenRect.size.height - screen_height) / 2,
                                    screen_width,
                                    screen_height);
    NSRect windowRect = [self.view.window frameRectForContentRect:contentRect];
    [self.view.window setFrame:windowRect display:TRUE];
    
    // Enable the window maximization.
    [self.view.window setCollectionBehavior:[self.view.window collectionBehavior] | NSWindowCollectionBehaviorFullScreenPrimary];
    
    // Set the window title.
    [self.view.window setTitle:[[NSString alloc] initWithUTF8String:window_title]];
    
    // Accept keyboard and mouse inputs.
    [self.view.window makeKeyAndOrderFront:nil];
    [self.view.window setAcceptsMouseMovedEvents:YES];
    [self.view.window makeFirstResponder:self];
    
    // Set the app name in the main menu.
    [self setAppName];

    // Recalculate the view size.
    [self updateViewport:_view.frame.size];
}

// A helper to set the app name.
- (void)setAppName {
    NSMenu *menu = [[[NSApp mainMenu] itemAtIndex:0] submenu];
    [menu setTitle:[[NSString alloc] initWithUTF8String:window_title]];
}

// Called every frame.
- (void)timerFired:(NSTimer *)timer {
    if (!gameRendererStartFlag)
        return;
    if (gameRendererExitFlag) {
        [NSApp stop:nil];
        return;
    }

    // Do a redraw later.
    [_view setNeedsDisplay:TRUE];
}

// Called when the window close button is pressed.
- (BOOL)windowShouldClose:(id)sender {
#if 0
    @autoreleasepool {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:@"Yes"];
        [alert addButtonWithTitle:@"No"];
        [alert setMessageText:@"Are you sure you want to exit?"];
        [alert setAlertStyle:NSAlertStyleWarning];
        if ([alert runModal] == NSAlertFirstButtonReturn)
            return YES;
        else
            return NO;
    }
#endif
    return YES;
}

// Called when the window is going to be closed.
- (void)windowWillClose:(NSNotification *)notification {
    // Exit the event loop.
    [NSApp stop:nil];

    // Magic: Post an empty event and make sure to exit the main loop.
    [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                        location:NSMakePoint(0, 0)
                                   modifierFlags:0
                                       timestamp:0
                                    windowNumber:0
                                         context:nil
                                         subtype:0
                                           data1:0
                                           data2:0]
             atStart:YES];
}

// Called when the Quit menu item is pressed.
- (IBAction)onQuit:(id)sender {
    if ([self windowShouldClose:sender])
        [NSApp stop:nil];
}

// Called before entering the full screen mode.
- (NSSize)window:(NSWindow *)window willUseFullScreenContentSize:(NSSize)proposedSize {
    // Calculate the viewport size.
    [self updateViewport:proposedSize];
    
    // Set the frame of the video player.
    if (_avPlayerLayer != nil)
        [_avPlayerLayer setFrame:NSMakeRect(_screenOffset.x, _screenOffset.y, _screenSize.width, _screenSize.height)];
    
    // Returns the screen size.
    return proposedSize;
}

// Called when entering the full screen mode.
- (void)windowWillEnterFullScreen:(NSNotification *)notification {
    // Set the flag.
    _isFullScreen = YES;
    
    // Save the window frame.
    _savedViewFrame = self.view.frame;

    // MAGIC: set the app name again to avoid the reset of the app name on the menu bar.
    [self setAppName];
}

// Called before returning to the windowed mode.
- (void)windowWillExitFullScreen:(NSNotification *)notification {
    // Set the flag.
    _isFullScreen = NO;
    
    // Reset the video player size.
    if(_avPlayerLayer != nil)
        [_avPlayerLayer setFrame:NSMakeRect(0, 0, _savedViewFrame.size.width, _savedViewFrame.size.height)];
    
    // Recalculate the viewport size.
    [self updateViewport:_savedViewFrame.size];
}

// Called when entered the full screen mode.
- (void)windowDidEnterFullScreen:(NSNotification *)notification {
    [self setAppName];
}

// Called when returned to the windowed mode.
- (void)windowDidLeaveFullScreen:(NSNotification *)notification {
    [self setAppName];
}

// Called when the window size is changing, including a window resize by mouse device.
- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize {
    // Get a new view rect from a window rect.
    NSRect contentRect = [self.view.window contentRectForFrameRect:NSMakeRect(0, 0, frameSize.width, frameSize.height)];

    // Calculate the content rect.
    float aspect = (float)screen_height / (float)screen_width;
    if (self.view.window.frame.size.width != frameSize.width)
        contentRect.size.height = contentRect.size.width * aspect;
    else
        contentRect.size.width = contentRect.size.height / aspect;

    // Save the contentRect.size for windowDidResize.
    _resizeFrame = contentRect.size;

    // Return the window size.
    NSRect windowRect = [self.view.window frameRectForContentRect:NSMakeRect(0, 0, contentRect.size.width, contentRect.size.height)];
    return windowRect.size;
}

// Called after a window resize.
- (void)windowDidResize:(NSNotification *)notification {
    // Recalculate the viewport size.
    [self updateViewport:_resizeFrame];
}

// Calculate the viewport size.
- (void)updateViewport:(NSSize)newViewSize {
    // For when the view is not initialized.
    if (newViewSize.width == 0 || newViewSize.height == 0) {
        _screenScale = 1.0f;
        _screenSize = NSMakeSize(screen_width, screen_height);
        _screenOffset.x = 0;
        _screenOffset.y = 0;
        return;
    }
    
    // Get the game aspect ratio.
    float aspect = (float)screen_height / (float)screen_width;
    
    // 1. Calculate by width-first.
    _screenSize.width = newViewSize.width;
    _screenSize.height = _screenSize.width * aspect;
    _screenScale = (float)screen_width / (float)_screenSize.width;
    
    // 2. If the height is not enough, use height-first.
    if(_screenSize.height > newViewSize.height) {
        _screenSize.height = newViewSize.height;
        _screenSize.width = _screenSize.height / aspect;
        _screenScale = (float)screen_height / (float)_screenSize.height;
    }

    // Multiply the scaling factor.
    _screenSize.width *= _view.layer.contentsScale;
    _screenSize.height *= _view.layer.contentsScale;
    newViewSize.width *= _view.layer.contentsScale;
    newViewSize.height *= _view.layer.contentsScale;

    // Calculate the offset.
    _screenOffset.x = (newViewSize.width - _screenSize.width) / 2.0f;
    _screenOffset.y = (newViewSize.height - _screenSize.height) / 2.0f;
}

// Called when a mouse is moved.
- (void)mouseMoved:(NSEvent *)event {
    NSPoint point = [event locationInWindow];
    int x = (int)((point.x - self.screenOffset.x) * _screenScale);
    int y = (int)((point.y - self.screenOffset.y) * _screenScale);
    hal_callback_on_event_mouse_move(x, screen_height - y);
}

// Called when a mouse is moved by a drag.
- (void)mouseDragged:(NSEvent *)event {
    NSPoint point = [event locationInWindow];
    int x = (int)((point.x - self.screenOffset.x) * _screenScale);
    int y = (int)((point.y - self.screenOffset.y) * _screenScale);
    hal_callback_on_event_mouse_move(x, screen_height - y);
}

// Called when a modifier key is pressed or released.
- (void)flagsChanged:(NSEvent *)theEvent {
    // Get the Control key state.
    BOOL shiftBit = ([theEvent modifierFlags] & NSEventModifierFlagShift) == NSEventModifierFlagShift;
    BOOL controlBit = ([theEvent modifierFlags] & NSEventModifierFlagControl) == NSEventModifierFlagControl;
    BOOL commandBit = ([theEvent modifierFlags] & NSEventModifierFlagCommand) == NSEventModifierFlagCommand;
    
    // Notify when the Shift key state is changed.
    if (!_isShiftPressed && shiftBit) {
        _isShiftPressed = YES;
        hal_callback_on_event_key_press(HAL_KEY_SHIFT);
    } else if (_isShiftPressed && !shiftBit) {
        _isShiftPressed = NO;
        hal_callback_on_event_key_release(HAL_KEY_SHIFT);
    }

    // Notify when the Control key state is changed.
    if (!_isControlPressed && controlBit) {
        _isControlPressed = YES;
        hal_callback_on_event_key_press(HAL_KEY_CONTROL);
    } else if (_isControlPressed && !controlBit) {
        _isControlPressed = NO;
        hal_callback_on_event_key_release(HAL_KEY_CONTROL);
    }

    // Notify when the Command key state is changed.
    if (!_isCommandPressed && commandBit) {
        _isCommandPressed = YES;
        hal_callback_on_event_key_press(HAL_KEY_ALT);
    } else if (_isCommandPressed && !commandBit) {
        _isCommandPressed = NO;
        hal_callback_on_event_key_release(HAL_KEY_ALT);
    }
}

// Called when a keyboard is pressed.
- (void)keyDown:(NSEvent *)theEvent {
    if ([theEvent isARepeat])
        return;
    
    int kc = [self convertKeyCode:[theEvent keyCode]];
    if (kc != -1)
        hal_callback_on_event_key_press(kc);
}

// Called when a keyboard is released.
- (void)keyUp:(NSEvent *)theEvent {
    int kc = [self convertKeyCode:[theEvent keyCode]];
    if (kc != -1)
        hal_callback_on_event_key_release(kc);
}

// A helper to convert a keycode.
- (int)convertKeyCode:(int)keyCode {
    switch(keyCode) {
        case 53: return HAL_KEY_ESCAPE;
        case 36: return HAL_KEY_RETURN;
        case 49: return HAL_KEY_SPACE;
        case 48: return HAL_KEY_TAB;
        case 51: return HAL_KEY_BACKSPACE;
        case 117: return HAL_KEY_DELETE;
        case 115: return HAL_KEY_HOME;
        case 119: return HAL_KEY_END;
        case 116: return HAL_KEY_PAGEUP;
        case 121: return HAL_KEY_PAGEDOWN;
        case 123: return HAL_KEY_LEFT;
        case 124: return HAL_KEY_RIGHT;
        case 125: return HAL_KEY_DOWN;
        case 126: return HAL_KEY_UP;
        case 0: return HAL_KEY_A;
        case 11: return HAL_KEY_B;
        case 8: return HAL_KEY_C;
        case 2: return HAL_KEY_D;
        case 14: return HAL_KEY_E;
        case 3: return HAL_KEY_F;
        case 5: return HAL_KEY_G;
        case 4: return HAL_KEY_H;
        case 34: return HAL_KEY_I;
        case 38: return HAL_KEY_J;
        case 40: return HAL_KEY_K;
        case 37: return HAL_KEY_L;
        case 46: return HAL_KEY_M;
        case 45: return HAL_KEY_N;
        case 31: return HAL_KEY_O;
        case 35: return HAL_KEY_P;
        case 12: return HAL_KEY_Q;
        case 15: return HAL_KEY_R;
        case 1: return HAL_KEY_S;
        case 17: return HAL_KEY_T;
        case 32: return HAL_KEY_U;
        case 9: return HAL_KEY_V;
        case 13: return HAL_KEY_W;
        case 7: return HAL_KEY_X;
        case 16: return HAL_KEY_Y;
        case 6: return HAL_KEY_Z;
        case 18: return HAL_KEY_1;
        case 19: return HAL_KEY_2;
        case 20: return HAL_KEY_3;
        case 21: return HAL_KEY_4;
        case 23: return HAL_KEY_5;
        case 22: return HAL_KEY_6;
        case 26: return HAL_KEY_7;
        case 28: return HAL_KEY_8;
        case 25: return HAL_KEY_9;
        case 29: return HAL_KEY_0;
        case 122: return HAL_KEY_F1;
        case 120: return HAL_KEY_F2;
        case 99: return HAL_KEY_F3;
        case 118: return HAL_KEY_F4;
        case 96: return HAL_KEY_F5;
        case 97: return HAL_KEY_F6;
        case 98: return HAL_KEY_F7;
        case 100: return HAL_KEY_F8;
        case 101: return HAL_KEY_F9;
        case 109: return HAL_KEY_F10;
        case 103: return HAL_KEY_F11;
        case 111: return HAL_KEY_F12;
    }
    return -1;
}

// Set the window title. (GameViewControllerProtocol)
- (void)setWindowTitle:(NSString *)name {
    [self.view.window setTitle:name];
}

// Get the view scale. (GameViewControllerProtocol)
- (float)screenScale {
    return _screenScale;
}

// Get the screen offset. (GameViewControllerProtocol)
- (NSPoint)screenOffset {
    return _screenOffset;
}

// Get the screen size. (GameViewControllerProtocol)
- (NSSize)screenSize {
    return _screenSize;
}

// Convert a screen point to window point. (GameViewControllerProtocol)
- (NSPoint)windowPointToScreenPoint:(NSPoint)windowPoint {
        float retinaScale = (float)_view.layer.contentsScale;
    
    int x = (int)((windowPoint.x - (_screenOffset.x / retinaScale)) * _screenScale);
    int y = (int)((windowPoint.y - (_screenOffset.y / retinaScale)) * _screenScale);
    
    return NSMakePoint(x, screen_height - y);
}

// Returns whether we are in the full screen mode or not. (GameViewControllerProtocol)
- (BOOL)isFullScreen {
    return _isFullScreen;
}

// Enter the full screen mode. (GameViewControllerProtocol)
- (void)enterFullScreen {
    if (!_isFullScreen) {
        [self.view.window toggleFullScreen:self.view];
        [self setAppName];
        _isFullScreen = YES;
    }
}

// Leave the full screen mode. (GameViewControllerProtocol)
- (void)leaveFullScreen {
    if (_isFullScreen) {
        [self.view.window toggleFullScreen:self.view];
        [self setAppName];
        _isFullScreen = NO;
    }
}

// Returns whether we are playing a video or not. (GameViewControllerProtocol)
- (BOOL)isVideoPlaying {
    return _isVideoPlaying;
}

// Play a video. (GameViewControllerProtocol)
- (void)playVideoWithPath:(NSString *)path skippable:(BOOL)isSkippable {
    // Create a player.
    NSURL *url = [NSURL URLWithString:[@"file://" stringByAppendingString:path]];
    AVPlayerItem *playerItem = [[AVPlayerItem alloc] initWithURL:url];
    _avPlayer = [[AVPlayer alloc] initWithPlayerItem:playerItem];
    
    // Create a player layer.
    [self.view setWantsLayer:YES];
    _avPlayerLayer = [AVPlayerLayer playerLayerWithPlayer:_avPlayer];
    [_avPlayerLayer setFrame:theViewController.view.bounds];
    [self.view.layer addSublayer:_avPlayerLayer];

    // Set a notification on finish playing.
    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(onPlayEnd:)
                                               name:AVPlayerItemDidPlayToEndTimeNotification
                                             object:playerItem];
    
    // Start playing.
    [_avPlayer play];
    
    _isVideoPlaying = YES;
}

// Called when a video playback is finished. (GameViewControllerProtocol)
- (void)onPlayEnd:(NSNotification *)notification {
    [_avPlayer replaceCurrentItemWithPlayerItem:nil];
    _isVideoPlaying = NO;
}

// Stop a video playback. (GameViewControllerProtocol)
- (void)stopVideo {
    if (_avPlayer != nil) {
        [_avPlayer replaceCurrentItemWithPlayerItem:nil];
        _isVideoPlaying = NO;
        _avPlayer = nil;
        _avPlayerLayer = nil;
    }
}

@end

//
// GameView
//  - Instanciated in Main.storyboard
//

@implementation GameView

// Declare that we use a video player layer.
+ (Class)layerClass {
    return AVPlayerLayer.class;
}

// Get a GameViewController from an event.
- (id<GameViewControllerProtocol>) viewControllerFrom:(NSEvent *)event {
    NSObject *viewController = event.window.contentViewController;
    if ([viewController conformsToProtocol:@protocol(GameViewControllerProtocol)])
        return (NSObject<GameViewControllerProtocol> *)viewController;
    return nil;
}

// Called when a mouse button is pressed.
- (void)mouseDown:(NSEvent *)event {
    id<GameViewControllerProtocol> viewController = [self viewControllerFrom:event];
    NSPoint point = [viewController windowPointToScreenPoint:[event locationInWindow]];
    hal_callback_on_event_mouse_press(HAL_MOUSE_LEFT, (int)point.x, (int)point.y);
}

// Called when a mouse button is released.
- (void)mouseUp:(NSEvent *)event {
    id<GameViewControllerProtocol> viewController = [self viewControllerFrom:event];
    NSPoint point = [viewController windowPointToScreenPoint:[event locationInWindow]];
    hal_callback_on_event_mouse_release(HAL_MOUSE_LEFT, (int)point.x, (int)point.y);
}

// Called when a right mouse button is pressed.
- (void)rightMouseDown:(NSEvent *)event {
    id<GameViewControllerProtocol> viewController = [self viewControllerFrom:event];
    NSPoint point = [viewController windowPointToScreenPoint:[event locationInWindow]];
    hal_callback_on_event_mouse_press(HAL_MOUSE_RIGHT, (int)point.x, (int)point.y);
}

// Called when a right mouse button is released.
- (void)rightMouseUp:(NSEvent *)event {
    id<GameViewControllerProtocol> viewController = [self viewControllerFrom:event];
    NSPoint point = [viewController windowPointToScreenPoint:[event locationInWindow]];
    hal_callback_on_event_mouse_release(HAL_MOUSE_RIGHT, (int)point.x, (int)point.y);
}

// Called when a mouse is dragged.
- (void)mouseDragged:(NSEvent *)event {
    id<GameViewControllerProtocol> viewController = [self viewControllerFrom:event];
    NSPoint point = [viewController windowPointToScreenPoint:[event locationInWindow]];
    hal_callback_on_event_mouse_move((int)point.x, (int)point.y);
}

// Called when a mouse wheel is pressed.
- (void)scrollWheel:(NSEvent *)event {
    int delta = (int)[event deltaY];
    if (delta > 0) {
        hal_callback_on_event_key_press(HAL_KEY_UP);
        hal_callback_on_event_key_release(HAL_KEY_UP);
    } else if (delta < 0) {
        hal_callback_on_event_key_press(HAL_KEY_DOWN);
        hal_callback_on_event_key_release(HAL_KEY_DOWN);
    }
}

@end

void
openLogWindow(void)
{
    if (theLogWindow != nil)
        return;

    theLogWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(100, 100, 600, 400)
                                               styleMask:(NSWindowStyleMaskTitled |
                                                          NSWindowStyleMaskClosable |
                                                          NSWindowStyleMaskResizable |
                                                          NSWindowStyleMaskMiniaturizable)
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];
    [theLogWindow setTitle:@"Console"];
    [theLogWindow makeKeyAndOrderFront:nil];

    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:[[theLogWindow contentView] bounds]];
    [scrollView setHasVerticalScroller:YES];
    [scrollView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];

    theLogTextView = [[NSTextView alloc] initWithFrame:[[scrollView contentView] bounds]];
    [theLogTextView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
    [theLogTextView setEditable:YES];
    [theLogTextView setFont:[NSFont systemFontOfSize:14]];

    [scrollView setDocumentView:theLogTextView];

    [[theLogWindow contentView] addSubview:scrollView];
}

void
putTextToLogWindow(
        const char *text)
{
    NSString *newLine = [[[NSString alloc] initWithUTF8String:text] stringByAppendingString:@"\n"];

    [[theLogTextView textStorage] appendAttributedString:
        [[NSAttributedString alloc] initWithString:newLine]];

    NSRange range = NSMakeRange([[theLogTextView string] length], 0);
    [theLogTextView scrollRangeToVisible:range];

    [theLogTextView setBackgroundColor:[NSColor blackColor]];
    [theLogTextView setTextColor:[NSColor whiteColor]];
    [theLogTextView setFont:[NSFont userFixedPitchFontOfSize:13]];
}

//
// HAL
//

// Make a save directory.
bool
hal_make_save_directory(void)
{
    @autoreleasepool {
        if (!is_bundled) {
            // If we are running a non-bundled app, use the current directory.
            NSString *basePath = [[NSFileManager defaultManager] currentDirectoryPath];
            NSString *savePath = [NSString stringWithFormat:@"%@/save", basePath];
            NSError *error;
            [[NSFileManager defaultManager] createDirectoryAtPath:savePath
                                      withIntermediateDirectories:NO
                                                       attributes:nil
                                                            error:&error];
            return true;
        } else if (releaseMode) {
            // We are in the release mode, use the home folder.
            NSString *path = NSHomeDirectory();
            path = [path stringByAppendingString:@"/Library/Application Support/"];
            path = [path stringByAppendingString:[[NSString alloc] initWithUTF8String:window_title]];
            path = [path stringByAppendingString:@"/save"];
            [[NSFileManager defaultManager] createDirectoryAtPath:path
                                      withIntermediateDirectories:YES
                                                       attributes:nil
                                                            error:NULL];
            return true;
        } else {
            // We are not in the release mode, use the folder where the app bundle exists.
            NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
            NSString *basePath = [bundlePath stringByDeletingLastPathComponent];
            NSString *savePath = [NSString stringWithFormat:@"%@/save", basePath];
            NSError *error;
            [[NSFileManager defaultManager] createDirectoryAtPath:savePath
                                      withIntermediateDirectories:NO
                                                       attributes:nil
                                                            error:&error];
            return true;
        }
    }
}

// Get a real path for a file.
char *
hal_make_real_path(
        const char *fname)
{
    @autoreleasepool {
        if (!is_bundled) {
            char *ret = strdup(fname);
            if (ret == NULL) {
                hal_log_out_of_memory();
                return NULL;
            }
            return ret;
        }

        if (releaseMode) {
            // If we are in the release mode, and...
            if (strncmp(fname, "save/", 5) == 0) {
                // If we are going to open a save file, use the home folder.
                assert(fname != NULL);
                NSString *path = NSHomeDirectory();
                path = [path stringByAppendingString:@"/Library/Application Support/"];
                path = [path stringByAppendingString:[[NSString alloc] initWithUTF8String:window_title]];
                path = [path stringByAppendingString:@"/"];
                path = [path stringByAppendingString:[[NSString alloc] initWithUTF8String:fname]];
                char *ret = strdup([path UTF8String]);
                if (ret == NULL) {
                    hal_log_out_of_memory();
                    return NULL;
                }   
                return ret;
            } else {
                // If we are going to open a game data file, use a bundle resource.
                NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
                NSString *filePath = [NSString stringWithFormat:@"%@/Contents/Resources/%s", bundlePath, fname];
                char *ret = strdup([filePath UTF8String]);
                if (ret == NULL) {
                    hal_log_out_of_memory();
                    return NULL;
                }
                return ret;
            }
        } else {
            // If we are not in the release mode, use a normal file in the current directory.
            NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
            NSString *basePath = [bundlePath stringByDeletingLastPathComponent];
            NSString *filePath = [NSString stringWithFormat:@"%@/%s", basePath, fname];
            const char *cstr = [filePath UTF8String];
            char *ret = strdup(cstr);
            if (ret == NULL) {
                hal_log_out_of_memory();
                return NULL;
            }
            return ret;
        }
    }
}

// Show an INFO log.
bool
hal_log_info(
        const char *s, ...)
{
    char buf[1024];
    va_list ap;
    
    va_start(ap, s);
    vsnprintf(buf, sizeof(buf), s, ap);
    va_end(ap);

    // Open the log window and put the text.
    openLogWindow();
    putTextToLogWindow(buf);

    // Write to the log file.
    FILE *fp = openLog();
    if (fp != NULL) {
        fprintf(fp, "%s\n", buf);
        fflush(fp);
    }
    NSLog(@"%@", [[NSString alloc] initWithUTF8String:buf]);

    __sync_synchronize();

    return true;
}

// Show a WARN log.
bool
hal_log_warn(
        const char *s, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, s);
    vsnprintf(buf, sizeof(buf), s, ap);
    va_end(ap);

    // Open the log window and put the text.
    openLogWindow();
    putTextToLogWindow(buf);

    // Write to the log file.
    FILE *fp = openLog();
    if (fp != NULL) {
        fprintf(fp, "%s\n", buf);
        fflush(fp);
    }
    NSLog(@"%@", [[NSString alloc] initWithUTF8String:buf]);

    __sync_synchronize();

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

    // Open the log window and put the text.
    openLogWindow();
    putTextToLogWindow(buf);

    // Write to the log file.
    FILE *fp = openLog();
    if (fp != NULL) {
        fprintf(fp, "%s\n", buf);
        fflush(fp);
    }
    NSLog(@"%@", [[NSString alloc] initWithUTF8String:buf]);

    __sync_synchronize();

    return true;
}

// Show an Out-of-memory error.
bool
hal_log_out_of_memory(void)
{
        hal_log_error(HAL_TR("Out of memory."));
        return true;
}

// Open the log file.
static FILE *
openLog(void)
{
    static FILE *fp = NULL;
    const char *cpath;

    // If already opened.
    if (fp != NULL)
        return fp;

    if (!is_bundled) {
        // We are running a non-bundled app, use the current directory.
        logFilePath = strdup("log.txt");
        fp = fopen("log.txt", "w");
        if (fp == NULL) {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Error"];
            [alert setInformativeText:@"Cannot open log file."];
            [alert addButtonWithTitle:@"OK"];
            [alert runModal];
            logFilePath = NULL;
            return NULL;
        }
        return fp;
    } else if (releaseMode) {
        // We are in the release mode, use the "Aplication Support" folder.
        NSString *path = NSHomeDirectory();
        path = [path stringByAppendingString:@"/Library/Application Support/"];
        path = [path stringByAppendingString:[[NSString alloc] initWithUTF8String:window_title]];
        [[NSFileManager defaultManager] createDirectoryAtPath:path
                                  withIntermediateDirectories:YES
                                                   attributes:nil
                                                        error:NULL];
        path = [path stringByAppendingString:@"/log.txt"];
        cpath = [path UTF8String];
        logFilePath = strdup(cpath);
        fp = fopen(cpath, "w");
        if (fp == NULL) {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Error"];
            [alert setInformativeText:@"Cannot open log file."];
            [alert addButtonWithTitle:@"OK"];
            [alert runModal];
            logFilePath = NULL;
            return NULL;
        }
        return fp;
    } else {
        // We are not in the release mode, use the directory where the .app bundle exists.
        NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
        NSString *basePath = [bundlePath stringByDeletingLastPathComponent];
        NSString *filePath = [NSString stringWithFormat:@"%@/log.txt", basePath];
        cpath = [filePath UTF8String];
        logFilePath = strdup(cpath);
        fp = fopen(cpath, "w");
        if (fp == NULL) {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Error"];
            [alert setInformativeText:@"Cannot open log file."];
            [alert addButtonWithTitle:@"OK"];
            [alert runModal];
            logFilePath = NULL;
            return NULL;
        }
        return fp;
    }
}

// Show the log file.
static void
showLogAtExit(void)
{
    __sync_synchronize();

    if (logFilePath != NULL) {
        NSString *path = [[NSString alloc] initWithUTF8String:logFilePath];
        [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:path]];
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
//
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
    UNUSED_PARAMETER(is_enabled);
}
