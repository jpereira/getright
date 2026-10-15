// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-14
// Description: HTTP transfers with validated resume and checked local writes.

#pragma once

#include <QElapsedTimer>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>

class TransferJob final : public QObject
{
    Q_OBJECT

public:
    struct Request {
        QUrl url;
        QString path;
        bool upload = false;
        bool restart = false;
        QByteArray etag;
        QByteArray lastModified;
    };

    explicit TransferJob(QObject *parent = nullptr);
    ~TransferJob() override;
    void start(const Request &request);
    void cancel();
    bool flush();

signals:
    void progress(qint64 received, qint64 total, qint64 speed);
    void validators(const QByteArray &etag, const QByteArray &lastModified);
    void finished(const QString &error);

private:
    bool prepareResponse();
    void receive();
    void fail(const QString &error);
    void finish();

    QNetworkAccessManager network_;
    QNetworkReply *reply_ = nullptr;
    QFile file_;
    Request request_;
    QString error_;
    qint64 offset_ = 0;
    qint64 received_ = 0;
    qint64 expected_ = -1;
    qint64 sampleBytes_ = 0;
    qint64 speed_ = 0;
    QElapsedTimer clock_;
    QTimer speedTimer_;
    bool prepared_ = false;
    bool cancelled_ = false;
};
