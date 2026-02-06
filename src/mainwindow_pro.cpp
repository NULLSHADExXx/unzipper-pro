#include "mainwindow.h"
#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QSettings>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QFont>
#include <QSplitter>
#include <QTableWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QDateTime>
#include <QComboBox>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QStyle>
#include <QStyleFactory>
#include <QProxyStyle>
#include <QSet>
#include <QThread>

// Custom style for professional look
class ProStyle : public QProxyStyle {
public:
    ProStyle() : QProxyStyle(QStyleFactory::create("Fusion")) {}
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), outputPath(QDir::homePath())
{
    setWindowTitle("Unzipper Pro");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setGeometry(100, 100, 1200, 750);
    setMinimumSize(900, 600);

    qApp->setStyle(new ProStyle());

    QPalette p;
    p.setColor(QPalette::Window, QColor(30, 30, 30));
    p.setColor(QPalette::WindowText, QColor(212, 212, 212));
    p.setColor(QPalette::Base, QColor(45, 45, 45));
    p.setColor(QPalette::AlternateBase, QColor(37, 37, 38));
    p.setColor(QPalette::Text, QColor(212, 212, 212));
    p.setColor(QPalette::Button, QColor(45, 45, 45));
    p.setColor(QPalette::ButtonText, QColor(212, 212, 212));
    p.setColor(QPalette::Highlight, QColor(0, 122, 204));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::PlaceholderText, QColor(109, 109, 109));
    qApp->setPalette(p);

    qApp->setStyleSheet(
        "QMainWindow, QWidget { background-color: #1e1e1e; color: #d4d4d4; }"
        "QLabel { color: #d4d4d4; }"
        "QLineEdit { background-color: #3c3c3c; color: #d4d4d4; border: 1px solid #555; border-radius: 3px; padding: 6px; selection-background-color: #007acc; }"
        "QCheckBox { color: #d4d4d4; spacing: 8px; }"
        "QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid #666; border-radius: 3px; background-color: #3c3c3c; }"
        "QCheckBox::indicator:checked { background-color: #007acc; border-color: #007acc; }"
        "QCheckBox::indicator:hover { border-color: #007acc; }"
        "QProgressBar { background-color: #3c3c3c; border: 1px solid #555; border-radius: 3px; text-align: center; color: #d4d4d4; }"
        "QProgressBar::chunk { background-color: #007acc; border-radius: 2px; }"
        "QMenuBar { background-color: #252526; color: #d4d4d4; }"
        "QMenuBar::item:selected { background-color: #3c3c3c; }"
        "QMenu { background-color: #252526; color: #d4d4d4; }"
        "QMenu::item:selected { background-color: #007acc; }"
        "QToolBar { background-color: #252526; border: none; }"
        "QStatusBar { background-color: #252526; color: #9d9d9d; }"
        "QSplitter::handle { background-color: #3c3c3c; width: 2px; height: 2px; }"
    );

    setupUI();
    createToolBar();
    createMenuBar();
    createStatusBar();
    loadSettings();

    // Setup worker thread
    worker = std::make_unique<ExtractionWorker>();
    workerThread = std::make_unique<QThread>();
    worker->moveToThread(workerThread.get());

    connect(this, &MainWindow::extractionRequested, worker.get(), &ExtractionWorker::extract);
    connect(worker.get(), &ExtractionWorker::logMessage, this, &MainWindow::onLogMessage);
    connect(worker.get(), &ExtractionWorker::archiveStarted, this, &MainWindow::onArchiveStarted);
    connect(worker.get(), &ExtractionWorker::progressUpdated, this, &MainWindow::onExtractionProgress);
    connect(worker.get(), &ExtractionWorker::finished, this, &MainWindow::onExtractionFinished);
    connect(worker.get(), &ExtractionWorker::errorOccurred, this, &MainWindow::onExtractionError);

    workerThread->start();

    archiveTopPanel->installEventFilter(this);
    archiveList->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    saveSettings();
    if (workerThread && worker) {
        worker->cancel();
        worker->moveToThread(thread());
        workerThread->quit();
        workerThread->wait();
    }
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Create splitter for resizable panels
    QSplitter *splitter = new QSplitter(Qt::Vertical);

    // ===== TOP PANEL: Archive List =====
    archiveTopPanel = new QWidget();
    archiveTopPanel->setAcceptDrops(true);
    QVBoxLayout *topLayout = new QVBoxLayout(archiveTopPanel);
    topLayout->setContentsMargins(12, 12, 12, 12);
    topLayout->setSpacing(8);

    // Title
    QLabel *titleLabel = new QLabel("Archives");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    topLayout->addWidget(titleLabel);

    // Archive list table
    archiveList = new QTableWidget();
    archiveList->setColumnCount(4);
    archiveList->setHorizontalHeaderLabels({"", "Name", "Size", "Password"});
    archiveList->horizontalHeader()->setStretchLastSection(false);
    archiveList->setColumnWidth(0, 30);
    archiveList->setColumnWidth(1, 400);
    archiveList->setColumnWidth(2, 90);
    archiveList->setColumnWidth(3, 220);
    archiveList->setSelectionBehavior(QAbstractItemView::SelectRows);
    archiveList->setSelectionMode(QAbstractItemView::MultiSelection);
    archiveList->setAlternatingRowColors(true);
    archiveList->verticalHeader()->setVisible(false);
    archiveList->verticalHeader()->setDefaultSectionSize(40);
    archiveList->setShowGrid(false);
    archiveList->setAcceptDrops(true);
    archiveList->setStyleSheet(
        "QTableWidget { background-color: #2d2d2d; alternate-background-color: #252526; gridline-color: #3c3c3c; "
        "border: 1px solid #404040; border-radius: 4px; color: #d4d4d4; }"
        "QHeaderView::section { background-color: #3c3c3c; color: #d4d4d4; padding: 6px; border: none; "
        "border-right: 1px solid #404040; font-weight: bold; }"
        "QTableWidget::item { padding: 4px; color: #d4d4d4; }"
    );
    topLayout->addWidget(archiveList);

    // Toolbar for archive list
    QHBoxLayout *archiveToolbarLayout = new QHBoxLayout();
    archiveToolbarLayout->setSpacing(6);

    QPushButton *addBtn = new QPushButton("+ Add");
    addBtn->setMinimumHeight(32);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(
        "QPushButton { background-color: #007acc; color: white; border: none; border-radius: 3px; "
        "padding: 6px 16px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background-color: #1a8ad4; }"
        "QPushButton:pressed { background-color: #005a9e; }"
    );
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddArchives);

    QPushButton *removeBtn = new QPushButton("- Remove");
    removeBtn->setMinimumHeight(32);
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setStyleSheet(
        "QPushButton { background-color: #c5282c; color: white; border: none; border-radius: 3px; "
        "padding: 6px 16px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background-color: #d13438; }"
        "QPushButton:pressed { background-color: #a4373a; }"
    );
    connect(removeBtn, &QPushButton::clicked, [this]() {
        QModelIndexList selected = archiveList->selectionModel()->selectedRows();
        for (int i = selected.count() - 1; i >= 0; --i) {
            archiveList->removeRow(selected[i].row());
        }
        updateArchiveCount();
    });

    QPushButton *clearBtn = new QPushButton("Clear All");
    clearBtn->setMinimumHeight(32);
    clearBtn->setCursor(Qt::PointingHandCursor);
    clearBtn->setStyleSheet(
        "QPushButton { background-color: #5c5c5c; color: white; border: none; border-radius: 3px; "
        "padding: 6px 16px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background-color: #6e6e6e; }"
        "QPushButton:pressed { background-color: #4e4e4e; }"
    );
    connect(clearBtn, &QPushButton::clicked, [this]() {
        archiveList->setRowCount(0);
        updateArchiveCount();
    });

    archiveToolbarLayout->addWidget(addBtn);
    archiveToolbarLayout->addWidget(removeBtn);
    archiveToolbarLayout->addWidget(clearBtn);
    archiveToolbarLayout->addStretch();

    QLabel *countLabel = new QLabel("0 archives");
    countLabel->setStyleSheet("color: #9d9d9d; font-size: 10px;");
    archiveToolbarLayout->addWidget(countLabel);

    topLayout->addLayout(archiveToolbarLayout);
    splitter->addWidget(archiveTopPanel);

    // ===== BOTTOM PANEL: Options & Progress =====
    QWidget *bottomPanel = new QWidget();
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomPanel);
    bottomLayout->setContentsMargins(12, 12, 12, 12);
    bottomLayout->setSpacing(12);

    // Options section
    QLabel *optionsLabel = new QLabel("Options");
    optionsLabel->setFont(titleFont);
    bottomLayout->addWidget(optionsLabel);

    QHBoxLayout *optionsLayout = new QHBoxLayout();
    optionsLayout->setSpacing(16);

    // Output path
    QVBoxLayout *outputLayout = new QVBoxLayout();
    QLabel *outputLabel = new QLabel("Extract to (each archive → own subfolder):");
    outputLabel->setStyleSheet("color: #9d9d9d; font-weight: bold; font-size: 10px;");
    outputLayout->addWidget(outputLabel);

    QHBoxLayout *outputPathLayout = new QHBoxLayout();
    outputPathEdit = new QLineEdit();
    outputPathEdit->setText(outputPath);
    outputPathEdit->setReadOnly(true);
    outputPathEdit->setStyleSheet(
        "QLineEdit { background-color: #3c3c3c; border: 1px solid #555; border-radius: 3px; "
        "padding: 6px; color: #d4d4d4; }"
    );
    outputPathLayout->addWidget(outputPathEdit);

    selectOutputBtn = new QPushButton("Browse...");
    selectOutputBtn->setMaximumWidth(90);
    selectOutputBtn->setMinimumHeight(28);
    selectOutputBtn->setStyleSheet(
        "QPushButton { background-color: #3c3c3c; color: #d4d4d4; border: 1px solid #555; "
        "border-radius: 3px; padding: 4px 12px; font-weight: bold; font-size: 10px; }"
        "QPushButton:hover { background-color: #4e4e4e; border-color: #666; }"
    );
    connect(selectOutputBtn, &QPushButton::clicked, this, &MainWindow::onSelectOutputFolder);
    outputPathLayout->addWidget(selectOutputBtn);
    outputLayout->addLayout(outputPathLayout);
    optionsLayout->addLayout(outputLayout);

    // Checkboxes
    QVBoxLayout *checksLayout = new QVBoxLayout();
    deleteAfterCheck = new QCheckBox("Delete archives after extraction");
    deleteAfterCheck->setToolTip("Remove zip/archive files after successful extraction");
    checksLayout->addWidget(deleteAfterCheck);

    overwriteCheck = new QCheckBox("Overwrite existing");
    checksLayout->addWidget(overwriteCheck);
    optionsLayout->addLayout(checksLayout);

    optionsLayout->addStretch();
    bottomLayout->addLayout(optionsLayout);

    // Progress section
    QLabel *progressSectionLabel = new QLabel("Progress");
    progressSectionLabel->setFont(titleFont);
    bottomLayout->addWidget(progressSectionLabel);

    progressLabel = new QLabel("Ready");
    progressLabel->setStyleSheet("font-size: 11px; color: #d4d4d4;");
    bottomLayout->addWidget(progressLabel);

    progressBar = new QProgressBar();
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(0);
    progressBar->setMinimumHeight(24);
    progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #555; border-radius: 3px; background-color: #3c3c3c; "
        "text-align: center; color: #d4d4d4; }"
        "QProgressBar::chunk { background-color: #007acc; border-radius: 2px; }"
    );
    bottomLayout->addWidget(progressBar);

    QHBoxLayout *statsLayout = new QHBoxLayout();
    speedLabel = new QLabel("Speed: -- MB/s");
    speedLabel->setStyleSheet("font-size: 10px; color: #9d9d9d;");
    statsLayout->addWidget(speedLabel);
    statsLayout->addStretch();
    etaLabel = new QLabel("ETA: --");
    etaLabel->setStyleSheet("font-size: 10px; color: #9d9d9d;");
    statsLayout->addWidget(etaLabel);
    bottomLayout->addLayout(statsLayout);

    // Extract buttons
    QHBoxLayout *extractButtonsLayout = new QHBoxLayout();
    extractButtonsLayout->setSpacing(8);

    extractBtn = new QPushButton("Extract");
    extractBtn->setMinimumHeight(36);
    extractBtn->setMinimumWidth(120);
    extractBtn->setCursor(Qt::PointingHandCursor);
    extractBtn->setStyleSheet(
        "QPushButton { background-color: #16825d; color: white; border: none; border-radius: 3px; "
        "font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background-color: #1a9a6d; }"
        "QPushButton:pressed { background-color: #126b48; }"
        "QPushButton:disabled { background-color: #4e4e4e; color: #808080; }"
    );
    connect(extractBtn, &QPushButton::clicked, this, &MainWindow::onExtract);

    cancelBtn = new QPushButton("Cancel");
    cancelBtn->setMinimumHeight(36);
    cancelBtn->setMinimumWidth(120);
    cancelBtn->setEnabled(false);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(
        "QPushButton { background-color: #c5282c; color: white; border: none; border-radius: 3px; "
        "font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background-color: #d13438; }"
        "QPushButton:pressed { background-color: #a4373a; }"
        "QPushButton:disabled { background-color: #4e4e4e; color: #808080; }"
    );
    connect(cancelBtn, &QPushButton::clicked, this, &MainWindow::onCancel);

    extractButtonsLayout->addStretch();
    extractButtonsLayout->addWidget(extractBtn);
    extractButtonsLayout->addWidget(cancelBtn);
    bottomLayout->addLayout(extractButtonsLayout);

    // Logs section
    QLabel *logsLabel = new QLabel("Logs");
    logsLabel->setFont(titleFont);
    bottomLayout->addWidget(logsLabel);

    logView = new QPlainTextEdit();
    logView->setReadOnly(true);
    logView->setMinimumHeight(120);
    logView->setMaximumHeight(200);
    logView->setStyleSheet(
        "QPlainTextEdit { background-color: #1e1e1e; color: #9d9d9d; font-family: monospace; font-size: 10px; "
        "border: 1px solid #404040; border-radius: 3px; padding: 6px; }"
    );
    logView->setPlaceholderText("Extraction logs will appear here...");
    bottomLayout->addWidget(logView);

    QPushButton *clearLogBtn = new QPushButton("Clear Logs");
    clearLogBtn->setMaximumWidth(100);
    clearLogBtn->setStyleSheet(
        "QPushButton { background-color: #3c3c3c; color: #d4d4d4; border: 1px solid #555; "
        "border-radius: 3px; padding: 4px 12px; font-size: 10px; }"
        "QPushButton:hover { background-color: #4e4e4e; }"
    );
    connect(clearLogBtn, &QPushButton::clicked, [this]() { logView->clear(); });
    bottomLayout->addWidget(clearLogBtn);

    splitter->addWidget(bottomPanel);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
    setCentralWidget(centralWidget);
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar("Main Toolbar");
    toolBar->setIconSize(QSize(16, 16));
    toolBar->setMovable(false);
    toolBar->setFloatable(false);

    QAction *openAction = toolBar->addAction("Open");
    connect(openAction, &QAction::triggered, this, &MainWindow::onAddArchives);

    toolBar->addSeparator();

    QAction *settingsAction = toolBar->addAction("Settings");
    connect(settingsAction, &QAction::triggered, [this]() {
        SettingsDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            loadSettings();
        }
    });
}

void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *fileMenu = menuBar->addMenu("File");
    QAction *openAction = fileMenu->addAction("Open Archives...");
    connect(openAction, &QAction::triggered, this, &MainWindow::onAddArchives);
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *editMenu = menuBar->addMenu("Edit");
    QAction *settingsAction = editMenu->addAction("Settings");
    connect(settingsAction, &QAction::triggered, [this]() {
        SettingsDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            loadSettings();
        }
    });

    QMenu *helpMenu = menuBar->addMenu("Help");
    QAction *aboutAction = helpMenu->addAction("About");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About Unzipper Pro",
            "Unzipper Pro 2.0\n\n"
            "Professional Archive Extraction Tool\n\n"
            "Supports: ZIP, 7Z, TAR, TAR.GZ, TAR.BZ2, TAR.XZ\n"
            "Handles files up to 100GB+\n\n"
            "© 2024");
    });
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage("Ready");
    statusBar()->setStyleSheet("color: #9d9d9d; font-size: 10px;");
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == archiveTopPanel || watched == archiveList) {
        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent *e = static_cast<QDragEnterEvent*>(event);
            if (e->mimeData()->hasUrls())
                e->acceptProposedAction();
            return true;
        }
        if (event->type() == QEvent::Drop) {
            QDropEvent *e = static_cast<QDropEvent*>(event);
            QStringList files;
            const QStringList exts = {"zip", "7z", "rar", "cbr", "tar", "gz", "bz2", "xz", "tgz", "tbz2", "txz"};
            for (const QUrl &url : e->mimeData()->urls()) {
                if (url.isLocalFile()) {
                    QString path = url.toLocalFile();
                    QFileInfo fi(path);
                    if (fi.isFile()) {
                        QString ext = fi.suffix().toLower();
                        bool isArchive = exts.contains(ext)
                            || path.endsWith(".tar.gz", Qt::CaseInsensitive)
                            || path.endsWith(".tar.bz2", Qt::CaseInsensitive)
                            || path.endsWith(".tar.xz", Qt::CaseInsensitive);
                        if (isArchive)
                            files.append(path);
                    }
                }
            }
            if (!files.isEmpty())
                addArchivesToList(files);
            e->acceptProposedAction();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::addArchivesToList(const QStringList &files)
{
    QSet<QString> existingPaths;
    for (int r = 0; r < archiveList->rowCount(); ++r) {
        QTableWidgetItem *item = archiveList->item(r, 1);
        if (item) {
            QString p = QFileInfo(item->data(Qt::UserRole).toString()).canonicalFilePath();
            if (p.isEmpty())
                p = item->data(Qt::UserRole).toString();
            existingPaths.insert(p);
        }
    }
    for (const QString &file : files) {
        QString canonical = QFileInfo(file).canonicalFilePath();
        if (canonical.isEmpty())
            canonical = file;
        if (existingPaths.contains(canonical))
            continue;
        existingPaths.insert(canonical);
        int row = archiveList->rowCount();
        archiveList->insertRow(row);

        QCheckBox *cb = new QCheckBox();
        cb->setChecked(true);
        archiveList->setCellWidget(row, 0, cb);

        QFileInfo info(file);
        QTableWidgetItem *nameItem = new QTableWidgetItem(info.fileName());
        nameItem->setData(Qt::UserRole, file);
        archiveList->setItem(row, 1, nameItem);

        qint64 size = info.size();
        QString sizeStr;
        if (size > 1e9) sizeStr = QString::number(size / 1e9, 'f', 2) + " GB";
        else if (size > 1e6) sizeStr = QString::number(size / 1e6, 'f', 2) + " MB";
        else sizeStr = QString::number(size / 1e3, 'f', 2) + " KB";

        QTableWidgetItem *sizeItem = new QTableWidgetItem(sizeStr);
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        archiveList->setItem(row, 2, sizeItem);

        QWidget *pwWidget = new QWidget();
        pwWidget->setAutoFillBackground(false);
        QHBoxLayout *pwLayout = new QHBoxLayout(pwWidget);
        pwLayout->setContentsMargins(4, 2, 4, 2);
        pwLayout->setSpacing(4);
        QLineEdit *passwordEdit = new QLineEdit();
        passwordEdit->setObjectName("passwordEdit");
        passwordEdit->setEchoMode(QLineEdit::Password);
        QFont pwFont = passwordEdit->font();
        pwFont.setPointSize(10);
        passwordEdit->setFont(pwFont);
        passwordEdit->setMinimumWidth(140);
        passwordEdit->setMinimumHeight(24);
        passwordEdit->setStyleSheet(
            "QLineEdit { background-color: #2d2d2d; color: #d4d4d4; border: 1px solid #454545; "
            "border-radius: 2px; padding: 3px 6px; selection-background-color: #007acc; }"
            "QLineEdit:focus { border-color: #5a5a5a; }"
        );
        pwLayout->addWidget(passwordEdit, 1);
        QToolButton *showPwBtn = new QToolButton();
        showPwBtn->setText("Show");
        showPwBtn->setToolTip("Show password");
        showPwBtn->setCheckable(true);
        showPwBtn->setCursor(Qt::PointingHandCursor);
        showPwBtn->setStyleSheet(
            "QToolButton { background: #2d2d2d; border: 1px solid #454545; border-radius: 2px; padding: 3px 8px; font-size: 10px; }"
            "QToolButton:hover { border-color: #5a5a5a; background: #353535; }"
            "QToolButton:checked { background: #007acc; border-color: #007acc; color: white; }"
        );
        connect(showPwBtn, &QToolButton::toggled, [passwordEdit, showPwBtn](bool checked) {
            passwordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
            showPwBtn->setText(checked ? "Hide" : "Show");
            showPwBtn->setToolTip(checked ? "Hide password" : "Show password");
        });
        pwLayout->addWidget(showPwBtn);
        archiveList->setCellWidget(row, 3, pwWidget);
    }
    updateArchiveCount();
}

void MainWindow::onAddArchives()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
        "Select Archives", QDir::homePath(),
        "Archives (*.zip *.7z *.rar *.cbr *.tar *.tar.gz *.tgz *.tar.bz2 *.tar.xz);;All Files (*)");
    if (!files.isEmpty())
        addArchivesToList(files);
}

void MainWindow::onSelectOutputFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Folder", outputPath);
    if (!dir.isEmpty()) {
        outputPath = dir;
        outputPathEdit->setText(outputPath);
    }
}

void MainWindow::onExtract()
{
    selectedArchives.clear();
    QStringList passwords;
    for (int i = 0; i < archiveList->rowCount(); ++i) {
        QCheckBox *cb = qobject_cast<QCheckBox*>(archiveList->cellWidget(i, 0));
        if (cb && cb->isChecked()) {
            QTableWidgetItem *item = archiveList->item(i, 1);
            selectedArchives.append(item->data(Qt::UserRole).toString());
            QWidget *pwCell = archiveList->cellWidget(i, 3);
            QLineEdit *pwEdit = pwCell ? pwCell->findChild<QLineEdit*>("passwordEdit") : nullptr;
            passwords.append(pwEdit ? pwEdit->text() : QString());
        }
    }

    if (selectedArchives.isEmpty()) {
        QMessageBox::warning(this, "No Archives", "Please select at least one archive.");
        return;
    }

    extractBtn->setEnabled(false);
    cancelBtn->setEnabled(true);
    progressBar->setValue(0);
    progressLabel->setText("Extracting...");
    m_currentArchiveIndex = 0;
    m_currentArchiveTotal = 0;
    m_currentArchiveName.clear();

    appendLog(QString("Starting extraction: %1 archive(s) → %2").arg(selectedArchives.size()).arg(outputPath));
    QSettings settings("UnzipperPro", "UnzipperPro");
    int ideal = qMin(8, qMax(1, QThread::idealThreadCount()));
    int maxParallel = settings.value("maxParallelExtractions", ideal).toInt();
    maxParallel = qBound(1, maxParallel, 8);
    bool verifyAfterExtract = settings.value("verifyAfterExtract", false).toBool();
    emit extractionRequested(selectedArchives, passwords, outputPath, deleteAfterCheck->isChecked(), overwriteCheck->isChecked(), maxParallel, verifyAfterExtract);
}

void MainWindow::onCancel()
{
    worker->cancel();
}

void MainWindow::onLogMessage(const QString &message)
{
    appendLog(message, "INFO");
}

void MainWindow::appendLog(const QString &message, const QString &level)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString prefix = level == "ERROR" ? "[ERROR]" : (level == "WARN" ? "[WARN] " : "");
    logView->appendPlainText(QString("[%1] %2%3").arg(timestamp, prefix, message));
    logView->verticalScrollBar()->setValue(logView->verticalScrollBar()->maximum());
}

void MainWindow::onArchiveStarted(int index, int total, const QString &archiveName)
{
    m_currentArchiveIndex = index;
    m_currentArchiveTotal = total;
    m_currentArchiveName = archiveName;
    progressLabel->setText(QString("Extracting %1/%2: %3").arg(index).arg(total).arg(archiveName));
}

void MainWindow::onExtractionProgress(qint64 current, qint64 total, const QString &file, double speed)
{
    int percent = 0;
    if (total > 0) {
        percent = (current >= total) ? 100 : qBound(0, (int)((current * 100LL) / total), 100);
    }
    progressBar->setValue(percent);
    if (m_currentArchiveTotal > 0)
        progressLabel->setText(QString("Extracting %1/%2: %3 (%4%)").arg(m_currentArchiveIndex).arg(m_currentArchiveTotal).arg(m_currentArchiveName).arg(percent));
    else
        progressLabel->setText(QString("Extracting: %1 (%2%)").arg(file).arg(percent));
    speedLabel->setText(QString("Speed: %1 MB/s").arg(speed, 0, 'f', 1));

    qint64 remainingBytes = qMax(0LL, (qint64)total - (qint64)current);
    int remainingSeconds = (speed > 0 && remainingBytes > 0) ? (int)(remainingBytes / (speed * 1e6)) : 0;
    remainingSeconds = qMax(0, remainingSeconds);
    int minutes = remainingSeconds / 60;
    int seconds = remainingSeconds % 60;
    etaLabel->setText(percent >= 100 ? QString("ETA: --") : QString("ETA: %1m %2s").arg(minutes).arg(seconds));
}

void MainWindow::onExtractionFinished(bool success, const QString &message)
{
    extractBtn->setEnabled(true);
    cancelBtn->setEnabled(false);
    appendLog(message, success ? "INFO" : "ERROR");

    if (success) {
        progressBar->setValue(100);
        progressLabel->setText("Extraction completed successfully!");
        if (playSoundOnComplete())
            QApplication::beep();
        if (clearListAfterExtract())
            archiveList->setRowCount(0);
        updateArchiveCount();
        QMessageBox::information(this, "Success", message);
    } else {
        progressLabel->setText("Extraction failed");
        QMessageBox::critical(this, "Error", message);
    }
}

void MainWindow::onExtractionError(const QString &error)
{
    extractBtn->setEnabled(true);
    cancelBtn->setEnabled(false);
    progressLabel->setText("Error during extraction");
    appendLog(error, "ERROR");
    QMessageBox::critical(this, "Error", error);
}

void MainWindow::onArchiveListChanged()
{
    updateArchiveCount();
}

void MainWindow::updateArchiveCount()
{
    int count = 0;
    for (int i = 0; i < archiveList->rowCount(); ++i) {
        QCheckBox *cb = qobject_cast<QCheckBox*>(archiveList->cellWidget(i, 0));
        if (cb && cb->isChecked()) count++;
    }
    statusBar()->showMessage(QString("%1 archive(s) selected").arg(count));
}

void MainWindow::loadSettings()
{
    QSettings settings("UnzipperPro", "UnzipperPro");
    outputPath = settings.value("outputPath", settings.value("defaultOutputPath", QDir::homePath()).toString()).toString();
    outputPathEdit->setText(outputPath);
    deleteAfterCheck->setChecked(settings.value("defaultDeleteAfter", false).toBool());
    overwriteCheck->setChecked(settings.value("defaultOverwrite", false).toBool());
    restoreGeometry(settings.value("geometry", QByteArray()).toByteArray());
}

void MainWindow::saveSettings()
{
    QSettings settings("UnzipperPro", "UnzipperPro");
    settings.setValue("outputPath", outputPath);
    settings.setValue("geometry", saveGeometry());
}

bool MainWindow::clearListAfterExtract() const
{
    return QSettings("UnzipperPro", "UnzipperPro").value("clearListAfterExtract", false).toBool();
}

bool MainWindow::playSoundOnComplete() const
{
    return QSettings("UnzipperPro", "UnzipperPro").value("playSoundOnComplete", true).toBool();
}
