#include "main_window.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDateTime>
#include <QCloseEvent>
#include <QClipboard>
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
#include <QTableView>
#include <QToolBar>
#include <QTimer>
#include <QTreeWidget>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>

namespace {

class ProgressDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        const bool selected = option.state & QStyle::State_Selected;
        painter->fillRect(option.rect, selected ? option.palette.highlight()
                                                : option.palette.base());

        const QModelIndex name = index.sibling(index.row(), 0);
        const qint64 received = name.data(Qt::UserRole + 4).toLongLong();
        const qint64 total = name.data(Qt::UserRole + 5).toLongLong();
        const int percent = total > 0
                                ? qBound(0, int(received * 100 / total), 100)
                                : 0;
        QRect bar = option.rect.adjusted(5, 6, -5, -6);
        painter->setPen(QColor("#a0a0a0"));
        painter->setBrush(QColor("#f4f4f4"));
        painter->drawRect(bar);
        QRect fill = bar.adjusted(1, 1, -1, -1);
        fill.setWidth(fill.width() * percent / 100);
        if (fill.width() > 0) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor("#55a85f"));
            painter->drawRect(fill);
        }
        painter->setPen(selected ? option.palette.highlightedText().color()
                                 : option.palette.text().color());
        painter->drawText(bar, Qt::AlignCenter,
                          total > 0 ? QStringLiteral("%1%").arg(percent)
                                    : QStringLiteral("%1 bytes").arg(received));
        painter->restore();
    }
};

QIcon standardIcon(QWidget *widget, QStyle::StandardPixmap pixmap)
{
    return widget->style()->standardIcon(pixmap);
}

QIcon coloredIcon(const QString &kind)
{
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#555555"), 1.2));

    if (kind == "start") {
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
      model_(new QStandardItemModel(0, 7, this)),
      summary_(new QLabel(this)),
      urlStatus_(new QLabel(this)),
      detailName_(new QLabel(this)),
      detailUrl_(new QLineEdit(this)),
      detailPath_(new QLineEdit(this)),
      detailTransfer_(new QLabel(this)),
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
    resize(1020, 620);
    restoreGeometry(QByteArray::fromBase64(
        settings_.value("windowGeometry").toString().toLatin1()));

    model_->setHorizontalHeaderLabels(
        {tr("File"), tr("Size"), tr("Status"), tr("Progress"),
         tr("Speed"), tr("Time left"), tr("Address")});
    downloads_->setModel(model_);
    downloads_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(downloads_, &QWidget::customContextMenuRequested,
            this, &MainWindow::showTransferMenu);
    downloads_->setItemDelegateForColumn(ProgressColumn,
                                         new ProgressDelegate(downloads_));
    downloads_->setSelectionBehavior(QAbstractItemView::SelectRows);
    downloads_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    downloads_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    downloads_->setAlternatingRowColors(true);
    downloads_->verticalHeader()->hide();
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
    auto *foldersRoot = addFilter(nullptr, tr("Folders"), "folders");
    foldersRoot->setIcon(0, standardIcon(this, QStyle::SP_DirIcon));
    addFilter(foldersRoot, tr("Downloads"), "folder_downloads");
    addFilter(foldersRoot, tr("Unfiled"), "folder_unfiled");
    auto *filtersRoot = addFilter(nullptr, tr("Filters"), "filters");
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
    folders_->expandAll();
    folders_->setCurrentItem(downloadsRoot->child(0));

    filter_->setPlaceholderText(tr("Filter downloads…"));
    filter_->setClearButtonEnabled(true);
    auto *sidebar = new QWidget(this);
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->addWidget(new QLabel(tr("Downloads"), sidebar));
    sideLayout->addWidget(folders_, 1);

    auto *content = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(8, 8, 8, 8);
    contentLayout->addWidget(filter_);
    contentLayout->addWidget(downloads_, 1);
    auto *details = new QGroupBox(tr("Download details"), content);
    auto *detailLayout = new QFormLayout(details);
    detailLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    detailName_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    for (QLineEdit *field : {detailUrl_, detailPath_}) {
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
    auto *splitter = new QSplitter(this);
    splitter->addWidget(sidebar);
    splitter->addWidget(content);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    auto *toolbar = addToolBar(tr("Downloads"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setIconSize(QSize(20, 20));
    QAction *add = toolbar->addAction(
        coloredIcon("add"), tr("Add"),
        this, &MainWindow::addDownload);
    add->setShortcut(QKeySequence::New);
    QAction *upload = toolbar->addAction(
        coloredIcon("upload"), tr("Upload"), this, &MainWindow::addUpload);
    toolbar->addSeparator();
    QAction *start = toolbar->addAction(
        coloredIcon("start"), tr("Start"),
        this, &MainWindow::startSelected);
    start->setShortcut(Qt::Key_F5);
    QAction *pause = toolbar->addAction(
        coloredIcon("pause"), tr("Pause"),
        this, &MainWindow::pauseSelected);
    QAction *stop = toolbar->addAction(
        coloredIcon("stop"), tr("Stop"),
        this, &MainWindow::stopSelected);
    stop->setShortcut(Qt::Key_Escape);
    QAction *remove = toolbar->addAction(
        coloredIcon("remove"), tr("Remove"),
        this, &MainWindow::removeSelected);
    remove->setShortcut(QKeySequence::Delete);
    toolbar->addSeparator();
    toolbar->addAction(coloredIcon("help"),
                       tr("Help"), this, &MainWindow::showHelp);

    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(add);
    fileMenu->addAction(upload);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), QKeySequence::Quit, this,
                        &QWidget::close);
    auto *downloadsMenu = menuBar()->addMenu(tr("&Downloads"));
    downloadsMenu->addAction(start);
    downloadsMenu->addAction(pause);
    downloadsMenu->addAction(stop);
    downloadsMenu->addAction(remove);
    auto *viewMenu = menuBar()->addMenu(tr("&View"));
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
        if (selectedRow() >= 0) {
            urlStatus_->setText(model_->item(selectedRow(), UrlColumn)->text());
            updateDetails();
        }
    });
    connect(downloads_, &QTableView::doubleClicked, this,
            [this](const QModelIndex &index) {
        if (index.isValid())
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
        detailName_->setText(tr("No download selected"));
        setDetailText(detailUrl_, {});
        setDetailText(detailPath_, {});
        detailTransfer_->clear();
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
                            ? qBound(0, int(received * 100 / total), 100)
                            : 0;
    detailName_->setText(item->text());
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

void MainWindow::showTransferDetails(int row)
{
    if (row < 0 || row >= model_->rowCount())
        return;
    transferDialogIndex_ = model_->index(row, NameColumn);
    if (!transferDialog_) {
        auto *dialog = new QDialog(this, Qt::Window);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        transferDialog_ = dialog;
        dialog->setWindowIcon(windowIcon());

        auto *layout = new QVBoxLayout(dialog);
        auto *form = new QFormLayout;
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        transferName_ = new QLabel(dialog);
        transferUrl_ = new QLineEdit(dialog);
        transferPath_ = new QLineEdit(dialog);
        transferAmount_ = new QLabel(dialog);
        transferSpeed_ = new QLabel(dialog);
        transferElapsed_ = new QLabel(dialog);
        transferRemaining_ = new QLabel(dialog);
        for (QLineEdit *field : {transferUrl_, transferPath_}) {
            field->setReadOnly(true);
            field->setMinimumWidth(0);
        }
        transferName_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        form->addRow(tr("File:"), transferName_);
        form->addRow(tr("Address:"), transferUrl_);
        form->addRow(tr("Save to:"), transferPath_);
        form->addRow(tr("Transferred:"), transferAmount_);
        layout->addLayout(form);

        auto *progressLine = new QWidget(dialog);
        auto *progressLayout = new QHBoxLayout(progressLine);
        progressLayout->setContentsMargins(0, 0, 0, 0);
        transferProgress_ = new QProgressBar(progressLine);
        transferProgress_->setRange(0, 100);
        transferPercent_ = new QLabel(progressLine);
        progressLayout->addWidget(transferProgress_, 1);
        progressLayout->addWidget(transferPercent_);
        layout->addWidget(progressLine);

        auto *stats = new QWidget(dialog);
        auto *statsLayout = new QHBoxLayout(stats);
        statsLayout->setContentsMargins(0, 0, 0, 0);
        statsLayout->addWidget(new QLabel(tr("Speed:"), stats));
        statsLayout->addWidget(transferSpeed_);
        statsLayout->addSpacing(16);
        statsLayout->addWidget(new QLabel(tr("Elapsed:"), stats));
        statsLayout->addWidget(transferElapsed_);
        statsLayout->addSpacing(16);
        statsLayout->addWidget(new QLabel(tr("Remaining:"), stats));
        statsLayout->addWidget(transferRemaining_);
        statsLayout->addStretch(1);
        layout->addWidget(stats);

        auto *buttons = new QWidget(dialog);
        auto *buttonLayout = new QHBoxLayout(buttons);
        buttonLayout->setContentsMargins(0, 8, 0, 0);
        buttonLayout->addStretch(1);
        transferPause_ = new QPushButton(tr("Pause"), buttons);
        auto *close = new QPushButton(tr("Close"), buttons);
        buttonLayout->addWidget(transferPause_);
        buttonLayout->addWidget(close);
        layout->addWidget(buttons);
        connect(close, &QPushButton::clicked, dialog, &QDialog::close);
        connect(transferPause_, &QPushButton::clicked, this, [this]() {
            const int selected = transferDialogIndex_.row();
            if (selected < 0 || selected >= model_->rowCount())
                return;
            downloads_->selectRow(selected);
            const QString state = model_->item(selected, NameColumn)
                                      ->data(StateRole).toString();
            if (state == "Downloading" || state == "Connecting")
                pauseSelected();
            else if (state != "Complete")
                startSelected();
        });
        connect(dialog, &QObject::destroyed, this, [this]() {
            transferDialogIndex_ = QPersistentModelIndex();
            transferName_ = nullptr;
            transferUrl_ = nullptr;
            transferPath_ = nullptr;
            transferAmount_ = nullptr;
            transferSpeed_ = nullptr;
            transferElapsed_ = nullptr;
            transferRemaining_ = nullptr;
            transferPercent_ = nullptr;
            transferProgress_ = nullptr;
            transferPause_ = nullptr;
        });
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
    const int percent = total > 0
        ? qBound(0, int(received * 100 / total), 100) : 0;
    transferDialog_->setWindowTitle(tr("Transfer Progress — %1")
                                        .arg(item->text()));
    transferName_->setText(item->text());
    setDetailText(transferUrl_, item->data(UrlRole).toString());
    setDetailText(transferPath_, item->data(PathRole).toString());
    transferAmount_->setText(total > 0
        ? QStringLiteral("%1 of %2")
              .arg(QLocale().formattedDataSize(received),
                   QLocale().formattedDataSize(total))
        : QLocale().formattedDataSize(received));
    transferProgress_->setValue(percent);
    transferPercent_->setText(total > 0
        ? QStringLiteral("%1% — %2").arg(percent).arg(state) : state);
    transferSpeed_->setText(QLocale().formattedDataSize(speed) + tr("/s"));
    transferElapsed_->setText(
        QTime(0, 0).addSecs(int(elapsed)).toString("hh:mm:ss"));
    transferRemaining_->setText(total > 0 && speed > 0
        ? QTime(0, 0).addSecs(int((total - received) / speed))
              .toString("hh:mm:ss")
        : tr("—"));
    const bool active = state == "Downloading" || state == "Connecting";
    transferPause_->setText(active ? tr("Pause") : tr("Resume"));
    transferPause_->setEnabled(state != "Complete");
}

void MainWindow::ensureFolderFilter(const QString &folder)
{
    auto *root = folders_->topLevelItem(1);
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
    const QString text = QInputDialog::getText(
        this, tr("Add download"), tr("URL:"), QLineEdit::Normal,
        QString(), &accepted).trimmed();
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
        standardIcon(this, QStyle::SP_FileIcon), name);
    fileItem->setData(address.toString(), UrlRole);
    fileItem->setData(QUuid::createUuid().toString(QUuid::WithoutBraces),
                      IdRole);
    fileItem->setData(path, PathRole);
    fileItem->setData(QStringLiteral("Queued"), StateRole);
    fileItem->setData(QStringLiteral("download"), DirectionRole);
    const QString folder = QFileInfo(path).dir().dirName();
    fileItem->setData(folder, FolderRole);
    ensureFolderFilter(folder);
    fileItem->setData(QDateTime::currentDateTime(), AddedRole);
    fileItem->setData(qint64(0), ReceivedRole);
    fileItem->setData(qint64(0), TotalRole);
    items << fileItem << new QStandardItem(tr("Unknown"))
          << new QStandardItem(tr("Queued")) << new QStandardItem()
          << new QStandardItem(tr("—")) << new QStandardItem(tr("—"))
          << new QStandardItem(address.toString());
    model_->appendRow(items);
    markTransfersDirty();
    folders_->setCurrentItem(folders_->topLevelItem(0));
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
    auto *fileItem = new QStandardItem(coloredIcon("upload"), info.fileName());
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
    folders_->setCurrentItem(folders_->topLevelItem(3)->child(0));
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
    downloads_->selectRow(index.row());
    const auto *item = model_->item(index.row(), NameColumn);
    const QString state = item->data(StateRole).toString();
    const bool active = activeReply_ && index.row() == activeRow_;

    QMenu menu(this);
    const auto addAction = [&](const QString &text, auto operation, bool enabled = true) {
        QAction *action = menu.addAction(text);
        action->setEnabled(enabled);
        connect(action, &QAction::triggered, this, [this, index, operation]() {
            if (!index.isValid())
                return;
            downloads_->selectRow(index.row());
            (this->*operation)();
        });
    };
    addAction(tr("Start"), &MainWindow::startSelected,
              !activeReply_ && state != "Complete");
    addAction(tr("Pause"), &MainWindow::pauseSelected, active || state == "Queued");
    addAction(tr("Stop"), &MainWindow::stopSelected,
              active || state == "Queued" || state == "Paused" || state == "Failed");
    addAction(tr("Remove"), &MainWindow::removeSelected);
    menu.addSeparator();
    QAction *copy = menu.addAction(tr("Copy URL"));
    connect(copy, &QAction::triggered, this, [index]() {
        if (index.isValid())
            QApplication::clipboard()->setText(index.data(UrlRole).toString());
    });
    addAction(tr("Move To"), &MainWindow::moveSelected,
              item->data(DirectionRole).toString() == "download" && !movingIndex_.isValid());
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

    if (activeReply_ && index.row() == activeRow_) {
        movingIndex_ = index;
        moveDestination_ = destination;
        resumeAfterMove_ = true;
        pausing_ = true;
        activeReply_->abort();
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

void MainWindow::startSelected()
{
    int row = selectedRow();
    if (row < 0) {
        startNext();
        return;
    }
    const QString state = model_->item(row, NameColumn)->data(StateRole).toString();
    if (state == "Complete" || state == "Downloading")
        return;
    if (activeReply_)
        return;

    activeRow_ = row;
    auto *nameItem = model_->item(row, NameColumn);
    const QString path = nameItem->data(PathRole).toString();
    const bool uploading =
        nameItem->data(DirectionRole).toString() == "upload";
    resumeOffset_ = !uploading && QFileInfo(path).exists()
                        ? QFileInfo(path).size() : 0;
    if (uploading) {
        uploadInput_.setFileName(path);
        if (!uploadInput_.open(QIODevice::ReadOnly)) {
            setRowState(row, tr("Failed"));
            QMessageBox::warning(this, tr("Cannot read upload file"),
                                 uploadInput_.errorString());
            activeRow_ = -1;
            return;
        }
        resumeOffset_ = 0;
    } else {
        output_.setFileName(path);
        if (!output_.open(QIODevice::WriteOnly | QIODevice::Append)) {
            setRowState(row, tr("Failed"));
            QMessageBox::warning(this, tr("Cannot save file"),
                                 output_.errorString());
            activeRow_ = -1;
            return;
        }
    }

    const QUrl address(nameItem->data(UrlRole).toString());
    QNetworkRequest request(address);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    if (resumeOffset_ > 0)
        request.setRawHeader("Range", QByteArray("bytes=") +
                                            QByteArray::number(resumeOffset_) + "-");
    responsePrepared_ = false;
    stopping_ = false;
    nameItem->setData(QDateTime::currentDateTime(), StartedRole);
    nameItem->setData(qint64(0), SpeedRole);
    activeReply_ = uploading ? network_.put(request, &uploadInput_)
                             : network_.get(request);
    setRowState(row, tr("Connecting"));
    if (uploading) {
        connect(activeReply_, &QNetworkReply::uploadProgress, this,
                &MainWindow::updateProgress);
    } else {
        connect(activeReply_, &QNetworkReply::downloadProgress, this,
                &MainWindow::updateProgress);
        connect(activeReply_, &QIODevice::readyRead, this,
                &MainWindow::receiveData);
    }
    connect(activeReply_, &QNetworkReply::finished, this,
            &MainWindow::finishDownload);
}

void MainWindow::startNext()
{
    if (activeReply_)
        return;
    for (int row = 0; row < model_->rowCount(); ++row) {
        const QString state = model_->item(row, NameColumn)
                                  ->data(StateRole).toString();
        const bool upload = model_->item(row, NameColumn)
                                ->data(DirectionRole).toString() == "upload";
        if (state == "Queued" && !upload) {
            downloads_->selectRow(row);
            startSelected();
            return;
        }
    }
}

void MainWindow::pauseSelected()
{
    const int row = selectedRow();
    if (row < 0)
        return;
    const QString state = model_->item(row, NameColumn)
                              ->data(StateRole).toString();
    if (row == activeRow_ && activeReply_ &&
        (state == "Downloading" || state == "Connecting")) {
        pausing_ = true;
        activeReply_->abort();
    } else if (state == "Queued") {
        setRowState(row, tr("Paused"));
    }
}

void MainWindow::stopSelected()
{
    const int row = selectedRow();
    if (row < 0)
        return;
    if (activeReply_ && row == activeRow_) {
        stopping_ = true;
        activeReply_->abort();
        return;
    }
    const QString state = model_->item(row, NameColumn)->data(StateRole).toString();
    if (state == "Queued" || state == "Paused" || state == "Failed")
        setRowState(row, tr("Stopped"));
}

void MainWindow::removeSelected()
{
    const QModelIndexList selected = downloads_->selectionModel()->selectedRows();
    for (auto it = selected.crbegin(); it != selected.crend(); ++it) {
        const int row = it->row();
        if (activeReply_ && row == activeRow_) {
            model_->item(row, NameColumn)->setData(true, RemoveWhenStoppedRole);
            stopping_ = true;
            activeReply_->abort();
        } else {
            model_->removeRow(row);
            if (activeReply_ && row < activeRow_)
                --activeRow_;
        }
    }
    updateSummary();
    markTransfersDirty();
}

void MainWindow::updateProgress(qint64 received, qint64 total)
{
    if (activeRow_ < 0)
        return;
    auto *nameItem = model_->item(activeRow_, NameColumn);
    const bool uploading =
        nameItem->data(DirectionRole).toString() == "upload";
    if (!uploading) {
        received += resumeOffset_;
        total += resumeOffset_;
    }
    nameItem->setData(received, ReceivedRole);
    nameItem->setData(total, TotalRole);
    const QDateTime started = nameItem->data(StartedRole).toDateTime();
    const qint64 seconds = qMax<qint64>(1,
        nameItem->data(ElapsedRole).toLongLong() +
        started.secsTo(QDateTime::currentDateTime()));
    const qint64 speed = received / seconds;
    nameItem->setData(speed, SpeedRole);
    model_->item(activeRow_, SizeColumn)->setText(
        total > 0 ? QLocale().formattedDataSize(total) : tr("Unknown"));
    model_->item(activeRow_, SpeedColumn)->setText(
        QLocale().formattedDataSize(speed) + tr("/s"));
    model_->item(activeRow_, RemainingColumn)->setText(
        total > received && speed > 0
            ? QTime(0, 0).addSecs(int((total - received) / speed))
                  .toString("hh:mm:ss")
            : total <= received ? tr("0:00:00") : tr("—"));
    setRowState(activeRow_, tr("Downloading"));
    markTransfersDirty();
    downloads_->viewport()->update();
    updateSummary();
}

void MainWindow::receiveData()
{
    if (!activeReply_ || !activeReply_->isReadable() ||
        !output_.isOpen() || activeRow_ < 0)
        return;
    if (model_->item(activeRow_, NameColumn)
            ->data(DirectionRole).toString() == "upload")
        return;
    if (!responsePrepared_) {
        responsePrepared_ = true;
        const int code = activeReply_->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (resumeOffset_ > 0 && code != 206) {
            output_.resize(0);
            resumeOffset_ = 0;
            auto *nameItem = model_->item(activeRow_, NameColumn);
            nameItem->setData(qint64(0), ReceivedRole);
        }
        setRowState(activeRow_, tr("Downloading"));
    }
    output_.write(activeReply_->readAll());
}

void MainWindow::finishDownload()
{
    if (!activeReply_)
        return;
    receiveData();
    const int row = activeRow_;
    const bool removed = model_->item(row, NameColumn)
                             ->data(RemoveWhenStoppedRole).toBool();
    const auto error = activeReply_->error();
    const QString errorText = activeReply_->errorString();
    output_.flush();
    output_.close();
    uploadInput_.close();
    auto *transfer = model_->item(row, NameColumn);
    const QDateTime started = transfer->data(StartedRole).toDateTime();
    if (started.isValid()) {
        transfer->setData(
            transfer->data(ElapsedRole).toLongLong() +
                started.secsTo(QDateTime::currentDateTime()),
            ElapsedRole);
        transfer->setData(QDateTime(), StartedRole);
    }
    activeReply_->deleteLater();
    activeReply_ = nullptr;
    activeRow_ = -1;

    if (removed) {
        model_->removeRow(row);
    } else if (pausing_) {
        setRowState(row, tr("Paused"));
    } else if (stopping_ || error == QNetworkReply::OperationCanceledError) {
        setRowState(row, tr("Stopped"));
    } else if (error == QNetworkReply::NoError) {
        setRowState(row, tr("Complete"));
        statusBar()->showMessage(tr("Download complete"), 5000);
    } else {
        setRowState(row, tr("Failed"));
        statusBar()->showMessage(errorText, 8000);
    }
    stopping_ = false;
    pausing_ = false;
    responsePrepared_ = false;
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
    if (state == tr("Complete"))
        item->setData(QDateTime::currentDateTime(), CompletedRole);
    model_->item(row, StatusColumn)->setText(state);
    const QIcon icon = state == tr("Complete")
                           ? standardIcon(this, QStyle::SP_DialogApplyButton)
                           : state == tr("Downloading") || state == tr("Connecting")
                                 ? coloredIcon("start")
                                 : state == tr("Stopped") || state == tr("Paused")
                                       ? coloredIcon("pause")
                                       : standardIcon(this, QStyle::SP_FileIcon);
    item->setIcon(icon);

    markTransfersDirty();
    applyFilters();
    updateDetails();
}

void MainWindow::updateSummary()
{
    int active = 0;
    int complete = 0;
    int uploads = 0;
    for (int row = 0; row < model_->rowCount(); ++row) {
        auto *item = model_->item(row, NameColumn);
        const QString state = item->data(StateRole).toString();
        active += state == "Downloading" || state == "Connecting";
        complete += state == "Complete";
        uploads += item->data(DirectionRole).toString() == "upload";
    }
    summary_->setText(tr("%1 items   •   %2 active   •   %3 completed   •   %4 uploads")
                          .arg(model_->rowCount()).arg(active).arg(complete)
                          .arg(uploads));
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
        qWarning("Could not save settings to %s", qPrintable(settingsFile_));
}

void MainWindow::saveTransfers()
{
    if (!historyWritable_)
        return;
    if (output_.isOpen())
        output_.flush();
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
            received = QFileInfo(path).size();
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
        qWarning("Could not save transfer history to %s",
                 qPrintable(transfersFile_));
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
            received = QFileInfo(path).size();
        const QDateTime added = dateTime("addedAt");
        const QDateTime completed = dateTime("completedAt");
        const QString folder = saved.value("folder").toString().isEmpty()
            ? QFileInfo(path).dir().dirName()
            : saved.value("folder").toString();

        const QIcon icon = state == "Complete"
            ? standardIcon(this, QStyle::SP_DialogApplyButton)
            : state == "Downloading" || state == "Connecting"
                ? coloredIcon("start")
                : state == "Paused" || state == "Stopped"
                    ? coloredIcon("pause")
                    : direction == "upload"
                        ? coloredIcon("upload")
                        : standardIcon(this, QStyle::SP_FileIcon);
        auto *fileItem = new QStandardItem(icon, name);
        fileItem->setData(saved.value("id").toString(), IdRole);
        fileItem->setData(url, UrlRole);
        fileItem->setData(path, PathRole);
        fileItem->setData(state, StateRole);
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
            ? qBound(0, int(received * 100 / total), 100) : 0;
        QList<QStandardItem *> items;
        items << fileItem
              << new QStandardItem(total > 0
                    ? QLocale().formattedDataSize(total) : tr("Unknown"))
              << new QStandardItem(state)
              << new QStandardItem(total > 0
                    ? QStringLiteral("%1%").arg(percent) : QString())
              << new QStandardItem(tr("—"))
              << new QStandardItem(tr("—"))
              << new QStandardItem(url);
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
    if (output_.isOpen())
        output_.flush();
    settings_.insert("windowGeometry", QString::fromLatin1(saveGeometry().toBase64()));
    saveSettings();
    saveTransfers();
    event->accept();
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
    QMessageBox::about(this, tr("About GetRight Port"),
        QStringLiteral(
            "<h3>GetRight Port</h3>"
            "<p><b>Author:</b> Jorge Pereira &lt;"
            "<a href=\"mailto:jpereiran@gmail.com\">jpereiran@gmail.com</a>&gt;</p>"
            "<p><b>Project:</b> <a href=\"https://github.com/jpereira/getright\">"
            "https://github.com/jpereira/getright</a></p>"
            "<p>Open source software, licensed under the GNU Lesser General "
            "Public License (LGPL), version 3.</p>"
            "<p>A cross-platform study port inspired by GetRight 6.5.</p>"));
}
