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
    QApplication::setOrganizationName("Jorge Pereira");

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption themeOption(
        "theme", "Set the preview theme: system, light, or dark.", "theme");
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
