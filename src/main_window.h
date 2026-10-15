// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-14
// Description: Main window and transfer state declarations.

#pragma once

#include <QFile>
#include <QMainWindow>
#include <QJsonObject>
#include <QPersistentModelIndex>
#include <QPointer>

class QAction;
class TransferJob;
class QDialog;
class QFormLayout;
class QCloseEvent;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QStandardItemModel;
class QTableView;
class QTreeWidget;
class QTreeWidgetItem;
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
    friend class WindowTests;

    enum Column { NameColumn, SizeColumn, StatusColumn, ProgressColumn,
                  SpeedColumn, RemainingColumn, UrlColumn, OrderColumn };
    enum Role { UrlRole = Qt::UserRole + 1, PathRole, StateRole,
                ReceivedRole, TotalRole, RemoveWhenStoppedRole,
                DirectionRole, AddedRole, CompletedRole, SpeedRole,
                StartedRole, FolderRole, ElapsedRole, IdRole, RestartRole, AutoStartRole,
                EtagRole, ModifiedRole, ErrorRole };

    void addDownload();
    void addUpload();
    void startSelected();
    void startNext();
    void startRow(int row);
    void pauseSelected();
    void pauseAll();
    void resumeAll();
    void stopSelected();
    void removeSelected();
    void showTransferMenu(const QPoint &position);
    void moveSelected();
    void downloadAgainSelected();
    bool moveTransfer(const QPersistentModelIndex &index, const QString &destination);
    void updateProgress(qint64 received, qint64 total, qint64 speed);
    void finishDownload(const QString &error);
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
    QTreeWidgetItem *filterItem(const QString &key) const;
    void showHelp();
    void showDataFolder();
    void openLocalFile(const QString &path);
    void revealLocalFile(const QString &path);
    void showCompletion(int row);
    void showStorageWarning(const QString &message);
    void showAbout();

    QLineEdit *filter_;
    QTreeWidget *folders_;
    QTableView *downloads_;
    QStandardItemModel *model_;
    QAction *startAction_ = nullptr;
    QAction *pauseAction_ = nullptr;
    QAction *stopAction_ = nullptr;
    QAction *removeAction_ = nullptr;
    QLabel *summary_;
    QLabel *storageWarning_;
    QLabel *urlStatus_;
    QLabel *detailName_;
    QLineEdit *detailUrl_;
    QLineEdit *detailPath_;
    QLabel *detailTransfer_;
    QLineEdit *detailError_;
    QLabel *detailSpeed_;
    QLabel *detailElapsed_;
    QLabel *detailRemaining_;
    QLabel *detailPercent_;
    QProgressBar *detailProgress_;
    QTimer *detailsTimer_;
    QTimer *persistenceTimer_ = nullptr;
    QSystemTrayIcon *trayIcon_ = nullptr;
    QPointer<QDialog> transferDialog_;
    QPointer<QWidget> completionPopup_;
    QFormLayout *transferForm_ = nullptr;
    QLineEdit *transferUrl_ = nullptr;
    QLineEdit *transferPath_ = nullptr;
    QLabel *transferAmount_ = nullptr;
    QLineEdit *transferError_ = nullptr;
    QLabel *transferSpeed_ = nullptr;
    QLabel *transferElapsed_ = nullptr;
    QLabel *transferRemaining_ = nullptr;
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
    TransferJob *activeJob_ = nullptr;
    int activeRow_ = -1;
    bool stopping_ = false;
    bool pausing_ = false;
    bool transfersDirty_ = false;
    QString theme_ = "system";
    QString lookAndFeel_ = "macos";
};
