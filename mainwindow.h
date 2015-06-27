#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QtSql>
#include <QCloseEvent>
#include <QTableView>
#include <QSqlQueryModel>
#include "Logger.h"

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
    //the following are also triggered via their respective keyboard shortcuts (if applicable)
    
    //triggered when about action pressed
    void on_actionAbout_triggered();
    
    //triggered when submitting new transaction
    void on_pushButtonSubmit_clicked();
    
    //called to reenable the submit btn after a few seconds, to prevent spamming of the submit btn.
    void reenable_submit_btn();
    
    //triggered when quit button pressed
    void on_actionQuit_triggered();
    
    //triggered when export button pressed
    void on_actionExport_triggered();
    
    //triggered when import button pressed
    void on_actionImport_triggered();
    
    //triggered when delete database btn pressed
    void on_actionDelete_triggered();
    
    //triggered when view all transactions btn pressed.
    void on_actionAll_Transactions_triggered();
    
    //triggered when a transaction(s) want to be edited/updated.
    void on_actionTransaction_triggered();
    
    //called whenever a database row is altered and handles the change.
    void record_changed(QModelIndex,QModelIndex);
    
    //sets up the database for the life of the program
    void setup_database();
    
    //opens the database so writing/reading can occur
    bool open_database();
    
    //safely closes the database
    void close_database();
    
    //called when the program closes (via keyboard shortcut, menu item or the X button
    void closeEvent(QCloseEvent*);  
    
    //queries the database for the last balance, and sets the balance label.
    double get_last_transaction_balance();
    
private:
    Ui::MainWindow *ui;
    
    //the database object.
    QSqlDatabase transaction_db;
    
    //the path of the database
    QString db_path;
    
    //used to format money to the locale of the program
    QLocale format;
    
    //used to display and edit database rows/columns so they can be updated
    QSqlTableModel* edit_trans_model;
    QTableView* edit_trans_view;
    
    //used to display database rows/columns for viewing only.
    QTableView* view_all_transactions_view;
    QSqlQueryModel* view_all_transactions_model;
    
    //global logger object.
    Logger * logger;
    
    /* column 0 = id
   * column 1 = descrip
   * column 2 = mode
   * column 3 = trans amount
   * column 4 = balance
   * column 5 = date added
  */
    enum Column{
        ID,
        DESCRIPTION,
        MODE,
        TRANSACTION_AMOUNT,
        BALANCE,
        DATA_ADDED
    };
    
};

#endif // MAINWINDOW_H
