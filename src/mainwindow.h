#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGlobal>
#include <QTableWidget>
#include <QUrl>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QThread>
#include <memory>
#include "extractionworker.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

signals:
    void extractionRequested(const QStringList &archives, const QStringList &passwords, const QString &outputPath, bool deleteAfter, bool overwrite, int maxParallel, bool verifyAfterExtract);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onAddArchives();
    void onSelectOutputFolder();
    void onExtract();
    void onCancel();
    void onArchiveStarted(int index, int total, const QString &archiveName);
    void onExtractionProgress(qint64 current, qint64 total, const QString &file, double speed);
    void onExtractionFinished(bool success, const QString &message);
    void onExtractionError(const QString &error);
    void onLogMessage(const QString &message);
    void onArchiveListChanged();

    bool clearListAfterExtract() const;
    bool playSoundOnComplete() const;

private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void updateArchiveCount();
    void loadSettings();
    void saveSettings();
    void addArchivesToList(const QStringList &files);
    void appendLog(const QString &message, const QString &level = "INFO");

    // UI Components
    QTableWidget *archiveList;
    QLineEdit *outputPathEdit;
    QProgressBar *progressBar;
    QLabel *progressLabel;
    QLabel *speedLabel;
    QLabel *etaLabel;
    QWidget *archiveTopPanel;
    QPushButton *addBtn;
    QPushButton *extractBtn;
    QPushButton *cancelBtn;
    QPushButton *selectOutputBtn;
    QCheckBox *deleteAfterCheck;
    QCheckBox *overwriteCheck;
    QPlainTextEdit *logView;

    // Worker
    std::unique_ptr<ExtractionWorker> worker;
    std::unique_ptr<QThread> workerThread;

    // State
    QString outputPath;
    QStringList selectedArchives;
    int m_currentArchiveIndex{0};
    int m_currentArchiveTotal{0};
    QString m_currentArchiveName;
};

#endif // MAINWINDOW_H
