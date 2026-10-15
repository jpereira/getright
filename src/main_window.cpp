// SPDX-License-Identifier: LGPL-3.0-only
// Copyright (C) 2026 Jorge Pereira
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-14
// Description: Transfer management, persistence, and user interface.

#include "main_window.h"
#include "transfer_job.h"
#include "completion_popup.h"
#include "ui_icons.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDateTime>
#include <QCloseEvent>
#include <QClipboard>
#include <QCursor>
#include <QDialog>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QProgressBar>
#include <QProcess>
#include <QStyleOptionProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTime>
#include <QStandardItemModel>
#include <QSplitter>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QSystemTrayIcon>
#include <QTableView>
#include <QToolBar>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>

#ifdef Q_OS_MACOS
void activateMacApplication();
#endif

namespace {

QIcon standardIcon(QWidget *widget, QStyle::StandardPixmap pixmap)
{
    return widget->style()->standardIcon(pixmap);
}

void setDetailText(QLineEdit *field, const QString &text)
{
    if (field->text() == text)
        return;
    field->setText(text);
    field->setToolTip(text);
    field->setCursorPosition(0);
}

} // namespace

void MainWindow::applyTheme(const QString &theme)
{
    theme_ = theme == "light" || theme == "dark" ? theme : "system";
    settings_.insert("theme", theme_);
    saveSettings();
    qApp->setStyleSheet({});
    QApplication::setPalette(QPalette());
    const Qt::ColorScheme scheme = theme_ == "light" ? Qt::ColorScheme::Light
        : theme_ == "dark" ? Qt::ColorScheme::Dark : Qt::ColorScheme::Unknown;
    QApplication::styleHints()->setColorScheme(scheme);
    applyLookAndFeel(lookAndFeel_);
    if (lookAndFeel_ == "macos" || theme_ == "system")
        return;

    QPalette palette;
    const auto setAllGroups = [&palette](QPalette::ColorRole role,
                                         const QColor &color) {
        palette.setColor(QPalette::Active, role, color);
        palette.setColor(QPalette::Inactive, role, color);
        palette.setColor(QPalette::Disabled, role, color);
    };
    if (theme_ == "dark") {
        setAllGroups(QPalette::Window, QColor("#25272b"));
        setAllGroups(QPalette::WindowText, QColor("#eeeeee"));
        setAllGroups(QPalette::Base, QColor("#181a1d"));
        setAllGroups(QPalette::AlternateBase, QColor("#22252a"));
        setAllGroups(QPalette::Text, QColor("#eeeeee"));
        setAllGroups(QPalette::Button, QColor("#35383d"));
        setAllGroups(QPalette::ButtonText, QColor("#f1f1f1"));
        setAllGroups(QPalette::Highlight, QColor("#346da5"));
        setAllGroups(QPalette::HighlightedText, Qt::white);
        setAllGroups(QPalette::ToolTipBase, QColor("#fff4c4"));
        setAllGroups(QPalette::ToolTipText, QColor("#222222"));
        setAllGroups(QPalette::PlaceholderText, QColor("#a0a4aa"));
    } else {
        setAllGroups(QPalette::Window, QColor("#eeeeee"));
        setAllGroups(QPalette::WindowText, QColor("#202020"));
        setAllGroups(QPalette::Base, Qt::white);
        setAllGroups(QPalette::AlternateBase, QColor("#f5f7fa"));
        setAllGroups(QPalette::Text, QColor("#202020"));
        setAllGroups(QPalette::Button, QColor("#e5e8ec"));
        setAllGroups(QPalette::ButtonText, QColor("#202020"));
        setAllGroups(QPalette::Highlight, QColor("#3b78b5"));
        setAllGroups(QPalette::HighlightedText, Qt::white);
        setAllGroups(QPalette::ToolTipBase, QColor("#fff4c4"));
        setAllGroups(QPalette::ToolTipText, QColor("#202020"));
        setAllGroups(QPalette::PlaceholderText, QColor("#777777"));
    }
    QApplication::setPalette(palette);
}

void MainWindow::applyLookAndFeel(const QString &lookAndFeel)
{
    lookAndFeel_ = lookAndFeel == "windows11" || lookAndFeel == "windowsxp"
                       ? lookAndFeel : "macos";
    settings_.insert("lookAndFeel", lookAndFeel_);
    saveSettings();

    const QString styleName = lookAndFeel_ == "macos" ? "macOS" : "Fusion";
    if (QApplication::style()->objectName().compare(
            styleName, Qt::CaseInsensitive) != 0) {
        if (QStyle *style = QStyleFactory::create(styleName))
            QApplication::setStyle(style);
    }

    const bool darkPalette = theme_ == "dark" ||
        (theme_ == "system" &&
         QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    const QString buttonText = darkPalette ? "#f1f1f1" : "#202124";
    const QString buttonFace = darkPalette ? "#383b41" : "#f3f4f6";
    const QString buttonBorder = darkPalette ? "#676c75" : "#c7c9cc";
    const QString hoverFace = darkPalette ? "#484d56" : "#e5effb";
    const QString pressedFace = darkPalette ? "#2b2e33" : "#d5e5f7";

    if (lookAndFeel_ == "windows11") {
        qApp->setStyleSheet(QStringLiteral(
            "QPushButton, QToolButton { color: %1; background: %2; "
            "border: 1px solid %3; border-radius: 5px; padding: 5px 9px; }"
            "QPushButton:hover, QToolButton:hover { background: %4; "
            "border-color: #78aee8; }"
            "QPushButton:pressed, QToolButton:pressed { background: %5; }")
                .arg(buttonText, buttonFace, buttonBorder, hoverFace,
                     pressedFace));
    } else if (lookAndFeel_ == "windowsxp") {
        const QString xpTop = darkPalette ? "#56647a" : "#ffffff";
        const QString xpBottom = darkPalette ? "#303a4a" : "#dce8f7";
        const QString xpHover = darkPalette ? "#665b38" : "#fff4c2";
        const QString xpPressed = darkPalette ? "#243247" : "#c9dcf2";
        qApp->setStyleSheet(QStringLiteral(
            "QPushButton, QToolButton { color: %1; "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "stop:0 %2, stop:1 %3); border: 1px solid %4; "
            "border-radius: 3px; padding: 4px 8px; }"
            "QPushButton:hover, QToolButton:hover { background: %5; "
            "border-color: #e6a700; }"
            "QPushButton:pressed, QToolButton:pressed { background: %6; }")
                .arg(darkPalette ? "#f1f1f1" : "#10274c", xpTop, xpBottom,
                     darkPalette ? "#8292aa" : "#7f9db9", xpHover,
                     xpPressed));
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      filter_(new QLineEdit(this)),
      folders_(new QTreeWidget(this)),
      downloads_(new QTableView(this)),
      model_(new QStandardItemModel(0, 8, this)),
      summary_(new QLabel(this)),
      storageWarning_(new QLabel(this)),
      urlStatus_(new QLabel(this)),
      detailName_(new QLabel(this)),
      detailUrl_(new QLineEdit(this)),
      detailPath_(new QLineEdit(this)),
      detailTransfer_(new QLabel(this)),
      detailError_(new QLineEdit(this)),
      detailSpeed_(new QLabel(this)),
      detailElapsed_(new QLabel(this)),
      detailRemaining_(new QLabel(this)),
      detailPercent_(new QLabel(this)),
      detailProgress_(new QProgressBar(this)),
      detailsTimer_(new QTimer(this))
{
    const QString dataDirectory = qEnvironmentVariable(
        "GETRIGHT_PORT_DATA_DIR", QDir::home().filePath(".getright"));
    settingsFile_ = QDir(dataDirectory).filePath("settings.json");
    transfersFile_ = QDir(dataDirectory).filePath("history.json");
    QFile settingsFile(settingsFile_);
    if (settingsFile.exists()) {
        if (!settingsFile.open(QIODevice::ReadOnly)) {
            settingsWritable_ = false;
        } else {
            QJsonParseError error;
            const auto document = QJsonDocument::fromJson(settingsFile.readAll(), &error);
            settingsWritable_ = error.error == QJsonParseError::NoError && document.isObject();
            if (settingsWritable_)
                settings_ = document.object();
        }
        if (!settingsWritable_)
            qWarning("Cannot read settings; preserving %s", qPrintable(settingsFile_));
    }
    theme_ = settings_.value("theme").toString("system");
    lookAndFeel_ = settings_.value("lookAndFeel").toString("macos");
    applyTheme(theme_);
    setWindowTitle(tr("GetRight Port — Download Status"));
    setWindowIcon(QIcon(QStringLiteral(":/getright-logo.png")));
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon_ = new QSystemTrayIcon(windowIcon(), this);
        trayIcon_->setToolTip(tr("GetRight Port — Download: 0 bytes/s"));
        auto *trayMenu = new QMenu(this);
        trayMenu->setObjectName("trayMenu");
        trayMenu->addAction(tr("Open"), this, &MainWindow::showMainWindow);
        trayMenu->addAction(tr("Pause All"), this, &MainWindow::pauseAll);
        trayMenu->addAction(tr("Resume All"), this, &MainWindow::resumeAll);
        trayMenu->addSeparator();
        trayMenu->addAction(tr("Close"), qApp, &QCoreApplication::quit);
        connect(trayIcon_, &QSystemTrayIcon::activated, this,
                [this, trayMenu](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Unknown || reason == QSystemTrayIcon::DoubleClick)
                return;
            if (reason == QSystemTrayIcon::Context) {
                trayMenu->popup(QCursor::pos());
                return;
            }
            if (isVisible() && !isMinimized())
                close();
            else
                showMainWindow();
        });
        trayIcon_->show();
        QApplication::setQuitOnLastWindowClosed(false);
    }
    resize(980, 600);
    restoreGeometry(QByteArray::fromBase64(
        settings_.value("windowGeometry").toString().toLatin1()));

    model_->setHorizontalHeaderLabels(
        {tr("Local File"), tr("Size"), tr("Status"), tr("Progress"),
         tr("Speed"), tr("Est. Time"), tr("Address"), tr("Order")});
    downloads_->setModel(model_);
    downloads_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(downloads_, &QWidget::customContextMenuRequested,
            this, &MainWindow::showTransferMenu);
    downloads_->setSelectionBehavior(QAbstractItemView::SelectRows);
    downloads_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    downloads_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    downloads_->setAlternatingRowColors(false);
    downloads_->setShowGrid(false);
    downloads_->verticalHeader()->hide();
    downloads_->verticalHeader()->setDefaultSectionSize(23);
    downloads_->horizontalHeader()->setStretchLastSection(false);
    downloads_->horizontalHeader()->setSectionResizeMode(
        NameColumn, QHeaderView::Stretch);
    downloads_->horizontalHeader()->setSectionResizeMode(
        UrlColumn, QHeaderView::Stretch);
    downloads_->setColumnWidth(SizeColumn, 100);
    downloads_->setColumnWidth(StatusColumn, 120);
    downloads_->setColumnWidth(ProgressColumn, 150);
    downloads_->setColumnWidth(SpeedColumn, 95);
    downloads_->setColumnWidth(RemainingColumn, 95);
    downloads_->setColumnHidden(UrlColumn, true);
    downloads_->setColumnWidth(OrderColumn, 48);
    auto *header = downloads_->horizontalHeader();
    header->moveSection(header->visualIndex(OrderColumn), 0);
    header->moveSection(header->visualIndex(ProgressColumn), 2);
    header->moveSection(header->visualIndex(SizeColumn), 3);
    header->moveSection(header->visualIndex(RemainingColumn), 4);
    header->moveSection(header->visualIndex(SpeedColumn), 5);
    downloads_->setSortingEnabled(false);

    folders_->setHeaderHidden(true);
    folders_->setMinimumWidth(180);
    auto addFilter = [this](QTreeWidgetItem *parent, const QString &label,
                            const QString &key) {
        auto *item = parent ? new QTreeWidgetItem(parent, {label})
                            : new QTreeWidgetItem(folders_, {label});
        item->setData(0, Qt::UserRole, key);
        return item;
    };
    auto *downloadsRoot = addFilter(nullptr, tr("Downloads"), "downloads");
    downloadsRoot->setIcon(0, standardIcon(this, QStyle::SP_DirOpenIcon));
    addFilter(downloadsRoot, tr("All Downloads"), "downloads_all");
    addFilter(downloadsRoot, tr("Active"), "downloads_active");
    addFilter(downloadsRoot, tr("Stopped / Queued"), "downloads_stopped");
    addFilter(downloadsRoot, tr("Completed"), "downloads_complete");
    auto *foldersRoot = addFilter(downloadsRoot, tr("Folders"), "folders");
    foldersRoot->setIcon(0, standardIcon(this, QStyle::SP_DirIcon));
    addFilter(foldersRoot, tr("Downloads"), "folder_downloads");
    addFilter(foldersRoot, tr("Unfiled"), "folder_unfiled");
    auto *filtersRoot = addFilter(downloadsRoot, tr("Filters"), "filters");
    filtersRoot->setIcon(0, standardIcon(this, QStyle::SP_FileDialogContentsView));
    addFilter(filtersRoot, tr("Applications"), "type_applications");
    addFilter(filtersRoot, tr("Archives"), "type_archives");
    addFilter(filtersRoot, tr("Images"), "type_images");
    addFilter(filtersRoot, tr("Music"), "type_music");
    addFilter(filtersRoot, tr("Videos"), "type_videos");
    auto *uploadsRoot = addFilter(nullptr, tr("Uploads"), "uploads_all");
    uploadsRoot->setIcon(0, standardIcon(this, QStyle::SP_ArrowUp));
    addFilter(uploadsRoot, tr("Queued / Active"), "uploads_active");
    addFilter(uploadsRoot, tr("Completed"), "uploads_complete");
    auto *historyRoot = addFilter(nullptr, tr("History"), "history_all");
    historyRoot->setIcon(0, standardIcon(this, QStyle::SP_FileDialogDetailedView));
    addFilter(historyRoot, tr("Today"), "history_today");
    addFilter(historyRoot, tr("Yesterday"), "history_yesterday");
    addFilter(historyRoot, tr("Past week"), "history_week");
    addFilter(historyRoot, tr("Past month"), "history_month");
    folders_->insertTopLevelItem(1, folders_->takeTopLevelItem(2));
    folders_->expandAll();
    folders_->setCurrentItem(downloadsRoot->child(0));

    filter_->setPlaceholderText(tr("Filter downloads…"));
    filter_->setClearButtonEnabled(true);
    auto *sidebar = new QWidget(this);
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->addWidget(folders_, 1);

    auto *content = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(3, 0, 0, 0);
    contentLayout->setSpacing(4);
    storageWarning_->setWordWrap(true);
    storageWarning_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    storageWarning_->setVisible(!storageWarning_->text().isEmpty());
    contentLayout->addWidget(storageWarning_);
    contentLayout->addWidget(filter_);
    contentLayout->addWidget(downloads_, 1);
    auto *details = new QGroupBox(tr("Transfer details"), content);
    auto *detailLayout = new QFormLayout(details);
    detailLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    detailName_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    for (QLineEdit *field : {detailUrl_, detailPath_, detailError_}) {
        field->setReadOnly(true);
        field->setMinimumWidth(0);
    }
    detailUrl_->setObjectName("downloadAddress");
    detailPath_->setObjectName("downloadLocalPath");
    detailProgress_->setRange(0, 100);
    detailProgress_->setValue(0);
    detailPercent_->setMinimumWidth(48);
    auto *progressLine = new QWidget(details);
    auto *progressLayout = new QHBoxLayout(progressLine);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->addWidget(detailProgress_, 1);
    progressLayout->addWidget(detailPercent_);
    detailLayout->addRow(tr("File"), detailName_);
    detailLayout->addRow(tr("Address"), detailUrl_);
    detailLayout->addRow(tr("Local path"), detailPath_);
    detailLayout->addRow(tr("Transferred"), detailTransfer_);
    detailLayout->addRow(tr("Error"), detailError_);
    detailLayout->addRow(tr("Progress"), progressLine);
    auto *stats = new QWidget(details);
    auto *statsLayout = new QHBoxLayout(stats);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->addWidget(new QLabel(tr("Speed:"), stats));
    statsLayout->addWidget(detailSpeed_);
    statsLayout->addSpacing(20);
    statsLayout->addWidget(new QLabel(tr("Elapsed:"), stats));
    statsLayout->addWidget(detailElapsed_);
    statsLayout->addSpacing(20);
    statsLayout->addWidget(new QLabel(tr("Remaining:"), stats));
    statsLayout->addWidget(detailRemaining_);
    statsLayout->addStretch(1);
    detailLayout->addRow(stats);
    contentLayout->addWidget(details);
    details->setVisible(settings_.value("showDetailsPanel").toBool(false));
    auto *splitter = new QSplitter(this);
    splitter->addWidget(sidebar);
    splitter->addWidget(content);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({190, 790});
    setCentralWidget(splitter);

    auto *toolbar = addToolBar(tr("Downloads"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(settings_.value("showToolbarLabels").toBool(false)
        ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
    toolbar->setIconSize(QSize(24, 24));
    QAction *add = toolbar->addAction(
        transferIcon("add"), tr("Add"),
        this, &MainWindow::addDownload);
    add->setShortcut(QKeySequence::New);
    QAction *upload = toolbar->addAction(
        transferIcon("upload"), tr("Upload"), this, &MainWindow::addUpload);
    toolbar->addSeparator();
    QAction *start = toolbar->addAction(
        transferIcon("start"), tr("Start"),
        this, &MainWindow::startSelected);
    start->setShortcut(Qt::Key_F5);
    QAction *pause = toolbar->addAction(
        transferIcon("pause"), tr("Pause"),
        this, &MainWindow::pauseSelected);
    QAction *stop = toolbar->addAction(
        transferIcon("stop"), tr("Stop"),
        this, &MainWindow::stopSelected);
    stop->setShortcut(Qt::Key_Escape);
    QAction *remove = toolbar->addAction(
        transferIcon("remove"), tr("Remove"),
        this, &MainWindow::removeSelected);
    remove->setShortcut(QKeySequence::Delete);
    toolbar->addSeparator();
    toolbar->addAction(transferIcon("help"),
                       tr("Help"), this, &MainWindow::showHelp);
    toolbar->addAction(windowIcon(), tr("About"), this, &MainWindow::showAbout);

    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(add);
    fileMenu->addAction(upload);
    fileMenu->addAction(tr("Open Data Folder"), this, &MainWindow::showDataFolder);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), QKeySequence::Quit, qApp,
                        &QCoreApplication::quit);
    auto *downloadsMenu = menuBar()->addMenu(tr("&Downloads"));
    downloadsMenu->addAction(start);
    downloadsMenu->addAction(pause);
    downloadsMenu->addAction(stop);
    downloadsMenu->addAction(remove);
    auto *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(tr("Copy URL"), QKeySequence::Copy, this, [this]() {
        QStringList urls;
        for (const auto &index : downloads_->selectionModel()->selectedRows())
            urls.append(model_->item(index.row(), NameColumn)->data(UrlRole).toString());
        QApplication::clipboard()->setText(urls.join('\n'));
    });
    editMenu->addAction(tr("Select All"), QKeySequence::SelectAll,
                        downloads_, &QTableView::selectAll);
    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    auto *showDetails = viewMenu->addAction(tr("Show transfer details"));
    showDetails->setCheckable(true);
    showDetails->setChecked(settings_.value("showDetailsPanel").toBool(false));
    connect(showDetails, &QAction::toggled, this, [this, details](bool checked) {
        details->setVisible(checked);
        settings_.insert("showDetailsPanel", checked);
        saveSettings();
    });
    auto *showLabels = viewMenu->addAction(tr("Show toolbar labels"));
    showLabels->setCheckable(true);
    showLabels->setChecked(settings_.value("showToolbarLabels").toBool(false));
    connect(showLabels, &QAction::toggled, this, [this, toolbar](bool checked) {
        toolbar->setToolButtonStyle(checked ? Qt::ToolButtonTextBesideIcon
                                           : Qt::ToolButtonIconOnly);
        settings_.insert("showToolbarLabels", checked);
        saveSettings();
    });
    viewMenu->addAction(tr("Show toolbar"), toolbar, &QToolBar::show);
    viewMenu->addAction(tr("Hide toolbar"), toolbar, &QToolBar::hide);
    auto *themeMenu = viewMenu->addMenu(tr("Theme"));
    auto *themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);
    const QList<QPair<QString, QString>> themes = {
        {tr("System"), "system"}, {tr("Light"), "light"},
        {tr("Dark"), "dark"}};
    for (const auto &entry : themes) {
        auto *action = themeMenu->addAction(entry.first);
        action->setCheckable(true);
        action->setData(entry.second);
        action->setChecked(theme_ == entry.second);
        themeGroup->addAction(action);
    }
    connect(themeGroup, &QActionGroup::triggered, this,
            [this](QAction *action) { applyTheme(action->data().toString()); });
    auto *lookMenu = viewMenu->addMenu(tr("Look & Feel"));
    auto *lookGroup = new QActionGroup(this);
    lookGroup->setExclusive(true);
    const QList<QPair<QString, QString>> looks = {
        {tr("macOS (native)"), "macos"},
        {tr("Windows 11"), "windows11"},
        {tr("Windows XP"), "windowsxp"}};
    for (const auto &entry : looks) {
        auto *action = lookMenu->addAction(entry.first);
        action->setCheckable(true);
        action->setData(entry.second);
        action->setChecked(lookAndFeel_ == entry.second);
        lookGroup->addAction(action);
    }
    connect(lookGroup, &QActionGroup::triggered, this,
            [this](QAction *action) {
        applyLookAndFeel(action->data().toString());
        applyTheme(theme_);
    });
    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("Help"), QKeySequence::HelpContents, this,
                        &MainWindow::showHelp);
    helpMenu->addAction(tr("About GetRight Port"), this,
                        &MainWindow::showAbout);

    statusBar()->addWidget(summary_, 1);
    urlStatus_->setMaximumWidth(430);
    statusBar()->addPermanentWidget(urlStatus_);

    connect(filter_, &QLineEdit::textChanged, this,
            [this]() { applyFilters(); });
    connect(folders_, &QTreeWidget::currentItemChanged, this,
            [this]() { applyFilters(); });
    connect(downloads_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
        urlStatus_->setText(selectedRow() >= 0
            ? model_->item(selectedRow(), UrlColumn)->text() : QString());
        updateDetails();
    });
    connect(downloads_, &QTableView::clicked, this,
            [this](const QModelIndex &index) {
        if (index.isValid() && downloads_->selectionModel()->selectedRows().size() == 1 &&
                QApplication::keyboardModifiers() == Qt::NoModifier)
            showTransferDetails(index.row());
    });
    connect(detailsTimer_, &QTimer::timeout, this, &MainWindow::updateDetails);
    detailsTimer_->start(1000);
    persistenceTimer_ = new QTimer(this);
    connect(persistenceTimer_, &QTimer::timeout, this, [this]() {
        if (transfersDirty_)
            saveTransfers();
    });
    persistenceTimer_->start(2000);
    loadTransfers();
    if (!settingsWritable_ || !historyWritable_)
        showStorageWarning(tr("Some saved data could not be read. Saving is disabled for the "
            "affected JSON files. Use File > Open Data Folder. Quit the app, keep a backup, "
            "then repair the affected settings.json or history.json, or rename it to .broken "
            "and reopen the app to create a new file."));
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        settings_.insert("windowGeometry", QString::fromLatin1(saveGeometry().toBase64()));
        saveSettings();
        saveTransfers();
    });
    connect(QApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            [this]() {
        if (theme_ == "system")
            QTimer::singleShot(0, this, [this]() { applyTheme(theme_); });
    });
    updateSummary();
    updateDetails();
}

void MainWindow::applyFilters()
{
    const QString key = folders_->currentItem()
                            ? folders_->currentItem()->data(0, Qt::UserRole).toString()
                            : QStringLiteral("downloads_all");
    const QString needle = filter_->text().trimmed();
    const QDate today = QDate::currentDate();

    for (int row = 0; row < model_->rowCount(); ++row) {
        const auto *nameItem = model_->item(row, NameColumn);
        const QString state = nameItem->data(StateRole).toString();
        const QString direction = nameItem->data(DirectionRole).toString();
        const QString name = nameItem->text();
        const QString ext = QFileInfo(name).suffix().toLower();
        const QDate completed = nameItem->data(CompletedRole).toDateTime().date();
        bool visible = true;

        if (key.startsWith("uploads")) {
            visible = direction == "upload";
            if (key == "uploads_active")
                visible &= state != "Complete" && state != "Failed";
            else if (key == "uploads_complete")
                visible &= state == "Complete";
        } else if (key.startsWith("history")) {
            visible = state == "Complete";
            const int age = completed.isValid() ? completed.daysTo(today) : -1;
            if (key == "history_today")
                visible &= age == 0;
            else if (key == "history_yesterday")
                visible &= age == 1;
            else if (key == "history_week")
                visible &= age >= 0 && age < 7;
            else if (key == "history_month")
                visible &= age >= 0 && age < 30;
        } else if (key.startsWith("type_")) {
            visible = direction != "upload" && state != "Complete";
            if (key == "type_applications")
                visible &= QStringList{"app", "dmg", "exe", "pkg"}.contains(ext);
            else if (key == "type_archives")
                visible &= QStringList{"7z", "bz2", "gz", "rar", "tar", "zip"}
                               .contains(ext);
            else if (key == "type_images")
                visible &= QStringList{"gif", "jpeg", "jpg", "png", "tif", "webp"}
                               .contains(ext);
            else if (key == "type_music")
                visible &= QStringList{"aac", "flac", "m4a", "mp3", "ogg", "wav"}
                               .contains(ext);
            else if (key == "type_videos")
                visible &= QStringList{"avi", "mkv", "mov", "mp4", "webm"}
                               .contains(ext);
        } else if (key == "downloads_active") {
            visible = direction != "upload" &&
                      (state == "Downloading" || state == "Connecting");
        } else if (key == "downloads_stopped") {
            visible = direction != "upload" &&
                      (state == "Stopped" || state == "Paused" ||
                       state == "Queued" || state == "Failed");
        } else if (key == "downloads_complete") {
            visible = direction != "upload" && state == "Complete";
        } else if (key.startsWith("folder_name:")) {
            visible = direction != "upload" &&
                nameItem->data(FolderRole).toString() == key.mid(12);
        } else if (key.startsWith("folder_")) {
            visible = direction != "upload" &&
                      nameItem->data(FolderRole).toString() ==
                          (key == "folder_downloads" ? "Downloads" : "Unfiled");
        } else {
            visible = direction != "upload";
        }

        const bool matchesText = name.contains(needle, Qt::CaseInsensitive) ||
            model_->item(row, UrlColumn)->text().contains(
                needle, Qt::CaseInsensitive);
        downloads_->setRowHidden(row, !visible || !matchesText);
    }
}

void MainWindow::updateDetails()
{
    updateTransferDialog();
    int row = selectedRow();
    if (row < 0)
        row = activeRow_;
    if (row < 0 || row >= model_->rowCount()) {
        detailName_->setText(tr("No transfer selected"));
        setDetailText(detailUrl_, {});
        setDetailText(detailPath_, {});
        detailTransfer_->clear();
        setDetailText(detailError_, {});
        detailSpeed_->clear();
        detailElapsed_->clear();
        detailRemaining_->clear();
        detailPercent_->setText("0%");
        detailProgress_->setValue(0);
        return;
    }

    const auto *item = model_->item(row, NameColumn);
    const qint64 received = item->data(ReceivedRole).toLongLong();
    const qint64 total = item->data(TotalRole).toLongLong();
    const qint64 speed = item->data(SpeedRole).toLongLong();
    const QDateTime started = item->data(StartedRole).toDateTime();
    const qint64 elapsed = item->data(ElapsedRole).toLongLong() +
        (started.isValid() ? started.secsTo(QDateTime::currentDateTime()) : 0);
    const int percent = total > 0
                            ? qBound(0, int(100.0 * received / total), 100)
                            : 0;
    detailName_->setText(item->text());
    setDetailText(detailError_, item->data(ErrorRole).toString());
    setDetailText(detailUrl_, item->data(UrlRole).toString());
    setDetailText(detailPath_, item->data(PathRole).toString());
    detailTransfer_->setText(total > 0
        ? QStringLiteral("%1 of %2")
              .arg(QLocale().formattedDataSize(received),
                   QLocale().formattedDataSize(total))
        : QLocale().formattedDataSize(received));
    detailSpeed_->setText(QLocale().formattedDataSize(speed) + tr("/s"));
    detailElapsed_->setText(QTime(0, 0).addSecs(int(elapsed)).toString("hh:mm:ss"));
    detailRemaining_->setText(total > 0 && speed > 0
        ? QTime(0, 0).addSecs(int((total - received) / speed))
              .toString("hh:mm:ss")
        : tr("—"));
    detailPercent_->setText(total > 0 ? QStringLiteral("%1%").arg(percent)
                                      : tr("—"));
    detailProgress_->setValue(percent);
}

QTreeWidgetItem *MainWindow::filterItem(const QString &key) const
{
    for (QTreeWidgetItemIterator it(folders_); *it; ++it) {
        if ((*it)->data(0, Qt::UserRole).toString() == key)
            return *it;
    }
    return nullptr;
}

void MainWindow::ensureFolderFilter(const QString &folder)
{
    auto *root = filterItem("folders");
    for (int index = 0; index < root->childCount(); ++index) {
        if (root->child(index)->text(0) == folder)
            return;
    }
    auto *item = new QTreeWidgetItem(root, {folder});
    item->setData(0, Qt::UserRole,
                  QStringLiteral("folder_name:") + folder);
    item->setIcon(0, standardIcon(this, QStyle::SP_DirIcon));
}

void MainWindow::addDownload()
{
    bool accepted = false;
    const QString clipboardText = QApplication::clipboard()->text().trimmed();
    const QUrl clipboardUrl(clipboardText);
    const bool hasUrl = clipboardUrl.isValid() && !clipboardUrl.host().isEmpty() &&
        (clipboardUrl.scheme() == "http" || clipboardUrl.scheme() == "https");
    const QString text = QInputDialog::getText(
        this, tr("Add download"), tr("URL:"), QLineEdit::Normal,
        hasUrl ? clipboardText : QString(), &accepted).trimmed();
    if (!accepted || text.isEmpty())
        return;

    const QUrl address(text);
    if (!address.isValid() ||
        (address.scheme() != "http" && address.scheme() != "https") ||
        address.host().isEmpty()) {
        QMessageBox::warning(this, tr("Invalid URL"),
                             tr("Enter a valid HTTP or HTTPS address."));
        return;
    }

    const QString name = address.fileName().isEmpty()
                             ? QStringLiteral("download")
                             : address.fileName();
    const QString downloadDirectory = settings_.value("downloadDirectory").toString(
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save download"), QDir(downloadDirectory).filePath(name));
    if (path.isEmpty())
        return;
    settings_.insert("downloadDirectory", QFileInfo(path).absolutePath());
    saveSettings();

    QList<QStandardItem *> items;
    auto *fileItem = new QStandardItem(
        transferIcon("address"), QFileInfo(path).fileName());
    fileItem->setData(address.toString(), UrlRole);
    fileItem->setData(QUuid::createUuid().toString(QUuid::WithoutBraces),
                      IdRole);
    fileItem->setData(path, PathRole);
    fileItem->setData(true, RestartRole);
    fileItem->setData(QStringLiteral("Queued"), StateRole);
    fileItem->setData(QStringLiteral("download"), DirectionRole);
    const QString folder = QFileInfo(path).dir().dirName();
    fileItem->setData(folder, FolderRole);
    ensureFolderFilter(folder);
    fileItem->setData(QDateTime::currentDateTime(), AddedRole);
    fileItem->setData(qint64(0), ReceivedRole);
    fileItem->setData(qint64(-1), TotalRole);
    items << fileItem << new QStandardItem(tr("Unknown"))
          << new QStandardItem(tr("Queued")) << new QStandardItem()
          << new QStandardItem(tr("—")) << new QStandardItem(tr("—"))
          << new QStandardItem(address.toString());
    model_->appendRow(items);
    markTransfersDirty();
    folders_->setCurrentItem(filterItem("downloads"));
    downloads_->selectRow(model_->rowCount() - 1);
    updateSummary();
    applyFilters();
    statusBar()->showMessage(tr("Download added to the queue"), 3000);
    startNext();
}

void MainWindow::addUpload()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Choose a file to upload"));
    if (path.isEmpty())
        return;
    bool accepted = false;
    const QString text = QInputDialog::getText(
        this, tr("Add upload"), tr("HTTP or HTTPS destination URL:"),
        QLineEdit::Normal, QString(), &accepted).trimmed();
    if (!accepted || text.isEmpty())
        return;

    const QUrl address(text);
    if (!address.isValid() ||
        (address.scheme() != "http" && address.scheme() != "https") ||
        address.host().isEmpty()) {
        QMessageBox::warning(this, tr("Invalid URL"),
                             tr("Enter a valid HTTP or HTTPS destination."));
        return;
    }

    QFileInfo info(path);
    QList<QStandardItem *> items;
    auto *fileItem = new QStandardItem(transferIcon("upload"), info.fileName());
    fileItem->setData(address.toString(), UrlRole);
    fileItem->setData(QUuid::createUuid().toString(QUuid::WithoutBraces),
                      IdRole);
    fileItem->setData(path, PathRole);
    fileItem->setData(QStringLiteral("Queued"), StateRole);
    fileItem->setData(QStringLiteral("upload"), DirectionRole);
    fileItem->setData(QDateTime::currentDateTime(), AddedRole);
    fileItem->setData(qint64(0), ReceivedRole);
    fileItem->setData(info.size(), TotalRole);
    items << fileItem << new QStandardItem(QLocale().formattedDataSize(info.size()))
          << new QStandardItem(tr("Queued")) << new QStandardItem()
          << new QStandardItem(tr("—")) << new QStandardItem(tr("—"))
          << new QStandardItem(address.toString());
    model_->appendRow(items);
    markTransfersDirty();
    folders_->setCurrentItem(filterItem("uploads_active"));
    downloads_->selectRow(model_->rowCount() - 1);
    updateSummary();
    applyFilters();
    statusBar()->showMessage(tr("Upload queued. Select Start to send it."), 4000);
}

int MainWindow::selectedRow() const
{
    const QModelIndexList rows = downloads_->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

void MainWindow::showTransferMenu(const QPoint &position)
{
    const QModelIndex clicked = downloads_->indexAt(position);
    if (!clicked.isValid())
        return;
    const QPersistentModelIndex index = model_->index(clicked.row(), NameColumn);
    if (!downloads_->selectionModel()->isRowSelected(index.row(), QModelIndex()))
        downloads_->selectRow(index.row());
    const bool multiple = downloads_->selectionModel()->selectedRows().size() > 1;
    const auto *item = model_->item(index.row(), NameColumn);
    const QString state = item->data(StateRole).toString();
    const bool active = activeJob_ && index.row() == activeRow_;

    QMenu menu(this);
    const auto addAction = [&](const QString &text, auto operation, bool enabled = true) {
        QAction *action = menu.addAction(text);
        action->setEnabled(enabled);
        connect(action, &QAction::triggered, this, [this, index, operation]() {
            if (!index.isValid())
                return;
            (this->*operation)();
        });
    };
    addAction(tr("Start"), &MainWindow::startSelected,
              multiple || state != "Complete");
    addAction(tr("Pause"), &MainWindow::pauseSelected, multiple || active || state == "Queued");
    addAction(tr("Stop"), &MainWindow::stopSelected,
              multiple || active || state == "Queued" || state == "Paused" || state == "Failed");
    addAction(tr("Remove"), &MainWindow::removeSelected);
    addAction(tr("Download Again"), &MainWindow::downloadAgainSelected,
              !multiple && state == "Complete" &&
              item->data(DirectionRole).toString() == "download");
    menu.addSeparator();
    QAction *copy = menu.addAction(multiple ? tr("Copy URLs") : tr("Copy URL"));
    connect(copy, &QAction::triggered, this, [this]() {
        QStringList urls;
        for (const auto &selected : downloads_->selectionModel()->selectedRows())
            urls.append(model_->item(selected.row(), NameColumn)->data(UrlRole).toString());
        QApplication::clipboard()->setText(urls.join('\n'));
    });
    const QString path = item->data(PathRole).toString();
    const bool completedFile = !multiple && state == "Complete" && QFileInfo(path).isFile();
    QAction *open = menu.addAction(tr("Open File"));
    open->setEnabled(completedFile);
    connect(open, &QAction::triggered, this, [this, path]() {
        openLocalFile(path);
    });
    QAction *reveal = menu.addAction(tr("Show in Finder"));
    reveal->setEnabled(!multiple && QFileInfo::exists(path));
    connect(reveal, &QAction::triggered, this, [this, path]() {
        revealLocalFile(path);
    });
    addAction(tr("Move To"), &MainWindow::moveSelected,
              !multiple && item->data(DirectionRole).toString() == "download" &&
              !movingIndex_.isValid());
    menu.exec(downloads_->viewport()->mapToGlobal(position));
}

void MainWindow::moveSelected()
{
    const int row = selectedRow();
    if (row < 0 || movingIndex_.isValid())
        return;
    const QPersistentModelIndex index = model_->index(row, NameColumn);
    if (index.data(DirectionRole).toString() != "download")
        return;
    const QString source = index.data(PathRole).toString();
    const QString directory = QFileDialog::getExistingDirectory(
        this, tr("Move download to"), QFileInfo(source).absolutePath());
    if (directory.isEmpty() || !index.isValid())
        return;
    const QString destination = QDir(directory).filePath(QFileInfo(source).fileName());
    if (QFileInfo(destination).absoluteFilePath() == QFileInfo(source).absoluteFilePath())
        return;
    if (QFileInfo::exists(destination)) {
        QMessageBox::warning(this, tr("Cannot move download"),
                             tr("A file with this name already exists in the selected folder."));
        return;
    }

    if (activeJob_ && index.row() == activeRow_) {
        movingIndex_ = index;
        moveDestination_ = destination;
        resumeAfterMove_ = true;
        pausing_ = true;
        activeJob_->cancel();
    } else {
        moveTransfer(index, destination);
    }
}

bool MainWindow::moveTransfer(const QPersistentModelIndex &index, const QString &destination)
{
    if (!index.isValid())
        return false;
    const QString source = index.data(PathRole).toString();
    if (QFileInfo::exists(destination)) {
        QMessageBox::warning(this, tr("Cannot move download"),
                             tr("The destination already exists. Choose another folder."));
        return false;
    }
    QFile file(source);
    if (file.exists() && !file.rename(destination)) {
        QMessageBox::warning(this, tr("Cannot move download"), file.errorString());
        return false;
    }
    if (!file.exists() && index.data(StateRole).toString() == "Complete" &&
        !QFileInfo::exists(destination)) {
        QMessageBox::warning(this, tr("Cannot move download"),
                             tr("The downloaded file could not be found."));
        return false;
    }
    auto *item = model_->item(index.row(), NameColumn);
    item->setData(destination, PathRole);
    const QString folder = QFileInfo(destination).dir().dirName();
    item->setData(folder, FolderRole);
    ensureFolderFilter(folder);
    settings_.insert("downloadDirectory", QFileInfo(destination).absolutePath());
    saveSettings();
    markTransfersDirty();
    saveTransfers();
    applyFilters();
    updateDetails();
    statusBar()->showMessage(tr("Download destination updated"), 3000);
    return true;
}

void MainWindow::downloadAgainSelected()
{
    const int row = selectedRow();
    if (row < 0)
        return;
    auto *item = model_->item(row, NameColumn);
    if (item->data(DirectionRole).toString() != "download" ||
        item->data(StateRole).toString() != "Complete")
        return;
    item->setData(true, RestartRole);
    item->setData(qint64(0), ReceivedRole);
    item->setData(qint64(0), SpeedRole);
    item->setData(qint64(0), ElapsedRole);
    item->setData(QDateTime(), StartedRole);
    item->setData(QDateTime(), CompletedRole);
    model_->item(row, ProgressColumn)->setText("0%");
    model_->item(row, SpeedColumn)->setText(tr("—"));
    model_->item(row, RemainingColumn)->setText(tr("—"));
    setRowState(row, "Queued");
    saveTransfers();
    startNext();
}

void MainWindow::startSelected()
{
    const auto rows = downloads_->selectionModel()->selectedRows();
    for (const auto &index : rows) {
        auto *item = model_->item(index.row(), NameColumn);
        const QString state = item->data(StateRole).toString();
        if (state == "Complete" || index.row() == activeRow_)
            continue;
        item->setData(true, AutoStartRole);
        setRowState(index.row(), "Queued");
    }
    startNext();
}

void MainWindow::startRow(int row)
{
    const QString state = model_->item(row, NameColumn)->data(StateRole).toString();
    if (state == "Complete" || state == "Downloading")
        return;
    if (activeJob_)
        return;

    activeRow_ = row;
    auto *item = model_->item(row, NameColumn);
    TransferJob::Request request;
    request.url = QUrl(item->data(UrlRole).toString());
    request.path = item->data(PathRole).toString();
    request.upload = item->data(DirectionRole).toString() == "upload";
    request.restart = item->data(RestartRole).toBool();
    request.etag = item->data(EtagRole).toByteArray();
    request.lastModified = item->data(ModifiedRole).toByteArray();
    item->setData(false, AutoStartRole);
    item->setData(QString(), ErrorRole);
    item->setData(QDateTime::currentDateTime(), StartedRole);
    item->setData(qint64(0), SpeedRole);
    stopping_ = false;
    pausing_ = false;
    activeJob_ = new TransferJob(this);
    connect(activeJob_, &TransferJob::progress, this, &MainWindow::updateProgress);
    connect(activeJob_, &TransferJob::validators, this,
            [this](const QByteArray &etag, const QByteArray &modified) {
        auto *transfer = model_->item(activeRow_, NameColumn);
        transfer->setData(etag, EtagRole);
        transfer->setData(modified, ModifiedRole);
        transfer->setData(false, RestartRole);
        saveTransfers();
    });
    connect(activeJob_, &TransferJob::finished, this, &MainWindow::finishDownload);
    setRowState(row, "Connecting");
    activeJob_->start(request);
}

void MainWindow::startNext()
{
    if (activeJob_)
        return;
    for (int row = 0; row < model_->rowCount(); ++row) {
        const QString state = model_->item(row, NameColumn)
                                  ->data(StateRole).toString();
        const bool upload = model_->item(row, NameColumn)
                                ->data(DirectionRole).toString() == "upload";
        const bool resumeUpload = model_->item(row, NameColumn)
                                      ->data(AutoStartRole).toBool();
        if (state == "Queued" && (!upload || resumeUpload)) {
            startRow(row);
            return;
        }
    }
}

void MainWindow::pauseSelected()
{
    bool pauseActive = false;
    const auto rows = downloads_->selectionModel()->selectedRows();
    for (const auto &index : rows) {
        auto *item = model_->item(index.row(), NameColumn);
        if (index.row() == activeRow_) {
            pauseActive = true;
        } else if (item->data(StateRole).toString() == "Queued") {
            item->setData(false, AutoStartRole);
            setRowState(index.row(), "Paused");
        }
    }
    if (pauseActive && activeJob_) {
        pausing_ = true;
        activeJob_->cancel();
    }
}

void MainWindow::pauseAll()
{
    resumeAfterMove_ = false;
    for (int row = 0; row < model_->rowCount(); ++row) {
        auto *item = model_->item(row, NameColumn);
        item->setData(false, AutoStartRole);
        if (item->data(StateRole).toString() == "Queued")
            setRowState(row, "Paused");
    }
    if (activeJob_) {
        pausing_ = true;
        activeJob_->cancel();
    }
    saveTransfers();
}

void MainWindow::resumeAll()
{
    for (int row = 0; row < model_->rowCount(); ++row) {
        auto *item = model_->item(row, NameColumn);
        const QString state = item->data(StateRole).toString();
        if (state == "Paused" || state == "Queued") {
            item->setData(true, AutoStartRole);
            if (state == "Paused")
                setRowState(row, "Queued");
        }
    }
    saveTransfers();
    startNext();
}

void MainWindow::stopSelected()
{
    bool stopActive = false;
    const auto rows = downloads_->selectionModel()->selectedRows();
    for (const auto &index : rows) {
        auto *item = model_->item(index.row(), NameColumn);
        const QString state = item->data(StateRole).toString();
        item->setData(false, AutoStartRole);
        if (index.row() == activeRow_)
            stopActive = true;
        else if (state == "Queued" || state == "Paused" || state == "Failed")
            setRowState(index.row(), "Stopped");
    }
    if (stopActive && activeJob_) {
        stopping_ = true;
        activeJob_->cancel();
    }
}

void MainWindow::removeSelected()
{
    QList<QPersistentModelIndex> selected;
    for (const auto &index : downloads_->selectionModel()->selectedRows())
        selected.append(QPersistentModelIndex(index));
    bool removeActive = false;
    for (const auto &index : selected) {
        const int row = index.row();
        if (activeJob_ && row == activeRow_) {
            model_->item(row, NameColumn)->setData(true, RemoveWhenStoppedRole);
            removeActive = true;
        } else {
            model_->removeRow(row);
            if (activeJob_ && row < activeRow_)
                --activeRow_;
        }
    }
    if (removeActive && activeJob_) {
        stopping_ = true;
        activeJob_->cancel();
    }
    updateSummary();
    updateDetails();
    markTransfersDirty();
}

void MainWindow::updateProgress(qint64 received, qint64 total, qint64 speed)
{
    if (activeRow_ < 0)
        return;
    auto *nameItem = model_->item(activeRow_, NameColumn);
    nameItem->setData(received, ReceivedRole);
    nameItem->setData(total, TotalRole);
    model_->item(activeRow_, ProgressColumn)->setText(total > 0
        ? QStringLiteral("%1%").arg(qBound(0, int(100.0 * received / total), 100)) : tr("—"));
    model_->item(activeRow_, ProgressColumn)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    nameItem->setData(speed, SpeedRole);
    model_->item(activeRow_, SizeColumn)->setText(
        total > 0 ? QLocale().formattedDataSize(total) : tr("Unknown"));
    model_->item(activeRow_, SpeedColumn)->setText(
        QLocale().formattedDataSize(speed) + tr("/s"));
    model_->item(activeRow_, RemainingColumn)->setText(
        total > received && speed > 0
            ? QTime(0, 0).addSecs(int((total - received) / speed))
                  .toString("hh:mm:ss")
            : total >= 0 && total <= received ? tr("0:00:00") : tr("—"));
    setRowState(activeRow_, tr("Downloading"));
    markTransfersDirty();
    downloads_->viewport()->update();
    updateSummary();
}

void MainWindow::finishDownload(const QString &error)
{
    if (!activeJob_)
        return;
    const int row = activeRow_;
    const bool removed = model_->item(row, NameColumn)
                             ->data(RemoveWhenStoppedRole).toBool();
    auto *transfer = model_->item(row, NameColumn);
    const QDateTime started = transfer->data(StartedRole).toDateTime();
    if (started.isValid()) {
        transfer->setData(
            transfer->data(ElapsedRole).toLongLong() +
                started.secsTo(QDateTime::currentDateTime()),
            ElapsedRole);
        transfer->setData(QDateTime(), StartedRole);
    }
    activeJob_->deleteLater();
    activeJob_ = nullptr;
    activeRow_ = -1;

    if (removed) {
        model_->removeRow(row);
    } else if (!error.isEmpty()) {
        transfer->setData(error, ErrorRole);
        setRowState(row, "Failed");
        statusBar()->showMessage(error, 8000);
    } else if (pausing_) {
        setRowState(row, tr("Paused"));
    } else if (stopping_) {
        setRowState(row, tr("Stopped"));
    } else {
        setRowState(row, tr("Complete"));
        if (transfer->data(DirectionRole).toString() == "download")
            showCompletion(row);
        statusBar()->showMessage(transfer->data(DirectionRole).toString() == "upload"
            ? tr("Upload complete") : tr("Download complete"), 5000);
    }
    stopping_ = false;
    pausing_ = false;
    markTransfersDirty();
    updateSummary();
    if (movingIndex_.isValid()) {
        const QPersistentModelIndex index = movingIndex_;
        const QString destination = moveDestination_;
        const bool resume = resumeAfterMove_;
        movingIndex_ = QPersistentModelIndex();
        moveDestination_.clear();
        resumeAfterMove_ = false;
        if (moveTransfer(index, destination) && resume && index.isValid()) {
            downloads_->selectRow(index.row());
            startSelected();
            return;
        }
    }
    startNext();
}

void MainWindow::setRowState(int row, const QString &state)
{
    if (row < 0 || row >= model_->rowCount())
        return;
    auto *item = model_->item(row, NameColumn);
    item->setData(state, StateRole);
    if (state != "Downloading" && state != "Connecting") {
        item->setData(qint64(0), SpeedRole);
        model_->item(row, SpeedColumn)->setText(tr("—"));
        model_->item(row, RemainingColumn)->setText(tr("—"));
    }
    if (state == "Complete")
        model_->item(row, ProgressColumn)->setText("100%");
    if (state == tr("Complete"))
        item->setData(QDateTime::currentDateTime(), CompletedRole);
    model_->item(row, StatusColumn)->setText(state == "Downloading" &&
        item->data(DirectionRole).toString() == "upload" ? tr("Uploading") : state);
    model_->item(row, StatusColumn)->setToolTip(item->data(ErrorRole).toString());
    const QIcon icon = state == tr("Complete")
                           ? transferIcon("complete")
                           : state == tr("Downloading") || state == tr("Connecting")
                                 ? transferIcon("start")
                                 : state == tr("Stopped") || state == tr("Paused")
                                       ? transferIcon("pause")
                                       : standardIcon(this, QStyle::SP_FileIcon);
    item->setIcon(icon);

    markTransfersDirty();
    applyFilters();
    updateDetails();
    updateSummary();
}

void MainWindow::updateSummary()
{
    int active = 0;
    int complete = 0;
    int uploads = 0;
    for (int row = 0; row < model_->rowCount(); ++row) {
        auto *item = model_->item(row, NameColumn);
        const QString state = item->data(StateRole).toString();
        auto *order = model_->item(row, OrderColumn);
        if (!order) {
            order = new QStandardItem;
            order->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            model_->setItem(row, OrderColumn, order);
        }
        order->setText(QString::number(row + 1));
        active += state == "Downloading" || state == "Connecting";
        complete += state == "Complete";
        uploads += item->data(DirectionRole).toString() == "upload";
    }
    summary_->setText(tr("%1 items   •   %2 active   •   %3 completed   •   %4 uploads")
                          .arg(model_->rowCount()).arg(active).arg(complete)
                          .arg(uploads));
    QStringList groups;
    qint64 downloadSpeed = 0;
    qint64 uploadSpeed = 0;
    for (const QString &direction : {QStringLiteral("download"), QStringLiteral("upload")}) {
        int count = 0;
        int running = 0;
        qint64 received = 0;
        qint64 total = 0;
        qint64 speed = 0;
        bool unknownSize = false;
        for (int row = 0; row < model_->rowCount(); ++row) {
            const auto *item = model_->item(row, NameColumn);
            if (item->data(DirectionRole).toString() != direction)
                continue;
            ++count;
            const QString state = item->data(StateRole).toString();
            const bool transferring = state == "Downloading" || state == "Connecting";
            running += transferring;
            if (transferring)
                speed += item->data(SpeedRole).toLongLong();
            const qint64 size = item->data(TotalRole).toLongLong();
            if (size > 0) {
                total += size;
                received += qBound(qint64(0), item->data(ReceivedRole).toLongLong(), size);
            } else if (state != "Complete") {
                unknownSize = true;
            }
        }
        if (direction == "download")
            downloadSpeed = speed;
        else
            uploadSpeed = speed;
        if (count == 0)
            continue;
        const QString progress = unknownSize || total <= 0 ? tr("size unknown")
            : QStringLiteral("%1%").arg(qBound(0, int(100.0 * received / total), 100));
        groups << tr("%1: %2, %3/s, %4/%5 active")
            .arg(direction == "download" ? tr("Downloads") : tr("Uploads"), progress,
                 QLocale().formattedDataSize(speed))
            .arg(running).arg(count);
    }
    setWindowTitle(groups.isEmpty() ? tr("GetRight Port — Download Status")
        : QStringLiteral("GetRight Port — %1").arg(groups.join(QStringLiteral(" | "))));
    if (trayIcon_) {
        const QString tooltip =
            tr("GetRight Port\nDownload: %1/s\nUpload: %2/s\nRight-click for actions")
            .arg(QLocale().formattedDataSize(downloadSpeed),
                 QLocale().formattedDataSize(uploadSpeed));
        if (trayIcon_->toolTip() != tooltip)
            trayIcon_->setToolTip(tooltip);
    }
}

void MainWindow::markTransfersDirty()
{
    transfersDirty_ = true;
}

void MainWindow::saveSettings()
{
    if (!settingsWritable_)
        return;
    settings_.insert("version", 1);
    QDir().mkpath(QFileInfo(settingsFile_).absolutePath());
    QSaveFile file(settingsFile_);
    const QByteArray data = QJsonDocument(settings_).toJson(QJsonDocument::Indented);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        showStorageWarning(tr("Settings could not be saved: %1").arg(file.errorString()));
}

void MainWindow::saveTransfers()
{
    if (!historyWritable_)
        return;
    if (activeJob_)
        activeJob_->flush();
    QDir().mkpath(QFileInfo(transfersFile_).absolutePath());
    QJsonArray transfers;
    const QDateTime now = QDateTime::currentDateTime();
    for (int row = 0; row < model_->rowCount(); ++row) {
        auto *item = model_->item(row, NameColumn);
        const QString direction = item->data(DirectionRole).toString();
        QString state = item->data(StateRole).toString();
        if (state == "Downloading" || state == "Connecting")
            state = "Paused";

        qint64 received = item->data(ReceivedRole).toLongLong();
        const QString path = item->data(PathRole).toString();
        if (direction == "download" && state != "Complete")
            received = item->data(RestartRole).toBool() ? 0 : QFileInfo(path).size();
        const QDateTime started = item->data(StartedRole).toDateTime();
        const qint64 elapsed = item->data(ElapsedRole).toLongLong() +
            (started.isValid() ? started.secsTo(now) : 0);
        QString id = item->data(IdRole).toString();
        if (id.isEmpty()) {
            id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            item->setData(id, IdRole);
        }

        QJsonObject transfer;
        transfer.insert("id", id);
        transfer.insert("name", item->text());
        transfer.insert("url", item->data(UrlRole).toString());
        transfer.insert("path", path);
        transfer.insert("folder", item->data(FolderRole).toString());
        transfer.insert("direction", direction);
        transfer.insert("state", state);
        transfer.insert("etag", QString::fromLatin1(item->data(EtagRole).toByteArray()));
        transfer.insert("lastModified",
                        QString::fromLatin1(item->data(ModifiedRole).toByteArray()));
        transfer.insert("error", item->data(ErrorRole).toString());
        transfer.insert("restartRequested", item->data(RestartRole).toBool());
        transfer.insert("bytesReceived", QString::number(received));
        transfer.insert("totalBytes",
                        QString::number(item->data(TotalRole).toLongLong()));
        transfer.insert("elapsedSeconds", QString::number(elapsed));
        transfer.insert("addedAt", item->data(AddedRole).toDateTime()
                                       .toString(Qt::ISODateWithMs));
        transfer.insert("completedAt", item->data(CompletedRole).toDateTime()
                                           .toString(Qt::ISODateWithMs));
        transfers.append(transfer);
    }

    QJsonObject root;
    root.insert("version", 1);
    root.insert("savedAt", now.toString(Qt::ISODateWithMs));
    root.insert("transfers", transfers);
    QSaveFile file(transfersFile_);
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() ||
        !file.commit()) {
        showStorageWarning(tr("Transfer history could not be saved: %1").arg(file.errorString()));
        return;
    }
    transfersDirty_ = false;
}

void MainWindow::loadTransfers()
{
    QFile file(transfersFile_);
    if (!file.exists())
        return;
    if (!file.open(QIODevice::ReadOnly)) {
        historyWritable_ = false;
        qWarning("Cannot read history; preserving %s", qPrintable(transfersFile_));
        return;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject() ||
        !document.object().value("transfers").isArray()) {
        historyWritable_ = false;
        qWarning("Could not read transfer history from %s: %s",
                 qPrintable(transfersFile_), qPrintable(error.errorString()));
        return;
    }

    const QJsonArray transfers = document.object().value("transfers").toArray();
    for (const QJsonValue &value : transfers) {
        if (!value.isObject())
            continue;
        const QJsonObject saved = value.toObject();
        const QString name = saved.value("name").toString();
        const QString path = saved.value("path").toString();
        const QString url = saved.value("url").toString();
        const QString direction = saved.value("direction").toString("download");
        QString state = saved.value("state").toString("Queued");
        if (state == "Downloading" || state == "Connecting")
            state = "Paused";
        const auto number = [&saved](const char *key) {
            const auto value = saved.value(QString::fromLatin1(key));
            return value.isString() ? value.toString().toLongLong()
                                    : value.toVariant().toLongLong();
        };
        const auto dateTime = [&saved](const char *key) {
            const QString value = saved.value(QString::fromLatin1(key)).toString();
            QDateTime date = QDateTime::fromString(value, Qt::ISODateWithMs);
            if (!date.isValid())
                date = QDateTime::fromString(value, Qt::ISODate);
            return date;
        };
        qint64 received = number("bytesReceived");
        const qint64 total = number("totalBytes");
        if (direction == "download" && state != "Complete")
            received = saved.value("restartRequested").toBool() ? 0 : QFileInfo(path).size();
        const QDateTime added = dateTime("addedAt");
        const QDateTime completed = dateTime("completedAt");
        const QString folder = saved.value("folder").toString().isEmpty()
            ? QFileInfo(path).dir().dirName()
            : saved.value("folder").toString();

        const QIcon icon = state == "Complete"
            ? transferIcon("complete")
            : state == "Downloading" || state == "Connecting"
                ? transferIcon("start")
                : state == "Paused" || state == "Stopped"
                    ? transferIcon("pause")
                    : direction == "upload"
                        ? transferIcon("upload")
                        : standardIcon(this, QStyle::SP_FileIcon);
        auto *fileItem = new QStandardItem(icon, name);
        fileItem->setData(saved.value("id").toString(), IdRole);
        fileItem->setData(url, UrlRole);
        fileItem->setData(saved.value("etag").toString().toLatin1(), EtagRole);
        fileItem->setData(saved.value("lastModified").toString().toLatin1(), ModifiedRole);
        fileItem->setData(saved.value("error").toString(), ErrorRole);
        fileItem->setData(path, PathRole);
        fileItem->setData(state, StateRole);
        fileItem->setData(saved.value("restartRequested").toBool(), RestartRole);
        fileItem->setData(direction, DirectionRole);
        fileItem->setData(folder, FolderRole);
        fileItem->setData(added.isValid() ? added : QDateTime::currentDateTime(),
                          AddedRole);
        fileItem->setData(completed, CompletedRole);
        fileItem->setData(received, ReceivedRole);
        fileItem->setData(total, TotalRole);
        fileItem->setData(qint64(0), SpeedRole);
        fileItem->setData(number("elapsedSeconds"), ElapsedRole);

        const int percent = total > 0
            ? qBound(0, int(100.0 * received / total), 100) : 0;
        QList<QStandardItem *> items;
        items << fileItem
              << new QStandardItem(total > 0
                    ? QLocale().formattedDataSize(total) : tr("Unknown"))
              << new QStandardItem(state == "Downloading" && direction == "upload"
                    ? tr("Uploading") : state)
              << new QStandardItem(total > 0
                    ? QStringLiteral("%1%").arg(percent) : QString())
              << new QStandardItem(tr("—"))
              << new QStandardItem(tr("—"))
              << new QStandardItem(url);
        items[StatusColumn]->setToolTip(saved.value("error").toString());
        model_->appendRow(items);
        if (direction == "download")
            ensureFolderFilter(folder);
    }
    transfersDirty_ = false;
    applyFilters();
    for (int row = 0; row < model_->rowCount(); ++row) {
        if (!downloads_->isRowHidden(row)) {
            downloads_->selectRow(row);
            break;
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (activeJob_)
        activeJob_->flush();
    settings_.insert("windowGeometry", QString::fromLatin1(saveGeometry().toBase64()));
    saveSettings();
    saveTransfers();
    if (trayIcon_ && trayIcon_->isVisible() && QSystemTrayIcon::isSystemTrayAvailable()) {
        event->ignore();
        if (transferDialog_)
            transferDialog_->hide();
        hide();
        return;
    }
    event->accept();
    QCoreApplication::quit();
}

void MainWindow::showMainWindow()
{
    if (isMinimized())
        setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
#ifdef Q_OS_MACOS
    activateMacApplication();
#endif
}

void MainWindow::openLocalFile(const QString &path)
{
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        QMessageBox::warning(this, tr("Cannot open file"), tr("No application opened the file."));
}

void MainWindow::revealLocalFile(const QString &path)
{
#ifdef Q_OS_MACOS
    if (!QProcess::startDetached("/usr/bin/open", {"-R", path}))
        QMessageBox::warning(this, tr("Cannot reveal file"), tr("Finder could not be opened."));
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
#endif
}

void MainWindow::showCompletion(int row)
{
    const auto *item = model_->item(row, NameColumn);
    const QString path = item->data(PathRole).toString();
    if (completionPopup_)
        completionPopup_->close();
    auto *popup = new CompletionPopup(item->text(), item->data(ElapsedRole).toLongLong(),
        QFileInfo(path).size(), windowIcon(), trayIcon_ ? trayIcon_->geometry() : QRect(), this);
    completionPopup_ = popup;
    connect(popup, &CompletionPopup::openRequested, this, [this, path]() { openLocalFile(path); });
    connect(popup, &CompletionPopup::revealRequested, this,
            [this, path]() { revealLocalFile(path); });
    connect(popup, &CompletionPopup::showMainRequested, this, &MainWindow::showMainWindow);
    popup->show();
}

void MainWindow::showStorageWarning(const QString &message)
{
    storageWarning_->setText(message);
    storageWarning_->show();
}

void MainWindow::showDataFolder()
{
    const QString directory = QFileInfo(settingsFile_).absolutePath();
    if (!QDir().mkpath(directory) || !QDesktopServices::openUrl(QUrl::fromLocalFile(directory)))
        QMessageBox::warning(this, tr("Cannot open data folder"), directory);
}

void MainWindow::showHelp()
{
    QMessageBox::information(this, tr("GetRight Port Help"),
        tr("Add adds an HTTP or HTTPS address to the download list.\n\n"
           "Start begins or resumes the selected transfer. Pause suspends it "
           "and keeps the partial file. Stop aborts it while keeping any "
           "partial data. Double-click a transfer to open its progress window. "
           "Remove deletes "
           "the selected item from the list. Use the filters on the left "
           "to view active, stopped, or completed downloads."));
}

void MainWindow::showAbout()
{
    QMessageBox about(this);
    about.setWindowTitle(tr("About GetRight Port"));
    about.setTextFormat(Qt::RichText);
    about.setIconPixmap(windowIcon().pixmap(96, 96));
    about.setText(QStringLiteral(
            "<h3>GetRight Port v%1</h3>"
            "<p><b>Author:</b> Jorge Pereira &lt;"
            "<a href=\"mailto:jpereiran@gmail.com\">jpereiran@gmail.com</a>&gt;</p>"
            "<p><b>Project:</b> <a href=\"https://github.com/jpereira/getright\">"
            "https://github.com/jpereira/getright</a></p>"
            "<p>Open source software, licensed under the GNU Lesser General "
            "Public License (LGPL), version 3.</p>"
            "<p>A cross-platform study port inspired by GetRight 6.5.</p>")
                .arg(QApplication::applicationVersion()));
    about.exec();
}
