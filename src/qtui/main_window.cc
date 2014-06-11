/*
 * main_window.cc
 * Copyright 2014 Michał Lipski
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <QtGui>

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/playlist.h>

#include "main_window.h"
#include "main_window.moc"
#include "playlist.h"
#include "utils.h"

MainWindow::MainWindow (QMainWindow * parent) : QMainWindow (parent)
{
    QIcon::setThemeName ("QtUi");

    QString appDir = qApp->applicationDirPath ();
    QStringList paths = QIcon::themeSearchPaths ();
    paths.prepend (appDir + "/../share/audacious");
    QIcon::setThemeSearchPaths (paths);

    setupUi (this);

    setUnifiedTitleAndToolBarOnMac (true);

    slider = new QSlider (Qt::Horizontal);
    slider->setDisabled (true);

    timeCounterLabel = new QLabel ("0:00 / 0:00");
    timeCounterLabel->setContentsMargins (5, 0, 0, 2);
    timeCounterLabel->setDisabled (true);

    timeCounter = new QTimer;
    timeCounter->setInterval (250);

    toolBar->insertWidget (actionRepeat, slider);
    toolBar->insertWidget (actionRepeat, timeCounterLabel);

    populatePlaylists ();

    connect (actionOpen,      &QAction::triggered, Utils::openFilesDialog);
    connect (actionAdd,       &QAction::triggered, Utils::addFilesDialog);
    connect (actionPlayPause, &QAction::triggered, aud_drct_play_pause);
    connect (actionStop,      &QAction::triggered, aud_drct_stop);
    connect (actionPrevious,  &QAction::triggered, aud_drct_pl_prev);
    connect (actionNext,      &QAction::triggered, aud_drct_pl_next);

    connect (timeCounter, &QTimer::timeout,         this, &MainWindow::timeCounterSlot);
    connect (slider,      &QSlider::valueChanged,   this, &MainWindow::sliderValueChanged);
    connect (slider,      &QSlider::sliderPressed,  this, &MainWindow::sliderPressed);
    connect (slider,      &QSlider::sliderReleased, this, &MainWindow::sliderReleased);

    hook_associate ("title change",     (HookFunction) title_change_cb, this);
    hook_associate ("playback begin",   (HookFunction) playback_begin_cb, this);
    hook_associate ("playback ready",   (HookFunction) playback_ready_cb, this);
    hook_associate ("playback pause",   (HookFunction) pause_cb, this);
    hook_associate ("playback unpause", (HookFunction) pause_cb, this);
    hook_associate ("playback stop",    (HookFunction) playback_stop_cb, this);

    if (aud_drct_get_playing ())
    {
        playback_begin_cb (NULL, this);
        if (aud_drct_get_ready ())
            playback_ready_cb (NULL, this);
    }
    else
        playback_stop_cb (NULL, this);

    title_change_cb (NULL, this);
}

MainWindow::~MainWindow ()
{
    hook_dissociate ("title change",     (HookFunction) title_change_cb);
    hook_dissociate ("playback begin",   (HookFunction) playback_begin_cb);
    hook_dissociate ("playback ready",   (HookFunction) playback_ready_cb);
    hook_dissociate ("playback pause",   (HookFunction) pause_cb);
    hook_dissociate ("playback unpause", (HookFunction) pause_cb);
    hook_dissociate ("playback stop",    (HookFunction) playback_stop_cb);

    delete slider;
    delete timeCounterLabel;
    delete timeCounter;

    // TODO: cleanup playlists
}

void MainWindow::timeCounterSlot ()
{
    if (slider->isSliderDown ())
        return;

    int time   = aud_drct_get_time ();
    int length = aud_drct_get_length ();
    setTimeCounterLabel (time, length);

    slider->setValue (time);
}

void MainWindow::setTimeCounterLabel (int time, int length)
{
    QString text = QString (str_format_time (time)) + QString (" / ") + QString (str_format_time (length));
    timeCounterLabel->setText (text);
}

void MainWindow::enableSlider ()
{
    int time = aud_drct_get_time ();
    int length = aud_drct_get_length ();

    slider->setRange (0, length);
    slider->setValue (time);
    slider->setDisabled (false);
}

void MainWindow::disableSlider ()
{
    slider->setRange (0, 0);
    slider->setDisabled (true);
}

void MainWindow::enableTimeCounter ()
{
    int time   = aud_drct_get_time ();
    int length = aud_drct_get_length ();

    setTimeCounterLabel (time, length);
    timeCounter->start ();
    timeCounterLabel->setDisabled (false);
}

void MainWindow::disableTimeCounter ()
{
    timeCounter->stop ();
    timeCounterLabel->setText ("0:00 / 0:00");
    timeCounterLabel->setDisabled (true);
}

void MainWindow::sliderValueChanged (int value)
{
    if (! slider->isSliderDown ())
        return;

    int length = aud_drct_get_length ();
    setTimeCounterLabel (value, length);
}

void MainWindow::sliderPressed ()
{
    timeCounter->stop ();
}

void MainWindow::sliderReleased ()
{
    aud_drct_seek (slider->value ());
    timeCounter->start ();
}

void MainWindow::populatePlaylists ()
{
    int playlists = aud_playlist_count ();

    for (int count = 0; count < playlists; count++)
    {
        auto playlistWidget = new Playlist (0, aud_playlist_get_unique_id (count));
        tabWidget->addTab ((QWidget *) playlistWidget, QString (aud_playlist_get_title (count)));
    }
}
