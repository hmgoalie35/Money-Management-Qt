#include "Logger.h"
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
Logger::Logger(QObject *parent) : QObject(parent)
{
//    QString log_folder_path = QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs");
    QString log_folder_path = QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/logs");
    QFileInfo log_folder(log_folder_path);
    if(!log_folder.exists()) QDir().mkdir(log_folder_path);
    QString file_name = QDir::fromNativeSeparators(log_folder_path + "/" + QDateTime::currentDateTime().toString("MMddyyyyhhmmssA") + ".txt");
    log_file = new QFile(file_name);
    stream = new QTextStream(log_file);
    if(!log_file->open(QFile::WriteOnly)) QMessageBox::critical(NULL, "Error", "Error opening " + file_name + " for writing");
}

Logger::~Logger(){
    stream->flush();
    log_file->flush();
    log_file->close();
    delete stream;
    delete log_file;
}

QString Logger::get_file_name(){
    return log_file->fileName();
}

void Logger::log(Level level, QString msg, QString qry_text){
    QString edited_msg;
    switch(level){
    case WARNING:
        edited_msg = "[WARNING] - ";
        break;
    case CRITICAL:
        edited_msg = "[CRITICAL] - ";
        break;
    case DEBUG:
        edited_msg = "[DEBUG] - ";
        break;
    case INFO:
        edited_msg = "[INFO] - ";
        break;
    default:
        break;
    }
    if(!qry_text.isNull()){
        if(qry_text != " "){
            edited_msg = "[CRITICAL] - ";
            edited_msg.append(msg);
            edited_msg.append('\n');
            edited_msg.append("Query Error: ");
            edited_msg.append(qry_text);
        }else{
            edited_msg.append(msg);
            edited_msg.append(": Successful");
        }
    }else{
        edited_msg += msg;
    }
    
    (*stream) << edited_msg << endl;
    stream->flush();
    log_file->flush();
}
