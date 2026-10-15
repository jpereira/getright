// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-15
// Description: Verify queue and selection behavior through the application window.

#include "main_window.h"
#include "transfer_job.h"
#include "completion_popup.h"

#include <QJsonArray>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QJsonDocument>
#include <QStandardItemModel>
#include <QTableView>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>

class WindowTests : public QObject
{
    Q_OBJECT

    std::unique_ptr<QTemporaryDir> directory_;
    QTcpServer server_;

    void seed(bool badDestination = false)
    {
        QJsonArray transfers;
        for (int i = 0; i < 3; ++i) {
            const QString name = QString("file%1").arg(i);
            const QString path = badDestination && i == 0 ? directory_->path()
                : directory_->filePath(name);
            transfers.append(QJsonObject{{"id", name}, {"name", name}, {"path", path},
                {"url", QString("http://127.0.0.1:%1/%2").arg(server_.serverPort()).arg(name)},
                {"state", "Paused"}, {"direction", "download"}});
        }
        QFile file(directory_->filePath("history.json"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QJsonDocument(QJsonObject{{"transfers", transfers}}).toJson());
    }

    void select(MainWindow &window, int last = 1)
    {
        window.downloads_->selectionModel()->select(
            QItemSelection(window.model_->index(0, 0), window.model_->index(last, 6)),
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    QString state(MainWindow &window, int row)
    {
        return window.model_->item(row, 0)->data(MainWindow::StateRole).toString();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(server_.listen(QHostAddress::LocalHost));
        connect(&server_, &QTcpServer::newConnection, this, [this]() {
            auto *socket = server_.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
                socket->readAll();
                socket->write("HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nNEW");
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        });
    }

    void init()
    {
        directory_ = std::make_unique<QTemporaryDir>();
        QVERIFY(directory_->isValid());
        qputenv("GETRIGHT_PORT_DATA_DIR", directory_->path().toUtf8());
        seed();
    }

    void startSelectedQueuesEverySelectedRow()
    {
        MainWindow window;
        select(window);
        window.startSelected();
        QTRY_COMPARE(state(window, 0), QString("Complete"));
        QTRY_COMPARE(state(window, 1), QString("Complete"));
        QCOMPARE(state(window, 2), QString("Paused"));
        QCOMPARE(window.downloads_->selectionModel()->selectedRows().size(), 2);
        QVERIFY(window.completionPopup_);
        QVERIFY(window.completionPopup_->testAttribute(Qt::WA_ShowWithoutActivating));
        QVERIFY(window.completionPopup_->windowFlags() & Qt::WindowDoesNotAcceptFocus);
        auto *popup = qobject_cast<CompletionPopup *>(window.completionPopup_.data());
        QVERIFY(popup);
        QSignalSpy open(popup, &CompletionPopup::openRequested);
        QVERIFY(open.isEmpty());
    }

    void pauseSelectedIncludesQueuedRows()
    {
        MainWindow window;
        select(window);
        window.startSelected();
        window.pauseSelected();
        QTRY_COMPARE(state(window, 0), QString("Paused"));
        QCOMPARE(state(window, 1), QString("Paused"));
        QVERIFY(!window.activeJob_);
    }

    void stopSelectedIncludesQueuedRows()
    {
        MainWindow window;
        select(window);
        window.startSelected();
        window.stopSelected();
        QTRY_COMPARE(state(window, 0), QString("Stopped"));
        QCOMPARE(state(window, 1), QString("Stopped"));
        QVERIFY(!window.activeJob_);
    }

    void removeSelectedWhileTransferring()
    {
        MainWindow window;
        select(window);
        window.startSelected();
        window.removeSelected();
        QTRY_COMPARE(window.model_->rowCount(), 1);
        QCOMPARE(window.model_->item(0, 0)->text(), QString("file2"));
        QVERIFY(!window.activeJob_);
    }

    void progressWindowControlsOnlyItsTransfer()
    {
        MainWindow window;
        window.showTransferDetails(1);
        QVERIFY(window.transferDialog_);
        QCOMPARE(window.transferPath_->text(), directory_->filePath("file1"));
        QCOMPARE(window.transferPause_->text(), QString("Resume"));
        window.transferPause_->click();
        QTRY_COMPARE(state(window, 1), QString("Complete"));
        QCOMPARE(state(window, 0), QString("Paused"));
        QCOMPARE(state(window, 2), QString("Paused"));
        QVERIFY(!window.transferPause_->isEnabled());
    }

    void corruptDataExplainsRecovery()
    {
        QFile history(directory_->filePath("history.json"));
        QVERIFY(history.open(QIODevice::WriteOnly));
        history.write("{broken");
        history.close();
        MainWindow window;
        QVERIFY(!window.historyWritable_);
        QVERIFY(!window.storageWarning_->isHidden());
        QVERIFY(window.storageWarning_->text().contains(".broken"));
    }

    void localFailureAdvancesQueue()
    {
        seed(true);
        MainWindow window;
        select(window);
        window.startSelected();
        QTRY_COMPARE(state(window, 0), QString("Failed"));
        QTRY_COMPARE(state(window, 1), QString("Complete"));
        QVERIFY(!window.model_->item(0, 0)->data(MainWindow::ErrorRole).toString().isEmpty());
        window.saveTransfers();
        QFile saved(directory_->filePath("history.json"));
        QVERIFY(saved.open(QIODevice::ReadOnly));
        const auto history = QJsonDocument::fromJson(saved.readAll()).object();
        QVERIFY(!history["transfers"].toArray()[0].toObject()["error"].toString().isEmpty());
    }
};

QTEST_MAIN(WindowTests)
#include "window.moc"
