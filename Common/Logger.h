#ifndef DNC_SCADA_LOGGER_H
#define DNC_SCADA_LOGGER_H

#include <QString>

#define LOG_INFO(message) DncScada::AsyncLogger::instance().log(QStringLiteral("INFO"), QStringLiteral(message), QStringLiteral(__FILE__), __LINE__)
#define LOG_WARN(message) DncScada::AsyncLogger::instance().log(QStringLiteral("WARN"), QStringLiteral(message), QStringLiteral(__FILE__), __LINE__)
#define LOG_ERROR(message) DncScada::AsyncLogger::instance().log(QStringLiteral("ERROR"), QStringLiteral(message), QStringLiteral(__FILE__), __LINE__)

namespace DncScada {

class AsyncLogger
{
public:
    static AsyncLogger &instance();

    void log(const QString &level, const QString &message, const QString &file, int line);
    void shutdown();

private:
    AsyncLogger();
    ~AsyncLogger();
    AsyncLogger(const AsyncLogger &) = delete;
    AsyncLogger &operator=(const AsyncLogger &) = delete;

    class Impl;
    Impl *d;
};

} // namespace DncScada

#endif // DNC_SCADA_LOGGER_H
