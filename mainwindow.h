#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QtSql>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
  void on_actionAbout_triggered();

  void on_pushButtonSubmit_clicked();
  void setup_database();
  bool open_database();
  void close_database();
  void reenable_submit_btn();
private:
    Ui::MainWindow *ui;
    QSqlDatabase transaction_db;
    QString db_path;
    QLocale format;
};

#endif // MAINWINDOW_H
