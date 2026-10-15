// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-15
// Description: Exercise real HTTP transfers against a local scripted server.

#include "transfer_job.h"

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>
#ifdef Q_OS_UNIX
#include <csignal>
#include <sys/resource.h>
#endif

class TransferTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir directory_;
    QTcpServer server_;
    QByteArray response_;
    QByteArray requestBytes_;

    TransferJob::Request request(bool resume = true)
    {
        TransferJob::Request result;
        result.url = QUrl(QString("http://127.0.0.1:%1/file").arg(server_.serverPort()));
        result.path = directory_.filePath("download.bin");
        if (resume)
            result.etag = "\"version-one\"";
        return result;
    }

    void write(const QByteArray &data)
    {
        QFile file(request().path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(data), data.size());
    }

    QByteArray read()
    {
        QFile file(request().path);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return file.readAll();
    }

    QString run(const TransferJob::Request &spec)
    {
        TransferJob job;
        QSignalSpy done(&job, &TransferJob::finished);
        job.start(spec);
        if (done.isEmpty() && !done.wait(5000))
            return "Test timed out";
        return done.first().first().toString();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(directory_.isValid());
        QVERIFY(server_.listen(QHostAddress::LocalHost));
        connect(&server_, &QTcpServer::newConnection, this, [this]() {
            auto *socket = server_.nextPendingConnection();
            auto bytes = std::make_shared<QByteArray>();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket, bytes]() {
                *bytes += socket->readAll();
                if (!bytes->contains("\r\n\r\n"))
                    return;
                requestBytes_ = *bytes;
                socket->write(response_);
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        });
    }

    void init()
    {
        QFile::remove(request().path);
        requestBytes_.clear();
    }

    void errorsPreservePartial_data()
    {
        QTest::addColumn<QByteArray>("response");
        QTest::newRow("404") << QByteArray(
            "HTTP/1.1 404 Not Found\r\nContent-Length: 3\r\n\r\nERR");
        QTest::newRow("416") << QByteArray(
            "HTTP/1.1 416 Range Not Satisfiable\r\nContent-Length: 3\r\n\r\nERR");
        QTest::newRow("wrong-offset") << QByteArray(
            "HTTP/1.1 206 Partial Content\r\nContent-Range: bytes 0-2/3\r\n"
            "Content-Length: 3\r\n\r\nNEW");
        QTest::newRow("changed-etag") << QByteArray(
            "HTTP/1.1 206 Partial Content\r\nContent-Range: bytes 3-5/6\r\n"
            "ETag: \"version-two\"\r\nContent-Length: 3\r\n\r\nNEW");
        QTest::newRow("malformed-range") << QByteArray(
            "HTTP/1.1 206 Partial Content\r\nContent-Range: invalid\r\n"
            "Content-Length: 3\r\n\r\nNEW");
    }

    void errorsPreservePartial()
    {
        QFETCH(QByteArray, response);
        response_ = response;
        write("OLD");
        QVERIFY(!run(request()).isEmpty());
        QCOMPARE(read(), QByteArray("OLD"));
    }

    void validResume()
    {
        write("OLD");
        response_ = "HTTP/1.1 206 Partial Content\r\nContent-Range: bytes 3-5/6\r\n"
                    "ETag: \"version-one\"\r\nContent-Length: 3\r\n\r\nNEW";
        QCOMPARE(run(request()), QString());
        QCOMPARE(read(), QByteArray("OLDNEW"));
        QVERIFY(requestBytes_.contains("Range: bytes=3-"));
        QVERIFY(requestBytes_.contains("If-Range: \"version-one\""));
    }

    void rangeIgnoredRestarts()
    {
        write("OLD");
        response_ = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nNEW";
        QCOMPARE(run(request()), QString());
        QCOMPARE(read(), QByteArray("NEW"));
    }

    void legacyPartialRestartsWithoutRange()
    {
        write("OLD");
        response_ = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nNEW";
        QCOMPARE(run(request(false)), QString());
        QCOMPARE(read(), QByteArray("NEW"));
        QVERIFY(!requestBytes_.contains("Range:"));
    }

    void replacementPreservesFileOnError()
    {
        write("OLD");
        response_ = "HTTP/1.1 404 Not Found\r\nContent-Length: 3\r\n\r\nERR";
        auto spec = request();
        spec.restart = true;
        QVERIFY(!run(spec).isEmpty());
        QCOMPARE(read(), QByteArray("OLD"));
        QVERIFY(!requestBytes_.contains("Range:"));
    }

    void emptyFile()
    {
        response_ = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        QCOMPARE(run(request()), QString());
        QVERIFY(QFileInfo::exists(request().path));
        QCOMPARE(read(), QByteArray());
    }

    void incompleteFileFails()
    {
        response_ = "HTTP/1.1 200 OK\r\nContent-Length: 9\r\n\r\nshort";
        QVERIFY(!run(request()).isEmpty());
    }

    void localOpenFailure()
    {
        response_ = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nNEW";
        auto spec = request();
        spec.path = directory_.path();
        QVERIFY(!run(spec).isEmpty());
    }

    void writeFailure()
    {
#ifdef Q_OS_UNIX
        struct rlimit original;
        QVERIFY(getrlimit(RLIMIT_FSIZE, &original) == 0);
        auto restore = qScopeGuard([&]() { setrlimit(RLIMIT_FSIZE, &original); });
        auto oldSignal = std::signal(SIGXFSZ, SIG_IGN);
        auto restoreSignal = qScopeGuard([&]() { std::signal(SIGXFSZ, oldSignal); });
        struct rlimit limited = original;
        limited.rlim_cur = 2;
        QVERIFY(setrlimit(RLIMIT_FSIZE, &limited) == 0);
        response_ = "HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nNEWNEW";
        QVERIFY(!run(request()).isEmpty());
#else
        QSKIP("File size limits require Unix.");
#endif
    }
};

QTEST_GUILESS_MAIN(TransferTests)
#include "transfer_job.moc"
