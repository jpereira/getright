// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-15
// Description: Capture real interface states during an isolated local HTTP download.

#include "main_window.h"

#include <QtWidgets>
#include <QtNetwork>

class ScreenshotCapture
{
public:
    static int run(int argc, char **argv)
    {
        QApplication app(argc, argv);
        app.setApplicationName("GetRight Port");
        app.setApplicationVersion(QStringLiteral(GETRIGHT_VERSION));
        QTemporaryDir data("/tmp/getright-capture-XXXXXX");
        qputenv("GETRIGHT_PORT_DATA_DIR", data.path().toUtf8());
        QDir().mkpath(data.filePath("Downloads"));
        const bool animation = argc == 4 && QByteArray(argv[3]) == "--animation";
        if (argc != 3 && !animation) {
            qCritical("Usage: capture-screenshots source.zip output-directory [--animation]");
            return 1;
        }
        const QString output = QString::fromLocal8Bit(argv[2]);
        if (!QDir().mkpath(output))
            return 1;
        QFile source(QString::fromLocal8Bit(argv[1]));
        if (!source.open(QIODevice::ReadOnly))
            return 1;
        const QByteArray archive = source.readAll();
        if (archive.isEmpty())
            return 1;
        QTcpServer server;
        if (!server.listen(QHostAddress::LocalHost))
            return 2;
        QObject::connect(&server, &QTcpServer::newConnection, [&]() {
            auto *socket = server.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket]() {
                const auto request = socket->readAll();
                if (!request.contains("\r\n\r\n") || socket->property("sent").toBool()) return;
                socket->setProperty("sent", true);
                socket->setProperty("offset", 0);
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/zip\r\n"
                              "Content-Length: " +
                              QByteArray::number(archive.size()) + "\r\n\r\n");
                auto *timer = new QTimer(socket);
                QObject::connect(timer, &QTimer::timeout, socket, [&, socket, timer]() {
                    const int offset = socket->property("offset").toInt();
                    if (offset >= archive.size()) {
                        timer->stop();
                        socket->disconnectFromHost();
                        return;
                    }
                    const auto chunk = archive.mid(offset, qMax(1, int(archive.size() / 160)));
                    socket->write(chunk);
                    socket->setProperty("offset", offset + chunk.size());
                });
                timer->start(50);
            });
        });
        QFile history(data.filePath("history.json"));
        if (!history.open(QIODevice::WriteOnly))
            return 3;
        history.write(QJsonDocument(QJsonObject{{"transfers", QJsonArray{
            QJsonObject{{"id", "site-capture"}, {"name", "getright-source.zip"},
              {"url", QString("http://127.0.0.1:%1/getright-source.zip").arg(server.serverPort())},
              {"path", data.filePath("Downloads/getright-source.zip")},
              {"direction", "download"}, {"state", "Queued"}}
        }}}).toJson());
        history.close();
        MainWindow window;
        window.applyLookAndFeel("macos");
        window.applyTheme("light");
        window.resize(980, 600);
        window.show();
        window.startSelected();
        const auto save = [&](const QString &name, QWidget *widget) {
            QImage shot = widget->grab().toImage();
            shot.setDevicePixelRatio(1);
            QPainter painter(&shot);
            painter.setPen(QPen(Qt::black, 1));
            painter.drawRect(0, 0, shot.width() - 1, shot.height() - 1);
            painter.end();
            if (!shot.save(QDir(output).filePath(name + ".png")))
                app.exit(5);
        };
        if (animation) {
            window.showTransferDetails(0);
            QTimer frames;
            int frame = 0;
            int finishedFrames = 0;
            QObject::connect(&frames, &QTimer::timeout, [&]() {
                if (!window.transferDialog_)
                    return;
                save(QStringLiteral("frame-%1").arg(frame++, 4, 10, QChar('0')),
                     window.transferDialog_);
                if (window.model_->item(0, 0)->data(MainWindow::StateRole) == "Complete" &&
                        ++finishedFrames >= 15)
                    app.exit(0);
            });
            frames.start(100);
            QTimer::singleShot(30000, [&]() { app.exit(8); });
            return app.exec();
        }
        QTimer::singleShot(1000, [&]() {
            save("port-light", &window);
            window.applyTheme("dark");
        });
        QTimer::singleShot(1600, [&]() {
            save("port-dark", &window);
            window.applyTheme("light");
            window.applyLookAndFeel("windowsxp");
            window.applyTheme("light");
        });
        QTimer::singleShot(2200, [&]() {
            save("port-classic", &window);
            window.applyLookAndFeel("macos");
            window.applyTheme("light");
            window.showTransferDetails(0);
        });
        QTimer::singleShot(2800, [&]() {
            save("port-progress", window.transferDialog_);
        });
        QTimer done;
        QObject::connect(&done, &QTimer::timeout, [&]() {
            if (window.completionPopup_) {
                save("port-finished", window.completionPopup_);
                app.exit(0);
            }
        });
        done.start(100);
        QTimer::singleShot(30000, [&]() { app.exit(8); });
        return app.exec();
    }
};

int main(int argc, char **argv)
{
    return ScreenshotCapture::run(argc, argv);
}
