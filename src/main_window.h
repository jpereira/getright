// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-14
// Description: Main window and transfer state declarations.

#pragma once

#include <QFile>
#include <QMainWindow>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPersistentModelIndex>
#include <QPointer>

class QDialog;
class QCloseEvent;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QStandardItemModel;
class QTableView;
class QTreeWidget;
class QTimer;
class QSystemTrayIcon;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void applyTheme(const QString &theme);
    void applyLookAndFeel(const QString &lookAndFeel);
    void showTransferDetails(int row);
    void showMainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    enum Column { NameColumn, SizeColumn, StatusColumn, ProgressColumn,
                  SpeedColumn, RemainingColumn, UrlColumn };
    enum Role { UrlRole = Qt::UserRole + 1, PathRole, StateRole,
                ReceivedRole, TotalRole, RemoveWhenStoppedRole,
                DirectionRole, AddedRole, CompletedRole, SpeedRole,
                StartedRole, FolderRole, ElapsedRole, IdRole, RestartRole, AutoStartRole };

    void addDownload();
    void addUpload();
    void startSelected();
    void startNext();
    void pauseSelected();
    void pauseAll();
    void resumeAll();
    void stopSelected();
    void removeSelected();
    void showTransferMenu(const QPoint &position);
    void moveSelected();
    void downloadAgainSelected();
    bool moveTransfer(const QPersistentModelIndex &index, const QString &destination);
    void updateProgress(qint64 received, qint64 total);
    void receiveData();
    void finishDownload();
    void setRowState(int row, const QString &state);
    int selectedRow() const;
    void updateSummary();
    void applyFilters();
    void updateDetails();
    void updateTransferDialog();
    void loadTransfers();
    void saveTransfers();
    void saveSettings();
    void markTransfersDirty();
    void ensureFolderFilter(const QString &folder);
    void showHelp();
    void showAbout();

    QNetworkAccessManager network_;
    QLineEdit *filter_;
    QTreeWidget *folders_;
    QTableView *downloads_;
    QStandardItemModel *model_;
    QLabel *summary_;
    QLabel *urlStatus_;
    QLabel *detailName_;
    QLineEdit *detailUrl_;
    QLineEdit *detailPath_;
    QLabel *detailTransfer_;
    QLabel *detailSpeed_;
    QLabel *detailElapsed_;
    QLabel *detailRemaining_;
    QLabel *detailPercent_;
    QProgressBar *detailProgress_;
    QTimer *detailsTimer_;
    QTimer *persistenceTimer_ = nullptr;
    QSystemTrayIcon *trayIcon_ = nullptr;
    QPointer<QDialog> transferDialog_;
    QLabel *transferName_ = nullptr;
    QLineEdit *transferUrl_ = nullptr;
    QLineEdit *transferPath_ = nullptr;
    QLabel *transferAmount_ = nullptr;
    QLabel *transferSpeed_ = nullptr;
    QLabel *transferElapsed_ = nullptr;
    QLabel *transferRemaining_ = nullptr;
    QLabel *transferPercent_ = nullptr;
    QProgressBar *transferProgress_ = nullptr;
    QPushButton *transferPause_ = nullptr;
    QPersistentModelIndex transferDialogIndex_;
    QPersistentModelIndex movingIndex_;
    QString moveDestination_;
    bool resumeAfterMove_ = false;
    QString transfersFile_;
    QString settingsFile_;
    QJsonObject settings_;
    bool settingsWritable_ = true;
    bool historyWritable_ = true;
    QNetworkReply *activeReply_ = nullptr;
    QFile output_;
    QFile uploadInput_;
    int activeRow_ = -1;
    qint64 resumeOffset_ = 0;
    bool responsePrepared_ = false;
    bool stopping_ = false;
    bool pausing_ = false;
    bool transfersDirty_ = false;
    QString theme_ = "system";
    QString lookAndFeel_ = "macos";
};
