// -*- coding: utf-8; tab-width: 4; indent-tabs-mode: nil; -*-

/*
 * StratoHAL
 * Main code for Swift
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
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

import SwiftUI
import MetalKit

#if os(iOS)
private typealias ViewRepresentable = UIViewRepresentable
#elseif os(macOS)
private typealias ViewRepresentable = NSViewRepresentable
#endif

//
// GameViewRepresentable
//
struct GameViewRepresentable: ViewRepresentable {
    #if os(macOS)
    typealias NSViewType = GameView
    #else
    typealias UIViewType = GameView
    #endif

    var view: GameView
    var coordinator: Renderer

    init() {
        view = GameView()
        coordinator = Renderer(view: view)
        view.delegate = coordinator
    }

    #if os(iOS)
    func makeCoordinator() -> Renderer { return coordinator }
    func makeUIView(context: Context) -> MTKView { return view }
    func updateUIView(_ uiView: GameView, context: Context) { }
    #endif

    #if os(macOS)
    func makeCoordinator() -> Renderer { return coordinator }
    func makeNSView(context: Context) -> GameView { return view }
    func updateNSView(_ nsView: GameView, context: Context) { }
    #endif
}

//
// GameView
//
class GameView: MTKView {
    #if os(iOS)
    func touchesBegan(_: Set<UITouch>, with: UIEvent) {
        Renderer.touchesBegan(_, with: with)
    }
    func touchesMoved(_: Set<UITouch>, with: UIEvent) {
        Renderer.touchesMoved(_, with: with)
    }
    func touchesEnded(_: Set<UITouch>, with: UIEvent) {
        Renderer.touchesEnded(_, with: with)
    }
    #endif

    #if os(macOS)
    override func mouseDown(with event: NSEvent) {
        Renderer.mouseDown(with: event)
    }
    override func mouseUp(with: NSEvent) {
        Renderer.mouseUp(with: with)
    }
    override func mouseDragged(with: NSEvent) {
        Renderer.mouseDragged(with: with)
    }
    override func scrollWheel(with: NSEvent) {
        Renderer.scrollWheel(with: with)
    }
    #endif
}

//
// Renderer
//
class Renderer: NSObject, MTKViewDelegate {
    // An MTKView
    static var mtkView: MTKView?

    // An MTLDevice.
    static var device: MTLDevice?

    // Rendering pipelines.
    static var normalPipelineState: MTLRenderPipelineState?
    static var addPipelineState: MTLRenderPipelineState?
    static var dimPipelineState: MTLRenderPipelineState?
    static var rulePipelineState: MTLRenderPipelineState?
    static var meltPipelineState: MTLRenderPipelineState?

    // Command buffer.
    static var commandQueue: MTLCommandQueue?
    static var commandBuffer: MTLCommandBuffer?
    static var blitEncoder: MTLBlitCommandEncoder?
    static var renderEncoder: MTLRenderCommandEncoder?
    static var inFlightSemaphore = DispatchSemaphore(value: 1)

    // Rendering timer.
    static var timer: Timer?

    // Game status.
    static var projectPath = ""
    static var isContinuousSwipeEnabled = false
    static var isStarted = false
    static var isFinished = false
    static var isVideoPlayingBack = false

    // View size.
    static var viewWidth: Float = 0.0
    static var viewHeight: Float = 0.0

    // Viewport size and scale.
    static var viewportWidth: Float = 0.0
    static var viewportHeight: Float = 0.0
    static var viewportLeft: Float = 0.0
    static var viewportTop: Float = 0.0
    static var viewportScale: Float = 0.0

    // Mouse scale.
    static var mouseScale: Float = 0.0

    // Touch status.
    static var isTouched = false
    static var touchStartX: Float = 0.0
    static var touchStartY: Float = 0.0
    static var touchLastY: Float = 0.0
    
    // A class to obtain an image.
    class ManagedImage {
        var width: Int
        var height: Int
        var pixels: UnsafeRawPointer
        var texture: MTLTexture?
        var needUpload: Bool

        init(width: Int, height: Int, pixels: UnsafeRawPointer, texture: MTLTexture? = nil, needUpload: Bool) {
            self.width = width
            self.height = height
            self.pixels = pixels
            self.texture = texture
            self.needUpload = needUpload
        }
    }

    // Dictionary that obtains all images.
    static var imageDict: [Int: ManagedImage] = [:]

    // Array that obtains indices of images to upload at the beginning of a frame.
    static var uploadArray: [Int] = []

    // Array that obtains indices of images to purge at the end of a frame.
    static var purgeArray: [Int] = []

    // Initializer.
    init(view: MTKView) {
        Renderer.mtkView = view
        Renderer.device = MTLCreateSystemDefaultDevice() //mtkView.preferredDevice

        // Setup MTKView.
        view.device = Renderer.mtkView?.preferredDevice
        view.enableSetNeedsDisplay = true
        view.device = MTLCreateSystemDefaultDevice()
        view.clearColor = MTLClearColorMake(0.0, 0, 0, 1.0)
        Renderer.calcViewport(width: Float(view.frame.size.width), height: Float(view.frame.size.height))

        // Construct the shader pipelines.
        Renderer.normalPipelineState = Renderer.makePipeline(view: view, device: Renderer.device!, name: "fragmentNormalShader")!
        Renderer.addPipelineState = Renderer.makePipeline(view: view, device: Renderer.device!, name: "fragmentAddShader")!
        Renderer.dimPipelineState = Renderer.makePipeline(view: view, device: Renderer.device!, name: "fragmentDimShader")!
        Renderer.rulePipelineState = Renderer.makePipeline(view: view, device: Renderer.device!, name: "fragmentRuleShader")!
        Renderer.meltPipelineState = Renderer.makePipeline(view: view, device: Renderer.device!, name: "fragmentMeltShader")!

        // Create a command queue.
        Renderer.commandQueue = Renderer.device!.makeCommandQueue()

        // Setup the callback function pointers.
        Renderer.setupCallbacks()
        Renderer.setupCallbacksForEditor()

        // Setup a rendering timer.
        Renderer.timer = Renderer.setupTimer()
    }

    static func setupCallbacks() {
        wrap_log_info = unsafeBitCast(logInfo, to: type(of: wrap_log_info))
        wrap_log_warn = unsafeBitCast(logWarn, to: type(of: wrap_log_warn))
        wrap_log_error = unsafeBitCast(logError, to: type(of: wrap_log_error))
        wrap_make_sav_dir = unsafeBitCast(makeSavDir, to: type(of: wrap_make_sav_dir))
        wrap_make_valid_path = unsafeBitCast(makeValidPath, to: type(of: wrap_make_valid_path))
        wrap_notify_image_update = unsafeBitCast(notifyImageUpdate, to: type(of: wrap_notify_image_update))
        wrap_notify_image_free = unsafeBitCast(notifyImageFree, to: type(of: wrap_notify_image_free))
        wrap_render_image_normal = unsafeBitCast(renderImageNormal, to: type(of: wrap_render_image_normal))
        wrap_render_image_add = unsafeBitCast(renderImageAdd, to: type(of: wrap_render_image_add))
        wrap_render_image_dim = unsafeBitCast(renderImageDim, to: type(of: wrap_render_image_dim))
        wrap_render_image_rule = unsafeBitCast(renderImageRule, to: type(of: wrap_render_image_rule))
        wrap_render_image_melt = unsafeBitCast(renderImageMelt, to: type(of: wrap_render_image_melt))
        wrap_render_image_3d_normal = unsafeBitCast(renderImage3dNormal, to: type(of: wrap_render_image_3d_normal))
        wrap_render_image_3d_add = unsafeBitCast(renderImage3dAdd, to: type(of: wrap_render_image_3d_add))
        wrap_reset_lap_timer = unsafeBitCast(resetLapTimer, to: type(of: wrap_reset_lap_timer))
        wrap_get_lap_timer_millisec = unsafeBitCast(getLapTimerMillisec, to: type(of: wrap_get_lap_timer_millisec))
        wrap_play_video = unsafeBitCast(playVideo, to: type(of: wrap_play_video))
        wrap_stop_video = unsafeBitCast(stopVideo, to: type(of: wrap_stop_video))
        wrap_is_video_playing = unsafeBitCast(isVideoPlaying, to: type(of: wrap_is_video_playing))
        wrap_update_window_title = unsafeBitCast(updateWindowTitle, to: type(of: wrap_update_window_title))
        wrap_is_full_screen_supported = unsafeBitCast(isFullScreenSupported, to: type(of: wrap_is_full_screen_supported))
        wrap_is_full_screen_mode = unsafeBitCast(isFullScreenSupported, to: type(of: wrap_is_full_screen_mode))
        wrap_enter_full_screen_mode = unsafeBitCast(enterFullScreenMode, to: type(of: wrap_enter_full_screen_mode))
        wrap_leave_full_screen_mode = unsafeBitCast(leaveFullScreenMode, to: type(of: wrap_leave_full_screen_mode))
        wrap_get_system_locale = unsafeBitCast(getSystemLocale, to: type(of: wrap_get_system_locale))
        wrap_speak_text = unsafeBitCast(speakText, to: type(of: wrap_speak_text))
        wrap_set_continuous_swipe_enabled = unsafeBitCast(setContinuousSwipeEnabled, to: type(of: wrap_set_continuous_swipe_enabled))
    }

    #if USE_EDITOR
    static func setupCallbacksForEditor() {
        wrap_is_continue_pushed = unsafeBitCast(isContinuePushed, to: type(of: wrap_is_continue_pushed))
        wrap_is_next_pushed = unsafeBitCast(isNextPushed, to: type(of: wrap_is_next_pushed))
        wrap_is_stop_pushed = unsafeBitCast(isStopPushed, to: type(of: wrap_is_stop_pushed))
        wrap_is_script_opened = unsafeBitCast(isScriptOpened, to: type(of: wrap_is_script_opened))
        wrap_get_opened_script = unsafeBitCast(getOpenedScript, to: type(of: wrap_get_opened_script))
        wrap_is_exec_line_changed = unsafeBitCast(isExecLineChanged, to: type(of: wrap_is_exec_line_changed))
        wrap_get_changed_exec_line = unsafeBitCast(getChangedExecLine, to: type(of: wrap_get_changed_exec_line))
        wrap_on_change_running_state = unsafeBitCast(onChangeRunningState, to: type(of: wrap_on_change_running_state))
        wrap_on_load_script = unsafeBitCast(onLoadScript, to: type(of: wrap_on_load_script))
        wrap_on_change_position = unsafeBitCast(onChangePosition, to: type(of: wrap_on_change_position))
        wrap_on_update_variable = unsafeBitCast(onUpdateVariable, to: type(of: wrap_on_update_variable))
    }
    #else
    static func setupCallbacksForEditor() {
    }
    #endif

    #if os(iOS)
    static func setupTimer() -> Timer {
        return Timer.scheduledTimer(withTimeInterval: 1.0 / 15.0, repeats: true) { _ in
            mtkView.setNeedsDisplay()
        }
    }
    #else
    static func setupTimer() -> Timer {
        return Timer.scheduledTimer(withTimeInterval: 1.0 / 15.0, repeats: true) { _ in
            Renderer.mtkView!.setNeedsDisplay(
                NSRect(origin: CGPoint(x: CGFloat(Renderer.viewportLeft),
                                       y: CGFloat(Renderer.viewportTop)),
                       size: CGSize(width: CGFloat(Renderer.viewportWidth),
                                    height: CGFloat(Renderer.viewportHeight))))
        }
    }
    #endif

    // Helper to make a rendering pipeline.
    static func makePipeline(view: MTKView, device: MTLDevice, name: String) -> MTLRenderPipelineState? {
        let defaultLibrary = device.makeDefaultLibrary()
        let descriptor = MTLRenderPipelineDescriptor()
        descriptor.label = "Texturing Pipeline " + name
        descriptor.vertexFunction = defaultLibrary?.makeFunction(name: "vertexShader")
        descriptor.fragmentFunction = defaultLibrary?.makeFunction(name: name)
        descriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat
        descriptor.colorAttachments[0].isBlendingEnabled = true
        descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperation.add
        descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperation.add
        descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactor.sourceAlpha
        descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactor.one
        descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactor.oneMinusSourceAlpha
        descriptor.colorAttachments[0].destinationAlphaBlendFactor =  MTLBlendFactor.one
        do {
            return try device.makeRenderPipelineState(descriptor: descriptor)
        } catch {
        }
        return nil
    }

    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        Renderer.viewWidth = Float(size.width)
        Renderer.viewHeight = Float(size.height)

        if conf_window_width > 0 && conf_window_height > 0 {
            Renderer.calcViewport(width: Renderer.viewWidth, height: Renderer.viewHeight)
        }
    }

    // Called when the "Restart" button is pressed.
    static func restart() {
        objc_sync_enter(Renderer.mtkView!)
        repeat {
            // Guard.
            if Renderer.projectPath == "" {
                break
            }

            // Empty the image dictionary.
            for image in Renderer.imageDict.values {
                image.texture!.setPurgeableState(MTLPurgeableState.empty)
            }
            Renderer.imageDict = [:]
            cleanup_image()

            // Translate a locale string to a locale code.
            init_locale_code()

            // Initialize the file subsystem.
            if !init_file() {
                break
            }

            // Initialize the config subsystem. (Load config.txt)
            if !init_conf() {
                break
            }

            // Calc the viewport. (This must be after init_conf())
            Renderer.calcViewport(width: Renderer.viewWidth, height: Renderer.viewHeight)

            // Initialize the sound subsystem.
            if !init_aunit() {
                break
            }

            // Initialize the event subsystem. (Load system images.)
            if !on_event_init() {
                break
            }

            // Enable rendering.
            Renderer.isStarted = true
            Renderer.isFinished = false
        } while false
        objc_sync_exit(Renderer.mtkView!)
    }
    
    static func calcViewport(width: Float, height: Float) {
        // Avoid zero divisions.
        if width == 0 || height == 0 {
            return
        }

        // Calc the aspect ratio of the game.
        let aspect: Float = Float(conf_window_height) / Float(conf_window_width)

        // 1. Width-first.
        viewportWidth = width
        viewportHeight = width * aspect
        viewportScale = Float(conf_window_width) / viewportWidth

        // 2. Height-first.
        if viewportHeight > height {
            viewportHeight = height
            viewportWidth = viewportHeight / aspect
            viewportScale = Float(conf_window_height) / viewportHeight
        }

        // Calc the viewport origin.
        viewportLeft = (width - viewportWidth) / 2.0
        viewportTop = (height - viewportHeight) / 2.0

        // Calc the mouse scale factor.
        mouseScale = Float(mtkView!.layer!.contentsScale) * Renderer.viewportScale
    }

    // Called back for a periodic rendering.
    func draw(in: MTKView) {
        objc_sync_enter(mtkView)
        repeat {
            // Guard.
            if Renderer.mtkView?.currentRenderPassDescriptor == nil {
                // View not ready.
                break
            }
            if !Renderer.isStarted {
                // Game not started.
                break
            }
            if Renderer.isFinished {
                // Game already finished.
                break
            }

            // When a video is playing back, we don't render here.
            if Renderer.isVideoPlayingBack {
                // Do a frame just for the click check.
                if !runFrame() {
                    // Reached end-of-game.
                    Renderer.isFinished = true
                }
                break
            }

            // Create a command buffer.
            Renderer.commandBuffer = Renderer.commandQueue?.makeCommandBuffer()
            Renderer.commandBuffer?.label = "MyCommand"

            // Synchronize to the previous rendering.
            _ = Renderer.inFlightSemaphore.wait(timeout: .distantFuture)

            // Nil-ify the encoders.
            Renderer.blitEncoder = nil
            Renderer.renderEncoder = nil

            // Update textures before a frame. (for images loaded at an initialization)
            for index in Renderer.uploadArray {
                Renderer.updateImage(index: index)
            }
            Renderer.uploadArray = []

            // Create an empty array for textures to be destroyed in a frame.
            Renderer.purgeArray = []

            // Run a frame event and do rendering.
            if !runFrame() {
                Renderer.isFinished = true
            }

            // End encodings.
            if Renderer.blitEncoder != nil {
                Renderer.blitEncoder?.endEncoding()
            }
            if Renderer.renderEncoder != nil {
                Renderer.renderEncoder?.endEncoding()
            }

            // Schedule a rendering to the current drawable.
            Renderer.commandBuffer?.present((Renderer.mtkView?.currentDrawable!)!)

            // Push the command buffer to the GPU.
            Renderer.commandBuffer?.commit()

            // Synchronize.
            Renderer.commandBuffer?.addCompletedHandler({ _ in
                Renderer.inFlightSemaphore.signal()
            })
            Renderer.commandBuffer?.waitUntilCompleted()
            Renderer.blitEncoder = nil
            Renderer.renderEncoder = nil

            // Set destroyed textures purgeable. (They will be purged at appropreate times.)
            for index in Renderer.purgeArray {
                let image = Renderer.imageDict[index]
                image?.texture?.setPurgeableState(MTLPurgeableState.empty)
            }
            Renderer.purgeArray = []
        } while false
        objc_sync_exit(mtkView)
    }

    // Helper to run a frame.
    func runFrame() -> Bool {
        if !on_event_frame() {
            save_global_data()
            save_seen()
            return false
        }
        return true
    }
    
    // Helper to update a texture for an image.
    static func updateImage(index: Int) {
        // Get a managed image.
        let image = Renderer.imageDict[index]!
        if image.texture == nil {
            // For the first time, create a texture for it.
            let textureDescriptor = MTLTextureDescriptor()
            textureDescriptor.pixelFormat = MTLPixelFormat.bgra8Unorm
            textureDescriptor.width = image.width
            textureDescriptor.height = image.height
            image.texture = Renderer.device?.makeTexture(descriptor: textureDescriptor)
        }
        
        // Upload the pixels to the texture.
        if Renderer.blitEncoder == nil {
            Renderer.blitEncoder = Renderer.commandBuffer?.makeBlitCommandEncoder()
            Renderer.blitEncoder?.label = "Texture Encoder"
        }
        let region = MTLRegion(origin: MTLOrigin(x: 0, y: 0, z: 0), size: MTLSize(width: image.width, height: image.height, depth: 1))
        image.texture?.replace(region: region, mipmapLevel: 0, withBytes: image.pixels, bytesPerRow: image.width * 4)
    }

    //
    // Event handlers.
    //

    #if os(iOS)
    static func touchesBegan(_: Set<UITouch>, with: UIEvent) {
        objc_sync_enter(Renderer.mtkView!)
        repeat {
            // Guard if not started.
            if !renderer?.isStarted {
                break
            }
            
            // Set touch enabled.
            isTouched = true
            
            // Get the position.
            let x = Float(with.allTouches?.first?.location(in: mtkView).x ?? 0)
            let y = Float(with.allTouches?.first?.location(in: mtkView).y ?? 0)
            touchStartX = (x - mouseLeft) * mouseScale
            touchStartY = (y - mouseTop) * mouseScale
            touchLastY = touchStartY
            
            // Emulate a mouse press.
            let MOUSE_LEFT: Int32 = 0
            on_event_mouse_press(MOUSE_LEFT, Int32(touchStartX), Int32(touchStartY));
        } while false
        objc_sync_exit(Renderer.mtkView!)
    }
    
    static func touchesMoved(_: Set<UITouch>, with: UIEvent) {
        objc_sync_enter(Renderer.mtkView!)
        repeat {
            // Guard if not started.
            if !renderer?.isStarted {
                break
            }
            
            // Get the position.
            let x = Float(with.allTouches?.first?.location(in: mtkView).x ?? 0)
            let y = Float(with.allTouches?.first?.location(in: mtkView).y ?? 0)
            let touchX = (x - mouseLeft) * mouseScale
            let touchY = (y - mouseTop) * mouseScale
            
            // Emulate a wheel down and a move.
            let FLICK_Y_DISTANCE: Float = 30.0
            let deltaY: Float = touchY - touchLastY
            touchLastY = touchY
            if GameView.isContinuousSwipeEnabled {
                if deltaY > 0 && deltaY < FLICK_Y_DISTANCE {
                    let KEY_DOWN: Int32 = 4
                    on_event_key_press(KEY_DOWN)
                    on_event_key_release(KEY_DOWN)
                    break
                }
            }
            on_event_mouse_move(Int32(touchX), Int32(touchY))
        } while false
        objc_sync_exit(Renderer.mtkView!)
    }
    
    static func touchesEnded(_: Set<UITouch>, with: UIEvent) {
        objc_sync_enter(Renderer.mtkView!)
        repeat {
            // Guard if not started.
            if !renderer?.isStarted {
                break
            }
            
            // Set touch disabled.
            isTouched = false
            
            // Get the position.
            let x = Float(with.allTouches?.first?.location(in: mtkView).x ?? 0)
            let y = Float(with.allTouches?.first?.location(in: mtkView).y ?? 0)
            let touchEndX = (x - mouseLeft) * mouseScale
            let touchEndY = (y - mouseTop) * mouseScale
            
            // Detect a swipe-up or a swipe-down.
            let FLICK_Y_DISTANCE: Float = 50.0
            let deltaY: Float = touchEndY - touchStartY
            if deltaY > FLICK_Y_DISTANCE {
                on_event_touch_cancel()
                on_event_swipe_down()
                break
            } else if deltaY < -FLICK_Y_DISTANCE {
                on_event_touch_cancel()
                on_event_swipe_up()
                break
            }
            
            // Emulate a left click.
            let FINGER_DISTANCE: Float = 10.0
            if with.allTouches?.count == 1 &&
                abs(touchEndX - touchStartX) < FINGER_DISTANCE &&
                abs(touchEndY - touchStartY) < FINGER_DISTANCE {
                let MOUSE_LEFT: Int32 = 0
                on_event_touch_cancel()
                on_event_mouse_press(MOUSE_LEFT, Int32(touchEndX), Int32(touchEndY))
                on_event_mouse_release(MOUSE_LEFT, Int32(touchEndX), Int32(touchEndY))
                break
            }
            
            // Emulate a right click.
            if with.allTouches?.count == 2 &&
                abs(touchEndX - touchStartX) < FINGER_DISTANCE &&
                abs(touchEndY - touchStartY) < FINGER_DISTANCE {
                let MOUSE_RIGHT: Int32 = 1
                on_event_touch_cancel()
                on_event_mouse_press(MOUSE_RIGHT, Int32(touchEndX), Int32(touchEndY))
                on_event_mouse_release(MOUSE_RIGHT, Int32(touchEndX), Int32(touchEndY))
                break
            }
            
            // Cancel the touch move.
            on_event_touch_cancel()
        } while false
        objc_sync_exit(Renderer.mtkView!)
    }
    #endif
    
    #if os(macOS)
    static func mouseDown(with event: NSEvent) {
        objc_sync_enter(Renderer.mtkView!)
        repeat {
            // Guard if not started.
            if !Renderer.isStarted {
                break
            }

            // Calc the mouse position.
            let pos = mtkView?.convert(event.locationInWindow, from: nil)
            let x: Float = Float(pos!.x) * mouseScale - Renderer.viewportLeft * viewportScale
            let y: Float = (Float(mtkView!.frame.size.height) - Float(pos!.y))  * mouseScale - viewportTop * viewportScale

            // Do event handling.
            let MOUSE_LEFT: Int32 = 0
            on_event_mouse_press(MOUSE_LEFT, Int32(x), Int32(y));
        } while false
        objc_sync_exit(Renderer.mtkView!)
    }

    static func mouseUp(with event: NSEvent) {
        objc_sync_enter(Renderer.mtkView!)
        repeat {
            // Guard if not started.
            if !Renderer.isStarted {
                break
            }

            // Calc the mouse position.
            let pos = mtkView?.convert(event.locationInWindow, from: nil)
            let x: Float = Float(pos!.x) * mouseScale - Renderer.viewportLeft * viewportScale
            let y: Float = (Float(mtkView!.frame.size.height) - Float(pos!.y))  * mouseScale - viewportTop * viewportScale

            // Do event handling.
            let MOUSE_LEFT: Int32 = 0
            on_event_mouse_release(MOUSE_LEFT, Int32(x), Int32(y))
        } while false
        objc_sync_exit(Renderer.mtkView!)
    }

    static func mouseRightDown(with event: NSEvent) {
        objc_sync_enter(Renderer.mtkView!)
        repeat {
            // Guard if not started.
            if !Renderer.isStarted {
                break
            }

            // Calc the mouse position.
            let pos = mtkView?.convert(event.locationInWindow, from: nil)
            let x: Float = Float(pos!.x) * mouseScale - Renderer.viewportLeft * viewportScale
            let y: Float = (Float(mtkView!.frame.size.height) - Float(pos!.y))  * mouseScale - viewportTop * viewportScale

            // Do event handling.
            let MOUSE_RIGHT: Int32 = 1
            on_event_mouse_press(MOUSE_RIGHT, Int32(x), Int32(y))
        } while false
        objc_sync_exit(Renderer.mtkView!)
    }

    static func mouseRightUp(with event: NSEvent) {
        objc_sync_enter(Renderer.mtkView!)
        repeat {
            // Guard if not started.
            if !Renderer.isStarted {
                break
            }

            // Calc the mouse position.
            let pos = mtkView?.convert(event.locationInWindow, from: nil)
            let x: Float = Float(pos!.x) * mouseScale - Renderer.viewportLeft * viewportScale
            let y: Float = (Float(mtkView!.frame.size.height) - Float(pos!.y))  * mouseScale - viewportTop * viewportScale

            // Do event handling.
            let MOUSE_RIGHT: Int32 = 1
            on_event_mouse_release(MOUSE_RIGHT, Int32(x), Int32(y))
        } while false
        objc_sync_exit(Renderer.mtkView!)
    }

    static func mouseDragged(with event: NSEvent) {
        objc_sync_enter(Renderer.mtkView!)
        repeat {
            // Guard if not started.
            if !Renderer.isStarted {
                break
            }

            // Calc the mouse position.
            let pos = mtkView?.convert(event.locationInWindow, from: nil)
            let x: Float = Float(pos!.x) * mouseScale - Renderer.viewportLeft * viewportScale
            let y: Float = (Float(mtkView!.frame.size.height) - Float(pos!.y))  * mouseScale - viewportTop * viewportScale

            // Do event handling.
            on_event_mouse_move(Int32(x), Int32(y))
        } while false
        objc_sync_exit(Renderer.mtkView!)
    }

    static func scrollWheel(with: NSEvent) {
        objc_sync_enter(Renderer.mtkView!)
        repeat {
            // Guard if not started.
            if !Renderer.isStarted {
                break
            }

            let delta = with.deltaY
            if delta > 0 {
                let KEY_UP: Int32 = 3
                on_event_key_press(KEY_UP)
                on_event_key_release(KEY_UP)
            } else if delta < 0 {
                let KEY_DOWN: Int32 = 4
                on_event_key_press(KEY_DOWN)
                on_event_key_release(KEY_DOWN)
            }
        } while false
        objc_sync_exit(Renderer.mtkView!)
    }
#endif

    //
    // HAL functions to called back from the C code.
    //

    static let logInfo: @convention(c) (UnsafeMutableRawPointer?) -> Void = { s -> Void in
        let str = String(validatingUTF8: s!.assumingMemoryBound(to: CChar.self))!
        print(str)
        ContentView.alert(text: str)
    }

    static let logWarn: @convention(c) (UnsafeMutableRawPointer?) -> Void = { s -> Void in
        let str = String(validatingUTF8: s!.assumingMemoryBound(to: CChar.self))!
        print(str)
    }

    static let logError: @convention(c) (UnsafeMutableRawPointer?) -> Void = { s -> Void in
        let str = String(validatingUTF8: s!.assumingMemoryBound(to: CChar.self))!
        print(str)
        ContentView.alert(text: str)
    }

    static let makeSavDir: @convention(c) () -> Void = {
    }

    static let makeValidPath: @convention(c) (UnsafeMutableRawPointer?, UnsafeMutableRawPointer?, UnsafeMutableRawPointer?, Int32) -> Void = { (dir, fname, dst, len) -> Void in
        var path = ""
        if let dirNonNull = dir {
            let dirStr = String(validatingUTF8: dirNonNull.assumingMemoryBound(to: CChar.self))!
            if let fileNonNull = fname {
                let fileStr = String(validatingUTF8: fileNonNull.assumingMemoryBound(to: CChar.self))!
                path = projectPath + dirStr + "/" + fileStr
            } else {
                path = projectPath + dirStr
            }
        } else {
            if let fileNonNull = fname {
                let fileStr = String(validatingUTF8: fileNonNull.assumingMemoryBound(to: CChar.self))!
                path = Renderer.projectPath + fileStr
            }
        }
        
        let p = dst!.bindMemory(to: Int8.self, capacity: Int(len) + 1)
        for i in 0 ... path.utf8CString.count - 1 {
            p[i] = path.utf8CString[i]
        }
        p[path.utf8CString.count] = 0
    }

    static let notifyImageUpdate: @convention(c) (Int32, Int32, Int32, UnsafeMutableRawPointer) -> Void = { (id, width, height, pixels) -> Void in
        // Add to the dictionary if image doesn't exist.
        if Renderer.imageDict[Int(id)] == nil {
            let image = ManagedImage(width: Int(width), height: Int(height), pixels: pixels, needUpload: true)
            Renderer.imageDict[Int(id)] = image
        }

        // Deffer uploads if not ready (For init time.)
        if Renderer.commandBuffer == nil {
            Renderer.uploadArray.append(Int(id))
            return
        }

        // Do an upload. (Or, deffer it to the next frame)
        Renderer.updateImage(index: Int(id))
    }

    static let notifyImageFree: @convention(c) (Int32) -> Void = { id -> Void in
        // Deffer a purge.
        Renderer.purgeArray.append(Int(id))
    }

    static let renderImageNormal: @convention(c) (Int32, Int32, Int32, Int32, Int32, Int32, Int32, Int32, Int32, Int32) -> Void = { (dst_left, dst_top, dst_width, dst_height, src_img, src_left, src_top, src_width, src_height, alpha) -> Void in
        drawPrimitives(dstLeft: Float(dst_left), dstTop: Float(dst_top), dstWidth: Float(dst_width), dstHeight: Float(dst_height), srcImageIndex: Int(src_img), ruleImageIndex: -1, srcLeft: Float(src_left), srcTop: Float(src_top), srcWidth: Float(src_width), srcHeight: Float(src_height), alpha: Float(alpha) / 255.0, pipeline: Renderer.normalPipelineState!);
    }

    static let renderImageAdd: @convention(c) (Int32, Int32, Int32, Int32, Int32, Int32, Int32, Int32, Int32, Int32) -> Void = { (dst_left, dst_top, dst_width, dst_height, src_img, src_left, src_top, src_width, src_height, alpha) -> Void in
        drawPrimitives(dstLeft: Float(dst_left), dstTop: Float(dst_top), dstWidth: Float(dst_width), dstHeight: Float(dst_height), srcImageIndex: Int(src_img), ruleImageIndex: -1, srcLeft: Float(src_left), srcTop: Float(src_top), srcWidth: Float(src_width), srcHeight: Float(src_height), alpha: Float(alpha) / 255.0, pipeline: Renderer.addPipelineState!);
    }

    static let renderImageDim: @convention(c) (Int32, Int32, Int32, Int32, Int32, Int32, Int32, Int32, Int32, Int32) -> Void = { (dst_left, dst_top, dst_width, dst_height, src_img, src_left, src_top, src_width, src_height, alpha) -> Void in
        drawPrimitives(dstLeft: Float(dst_left), dstTop: Float(dst_top), dstWidth: Float(dst_width), dstHeight: Float(dst_height), srcImageIndex: Int(src_img), ruleImageIndex: -1, srcLeft: Float(src_left), srcTop: Float(src_top), srcWidth: Float(src_width), srcHeight: Float(src_height), alpha: Float(alpha) / 255.0, pipeline: Renderer.dimPipelineState!);
    }

    static let renderImageRule: @convention(c) (Int32, Int32, Int32) -> Void = { (src_img, rule_img, threshold) in
        drawPrimitives(dstLeft: 0.0, dstTop: 0.0, dstWidth: Float(conf_window_width), dstHeight: Float(conf_window_height), srcImageIndex: Int(src_img), ruleImageIndex: Int(rule_img), srcLeft: 0.0, srcTop: 0.0, srcWidth: Float(conf_window_width), srcHeight: Float(conf_window_height), alpha: Float(threshold) / 255.0, pipeline: Renderer.rulePipelineState!);
    }
    
    static let renderImageMelt: @convention(c) (Int32, Int32, Int32) -> Void = { (src_img, rule_img, progress) in
        drawPrimitives(dstLeft: 0.0, dstTop: 0.0, dstWidth: Float(conf_window_width), dstHeight: Float(conf_window_height), srcImageIndex: Int(src_img), ruleImageIndex: Int(rule_img), srcLeft: 0.0, srcTop: 0.0, srcWidth: Float(conf_window_width), srcHeight: Float(conf_window_height), alpha: Float(progress) / 255.0, pipeline: Renderer.meltPipelineState!);
    }

    // Helper tp render a rectangle.
    static func drawPrimitives(dstLeft: Float, dstTop: Float, dstWidth: Float, dstHeight: Float, srcImageIndex: Int, ruleImageIndex: Int, srcLeft: Float, srcTop: Float, srcWidth: Float, srcHeight: Float, alpha: Float, pipeline: MTLRenderPipelineState) {
        // Get the images.
        let srcImage = Renderer.imageDict[Int(srcImageIndex)]!
        let ruleImage = Renderer.imageDict[Int(ruleImageIndex)]

        // Get the screen size.
        let screenWidth = Float(conf_window_width)
        let screenHeight = Float(conf_window_height)

        // Get the texture size.
        let texWidth = Float(srcImage.width)
        let texHeight = Float(srcImage.height)

        // X: -1.0 .. 1.0 left to right
        // Y: -1.0 .. 1.0 bottom to up
        // U:  0.0 .. 1.0 left to right
        // V:  0.0 .. 1.0 top to bottom

        // Set the left top vertex.
        let x1: Float = (dstLeft / screenWidth - 0.5) * 2.0
        let y1: Float = (dstTop / screenHeight - 0.5) * -2.0
        let u1: Float = srcLeft / texWidth
        let v1: Float = srcTop / texHeight

        // Set the right top vertex.
        let x2: Float = ((dstLeft + dstWidth) / screenWidth - 0.5) * 2.0
        let y2: Float = (dstTop / screenHeight - 0.5) * -2.0
        let u2: Float = (srcLeft + srcWidth) / texWidth
        let v2: Float = srcTop / texHeight

        // Set the left bottom vertex.
        let x3: Float = (dstLeft / screenWidth - 0.5) * 2.0
        let y3: Float = ((dstTop + dstHeight) / screenHeight - 0.5) * -2.0
        let u3: Float = srcLeft / texWidth
        let v3: Float = (srcTop + srcHeight) / texHeight

        // Set the right bottom vertex.
        let x4: Float = ((dstLeft + dstWidth) / screenWidth - 0.5) * 2.0
        let y4: Float = ((dstTop + dstHeight) / screenHeight - 0.5) * -2.0
        let u4: Float = (srcLeft + srcWidth) / texWidth
        let v4: Float = (srcTop + srcHeight) / texHeight

        // Make an array to input to the vertex shader.
        let vertexShaderIn: Array<Float> = [
            x1, y1, u1, v1, alpha, 0,
            x2, y2, u2, v2, alpha, 0,
            x3, y3, u3, v3, alpha, 0,
            x4, y4, u4, v4, alpha, 0
        ]

        // Upload textures if they are pending.
        if Renderer.blitEncoder != nil {
            Renderer.blitEncoder!.endEncoding()
            Renderer.blitEncoder = nil
        }

        // Draw two triangles.
        if Renderer.renderEncoder == nil {
            if let desc = Renderer.mtkView?.currentRenderPassDescriptor {
                desc.colorAttachments[0].loadAction = MTLLoadAction.clear
                desc.colorAttachments[0].storeAction = MTLStoreAction.store
                desc.colorAttachments[0].clearColor = conf_window_white != 0 ?
                MTLClearColor(red: 1.0, green: 1.0, blue: 1.0, alpha: 1.0) :
                MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
                Renderer.renderEncoder = Renderer.commandBuffer?.makeRenderCommandEncoder(descriptor: desc)
                Renderer.renderEncoder!.label = "MyRenderEncoder"
            }

            let viewport = MTLViewport(originX: Double(Renderer.viewportLeft), originY: Double(Renderer.viewportTop), width: Double(Renderer.viewportWidth), height: Double(Renderer.viewportHeight), znear: 0.0, zfar: 0.0)
            Renderer.renderEncoder?.setViewport(viewport)
        }
        Renderer.renderEncoder!.setRenderPipelineState(pipeline)
        Renderer.renderEncoder!.setVertexBytes(vertexShaderIn, length: 4 * 24, index: 0)
        Renderer.renderEncoder!.setFragmentTexture(srcImage.texture, index:GameTextureIndexColor)
        if ruleImage != nil {
            Renderer.renderEncoder!.setFragmentTexture(ruleImage!.texture, index:GameTextureIndexRule)
        }
        Renderer.renderEncoder!.drawPrimitives(type: MTLPrimitiveType.triangleStrip, vertexStart: 0, vertexCount: 4)
    }

    static let renderImage3dNormal: @convention(c) (Float32, Float32, Float32, Float32, Float32, Float32, Float32, Float32, Int32, Float32, Float32, Float32, Float32, Float32) -> Void = { (x1, y1, x2, y2, x3, y3, x4, y4, srcImageIndex, srcLeft, srcTop, srcWidth, srcHeight, alpha) -> Void in
        drawPrimitives3D(x1: x1, y1: y1, x2: x2, y2: y2, x3: x3, y3: y3, x4: x4, y4: y4, srcImageIndex: srcImageIndex, srcLeft: srcLeft, srcTop: srcTop, srcWidth: srcWidth, srcHeight: srcHeight, alpha: alpha, pipeline: Renderer.normalPipelineState!)
    }

    static let renderImage3dAdd: @convention(c) (Float32, Float32, Float32, Float32, Float32, Float32, Float32, Float32, Int32, Float32, Float32, Float32, Float32, Float32) -> Void = { (x1, y1, x2, y2, x3, y3, x4, y4, srcImageIndex, srcLeft, srcTop, srcWidth, srcHeight, alpha) -> Void in
        drawPrimitives3D(x1: x1, y1: y1, x2: x2, y2: y2, x3: x3, y3: y3, x4: x4, y4: y4, srcImageIndex: srcImageIndex, srcLeft: srcLeft, srcTop: srcTop, srcWidth: srcWidth, srcHeight: srcHeight, alpha: alpha, pipeline: Renderer.addPipelineState!)
    }

    // Helper tp render a polygon.
    static func drawPrimitives3D(x1: Float, y1: Float, x2: Float, y2: Float, x3: Float, y3: Float, x4: Float, y4: Float, srcImageIndex: Int32, srcLeft: Float, srcTop: Float, srcWidth: Float, srcHeight: Float, alpha: Float, pipeline: MTLRenderPipelineState) {
        // Get the images.
        let srcImage = Renderer.imageDict[Int(srcImageIndex)]!

        // Get the screen size.
        let screenWidth = Float(conf_window_width)
        let screenHeight = Float(conf_window_height)

        // Get the texture size.
        let texWidth = Float(srcImage.width)
        let texHeight = Float(srcImage.height)

        // X: -1.0 .. 1.0 left to right
        // Y: -1.0 .. 1.0 bottom to up
        // U:  0.0 .. 1.0 left to right
        // V:  0.0 .. 1.0 top to bottom

        // Set the left top vertex.
        let _x1: Float = (x1 / screenWidth - 0.5) * 2.0
        let _y1: Float = (y1 / screenHeight - 0.5) * -2.0
        let u1: Float = srcLeft / texWidth
        let v1: Float = srcTop / texHeight

        // Set the right top vertex.
        let _x2: Float = (x2 / screenWidth - 0.5) * 2.0
        let _y2: Float = (y2 / screenHeight - 0.5) * -2.0
        let u2: Float = (srcLeft + srcWidth) / texWidth
        let v2: Float = srcTop / texHeight

        // Set the left bottom vertex.
        let _x3: Float = (x3 / screenWidth - 0.5) * 2.0
        let _y3: Float = (y3 / screenHeight - 0.5) * -2.0
        let u3: Float = srcLeft / texWidth
        let v3: Float = (srcTop + srcHeight) / texHeight

        // Set the right bottom vertex.
        let _x4: Float = (x4 / screenWidth - 0.5) * 2.0
        let _y4: Float = (y4 / screenHeight - 0.5) * -2.0
        let u4: Float = (srcLeft + srcWidth) / texWidth
        let v4: Float = (srcTop + srcHeight) / texHeight

        // Make an array to input to the vertex shader.
        let vertexShaderIn: Array<Float> = [
            _x1, _y1, u1, v1, alpha, 0,
            _x2, _y2, u2, v2, alpha, 0,
            _x3, _y3, u3, v3, alpha, 0,
            _x4, _y4, u4, v4, alpha, 0
        ]

        // Upload textures if they are pending.
        if Renderer.blitEncoder != nil {
            Renderer.blitEncoder!.endEncoding()
            Renderer.blitEncoder = nil
        }

        // Draw two triangles.
        if Renderer.renderEncoder == nil {
            if let desc = Renderer.mtkView?.currentRenderPassDescriptor {
                desc.colorAttachments[0].loadAction = MTLLoadAction.clear
                desc.colorAttachments[0].storeAction = MTLStoreAction.store
                desc.colorAttachments[0].clearColor = conf_window_white != 0 ?
                MTLClearColor(red: 1.0, green: 1.0, blue: 1.0, alpha: 1.0) :
                MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
                Renderer.renderEncoder = Renderer.commandBuffer?.makeRenderCommandEncoder(descriptor: desc)
                Renderer.renderEncoder!.label = "MyRenderEncoder"
            }

            let viewport = MTLViewport(originX: Double(Renderer.viewportLeft), originY: Double(Renderer.viewportTop), width: Double(Renderer.viewportWidth), height: Double(Renderer.viewportHeight), znear: 0.0, zfar: 0.0)
            Renderer.renderEncoder?.setViewport(viewport)
        }
        Renderer.renderEncoder!.setRenderPipelineState(pipeline)
        Renderer.renderEncoder!.setVertexBytes(vertexShaderIn, length: 4 * 24, index: 0)
        Renderer.renderEncoder!.setFragmentTexture(srcImage.texture, index:GameTextureIndexColor)
        Renderer.renderEncoder!.drawPrimitives(type: MTLPrimitiveType.triangleStrip, vertexStart: 0, vertexCount: 4)
    }

    static let resetLapTimer: @convention(c) (UnsafeMutableRawPointer?) -> Void = { origin -> Void in
        let ms = Int64((Date().timeIntervalSince1970 * 1000.0).rounded())
        let p = origin!.bindMemory(to: Int64.self, capacity: 1)
        p[0] = ms
    }

    static let getLapTimerMillisec: @convention(c) (UnsafeMutableRawPointer?) -> Int64 = { origin -> Int64 in
        let p = origin!.bindMemory(to: Int64.self, capacity: 1)
        let origin = p[0]
        let now = Int64((Date().timeIntervalSince1970 * 1000.0).rounded())
        return Int64(now - origin)
    }

    static let playVideo: @convention(c) (UnsafePointer<CChar>?, Int32) -> Int32 = { (fname, is_skippable) in
        return 0
    }

    static let stopVideo: @convention(c) () -> Void = {
    }

    static let isVideoPlaying: @convention(c) () -> Bool = {
        return false
    }

    static let updateWindowTitle: @convention(c) () -> Void = {
        
    }

    static let isFullScreenSupported: @convention(c) () -> Int32 = {
        return 0;
    }

    static let isFullScreenMode: @convention(c) () -> Int32 = {
        return 0;
    }

    static let enterFullScreenMode: @convention(c) () -> Void = {
    }

    static let leaveFullScreenMode: @convention(c) () -> Void = {
    }

    static let getSystemLocale: @convention(c) (UnsafeMutableRawPointer?, Int32) -> Void = { (dst, len) in
        let p = dst!.bindMemory(to: CChar.self, capacity: 3)
        p[0] = 106 // 'j'
        p[1] = 97  // 'a'
        p[2] = 0   // NUL
    }

    static let speakText: @convention(c) (UnsafePointer<CChar>) -> Void = { (text) in
    }

    static let setContinuousSwipeEnabled: @convention(c) (Bool) -> Void = { (is_enabled) in
        Renderer.isContinuousSwipeEnabled = is_enabled
    }

    //
    // Editor HAL
    //

    #if USE_EDITOR
    static let isContinuePushed: @convention(c) () -> Bool = {
        return true
    }

    static let isNextPushed: @convention(c) () -> Bool = {
        return false
    }

    static let isStopPushed: @convention(c) () -> Bool = {
        return false
    }

    static let isScriptOpened: @convention(c) () -> Bool = {
        return false
    }

    static let getOpenedScript: @convention(c) (UnsafeMutableRawPointer, Int32) -> Void = { (dst, len) in
        let path = "aaa"
        let p = dst.bindMemory(to: Int8.self, capacity: Int(len) + 1)
        for i in 0 ... path.utf8CString.count - 1 {
            p[i] = path.utf8CString[i]
        }
        p[path.utf8CString.count] = 0
    }

    static let isExecLineChanged: @convention(c) () -> Bool = {
        return false
    }

    static let getChangedExecLine: @convention(c) () -> Int32 = {
        return 0
    }

    static let onChangeRunningState: @convention(c) (Int32, Int32) -> Void = { (running, request_stop) in
    }

    static let onLoadScript: @convention(c) () -> Void = {
    }

    static let onChangePosition: @convention(c) () -> Void = {
    }

    static let onUpdateVariable: @convention(c) () -> Void = {
    }
    #endif
}
