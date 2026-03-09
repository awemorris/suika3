/* -*- coding: utf-8; tab-width: 4; indent-tabs-mode: nil; -*- */

/*
 * Playfield Engine
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

#ifndef STRATOHAL_QTGAMEWIDGET_H
#define STRATOHAL_QTGAMEWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QAudioSink>
#include <QIODevice>

extern "C" {
#include "stratohal/platform.h"
};

class GameWidget : public QOpenGLWidget, public QOpenGLFunctions
{
    Q_OBJECT
public:
    // Singleton.
    static GameWidget *createSingleton(QString path);
    static void destroySingleton();
    static GameWidget *getSingleton();

    // Starts a game.
    void start();

signals:

public slots:
    // The frame timer handler.
    void onTimer();

protected:
    // Overrides.
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    //
    // Constructor
    //

    explicit GameWidget(QWidget *parent = nullptr);
    virtual ~GameWidget();

    static GameWidget *singleton;

    //
    // Status
    //

    // Whether the game is loaded.
    bool m_isGameStarted;

    // Whether the rendering subsystem initialized.
    bool m_isOpenGLInitialized;

    // Whether the first frame is going to be processed.
    bool m_isFirstFrame;

    //
    // Timer
    //

    // The rendering and sound-filling timer.
    QTimer *m_timer;

    //
    // Game Settings
    //
    QString m_gameTitle;
    int m_gameWidth;
    int m_gameHeight;

    //
    // OpenGL
    //

    // OpenGL rendering scale and offsets for mouse coordinate calculations.
    int m_viewportX;
    int m_viewportY;
    int m_viewportWidth;
    int m_viewportHeight;

    //
    // Mouse
    //

    // View mouse scale and origin.
    float m_mouseScale;
    int m_mouseLeft;
    int m_mouseTop;

public:
    //
    // Sound
    //

    // Is sound started?
    bool m_isSoundStarted;

    // The sound sinks.
    QAudioSink *m_soundSink[SOUND_TRACKS];

    // The sound devices.
    QIODevice *m_soundDevice[SOUND_TRACKS];

    // Wave streams.
    struct wave *m_wave[SOUND_TRACKS];

    // Whether waves stream are finished.
    bool m_waveFinish[SOUND_TRACKS];
};

#endif // QTGAMEWIDGET_H
