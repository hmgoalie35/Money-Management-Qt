#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QtSql>
#include <QCloseEvent>
#include <QTableView>
#include <QSqlQueryModel>

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
  void record_changed(QModelIndex,QModelIndex);
  void reenable_submit_btn();
  void on_actionQuit_triggered();
  void closeEvent(QCloseEvent*);

  void on_actionExport_triggered();

  void on_actionImport_triggered();

  void on_actionDelete_triggered();

  void on_actionAll_Transactions_triggered();

  void on_actionTransaction_triggered();

private:
    Ui::MainWindow *ui;
    QSqlDatabase transaction_db;
    QString db_path;
    QLocale format;
    QSqlTableModel* edit_trans_model;
    QTableView* edit_trans_view;
    QTableView* view_all_transactions_view;
    QSqlQueryModel* view_all_transactions_model;

};

#endif // MAINWINDOW_H
