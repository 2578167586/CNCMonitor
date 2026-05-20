#include "Logger.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QQueue>
#include <QTextStream>
#include <QWaitCondition>

#include <atomic>
#include <thread>

namespace DncScada {

class AsyncLogger::Impl
{
public:
    Impl()
    {
        worker = std::thread([this]() { run(); });
    }

    ~Impl()
    {
        stop();
    }

    void enqueue(const QString &line)
    {
        QMutexLocker locker(&mutex);
        queue.enqueue(line);
        condition.wakeOne();
    }

    void stop()
    {
        if (stopped.exchange(true)) {
            return;
        }
        condition.wakeAll();
        if (worker.joinable()) {
            worker.join();
        }
    }

private:
    void run()
    {
        while (!stopped.load()) {
            QQueue<QString> batch;
            {
                QMutexLocker locker(&mutex);
                if (queue.isEmpty()) {
                    condition.wait(&mutex, 500);
                }
                qSwap(batch, queue);
            }
            flush(batch);
        }

        QQueue<QString> tail;
        {
            QMutexLocker locker(&mutex);
            qSwap(tail, queue);
        }
        flush(tail);
    }

    QString logDirectory() const
    {
        const QString base = QCoreApplication::applicationDirPath().isEmpty()
            ? QDir::currentPath()
            : QCoreApplication::applicationDirPath();
        QDir dir(base);
        if (!dir.exists(QStringLiteral("logs"))) {
            dir.mkpath(QStringLiteral("logs"));
        }
        return dir.filePath(QStringLiteral("logs"));
    }

    QString currentLogPath()
    {
        const QString date = QDate::currentDate().toString(QStringLiteral("yyyyMMdd"));
        QDir dir(logDirectory());
        QString candidate = dir.filePath(QStringLiteral("dnc_scada_%1.log").arg(date));
        QFileInfo info(candidate);
        if (info.exists() && info.size() >= 10 * 1024 * 1024) {
            int index = 1;
            do {
                candidate = dir.filePath(QStringLiteral("dnc_scada_%1_%2.log").arg(date).arg(index++));
                info.setFile(candidate);
            } while (info.exists() && info.size() >= 10 * 1024 * 1024);
        }
        return candidate;
    }

    void flush(const QQueue<QString> &batch)
    {
        if (batch.isEmpty()) {
            return;
        }

        QFile file(currentLogPath());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            return;
        }

        QTextStream stream(&file);
        for (const QString &line : batch) {
            stream << line << '\n';
        }
    }

    QMutex mutex;
    QWaitCondition condition;
    QQueue<QString> queue;
    std::atomic_bool stopped { false };
    std::thread worker;
};

AsyncLogger &AsyncLogger::instance()
{
    static AsyncLogger logger;
    return logger;
}

AsyncLogger::AsyncLogger()
    : d(new Impl)
{
}

AsyncLogger::~AsyncLogger()
{
    shutdown();
    delete d;
}

void AsyncLogger::log(const QString &level, const QString &message, const QString &file, int line)
{
    const QString formatted = QStringLiteral("%1 [%2] %3 (%4:%5)")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
             level,
             message,
             QFileInfo(file).fileName())
        .arg(line);
    d->enqueue(formatted);
}

void AsyncLogger::shutdown()
{
    d->stop();
}

} // namespace DncScada
