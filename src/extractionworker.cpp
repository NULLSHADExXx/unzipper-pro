#include "extractionworker.h"
#include <QtConcurrent>
#include <QThreadPool>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <archive.h>
#include <archive_entry.h>
#include <cerrno>
#include <zlib.h>
#include <bzlib.h>
#include <lzma.h>
#include <cstring>
#include <chrono>
#include <QTemporaryDir>
#include <QMetaObject>
#include <QStandardPaths>

const int CHUNK_SIZE = 256 * 1024;

enum ArchiveFormat {
    FmtZip, FmtSevenZ, FmtRar, FmtTar, FmtTarGz, FmtTarBz2, FmtTarXz
};

static bool setupArchiveFormat(struct archive *a, int format, const QString &password)
{
    switch (format) {
    case FmtZip:
        archive_read_support_format_zip(a);
        archive_read_support_filter_all(a);
        break;
    case FmtSevenZ:
        archive_read_support_format_7zip(a);
        archive_read_support_filter_all(a);
        break;
    case FmtRar:
        archive_read_support_format_rar(a);
        archive_read_support_format_rar5(a);
        archive_read_support_filter_all(a);
        break;
    case FmtTar:
        archive_read_support_format_tar(a);
        archive_read_support_filter_all(a);
        break;
    case FmtTarGz:
        archive_read_support_format_tar(a);
        archive_read_support_filter_gzip(a);
        break;
    case FmtTarBz2:
        archive_read_support_format_tar(a);
        archive_read_support_filter_bzip2(a);
        break;
    case FmtTarXz:
        archive_read_support_format_tar(a);
        archive_read_support_filter_xz(a);
        break;
    default:
        return false;
    }
    if (format == FmtZip || format == FmtSevenZ || format == FmtRar) {
        if (!password.isEmpty()) {
            QByteArray pwBytes = password.toUtf8();
            if (archive_read_add_passphrase(a, pwBytes.constData()) != ARCHIVE_OK) {
                return false;
            }
        }
    }
    return true;
}

static QString libarchiveError(struct archive *a, const QString &context)
{
    const char *msg = archive_error_string(a);
    int err = archive_errno(a);
    QString detail = msg ? QString::fromUtf8(msg) : QString();
    QString fileName = QFileInfo(context).fileName();

    if (detail.contains("passphrase", Qt::CaseInsensitive) || detail.contains("password", Qt::CaseInsensitive)
        || detail.contains("decryption", Qt::CaseInsensitive) || detail.contains("crypt", Qt::CaseInsensitive))
        return QString("%1: Wrong password or decryption failed").arg(fileName);
    if (err == EACCES || detail.contains("Permission denied", Qt::CaseInsensitive))
        return QString("%1: Permission denied").arg(fileName);
    if (err == ENOENT)
        return QString("%1: File not found").arg(fileName);
    if (err == ENOSPC)
        return QString("%1: No space left on disk").arg(fileName);
    if (detail.contains("truncated", Qt::CaseInsensitive) || detail.contains("corrupt", Qt::CaseInsensitive)
        || detail.contains("invalid", Qt::CaseInsensitive))
        return QString("%1: Archive may be corrupted").arg(fileName);
    if (!detail.isEmpty())
        return QString("%1: %2").arg(fileName, detail);
    return QString("%1: Unknown error").arg(fileName);
}

static QString archiveBaseName(const QString &archivePath)
{
    QString name = QFileInfo(archivePath).fileName();
    if (name.endsWith(".tar.gz", Qt::CaseInsensitive) || name.endsWith(".tgz", Qt::CaseInsensitive))
        return name.left(name.length() - (name.endsWith(".tgz", Qt::CaseInsensitive) ? 4 : 7));
    if (name.endsWith(".tar.bz2", Qt::CaseInsensitive))
        return name.left(name.length() - 8);
    if (name.endsWith(".tar.xz", Qt::CaseInsensitive))
        return name.left(name.length() - 7);
    return QFileInfo(archivePath).completeBaseName();
}

ExtractionWorker::ExtractionWorker() : QObject() {}

ExtractionWorker::~ExtractionWorker() {}

bool ExtractionWorker::extractOneArchive(const QString &archive, const QString &password, const QString &outputBasePath, bool overwrite)
{
    QFileInfo info(archive);
    QString ext = info.suffix().toLower();
    QString archiveFolder = QDir(outputBasePath).filePath(archiveBaseName(archive));
    QDir().mkpath(archiveFolder);

    int format = -1;
    if (ext == "zip")
        format = FmtZip;
    else if (ext == "7z")
        format = FmtSevenZ;
    else if (ext == "rar" || ext == "cbr")
        format = FmtRar;
    else if (ext == "tar")
        format = FmtTar;
    else if ((ext == "gz" && archive.endsWith(".tar.gz", Qt::CaseInsensitive)) || ext == "tgz")
        format = FmtTarGz;
    else if (ext == "bz2" && archive.endsWith(".tar.bz2", Qt::CaseInsensitive))
        format = FmtTarBz2;
    else if (ext == "xz" && archive.endsWith(".tar.xz", Qt::CaseInsensitive))
        format = FmtTarXz;
    if (format < 0)
        return false;
    return extractWithLibarchive(archive, archiveFolder, overwrite, password, format);
}

bool ExtractionWorker::verifyArchive(const QString &archivePath, const QString &password)
{
    struct archive *a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    if (!password.isEmpty()) {
        QByteArray pwBytes = password.toUtf8();
        archive_read_add_passphrase(a, pwBytes.constData());
    }
    int r = archive_read_open_filename(a, archivePath.toStdString().c_str(), 10240);
    if (r != ARCHIVE_OK) {
        emit logMessage(QString("  ⚠ %1: Verification failed - %2").arg(QFileInfo(archivePath).fileName(), libarchiveError(a, archivePath)));
        archive_read_free(a);
        return false;
    }
    struct archive_entry *entry;
    char buffer[CHUNK_SIZE];
    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        if (m_cancelled) {
            archive_read_free(a);
            return false;
        }
        while (archive_read_data(a, buffer, sizeof(buffer)) > 0)
            ;
    }
    archive_read_free(a);
    if (r != ARCHIVE_EOF) {
        emit logMessage(QString("  ⚠ %1: Verification failed").arg(QFileInfo(archivePath).fileName()));
        return false;
    }
    return true;
}

bool ExtractionWorker::extractWithLibarchive(const QString &archivePath, const QString &outputPath, bool overwrite, const QString &password, int format)
{
    struct archive *a = archive_read_new();
    if (!setupArchiveFormat(a, format, password)) {
        archive_read_free(a);
        return false;
    }
    int r = archive_read_open_filename(a, archivePath.toStdString().c_str(), 65536);
    if (r != ARCHIVE_OK) {
        emit errorOccurred(libarchiveError(a, archivePath));
        archive_read_free(a);
        return false;
    }
    struct archive *ext = archive_write_disk_new();
    int extractFlags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_SECURE_NODOTDOT;
    if (!overwrite)
        extractFlags |= ARCHIVE_EXTRACT_NO_OVERWRITE;
    archive_write_disk_set_options(ext, extractFlags);
    archive_write_disk_set_standard_lookup(ext);

    struct archive_entry *entry;
    char buffer[CHUNK_SIZE];
    la_ssize_t size;
    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        if (m_cancelled) {
            archive_write_free(ext);
            archive_read_free(a);
            return false;
        }
        QString entryPath = QString::fromUtf8(archive_entry_pathname(entry));
        QString fullPath = QDir(outputPath).filePath(QDir::cleanPath(entryPath));
        if (fullPath.isEmpty() || !fullPath.startsWith(QDir::cleanPath(outputPath)))
            fullPath = QDir(outputPath).filePath(entryPath);
        QByteArray fullPathBytes = fullPath.toUtf8();
        archive_entry_set_pathname(entry, fullPathBytes.constData());

        r = archive_write_header(ext, entry);
        if (r < ARCHIVE_WARN) {
            emit errorOccurred(QString("%1: %2 - %3").arg(QFileInfo(archivePath).fileName(), entryPath, QString::fromUtf8(archive_error_string(ext))));
            archive_write_free(ext);
            archive_read_free(a);
            return false;
        }
        if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
            archive_read_data_skip(a);
        } else {
            while ((size = archive_read_data(a, buffer, sizeof(buffer))) > 0) {
                if (m_cancelled) {
                    archive_write_free(ext);
                    archive_read_free(a);
                    return false;
                }
                la_ssize_t written = archive_write_data(ext, buffer, size);
                if (written < 0) {
                    emit errorOccurred(QString("%1: Write error for %2").arg(QFileInfo(archivePath).fileName(), entryPath));
                    archive_write_free(ext);
                    archive_read_free(a);
                    return false;
                }
                m_totalExtracted += written;
                maybeEmitProgress(entryPath);
            }
            if (size < 0) {
                emit errorOccurred(QString("%1: Read error for %2").arg(QFileInfo(archivePath).fileName(), entryPath));
                archive_write_free(ext);
                archive_read_free(a);
                return false;
            }
        }

        r = archive_write_finish_entry(ext);
        if (r < ARCHIVE_WARN) {
            emit errorOccurred(QString("%1: %2 - %3").arg(QFileInfo(archivePath).fileName(), entryPath, QString::fromUtf8(archive_error_string(ext))));
            archive_write_free(ext);
            archive_read_free(a);
            return false;
        }
    }
    archive_write_free(ext);
    if (r != ARCHIVE_EOF) {
        emit errorOccurred(libarchiveError(a, archivePath));
        archive_read_free(a);
        return false;
    }
    archive_read_free(a);
    return true;
}

void ExtractionWorker::cancel()
{
    m_cancelled = true;
}

void ExtractionWorker::maybeEmitProgress(const QString &entryPath)
{
    const uint64_t kThrottleMs = 150;
    auto now = std::chrono::steady_clock::now();
    uint64_t elapsedMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_extractStartTime).count());
    uint64_t last = m_lastProgressEmitMs.load();
    if (elapsedMs >= last + kThrottleMs && m_lastProgressEmitMs.compare_exchange_strong(last, elapsedMs)) {
        double secs = elapsedMs / 1000.0;
        double speedMBs = (secs > 0) ? (m_totalExtracted.load() / 1e6) / secs : 0;
        emit progressUpdated(m_totalExtracted.load(), m_totalSize.load(), entryPath, speedMBs);
    }
}

void ExtractionWorker::extract(const QStringList &archives, const QStringList &passwords, const QString &outputPath, bool deleteAfter, bool overwrite, int maxParallel, bool verifyAfterExtract)
{
    m_cancelled = false;
    m_totalExtracted = 0;
    m_totalSize = 0;
    m_lastProgressEmitMs = 0;
    m_extractStartTime = std::chrono::steady_clock::now();

    try {
        // Calculate total size
        for (const QString &archive : archives) {
            QFileInfo info(archive);
            m_totalSize += info.size() * 3; // Estimate uncompressed size
        }

        int parallel = (maxParallel > 0 && maxParallel <= 8) ? maxParallel : qMin(8, qMax(1, QThread::idealThreadCount()));
        auto startTime = std::chrono::high_resolution_clock::now();
        int successCount = 0;
        QString lastError;

        struct Job {
            QString archive;
            QString password;
            bool verify;
        };
        QVector<Job> jobs;
        for (int i = 0; i < archives.size(); ++i) {
            jobs.append({archives[i], (i < passwords.size()) ? passwords[i] : QString(), verifyAfterExtract});
        }

        QThreadPool pool;
        pool.setMaxThreadCount(parallel);
        pool.setThreadPriority(QThread::LowPriority);
        ExtractionWorker *worker = this;
        for (int start = 0; start < jobs.size() && !m_cancelled; start += parallel) {
            int end = qMin(start + parallel, jobs.size());
            QVector<QFuture<bool>> futures;
            for (int i = start; i < end; ++i) {
                const Job &j = jobs[i];
                QString fileName = QFileInfo(j.archive).fileName();
                emit archiveStarted(i + 1, jobs.size(), fileName);
                emit logMessage(QString("Extracting %1/%2: %3").arg(i + 1).arg(jobs.size()).arg(fileName));
                futures.append(QtConcurrent::run(&pool, [worker, j, outputPath, overwrite, deleteAfter]() {
                    if (worker->isCancelled())
                        return false;
                    QString extractBase = outputPath;
                    std::unique_ptr<QTemporaryDir> tempDir;
                    if (deleteAfter) {
                        QDir outDir(outputPath);
                        outDir.mkpath(".");
                        QString tempTemplate = outDir.exists() ? outDir.filePath(".unzipper_temp_XXXXXX") : QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("unzipper_XXXXXX");
                        tempDir = std::make_unique<QTemporaryDir>(tempTemplate);
                        if (!tempDir->isValid())
                            return false;
                        extractBase = tempDir->path();
                    }
                    bool ok = worker->extractOneArchive(j.archive, j.password, extractBase, overwrite);
                    if (ok && deleteAfter && tempDir) {
                        if (j.verify && !worker->verifyArchive(j.archive, j.password)) {
                            tempDir->setAutoRemove(false);
                            return ok;
                        }
                        QString baseName = archiveBaseName(j.archive);
                        QString srcDir = QDir(extractBase).filePath(baseName);
                        QString destDir = QDir(outputPath).filePath(baseName);
                        if (QDir(destDir).exists() && !QDir(destDir).removeRecursively()) {
                            tempDir->setAutoRemove(false);
                            QMetaObject::invokeMethod(worker, "reportError", Qt::QueuedConnection, Q_ARG(QString, QString("Cannot replace %1. Extracted files kept in %2").arg(destDir, srcDir)));
                            return ok;
                        }
                        if (!QDir().rename(srcDir, destDir)) {
                            tempDir->setAutoRemove(false);
                            QMetaObject::invokeMethod(worker, "reportError", Qt::QueuedConnection, Q_ARG(QString, QString("Cannot move to %1. Extracted files kept in %2").arg(destDir, srcDir)));
                            return ok;
                        }
                        QFile::remove(j.archive);
                    }
                    return ok;
                }));
            }
            for (auto &f : futures) {
                f.waitForFinished();
                if (f.result())
                    successCount++;
            }
            for (int i = start; i < end; ++i) {
                QString fn = QFileInfo(jobs[i].archive).fileName();
                if (futures[i - start].result()) {
                    emit logMessage(QString("  ✓ %1").arg(fn));
                } else {
                    lastError = "Failed to extract: " + fn;
                    emit logMessage(QString("  ✗ %1 - %2").arg(fn).arg(lastError));
                }
            }
        }

        if (m_cancelled) {
            emit logMessage("Extraction cancelled by user");
            emit errorOccurred("Extraction cancelled by user");
            return;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();

        emit logMessage(QString("Done: %1/%2 archives in %3 seconds").arg(successCount).arg(archives.count()).arg(duration));

        if (successCount == archives.count()) {
            emit finished(true, QString("Successfully extracted %1 archive(s) in %2 seconds")
                .arg(successCount).arg(duration));
        } else {
            emit finished(false, QString("Extracted %1/%2 archives. Last error: %3")
                .arg(successCount).arg(archives.count()).arg(lastError));
        }

    } catch (const std::exception &e) {
        emit errorOccurred(QString("Exception: %1").arg(e.what()));
    } catch (...) {
        emit errorOccurred("Unknown error during extraction");
    }
}
