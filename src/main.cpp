// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-14
// Description: Application startup and command-line options.

#include "main_window.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("GetRight Port");
    QApplication::setApplicationVersion(QStringLiteral(GETRIGHT_VERSION));
    QApplication::setOrganizationName("Jorge Pereira");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption themeOption(
        "theme", "Set and save the theme: system, light, or dark.", "theme");
    QCommandLineOption screenshotOption(
        "screenshot", "Save a window screenshot to the given path.",
        "path");
    QCommandLineOption styleOption(
        "look-and-feel",
        "Set the look and feel: macos, windows11, or windowsxp.", "style");
    QCommandLineOption progressWindowOption(
        "progress-window", "Show the floating transfer progress window.");
    parser.addOption(themeOption);
    parser.addOption(screenshotOption);
    parser.addOption(styleOption);
    parser.addOption(progressWindowOption);
    parser.process(app);

    MainWindow window;
    if (parser.isSet(themeOption))
        window.applyTheme(parser.value(themeOption));
    if (parser.isSet(styleOption))
        window.applyLookAndFeel(parser.value(styleOption));
    window.show();
    const auto bringForward = [&window]() { window.showMainWindow(); };
    QTimer::singleShot(0, &window, bringForward);
    if (!parser.isSet(screenshotOption)) {
        // Cocoa can finish launching after the first event loop turn.
        QTimer::singleShot(200, &window, [&window, bringForward]() {
            if (window.isVisible())
                bringForward();
        });
        QObject::connect(&app, &QGuiApplication::applicationStateChanged, &window,
                         [&window, bringForward](Qt::ApplicationState state) {
            if (state == Qt::ApplicationActive &&
                (!window.isVisible() || window.isMinimized()))
                bringForward();
        });
    }
    if (parser.isSet(progressWindowOption))
        QTimer::singleShot(100, &window, [&window]() {
            window.showTransferDetails(0);
        });

    if (parser.isSet(screenshotOption)) {
        const QString path = parser.value(screenshotOption);
        QDir().mkpath(QFileInfo(path).absolutePath());
        QTimer::singleShot(700, &window,
                           [&app, &window, path, &parser,
                            progressWindowOption]() {
            QWidget *target = parser.isSet(progressWindowOption)
                ? QApplication::activeWindow() : &window;
            (target ? target : &window)->grab().save(path, "PNG");
            app.quit();
        });
    }
    return app.exec();
}
