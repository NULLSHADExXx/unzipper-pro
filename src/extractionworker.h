#ifndef EXTRACTIONWORKER_H
#define EXTRACTIONWORKER_H

#include <QObject>
#include <QStringList>
#include <QtGlobal>
#include <atomic>
#include <chrono>

class ExtractionWorker : public QObject {
    Q_OBJECT

public:
    ExtractionWorker();
    ~ExtractionWorker();

    bool isCancelled() const { return m_cancelled; }

public slots:
    void extract(const QStringList &archives, const QStringList &passwords, const QString &outputPath, bool deleteAfter, bool overwrite, int maxParallel = -1, bool verifyAfterExtract = false);
    void cancel();
    void reportError(const QString &msg) { emit errorOccurred(msg); }

signals:
    void logMessage(const QString &message);
    void archiveStarted(int index, int total, const QString &archiveName);
    void progressUpdated(qint64 current, qint64 total, const QString &file, double speed);
    void finished(bool success, const QString &message);
    void errorOccurred(const QString &error);

private:
    bool extractOneArchive(const QString &archive, const QString &password, const QString &outputBasePath, bool overwrite);
    bool extractWithLibarchive(const QString &archivePath, const QString &outputPath, bool overwrite, const QString &password, int format);
    bool verifyArchive(const QString &archivePath, const QString &password);

    std::atomic<bool> m_cancelled{false};
    std::atomic<long long> m_totalExtracted{0};
    std::atomic<long long> m_totalSize{0};
    std::atomic<uint64_t> m_lastProgressEmitMs{0};
    std::chrono::steady_clock::time_point m_extractStartTime;

    void maybeEmitProgress(const QString &entryPath);
};

#endif // EXTRACTIONWORKER_H
