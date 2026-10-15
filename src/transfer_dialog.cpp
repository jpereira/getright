// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-15
// Description: Compact transfer window inspired by the original GetRight dialog.

#include "main_window.h"
#include "ui_icons.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStyle>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <functional>

namespace {

QString duration(qint64 seconds)
{
    seconds = qMax(qint64(0), seconds);
    return QStringLiteral("%1:%2:%3").arg(seconds / 3600, 2, 10, QChar('0'))
        .arg(seconds / 60 % 60, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0'));
}

void setField(QLineEdit *field, const QString &value)
{
    if (field->text() != value) {
        field->setText(value);
        field->setToolTip(value);
        field->setCursorPosition(0);
    }
}

class SpeedGraph final : public QWidget
{
public:
    SpeedGraph(std::function<qint64()> speed, QWidget *parent) : QWidget(parent)
    {
        setMinimumHeight(65);
        setToolTip(tr("Recent transfer speed"));
        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this, speed]() {
            samples_.append(qMax(qint64(0), speed()));
            if (samples_.size() > 60)
                samples_.removeFirst();
            update();
        });
        timer->start(1000);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), palette().base());
        painter.setPen(palette().mid().color());
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
        for (int y = 1; y < 4; ++y)
            painter.drawLine(1, height() * y / 4, width() - 2, height() * y / 4);
        qint64 maximum = 1;
        for (qint64 speed : samples_)
            maximum = qMax(maximum, speed);
        QPainterPath path;
        for (int i = 0; i < samples_.size(); ++i) {
            const QPointF point(2 + (width() - 4.0) * i / 59,
                height() - 2 - (height() - 4.0) * samples_[i] / maximum);
            if (i == 0)
                path.moveTo(point);
            else
                path.lineTo(point);
        }
        painter.setPen(QPen(QColor("#55a85f"), 2));
        painter.drawPath(path);
    }

private:
    QList<qint64> samples_;
};

} // namespace

void MainWindow::showTransferDetails(int row)
{
    if (row < 0 || row >= model_->rowCount())
        return;
    transferDialogIndex_ = model_->index(row, NameColumn);
    if (!transferDialog_) {
        auto *dialog = new QDialog(this, Qt::Window);
        dialog->setObjectName("transferDialog");
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowIcon(windowIcon());
        transferDialog_ = dialog;
        auto *layout = new QVBoxLayout(dialog);
        layout->setContentsMargins(12, 10, 12, 10);
        layout->setSpacing(9);
        auto *form = new QFormLayout;
        transferForm_ = form;
        form->setHorizontalSpacing(9);
        form->setVerticalSpacing(7);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        transferUrl_ = new QLineEdit(dialog);
        transferPath_ = new QLineEdit(dialog);
        transferError_ = new QLineEdit(dialog);
        transferAmount_ = new QLabel(dialog);
        transferSpeed_ = new QLabel(dialog);
        transferElapsed_ = new QLabel(dialog);
        transferRemaining_ = new QLabel(dialog);
        for (QLineEdit *field : {transferUrl_, transferPath_, transferError_}) {
            field->setReadOnly(true);
            field->setMinimumWidth(0);
        }
        transferPath_->setFrame(false);
        const auto addIconRow = [&](const QString &icon, const QString &description,
                                    QWidget *field) {
            auto *label = new QLabel(dialog);
            label->setPixmap(transferIcon(icon).pixmap(18, 18));
            label->setToolTip(description);
            label->setAccessibleName(description);
            field->setAccessibleName(description);
            form->addRow(label, field);
        };
        addIconRow("address", tr("Address"), transferUrl_);
        addIconRow("path", tr("Local path"), transferPath_);
        addIconRow("amount", tr("Transferred"), transferAmount_);
        auto *timing = new QWidget(dialog);
        auto *timingLayout = new QHBoxLayout(timing);
        timingLayout->setContentsMargins(0, 0, 0, 0);
        timingLayout->setSpacing(5);
        timingLayout->addWidget(transferElapsed_);
        timingLayout->addWidget(transferSpeed_);
        timingLayout->addStretch();
        addIconRow("clock", tr("Elapsed time and speed"), timing);
        form->addRow(tr("Error:"), transferError_);
        layout->addLayout(form);
        transferProgress_ = new QProgressBar(dialog);
        transferProgress_->setRange(0, 100);
        transferProgress_->setTextVisible(false);
        transferProgress_->setFixedHeight(13);
        transferProgress_->setStyleSheet(QStringLiteral(
            "QProgressBar { border: 1px solid palette(mid); border-radius: 2px; "
            "background: palette(base); } QProgressBar::chunk { background: #55a85f; "
            "width: 5px; margin: 1px; }"));
        layout->addWidget(transferProgress_);
        transferRemaining_->hide();
        auto *graph = new SpeedGraph([this]() {
            return transferDialogIndex_.data(SpeedRole).toLongLong();
        }, dialog);
        graph->setVisible(settings_.value("showSpeedGraph").toBool(false));
        layout->addWidget(graph);
        auto *buttons = new QHBoxLayout;
        auto *logo = new QLabel(dialog);
        logo->setPixmap(windowIcon().pixmap(34, 34));
        buttons->addWidget(logo);
        buttons->addStretch();
        auto *options = new QPushButton(tr("Options"), dialog);
        auto *menu = new QMenu(options);
        auto *showGraph = menu->addAction(tr("Show speed graph"));
        showGraph->setCheckable(true);
        showGraph->setChecked(settings_.value("showSpeedGraph").toBool(false));
        connect(showGraph, &QAction::toggled, dialog, [this, graph, dialog](bool checked) {
            settings_.insert("showSpeedGraph", checked);
            saveSettings();
            graph->setVisible(checked);
            dialog->adjustSize();
        });
        menu->addAction(tr("Copy URL"), this, [this]() {
            QApplication::clipboard()->setText(transferDialogIndex_.data(UrlRole).toString());
        });
        menu->addAction(tr("Show in Finder"), this, [this]() {
            revealLocalFile(transferDialogIndex_.data(PathRole).toString());
        });
        menu->addAction(tr("Open Download Status"), this, &MainWindow::showMainWindow);
        options->setMenu(menu);
        transferPause_ = new QPushButton(dialog);
        auto *cancel = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton),
                                       tr("Cancel"), dialog);
        cancel->setToolTip(tr("Stop this transfer and close its window"));
        connect(cancel, &QPushButton::clicked, dialog, [this, dialog]() {
            if (transferDialogIndex_.isValid()) {
                downloads_->selectRow(transferDialogIndex_.row());
                stopSelected();
            }
            dialog->close();
        });
        connect(transferPause_, &QPushButton::clicked, this, [this]() {
            if (!transferDialogIndex_.isValid())
                return;
            downloads_->selectRow(transferDialogIndex_.row());
            const QString state = transferDialogIndex_.data(StateRole).toString();
            if (state == "Downloading" || state == "Connecting")
                pauseSelected();
            else if (state != "Complete")
                startSelected();
        });
        buttons->addWidget(options);
        buttons->addWidget(transferPause_);
        buttons->addWidget(cancel);
        layout->addLayout(buttons);
        connect(dialog, &QObject::destroyed, this, [this]() {
            transferDialogIndex_ = QPersistentModelIndex();
            transferForm_ = nullptr;
            transferUrl_ = nullptr;
            transferPath_ = nullptr;
            transferAmount_ = nullptr;
            transferError_ = nullptr;
            transferSpeed_ = nullptr;
            transferElapsed_ = nullptr;
            transferRemaining_ = nullptr;
            transferProgress_ = nullptr;
            transferPause_ = nullptr;
        });
        dialog->resize(470, 205);
    }
    updateTransferDialog();
    transferDialog_->show();
    transferDialog_->raise();
    transferDialog_->activateWindow();
}

void MainWindow::updateTransferDialog()
{
    if (!transferDialog_)
        return;
    const int row = transferDialogIndex_.row();
    if (row < 0 || row >= model_->rowCount()) {
        transferDialog_->close();
        return;
    }
    const auto *item = model_->item(row, NameColumn);
    const qint64 received = item->data(ReceivedRole).toLongLong();
    const qint64 total = item->data(TotalRole).toLongLong();
    const qint64 speed = item->data(SpeedRole).toLongLong();
    const QString state = item->data(StateRole).toString();
    const QDateTime started = item->data(StartedRole).toDateTime();
    const qint64 elapsed = item->data(ElapsedRole).toLongLong() +
        (started.isValid() ? started.secsTo(QDateTime::currentDateTime()) : 0);
    const int percent = state == "Complete" ? 100
        : total > 0 ? qBound(0, int(100.0 * received / total), 100) : 0;
    const QString status = model_->item(row, StatusColumn)->text();
    transferDialog_->setWindowTitle(tr("%1% - %2 — GetRight — %3")
        .arg(percent).arg(item->text(), status));
    setField(transferUrl_, item->data(UrlRole).toString());
    setField(transferPath_, item->data(PathRole).toString());
    const QString error = item->data(ErrorRole).toString();
    setField(transferError_, error);
    transferForm_->setRowVisible(transferError_, !error.isEmpty());
    transferAmount_->setText(total >= 0
        ? tr("%1 / %2 (%3%)").arg(QLocale().formattedDataSize(received),
                                 QLocale().formattedDataSize(total)).arg(percent)
        : QLocale().formattedDataSize(received));
    transferProgress_->setRange(0, total < 0 ? 0 : 100);
    transferProgress_->setValue(percent);
    transferElapsed_->setText(tr("Elapsed: %1").arg(duration(elapsed)));
    transferSpeed_->setText(tr("(%1/s)").arg(QLocale().formattedDataSize(speed)));
    transferRemaining_->setText(total > received && speed > 0
        ? tr("Time remaining: %1").arg(duration((total - received) / speed)) : status);
    transferAmount_->setToolTip(transferRemaining_->text());
    const bool active = state == "Downloading" || state == "Connecting";
    transferPause_->setText(active ? tr("Pause") : tr("Resume"));
    transferPause_->setIcon(style()->standardIcon(active ? QStyle::SP_MediaPause
                                                       : QStyle::SP_MediaPlay));
    transferPause_->setEnabled(state != "Complete");
}
