// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-14
// Description: HTTP response validation and transfer file handling.

#include "transfer_job.h"

#include <QFileInfo>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>

TransferJob::TransferJob(QObject *parent) : QObject(parent)
{
    speedTimer_.setInterval(500);
    connect(&speedTimer_, &QTimer::timeout, this, [this]() {
        if (!prepared_ && !request_.upload)
            return;
        const qint64 elapsed = qMax(qint64(1), clock_.restart());
        speed_ = (received_ - offset_ - sampleBytes_) * 1000 / elapsed;
        sampleBytes_ = received_ - offset_;
        emit progress(received_, expected_, speed_);
    });
}

TransferJob::~TransferJob()
{
    if (reply_) {
        disconnect(reply_, nullptr, this, nullptr);
        reply_->abort();
    }
}

void TransferJob::start(const Request &request)
{
    request_ = request;
    file_.setFileName(request.path);
    QNetworkRequest http(request.url);
    http.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                      QNetworkRequest::NoLessSafeRedirectPolicy);
    if (request.upload) {
        if (!file_.open(QIODevice::ReadOnly)) {
            error_ = file_.errorString();
            QTimer::singleShot(0, this, &TransferJob::finish);
            return;
        }
        reply_ = network_.put(http, &file_);
    } else {
        // Weak ETags cannot identify a resumable representation.
        const QByteArray validator = !request.etag.isEmpty()
                && !request.etag.startsWith("W/")
            ? request.etag : request.lastModified;
        if (!request.restart && !validator.isEmpty())
            offset_ = qMax(qint64(0), QFileInfo(request.path).size());
        if (offset_ > 0) {
            http.setRawHeader("Range", "bytes=" + QByteArray::number(offset_) + "-");
            http.setRawHeader("If-Range", validator);
        }
        http.setRawHeader("Accept-Encoding", "identity");
        reply_ = network_.get(http);
        connect(reply_, &QIODevice::readyRead, this, &TransferJob::receive);
    }
    clock_.start();
    speedTimer_.start();
    connect(reply_, &QNetworkReply::finished, this, &TransferJob::finish);
    if (request.upload) {
        connect(reply_, &QNetworkReply::uploadProgress, this,
                [this](qint64 received, qint64 total) {
            if (cancelled_ || !error_.isEmpty())
                return;
            received_ = received;
            expected_ = total;
            emit progress(received_, expected_, speed_);
        });
    }
}

bool TransferJob::prepareResponse()
{
    if (prepared_)
        return error_.isEmpty();
    const int status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // Redirect bodies and HTTP error pages never belong in the destination.
    if (status >= 300 && status < 400)
        return false;
    if (status != 200 && status != 206) {
        fail(tr("HTTP request failed (%1): %2").arg(status).arg(reply_->errorString()));
        return false;
    }
    const QByteArray etag = reply_->rawHeader("ETag");
    const QByteArray modified = reply_->rawHeader("Last-Modified");
    qint64 start = 0;
    expected_ = -1;
    if (status == 206) {
        const auto match = QRegularExpression(
            QStringLiteral("^bytes ([0-9]+)-([0-9]+)/([0-9]+)$"))
            .match(QString::fromLatin1(reply_->rawHeader("Content-Range")));
        bool startOk = false, endOk = false, totalOk = false;
        start = match.captured(1).toLongLong(&startOk);
        const qint64 end = match.captured(2).toLongLong(&endOk);
        expected_ = match.captured(3).toLongLong(&totalOk);
        const bool strongTag = !request_.etag.isEmpty() && !request_.etag.startsWith("W/");
        const bool changed = strongTag ? (!etag.isEmpty() && etag != request_.etag)
            : (!modified.isEmpty() && modified != request_.lastModified);
        if (offset_ <= 0 || !startOk || !endOk || !totalOk || start != offset_
                || end < start || end != expected_ - 1 || changed) {
            fail(tr("The server returned an invalid or changed download range."));
            return false;
        }
    } else {
        bool ok = false;
        const qint64 length = reply_->rawHeader("Content-Length").toLongLong(&ok);
        expected_ = ok && length >= 0 ? length : -1;
    }
    if (!file_.open(QIODevice::ReadWrite | QIODevice::Unbuffered)) {
        fail(tr("Cannot save file: %1").arg(file_.errorString()));
        return false;
    }
    if ((start > 0 && file_.size() != start) || !file_.resize(start) || !file_.seek(start)) {
        fail(tr("Cannot prepare destination: %1").arg(file_.errorString()));
        return false;
    }
    offset_ = start;
    received_ = start;
    prepared_ = true;
    emit validators(etag, modified);
    emit progress(received_, expected_, 0);
    return true;
}

void TransferJob::receive()
{
    if (!reply_ || request_.upload || cancelled_ || !error_.isEmpty()
            || !reply_->isReadable() || !prepareResponse())
        return;
    while (reply_->bytesAvailable() > 0) {
        const QByteArray data = reply_->read(256 * 1024);
        if (data.isEmpty())
            break;
        if (expected_ >= 0 && data.size() > expected_ - received_) {
            fail(tr("The server sent more data than the declared file size."));
            return;
        }
        const qint64 written = file_.write(data);
        if (written != data.size()) {
            if (written > 0)
                received_ += written;
            fail(tr("Cannot write download: %1").arg(file_.errorString()));
            return;
        }
        received_ += written;
    }
    emit progress(received_, expected_, speed_);
}

void TransferJob::fail(const QString &error)
{
    if (error_.isEmpty())
        error_ = error;
    // Defer abort so finished cannot reenter the file preparation or write path.
    QTimer::singleShot(0, this, [this]() {
        if (reply_ && !reply_->isFinished())
            reply_->abort();
    });
}

bool TransferJob::flush()
{
    if (request_.upload || !file_.isOpen())
        return true;
    if (file_.flush())
        return true;
    fail(tr("Cannot flush download: %1").arg(file_.errorString()));
    return false;
}

void TransferJob::cancel()
{
    cancelled_ = true;
    if (reply_)
        reply_->abort();
}

void TransferJob::finish()
{
    speedTimer_.stop();
    if (reply_ && !request_.upload && !cancelled_ && error_.isEmpty()) {
        receive();
        if (!prepared_ && reply_->error() == QNetworkReply::NoError)
            prepareResponse();
    }
    flush();
    if (reply_ && !cancelled_ && error_.isEmpty()) {
        const int status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply_->error() != QNetworkReply::NoError)
            error_ = reply_->errorString();
        else if (status < 200 || status >= 300)
            error_ = tr("HTTP request failed (%1).").arg(status);
        else if (!request_.upload && expected_ >= 0 && received_ != expected_)
            error_ = tr("The downloaded file is incomplete.");
    }
    file_.close();
    if (reply_) {
        reply_->deleteLater();
        reply_ = nullptr;
    }
    emit finished(error_);
}
