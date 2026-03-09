/* -*- coding: utf-8; tab-width: 4; indent-tabs-mode: nil; -*- */

/*
 * StratoHAL
 * Main code for Qt6
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

#include "qtgamewidget.h"

// UI
#include <QApplication>
#include <QMouseEvent>
#include <QMessageBox>
#include <QDir>

// Sound
#include <QMediaDevices>
#include <QAudioFormat>

// File
#include <QFile>

// OpenGL
#include <QOpenGLExtraFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

extern "C" {
#include "glrender.h"
#include "stdfile.h"
}

GameWidget *GameWidget::singleton;

GameWidget *GameWidget::createSingleton(QString path)
{
    GameWidget::singleton = new GameWidget();

    // Call on_boot().
    char *title;
    int width;
    int height;
    if (!on_event_boot(&title, &width, &height)) {
        // Error.
        GameWidget::singleton->m_isGameStarted = false;
        return nullptr;
    }
    GameWidget::singleton->m_gameTitle = title;
    GameWidget::singleton->m_gameWidth = width;
    GameWidget::singleton->m_gameHeight = height;

    return GameWidget::singleton;
}

void GameWidget::destroySingleton()
{
    // TODO
}

GameWidget *GameWidget::getSingleton()
{
    return GameWidget::singleton;
}

GameWidget::GameWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    // Initialize the status.
    m_isGameStarted = false;
    m_isOpenGLInitialized = false;
    m_isFirstFrame = false;

    // Set no transparency.
    setAttribute(Qt::WA_TranslucentBackground, false);

    // Receive mouse move events.
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setEnabled(true);

    // Setup the sound outputs.
    QAudioFormat format;
    format.setSampleFormat(QAudioFormat::Int16);
    format.setChannelCount(2);
    format.setSampleRate(44100);
    auto device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(format)) {
        qWarning("format not supported, using preferred format");
        format = device.preferredFormat();
    }
    for (int i = 0; i < SOUND_TRACKS; i++) {
        m_wave[i] = NULL;
        m_waveFinish[i] = false;
        m_soundSink[i] = new QAudioSink(device, format);
    }

    // Setup a 33ms timer for game frames.
    m_timer = new QTimer();
    connect(m_timer, &QTimer::timeout, this, &GameWidget::onTimer);
    m_timer->start(33);

    this->start();
}

GameWidget::~GameWidget()
{
}

void GameWidget::start()
{
    m_isGameStarted = true;
}

void GameWidget::initializeGL()
{
    // Initialize Qt's OpenGL function pointers.
    initializeOpenGLFunctions();

    // Start using the OpenGL ES rendering subsystem.
    if (!init_opengl(m_gameWidth, m_gameHeight)) {
        // Error.
        return;
    }

    glViewport(m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight);

    m_isOpenGLInitialized = true;
    m_isFirstFrame = true;
}

void GameWidget::paintGL()
{
    // Guard if not started.
    if (!m_isGameStarted)
        return;

    // Guard if not initialized.
    if (!m_isOpenGLInitialized)
        return;

    // On the first frame.
    if (m_isFirstFrame) {
        // Call on_start().
        if (!on_event_start()) {
            // Error.
            m_isGameStarted = false;
            return;
        }

        // Initialization succeeded.
        m_isFirstFrame = false;
    }

    // Set the viewport.
    // Do a lazy update because we can't set viewport size in resizeGL().
    glViewport(m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight);

    // Start an OpenGL rendering.
    opengl_start_rendering();

    // Run an event frame rendering.
    bool cont = on_event_frame();

    // End an OpenGL rendering.
    opengl_end_rendering();

    // If we reached EOF of a script.
    if (!cont) {
        // Do not continue.
        m_isGameStarted = false;
    }
}

void GameWidget::resizeGL(int width, int height)
{
    // Calc the aspect ratio of the game.
    float aspect = (float)m_gameHeight / (float)m_gameWidth;

    // Get the view size.
    float viewWidthHiDPI = width * devicePixelRatio();
    float viewHeightHiDPI = height * devicePixelRatio();

    // Set the height temporarily with "width-first".
    float useWidthHiDPI = viewWidthHiDPI;
    float useHeightHiDPI = viewWidthHiDPI * aspect;
    m_mouseScale = (float)m_gameWidth / (float)width;

    // If height is not enough, determine width with "height-first".
    if(useHeightHiDPI > viewHeightHiDPI) {
        useHeightHiDPI = viewHeightHiDPI;
        useWidthHiDPI = viewHeightHiDPI / aspect;
        m_mouseScale = (float)m_gameHeight / (float)height;
    }

    // Calc the OpenGL screen origin.
    float originXHiDPI = (viewWidthHiDPI - useWidthHiDPI) / 2.0f;
    float originYHiDPI = (viewHeightHiDPI - useHeightHiDPI) / 2.0f;

    // Will be applied in a next frame.
    m_viewportX = (int)originXHiDPI;
    m_viewportY = (int)originYHiDPI;
    m_viewportWidth = (int)useWidthHiDPI;
    m_viewportHeight = (int)useHeightHiDPI;

    // Mouse events are not HiDPI.
    m_mouseLeft = (int)(originXHiDPI / devicePixelRatio());
    m_mouseTop = (int)(originYHiDPI / devicePixelRatio());
}

void GameWidget::mousePressEvent(QMouseEvent *event)
{
    int x = event->position().x();
    int y = event->position().y();

    x = (int)((float)(x - m_mouseLeft) * m_mouseScale);
    y = (int)((float)(y - m_mouseTop) * m_mouseScale);
    if (x < 0 || x > m_gameWidth)
        return;
    if (y < 0 || y > m_gameHeight)
        return;

    if (event->button() == Qt::LeftButton)
        on_event_mouse_press(MOUSE_LEFT, x, y);
    else if (event->button() == Qt::RightButton)
        on_event_mouse_press(MOUSE_RIGHT, x, y);
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event)
{
    int x = event->position().x();
    int y = event->position().y();

    x = (int)((float)(x - m_mouseLeft) * m_mouseScale);
    y = (int)((float)(y - m_mouseTop) * m_mouseScale);
    if (x < 0 || x > m_gameWidth)
        return;
    if (y < 0 || y > m_gameHeight)
        return;

    if (event->button() == Qt::LeftButton)
        on_event_mouse_release(MOUSE_LEFT, x, y);
    else if (event->button() == Qt::RightButton)
        on_event_mouse_release(MOUSE_RIGHT, x, y);
}

void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->position().x();
    int y = event->position().y();

    x = (int)((float)(x - m_mouseLeft) * m_mouseScale);
    y = (int)((float)(y - m_mouseTop) * m_mouseScale);
    if (x < 0 || x > m_gameWidth)
        return;
    if (y < 0 || y > m_gameHeight)
        return;

    on_event_mouse_move(x, y);
}

void GameWidget::onTimer()
{
    // Do a game frame.
    this->update();

    // Start sound on the first call.
    if (!m_isSoundStarted) {
        for (int i = 0; i < SOUND_TRACKS; i++)
            m_soundDevice[i] = m_soundSink[i]->start();
        m_isSoundStarted = true;
    }

    // Do sound bufferings.
    const int SNDBUFSIZE = 4096;
    static uint32_t soundBuf[SNDBUFSIZE];
    for (int i = 0; i < SOUND_TRACKS; i++) {
        if (m_wave[i] == NULL)
            continue;
        if (m_soundDevice[i] == NULL)
            continue;
        if (!m_soundDevice[i]->isWritable())
            continue;
        if (m_soundSink[i] == NULL)
            continue;

        int needSamples = m_soundSink[i]->bytesFree() / 4;
        int restSamples = needSamples;
        while (restSamples > 0) {
            int reqSamples = restSamples > SNDBUFSIZE ? SNDBUFSIZE : restSamples;
            int readSamples = get_wave_samples(m_wave[i], &soundBuf[0], reqSamples);
            if (readSamples <= 0) {
                m_wave[i] = NULL;
                m_waveFinish[i] = true;
                break;
            }
            m_soundDevice[i]->write((char const *)&soundBuf[0], readSamples * 4);
            restSamples -= readSamples;
        }
    }
}

//
// HAL (See also src/hal.h)
//

extern "C"
struct rfile {
    QFile *qf;
};

extern "C"
struct wfile {
    QFile *qf;
};

extern "C"
bool log_info(const char *s, ...)
{
    char buf[1024];
    va_list ap;
    
    va_start(ap, s);
    vsnprintf(buf, sizeof(buf), s, ap);
    va_end(ap);

    QMessageBox msgbox(nullptr);
    msgbox.setIcon(QMessageBox::Information);
    msgbox.setWindowTitle("Info");
    msgbox.setText(buf);
    msgbox.addButton(QMessageBox::Close);
    msgbox.exec();

    return true;
}

extern "C"
bool log_warn(const char *s, ...)
{
    char buf[1024];
    va_list ap;
    
    va_start(ap, s);
    vsnprintf(buf, sizeof(buf), s, ap);
    va_end(ap);

    QMessageBox msgbox(nullptr);
    msgbox.setIcon(QMessageBox::Warning);
    msgbox.setWindowTitle("Warn");
    msgbox.setText(buf);
    msgbox.addButton(QMessageBox::Close);
    msgbox.exec();

    return true;
}

extern "C"
bool log_error(const char *s, ...)
{
    char buf[1024];
    va_list ap;
    
    va_start(ap, s);
    vsnprintf(buf, sizeof(buf), s, ap);
    va_end(ap);

    QMessageBox msgbox(nullptr);
    msgbox.setIcon(QMessageBox::Warning); // Intended, it's not critical actually.
    msgbox.setWindowTitle("Error");
    msgbox.setText(buf);
    msgbox.addButton(QMessageBox::Close);
    msgbox.exec();

    return true;
}

extern "C"
bool log_out_of_memory(void)
{
    log_error("Out of memory.\n");
    return true;
}

extern "C"
void reset_lap_timer(uint64_t *t)
{
    *t = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
}

extern "C"
uint64_t get_lap_timer_millisec(uint64_t *t)
{
    uint64_t ms = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
    return ms - *t;
}

extern "C"
bool make_save_directory(void)
{
    if (QDir("save").exists())
        return true;

    QDir qdir;
    if (!qdir.mkdir("save"))
        return false;

    return true;
}

extern "C"
char *make_real_path(const char *fname)
{
    QDir qdir(QCoreApplication::applicationDirPath());
    QString path = qdir.currentPath() + QString("/") + QString(fname);
    char *ret = strdup(path.toUtf8().data());
    if (ret == NULL) {
        log_out_of_memory();
        return NULL;
    }
    return ret;
}

extern "C" bool check_file_exist(const char *file)
{
    assert(file != NULL);

    return QFile::exists(QString::fromUtf8(file));
}

extern "C" bool open_rfile(const char *file, struct rfile **f)
{
    auto rf = new rfile;
    rf->qf = new QFile(QString::fromUtf8(file));
    if (!rf->qf->open(QIODevice::ReadOnly)) {
        delete rf->qf;
        delete rf;
        return false;
    }
    *f = rf;
    return true;
}

extern "C" bool get_rfile_size(struct rfile *rf, size_t *ret)
{
    assert(rf != NULL);
    assert(rf->qf != NULL);

    if (ret)
        *ret = rf->qf->size();

    return true;
}

extern "C" bool read_rfile(struct rfile *rf, void *buf, size_t size, size_t *ret)
{
    assert(rf != NULL);
    assert(rf->qf != NULL);

    *ret = rf->qf->read((char*)buf, size);
    if (*ret == 0)
        return false;

    return true;

}

extern "C" bool get_rfile_u64(struct rfile *rf, uint64_t *data)
{
    assert(rf != NULL);
    assert(rf->qf != NULL);

    if (rf->qf->read((char *)data, 8) != 8)
        return false;

    return true;
}

extern "C" bool get_rfile_u32(struct rfile *rf, uint32_t *data)
{
    assert(rf != NULL);
    assert(rf->qf != NULL);

    if (rf->qf->read((char *)data, 8) != 4)
        return false;

    return true;
}

extern "C" bool get_rfile_u16(struct rfile *rf, uint16_t *data)
{
    assert(rf != NULL);
    assert(rf->qf != NULL);

    if (rf->qf->read((char *)data, 8) != 2)
        return false;

    return true;
}

extern "C" bool get_rfile_u8(struct rfile *rf, uint8_t *data)
{
    assert(rf != NULL);
    assert(rf->qf != NULL);

    if (rf->qf->read((char *)data, 8) != 1)
        return false;

    return true;
}

extern "C" bool get_rfile_string(struct rfile *f, char *buf, size_t size)
{
	char *ptr;
	size_t len, read_size;
	char c;

	assert(f != NULL);
	assert(f->qf != NULL);
	assert(buf != NULL);
	assert(size > 0);

	ptr = buf;
	for (len = 0; len < size - 1; len++) {
		if (!read_rfile(f, &c, 1, &read_size)) {
			*ptr = '\0';
			if (len == 0)
				return false;
			return true;
		}
		if (c == '\n' || c == '\0') {
			*ptr = '\0';
			return true;
		}
		if (c == '\r') {
			if (!read_rfile(f, &c, 1, &read_size)) {
				*ptr = '\0';
				return true;
			}
			if (c == '\n') {
				*ptr = '\0';
				return true;
			}
            
            // ungetc()
            f->qf->seek(f->qf->pos() - 1);
			*ptr = '\0';
			return true;
		}
		*ptr++ = c;
	}
	*ptr = '\0';

	if (len == 0)
		return false;

	return true;
}

extern "C" void close_rfile(struct rfile *rf)
{
    rf->qf->close();
    delete rf->qf;
    delete rf;
}

extern "C" void rewind_rfile(struct rfile *rf)
{
    assert(rf != NULL);
    assert(rf->qf != NULL);

    rf->qf->seek(0);
}

extern "C" bool open_wfile(const char *file, struct wfile **wf)
{
    assert(file != NULL);
    assert(wf != NULL);

    QFile *qf = new QFile(QString::fromUtf8(file));
    if (!qf->open(QIODevice::WriteOnly)) {
        delete qf;
        *wf = NULL;
        return false;
    }

    *wf = new wfile;
    (*wf)->qf = qf;
    return true;
}

extern "C" bool write_wfile(struct wfile *wf, const void *buf, size_t size, size_t *ret)
{
    assert(wf != NULL);
    assert(buf != NULL);
    assert(ret != NULL);

    qint64 written = wf->qf->write(static_cast<const char *>(buf), static_cast<qint64>(size));
    if (written < 0) {
        *ret = 0;
        return false;
    }

    *ret = static_cast<size_t>(written);
    return true;
}

extern "C" void close_wfile(struct wfile *wf)
{
    wf->qf->close();
    delete wf->qf;
    delete wf;
}

extern "C" void remove_file(const char *file)
{
    QFile::remove(QString::fromUtf8(file));
}

extern "C"
void notify_image_update(struct image *img)
{
    opengl_notify_image_update(img);
}

extern "C"
void notify_image_free(struct image *img)
{
    opengl_notify_image_free(img);
}

extern "C"
void render_image_normal(
    int dst_left,
    int dst_top,
    int dst_width,
    int dst_height,
    struct image *src_image,
    int src_left,
    int src_top,
    int src_width,
    int src_height,
                         int alpha)
{
    opengl_render_image_normal(dst_left,
                               dst_top,
                               dst_width,
                               dst_height,
                               src_image,
                               src_left,
                               src_top,
                               src_width,
                               src_height,
                               alpha);
}

extern "C"
void render_image_add(
    int dst_left,
    int dst_top,
    int dst_width,
    int dst_height,
    struct image *src_image,
    int src_left,
    int src_top,
    int src_width,
    int src_height,
    int alpha)
{
    opengl_render_image_add(dst_left,
                            dst_top,
                            dst_width,
                            dst_height,
                            src_image,
                            src_left,
                            src_top,
                            src_width,
                            src_height,
                            alpha);
}

extern "C"
void render_image_sub(
    int dst_left,
    int dst_top,
    int dst_width,
    int dst_height,
    struct image *src_image,
    int src_left,
    int src_top,
    int src_width,
    int src_height,
    int alpha)
{
    opengl_render_image_sub(dst_left,
                            dst_top,
                            dst_width,
                            dst_height,
                            src_image,
                            src_left,
                            src_top,
                            src_width,
                            src_height,
                            alpha);
}

extern "C"
void render_image_dim(
    int dst_left,
    int dst_top,
    int dst_width,
    int dst_height,
    struct image *src_image,
    int src_left,
    int src_top,
    int src_width,
    int src_height,
    int alpha)
{
    opengl_render_image_dim(dst_left,
                            dst_top,
                            dst_width,
                            dst_height,
                            src_image,
                            src_left,
                            src_top,
                            src_width,
                            src_height,
                            alpha);
}

extern "C"
void render_image_rule(
    struct image *src_img,
    struct image *rule_img,
    int threshold)
{
    opengl_render_image_rule(src_img, rule_img, threshold);
}

extern "C"
void render_image_melt(
    struct image *src_img,
    struct image *rule_img,
    int progress)
{
    opengl_render_image_melt(src_img, rule_img, progress);
}

extern "C"
void render_image_3d_normal(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	opengl_render_image_3d_normal(x1,
				      y1,
				      x2,
				      y2,
				      x3,
				      y3,
				      x4,
				      y4,
				      src_image,
				      src_left,
				      src_top,
				      src_width,
				      src_height,
				      alpha);
}

extern "C"
void render_image_3d_add(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	opengl_render_image_3d_add(x1,
				   y1,
				   x2,
				   y2,
				   x3,
				   y3,
				   x4,
				   y4,
				   src_image,
				   src_left,
				   src_top,
				   src_width,
				   src_height,
				   alpha);
}

extern "C"
void render_image_3d_sub(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	opengl_render_image_3d_sub(x1,
				   y1,
				   x2,
				   y2,
				   x3,
				   y3,
				   x4,
				   y4,
				   src_image,
				   src_left,
				   src_top,
				   src_width,
				   src_height,
				   alpha);
}

extern "C"
void render_image_3d_dim(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	opengl_render_image_3d_dim(x1,
				   y1,
				   x2,
				   y2,
				   x3,
				   y3,
				   x4,
				   y4,
				   src_image,
				   src_left,
				   src_top,
				   src_width,
				   src_height,
				   alpha);
}

extern "C"
bool play_sound(int stream, struct wave *w)
{
    GameWidget *widget = GameWidget::getSingleton();
    if (widget == NULL)
        return true;
    if (widget->m_soundSink[stream] == NULL)
        return true;

    if (widget->m_wave[stream] == NULL)
        widget->m_soundSink[stream]->start();

    widget->m_wave[stream] = w;
    widget->m_waveFinish[stream] = false;
    return true;
}

extern "C"
bool stop_sound(int stream)
{
    GameWidget *widget = GameWidget::getSingleton();
    if (widget == NULL)
        return true;
    if (widget->m_soundSink[stream] == NULL)
        return true;

    widget->m_soundSink[stream]->stop();
    widget->m_wave[stream] = NULL;
    widget->m_waveFinish[stream] = false;
    return true;
}

extern "C"
bool set_sound_volume(int stream, float vol)
{
    GameWidget *widget = GameWidget::getSingleton();
    if (widget == NULL)
        return true;
    if (widget->m_soundSink[stream] == NULL)
        return true;

    widget->m_soundSink[stream]->setVolume(vol);
    return true;
}

extern "C"
bool is_sound_finished(int stream)
{
    GameWidget *widget = GameWidget::getSingleton();
    if (widget == NULL)
        return true;
    if (widget->m_soundSink[stream] == NULL)
        return true;

    if (!widget->m_waveFinish[stream])
        return false;

    return true;
}

extern "C"
bool play_video(const char *fname, bool is_skippable)
{
    // TODO:

    // char *path;
    // path = make_valid_path(MOV_DIR, fname);
    // is_gst_playing = true;
    // is_gst_skippable = is_skippable;
    // gstplay_play(path, window);
    // free(path);
    return true;
}

extern "C"
void stop_video(void)
{
    // TODO:

    //gstplay_stop();
    //is_gst_playing = false;
}

extern "C"
bool is_video_playing(void)
{
    // TODO:

    //return is_gst_playing;
    return false;
}

extern "C"
bool is_full_screen_supported(void)
{
    // We don't use full screen mode for a debugger.
    return false;
}

extern "C"
bool is_full_screen_mode(void)
{
    // We don't use full screen mode for a debugger.
    return false;
}

extern "C"
void enter_full_screen_mode(void)
{
    // We don't use full screen mode for a debugger.
}

extern "C"
void leave_full_screen_mode(void)
{
    // We don't use full screen mode for a debugger.
}

extern "C"
const char *get_system_language(void)
{
    const char *locale = QLocale().name().toUtf8().data();
    if (locale == NULL)
        return "en";
    else if (locale[0] == '\0' || locale[1] == '\0')
        return "en";
    else if (strncmp(locale, "en", 2) == 0)
        return "en";
    else if (strncmp(locale, "fr", 2) == 0)
        return "fr";
    else if (strncmp(locale, "de", 2) == 0)
        return "de";
    else if (strncmp(locale, "it", 2) == 0)
        return "it";
    else if (strncmp(locale, "es", 2) == 0)
        return "es";
    else if (strncmp(locale, "el", 2) == 0)
        return "el";
    else if (strncmp(locale, "ru", 2) == 0)
        return "ru";
    else if (strncmp(locale, "zh_CN", 5) == 0)
        return "zh";
    else if (strncmp(locale, "zh_TW", 5) == 0)
        return "tw";
    else if (strncmp(locale, "ja", 2) == 0)
        return "ja";

    return "other";
}

//
// OpenGL Wrapper
//

extern "C" {

// Maximum number of fragment shaders we support.
#define FSHADER_COUNT 10

//
// The sole vertex shader.
//
static QOpenGLShader *vshader;

//
// Number of fragment shaders registered.
//
static int fshader_count;

//
// Object tables.
//
static QOpenGLShader *fshader_tbl[FSHADER_COUNT];
static QOpenGLShaderProgram *prog_tbl[FSHADER_COUNT];
static QOpenGLVertexArrayObject *vao_tbl[FSHADER_COUNT];
static QOpenGLBuffer *vbo_tbl[FSHADER_COUNT];
static QOpenGLBuffer *ibo_tbl[FSHADER_COUNT];

//
// Current selected program.
//
static GLuint cur_program = (GLuint)-1;

//
// OpenGL 2+/3+ initialization
//  - Here we do initialization of shader/program/VAO/VBO/IBO
//  - We can't call raw OpenGL 2+ API on Qt in glrender.c
//  - Alternatively, setup_*() is called from init_opengl() in glrender.c
//

//
// Setup a vertex shader.
//
bool setup_vertex_shader(const char **vshader_src, GLuint *ret_vshader)
{
    // Assert the sole vertex shader is not created.
    assert(vshader == NULL);

    // Create the vertex shader.
    vshader = new QOpenGLShader(QOpenGLShader::Vertex);
    vshader->compileSourceCode(*vshader_src);
    if (!vshader->isCompiled()) {
        log_info("Failed to compile a vertex shader: %s", vshader->log().toUtf8().data());
        return false;
    }
    *ret_vshader = vshader->shaderId();

    return true;
}

//
// Cleanup a vertex shader.
//
void cleanup_vertex_shader(GLuint vshader_id)
{
    // Delete the sole vertex shader.
    if (vshader != NULL) {
        assert(vshader->shaderId() == vshader_id);
        delete vshader;
        vshader = NULL;
    }
}

//
// Setup a fragment shader, a program, a VAO, a VBO and an IBO.
//
bool
setup_fragment_shader(
    const char **fshader_src,
    GLuint vshader_id,
    bool use_second_texture,
    GLuint *ret_fshader,        // OUT
    GLuint *ret_prog,           // OUT
    GLuint *ret_vao,            // OUT
    GLuint *ret_vbo,            // OUT
    GLuint *ret_ibo)            // OUT
{
    // Assert non-NULL.
    assert(fshader_src != NULL);
    assert(*fshader_src != NULL);
    assert(ret_fshader != NULL);
    assert(ret_prog != NULL);
    assert(ret_vao != NULL);
    assert(ret_vbo != NULL);
    assert(ret_ibo != NULL);

    // Assert the sole vertex shader is created.
    assert(vshader != NULL);

    // Assert we have room for the tables.
    assert(fshader_count < FSHADER_COUNT);

    // Create a fragment shader.
    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment);
    fshader->compileSourceCode(*fshader_src);
    if (!fshader->isCompiled()) {
        log_info("Failed to compile a fragment shader: %s", fshader->log().toUtf8().data());
        return false;
    }
    *ret_fshader = fshader->shaderId();

    // Create a program.
    QOpenGLShaderProgram *prog = new QOpenGLShaderProgram();
    prog->addShader(vshader);
    prog->addShader(fshader);
    prog->link();
    prog->bind();
    *ret_prog = prog->programId();

    // Create a VAO.
    QOpenGLVertexArrayObject *vao = new QOpenGLVertexArrayObject();
    vao->create();
    vao->bind();
    *ret_vao = vao->objectId();

    // Create a VBO for "XYZUVA" * 4vertices.
    QOpenGLBuffer *vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo->allocate(6 * 4 * sizeof(GLfloat)); // 6 * 4 = "XYZUVA" * 4vertices
    *ret_vbo = vbo->bufferId();

    // Set the vertex attibute for "a_position" in the vertex shader.
    int posLoc = prog->attributeLocation("a_position");
    prog->enableAttributeArray(posLoc);
    prog->setAttributeBuffer(posLoc,
                             GL_FLOAT,
                             0,                         // pos 0: at "X" in "XYZUVA"
                             3,                         // for "XYZ"
                             6 * sizeof(GLfloat));      // for "XYZUVA"

    // Set the vertex attibute for "a_texCoord" in the vertex shader.
    int texLoc = prog->attributeLocation("a_texCoord");
    prog->enableAttributeArray(texLoc);
    prog->setAttributeBuffer(texLoc,
                             GL_FLOAT,
                             3 * sizeof(GLfloat),       // pos 3: at "U" in "XYZUVA"
                             2,                         // for "UV"
                             6 * sizeof(GLfloat));      // for "XYZUVA"

    // Set the vertex attibute for "a_alpha" in the vertex shader.
    int alphaLoc = prog->attributeLocation("a_alpha");
    prog->enableAttributeArray(alphaLoc);
    prog->setAttributeBuffer(alphaLoc,
                             GL_FLOAT,
                             5 * sizeof(GLfloat),       // pos 5: at "A" in "XYZUVA"
                             1,                         // for "A"
                             6 * sizeof(GLfloat));      // for "XYZUVA"

    // Setup "s_texture" in a fragment shader.
    int samplerLoc = prog->uniformLocation("s_texture");
    prog->setUniformValue(samplerLoc, 0);

    // Setup "s_rule" in a fragment shader if we use a second texture.
    if (use_second_texture) {
        int ruleLoc = prog->uniformLocation("s_rule");
        prog->setUniformValue(ruleLoc, 1);
    }

    // Create an IBO.
    const GLushort indices[] = {0, 1, 2, 3};
    QOpenGLBuffer *ibo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ibo->create();
    ibo->bind();
    ibo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    ibo->allocate(4 * sizeof(GLushort));
    ibo->write(0, indices, 4 * sizeof(GLushort));
    *ret_ibo = ibo->bufferId();

    // Store objects to the tables.
    fshader_tbl[fshader_count] = fshader;
    prog_tbl[fshader_count] = prog;
    vao_tbl[fshader_count] = vao;
    vbo_tbl[fshader_count] = vbo;
    ibo_tbl[fshader_count] = ibo;
    fshader_count++;

    return true;
}

//
// Cleanup a fragment shader with a program, a VAO, a VBO and an IBO.
//
void
cleanup_fragment_shader(
    GLuint fshader,
    GLuint prog,
    GLuint vao,
    GLuint vbo,
    GLuint ibo)
{
    // Search the fragment shader index.
    int index;
    for (index = 0; index < fshader_count; index++) {
        assert(fshader_tbl[index] != NULL);
        if (fshader_tbl[index]->shaderId() == fshader)
            break;
    }
    if (index == fshader_count)
        return;

    // Check integity.
    assert(prog_tbl[index]->programId() == prog);
    assert(vao_tbl[index]->objectId() == vao);
    assert(vbo_tbl[index]->bufferId() == vbo);
    assert(ibo_tbl[index]->bufferId() == ibo);

    // Delete all.
    delete prog_tbl[index];
    delete ibo_tbl[index];
    delete vbo_tbl[index];
    delete vao_tbl[index];
    delete fshader_tbl[index];

    // NULLify the table entries.
    fshader_tbl[index] = NULL;
    prog_tbl[index] = NULL;
    vao_tbl[index] = NULL;
    vbo_tbl[index] = NULL;
    ibo_tbl[index] = NULL;
}

}; // extern "C"

//
// OpenGL API 1.1/2+/3+ wrapper
//   - Here, we define API wrappers named q_gl*().
//   - gl*() call sites in glrender.c are repladed to q_gl*() by macros defined in glhelper.h.
//

// F: the abbreviated pointer to the QOpenGLFunctions object.
#define F QOpenGLContext::currentContext()->functions()

// -- OpenGL 1.1 --

extern "C"
void q_glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    // Nothing to do here.
    // See also openglview.cpp
}

extern "C"
void q_glClear(GLbitfield mask)
{
    // Just map.
    F->glClear(mask);
}

extern "C"
void q_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    // Just map.
    F->glClearColor(red, green, blue, alpha);
}

extern "C"
void q_glEnable(GLenum cap)
{
    // Just map.
    F->glEnable(cap);
}

extern "C"
void q_glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    // Just map.
    F->glBlendFunc(sfactor, dfactor);
}

extern "C"
void q_glFlush(void)
{
    // Just map.
    F->glFlush();
}

extern "C"
void q_glGenTextures(GLsizei n, GLuint *textures)
{
    // Just map.
    F->glGenTextures(n, textures);
}

extern "C"
void q_glActiveTexture(GLenum texture)
{
    // Just map.
    F->glActiveTexture(texture);
}

extern "C"
void q_glBindTexture(GLenum target, GLuint texture)
{
    // Just map.
    F->glBindTexture(target, texture);
}

extern "C"
void q_glDeleteTextures(GLsizei n, const GLuint *textures)
{
    // Just map.
    F->glDeleteTextures(n,textures);
}

extern "C"
void q_glPixelStorei(GLenum pname, GLint param)
{
    // Just map.
    F->glPixelStorei(pname, param);
}

extern "C"
void q_glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    // Just map.
    F->glTexParameteri(target, pname, param);
}

extern "C"
void q_glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    // Just map.
    F->glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
}

extern "C"
void q_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    // Just map.
    F->glDrawElements(mode, count, type, indices);
}

// -- OpenGL 2+ --

extern "C"
void q_glUseProgram(GLuint program)
{
    // Re-implement using the QOpenGLShaderProgram* table.

    // Search the program index that was created in setup_fragment_shader().
    int index;
    for (index = 0; index < fshader_count; index++) {
        assert(prog_tbl[index] != NULL);
        if (prog_tbl[index]->programId() == program)
            break;
    }
    assert(index != fshader_count);
    if (index == fshader_count)
        return;

    // Bind a program.
    prog_tbl[index]->bind();

    // Bind related VAO/VBO/IBO here. Thereby, subsequent
    // calls of glBindVertexArray() and glBindBuffer() are pure stubs
    vao_tbl[index]->bind();
    vbo_tbl[index]->bind();
    ibo_tbl[index]->bind();

    // Select the index for subsequent glBufferData().
    cur_program = index;
}

extern "C"
void q_glBindBuffer(GLenum target, GLuint buffer)
{
    // Already binded by q_glUseProgram().
}

extern "C"
void q_glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
    // Re-implement using the QOpenGLBuffer* table.

    // Assert q_glUseProgram() is already called.
    assert(cur_program != (GLuint)-1);

    // Ignore IBO selection. We do it in q_glUseProgram().
    if (target == GL_ELEMENT_ARRAY_BUFFER)
        return;

    // Do VBO write, we do it in q_glUseProgram().
    if (target == GL_ARRAY_BUFFER) {
        // Assert this function is called from draw_elements() and
        // we write 4 vertices in a triangle strip to VBO.
        assert(size == 6 * 4 * sizeof(GLfloat));
        assert(usage == GL_STATIC_DRAW);

        // Note: "usage" is already set by setUsagePattern().
        // Note: memory space is already allocated by allocate().

        // Write the vertex data.
        vbo_tbl[cur_program]->write(0, data, size);
    }
}

// -- OpenGL 3+ --

extern "C"
void q_glBindVertexArray(GLuint array)
{
    // Already binded by q_glUseProgram().
}
