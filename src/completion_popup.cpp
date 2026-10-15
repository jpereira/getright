// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-15
// Description: Classic completion tip with file and folder actions.

#include "completion_popup.h"

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QScreen>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

CompletionPopup::CompletionPopup(const QString &name, qint64 elapsed, qint64 bytes,
                               const QIcon &icon, const QRect &anchor, QWidget *parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                     Qt::WindowDoesNotAcceptFocus)
{
    setObjectName("completionPopup");
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose);
    setFrameStyle(QFrame::Box | QFrame::Plain);
    QFont tipFont = font();
    tipFont.setPointSize(10);
    setFont(tipFont);
    setStyleSheet(QStringLiteral(
        "QToolButton { border: none; background: transparent; padding: 0; }"));
    setAutoFillBackground(true);
    QPalette colors = palette();
    const bool dark = colors.color(QPalette::Window).lightness() < 128;
    colors.setColor(QPalette::Window, QColor(dark ? "#333326" : "#ffffe1"));
    colors.setColor(QPalette::WindowText, QColor(dark ? "#ffffe1" : "#171717"));
    colors.setColor(QPalette::ButtonText, colors.color(QPalette::WindowText));
    colors.setColor(QPalette::Link, QColor(dark ? "#9bc6ff" : "#0055d4"));
    setPalette(colors);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(9, 6, 9, 8);
    layout->setSpacing(5);
    auto *heading = new QHBoxLayout;
    auto *title = new QLabel(tr("GetRight Finished a Download"), this);
    QFont bold = title->font();
    bold.setBold(true);
    title->setFont(bold);
    heading->addWidget(title, 1);
    auto *close = new QToolButton(this);
    close->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    close->setToolTip(tr("Dismiss notification"));
    close->setAutoRaise(true);
    close->setFixedSize(14, 14);
    connect(close, &QToolButton::clicked, this, &QWidget::close);
    heading->addWidget(close);
    layout->addLayout(heading);

    auto *body = new QHBoxLayout;
    auto *image = new QToolButton(this);
    image->setIcon(icon);
    image->setIconSize(QSize(32, 32));
    image->setAutoRaise(true);
    image->setToolTip(tr("Open downloaded file"));
    connect(image, &QToolButton::clicked, this, &CompletionPopup::openRequested);
    body->addWidget(image);
    const QString duration = QStringLiteral("%1:%2:%3").arg(elapsed / 3600, 2, 10, QChar('0'))
        .arg(elapsed / 60 % 60, 2, 10, QChar('0')).arg(elapsed % 60, 2, 10, QChar('0'));
    const QString speed = QLocale().formattedDataSize(bytes / qMax(qint64(1), elapsed));
    auto *text = new QLabel(tr("File: %1\nTime: %2\nSpeed: %3/s").arg(name, duration, speed), this);
    text->setTextFormat(Qt::PlainText);
    text->setWordWrap(true);
    text->setMaximumWidth(280);
    body->addWidget(text, 1);
    layout->addLayout(body);
    auto *actions = new QHBoxLayout;
    auto *open = new QLabel(tr("<a href=\"open\">Click here to open this file.</a>"), this);
    open->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    connect(open, &QLabel::linkActivated, this, &CompletionPopup::openRequested);
    actions->addWidget(open, 1);
    auto *folder = new QToolButton(this);
    folder->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    folder->setAutoRaise(true);
    folder->setToolTip(tr("Show downloaded file in its folder"));
    connect(folder, &QToolButton::clicked, this, &CompletionPopup::revealRequested);
    actions->addWidget(folder);
    layout->addLayout(actions);

    dismiss_.setSingleShot(true);
    connect(&dismiss_, &QTimer::timeout, this, &QWidget::close);
    dismiss_.start(15000);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &point) {
        dismiss_.stop();
        auto *menu = new QMenu(this);
        auto *keep = menu->addAction(tr("Keep visible"));
        keep->setCheckable(true);
        keep->setChecked(keepVisible_);
        connect(keep, &QAction::toggled, this, [this](bool checked) {
            keepVisible_ = checked;
        });
        menu->addAction(tr("Open GetRight"), this, &CompletionPopup::showMainRequested);
        connect(menu, &QMenu::aboutToHide, this, [this, menu]() {
            if (!keepVisible_)
                dismiss_.start(15000);
            menu->deleteLater();
        });
        menu->popup(mapToGlobal(point));
    });
    adjustSize();
    QScreen *screen = QGuiApplication::screenAt(anchor.center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (screen) {
        const QRect available = screen->availableGeometry();
        int x = anchor.isValid() ? anchor.right() - width() : available.right() - width() - 12;
        int y = available.top() + 8;
        if (anchor.isValid())
            y = anchor.center().y() < available.center().y()
                ? anchor.bottom() + 8 : anchor.top() - height() - 8;
        x = qBound(available.left(), x, qMax(available.left(), available.right() - width()));
        y = qBound(available.top(), y, qMax(available.top(), available.bottom() - height()));
        move(x, y);
    }
}
