// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-15
// Description: Non-activating download completion tip near the tray icon.

#pragma once

#include <QFrame>
#include <QTimer>

class CompletionPopup final : public QFrame
{
    Q_OBJECT

public:
    CompletionPopup(const QString &name, qint64 elapsed, qint64 bytes,
                    const QIcon &icon, const QRect &anchor, QWidget *parent = nullptr);

signals:
    void openRequested();
    void revealRequested();
    void showMainRequested();

private:
    QTimer dismiss_;
};
