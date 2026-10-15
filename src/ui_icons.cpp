// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-15
// Description: Painted classic toolbar and transfer icons.

#include "ui_icons.h"

#include <QPainter>
#include <QPixmap>
#include <QPolygon>

QIcon transferIcon(const QString &kind)
{
    QPixmap pixmap(64, 64);
    pixmap.setDevicePixelRatio(2);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#555555"), 1.2));

    if (kind == "clock") {
        painter.setPen(QPen(QColor("#386b9e"), 2));
        painter.setBrush(QColor("#f2f7fc"));
        painter.drawEllipse(QRect(6, 7, 21, 21));
        painter.drawLine(13, 3, 20, 3);
        painter.drawLine(16, 3, 16, 7);
        painter.drawLine(16, 10, 16, 18);
        painter.drawLine(16, 18, 22, 18);
    } else if (kind == "path") {
        painter.setBrush(QColor("#e6c163"));
        painter.drawRoundedRect(QRect(3, 11, 26, 16), 2, 2);
        painter.setBrush(QColor("#f7d982"));
        painter.drawRect(QRect(4, 8, 11, 5));
        painter.drawRoundedRect(QRect(4, 13, 25, 14), 2, 2);
    } else if (kind == "amount") {
        painter.setBrush(QColor("#c2a187"));
        painter.drawRoundedRect(QRect(4, 18, 24, 9), 2, 2);
        painter.setBrush(QColor("#55a85f"));
        painter.drawPolygon(QPolygon{{12, 4}, {20, 4}, {20, 13}, {25, 13},
                                    {16, 23}, {7, 13}, {12, 13}});
    } else if (kind == "address" || kind == "complete") {
        painter.drawPixmap(QRect(1, 1, 30, 30),
                           QIcon(":/getright-logo.png").pixmap(60, 60));
        if (kind == "complete") {
            painter.setPen(QPen(QColor("#247b35"), 4, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(16, 23, 21, 28);
            painter.drawLine(21, 28, 29, 17);
        }
    } else if (kind == "start") {
        painter.setBrush(QColor("#39a84a"));
        QPolygon triangle{{9, 5}, {25, 16}, {9, 27}};
        painter.drawPolygon(triangle);
    } else if (kind == "stop") {
        painter.setBrush(QColor("#d94136"));
        painter.drawRoundedRect(QRect(7, 7, 18, 18), 2, 2);
    } else if (kind == "pause") {
        painter.setBrush(QColor("#377ac5"));
        painter.drawRect(QRect(8, 6, 6, 20));
        painter.drawRect(QRect(19, 6, 6, 20));
    } else if (kind == "remove") {
        painter.setPen(QPen(QColor("#c7362d"), 4, Qt::SolidLine,
                            Qt::RoundCap));
        painter.drawLine(QPoint(9, 9), QPoint(23, 23));
        painter.drawLine(QPoint(23, 9), QPoint(9, 23));
    } else if (kind == "add") {
        painter.setPen(QPen(QColor("#376fae"), 2));
        painter.setBrush(QColor("#eaf2fb"));
        painter.drawRoundedRect(QRect(6, 5, 20, 22), 2, 2);
        painter.setPen(QPen(QColor("#32964b"), 3, Qt::SolidLine,
                            Qt::RoundCap));
        painter.drawLine(QPoint(16, 10), QPoint(16, 21));
        painter.drawLine(QPoint(11, 16), QPoint(21, 16));
    } else if (kind == "upload") {
        painter.setPen(QPen(QColor("#376fae"), 2));
        painter.setBrush(QColor("#eaf2fb"));
        painter.drawRoundedRect(QRect(6, 6, 20, 20), 2, 2);
        painter.setPen(QPen(QColor("#d58b20"), 3, Qt::SolidLine,
                            Qt::RoundCap));
        painter.drawLine(QPoint(16, 22), QPoint(16, 11));
        painter.drawLine(QPoint(11, 16), QPoint(16, 11));
        painter.drawLine(QPoint(21, 16), QPoint(16, 11));
    } else if (kind == "help") {
        painter.setBrush(QColor("#397bc0"));
        painter.drawEllipse(QRect(5, 5, 22, 22));
        painter.setPen(QPen(Qt::white, 2));
        painter.setFont(QFont("Arial", 16, QFont::Bold));
        painter.drawText(QRect(5, 5, 22, 22), Qt::AlignCenter, "?");
    } else {
        painter.setBrush(QColor("#2679bb"));
        painter.drawEllipse(QRect(3, 3, 26, 26));
        painter.setPen(QPen(Qt::white, 1.4));
        painter.drawEllipse(QRect(10, 3, 12, 26));
        painter.drawLine(QPoint(4, 12), QPoint(28, 12));
        painter.drawLine(QPoint(4, 20), QPoint(28, 20));
        painter.setPen(QPen(QColor("#ffd044"), 3, Qt::SolidLine,
                            Qt::RoundCap));
        painter.drawLine(QPoint(16, 8), QPoint(16, 23));
        painter.drawLine(QPoint(11, 18), QPoint(16, 23));
        painter.drawLine(QPoint(21, 18), QPoint(16, 23));
    }
    return QIcon(pixmap);
}

