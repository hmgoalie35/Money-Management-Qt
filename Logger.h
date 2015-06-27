#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = 0);
    ~Logger();
    enum Level{
        WARNING,
        CRITICAL,
        INFO,
        DEBUG
    };
    
    QString get_file_name();
    void log(Level, QString msg, QString = QString());
private:
    QFile * log_file;
    QTextStream * stream;
};

#endif // LOGGER_H
