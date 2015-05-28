#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
#include <QDebug>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>

#define MESSAGE_DISPLAY_LENGTH 4000

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/imgs/money_management.gif"));
    setWindowTitle("Money Management");
    setFixedSize(width(), height());
    ui->dateEdit->setDate(QDate::currentDate());
    ui->comboBoxMode->addItem("Deposit");
    ui->comboBoxMode->addItem("Withdraw");
    ui->comboBoxMode->setCurrentIndex(-1);
    ui->lineEditDepWithdr->setValidator(new QDoubleValidator(1, 10000000, 2, ui->lineEditDepWithdr));
    db_path = QDir::currentPath() + "/transaction_db.db";
    qDebug() << "DB Path: " << db_path;

    setup_database();
    open_database();
    QSqlQuery qry = transaction_db.exec("SELECT balance FROM transactions ORDER BY id DESC LIMIT 1;");
    qDebug() << "Initial Setup Query Last Balance Status: " << qry.lastError();
    if(qry.next()){
      qDebug() << "Initially setting total label to: " << format.toCurrencyString(qry.value(0).toDouble());
      ui->labelTotal->setText("Total: " + format.toCurrencyString(qry.value(0).toDouble()));
    }else{
      qDebug() << "Initially setting total label to: " << format.toCurrencyString(0.00);
      ui->labelTotal->setText("Total: " + format.toCurrencyString(0.00));
    }
    close_database();
}

MainWindow::~MainWindow()
{
//    QDir d;
//    d.remove(db_path);
    delete ui;
}
void MainWindow::setup_database(){
  transaction_db = QSqlDatabase::addDatabase("QSQLITE");
  transaction_db.setDatabaseName(db_path);
  QFileInfo db_info(db_path);
  bool exists = db_info.exists();
  bool status = transaction_db.open();
  qDebug() << "DB Exists: " << exists;
  qDebug() << "DB Opened Successfully: " << status;
  if(!status){
      qDebug() << "Error connecting to database (setup_database)";
      ui->statusBar->showMessage("Error connecting to database", MESSAGE_DISPLAY_LENGTH);
      QMessageBox::critical(this, "Error Connecting To Database", "Error opening database, please restart the program");
      ui->lineEditDepWithdr->setEnabled(false);
      ui->lineEditDescription->setEnabled(false);
      ui->pushButtonSubmit->setEnabled(false);
      ui->comboBoxMode->setEnabled(false);
      ui->dateEdit->setEnabled(false);
      ui->menuBar->setEnabled(false);
      return;
  }
  ui->statusBar->showMessage("Connected...", MESSAGE_DISPLAY_LENGTH);
  if(!exists){
      qDebug() << "Creating transaction table";
      QSqlQuery create_transaction_table_qry = transaction_db.exec("CREATE TABLE transactions(id INTEGER PRIMARY KEY AUTOINCREMENT, description TEXT, mode TEXT, trans_amount DOUBLE, balance DOUBLE, date_added DATE);");
      qDebug() << "Create Trans Table Status: " << create_transaction_table_qry.lastError();
  }
  close_database();
}

bool MainWindow::open_database(){
  bool status = transaction_db.open();
  if(status){
      ui->statusBar->showMessage("Connected...", MESSAGE_DISPLAY_LENGTH);
  }else{
      qDebug() << "Error opening database";
      ui->statusBar->showMessage("Error connecting to database", MESSAGE_DISPLAY_LENGTH);
      QMessageBox::critical(this, "Error Connecting To Database", "Error opening database, please restart the program");
      ui->lineEditDepWithdr->setEnabled(false);
      ui->lineEditDescription->setEnabled(false);
      ui->pushButtonSubmit->setEnabled(false);
      ui->comboBoxMode->setEnabled(false);
      ui->dateEdit->setEnabled(false);
      ui->menuBar->setEnabled(false);
    }
  return status;
}

void MainWindow::close_database(){
//  bool status = transaction_db.commit();
//  qDebug() << "Closing database: " << status;
  transaction_db.close();
}

void MainWindow::on_actionAbout_triggered()
{
}

void MainWindow::on_pushButtonSubmit_clicked()
{
  if(!open_database()){
      QMessageBox::critical(this, "Error Saving Transaction", "Error saving the transaction, please try again");
      qDebug() << "Error connecting to database (submit button)\n Transaction not saved";
      return;
  }
  QString description = ui->lineEditDescription->text();
  QString mode = ui->comboBoxMode->currentText();
  double amount = ui->lineEditDepWithdr->text().toDouble();

  if(amount == 0 || description == "" || mode == ""){
      QMessageBox::critical(this, "Invalid Input", "Please provide a description, amount and if this transaction is a deposit or withdrawal.");
      qDebug() << "Invalid Input\nAmount: " << amount << " Description: " << description << " Mode: " << mode;
      return;
  }
  QSqlQuery last_trans = transaction_db.exec("SELECT balance FROM transactions ORDER BY id DESC LIMIT 1;");
  qDebug() << "Get last balance qry psh btn submit clicked status: " << last_trans.lastError();
  double last_balance = 0;
  if(last_trans.next()){
      qDebug() << "Checking last known balance (this should print once)";
      last_balance = last_trans.value(0).toDouble();
  }
  qDebug() << "last known balance (submit btn pressed): " << last_balance;
  if(mode == "Deposit"){
      qDebug() << "Depositing " << amount << " to " << last_balance;
      last_balance += amount;
      ui->labelTotal->setText("Total: " + format.toCurrencyString(last_balance));
  }else if(mode == "Withdraw"){
      qDebug() << "Withdrawing " << amount << " from " << last_balance;
      if(last_balance - amount < 0){
          qDebug() << "Can't withdraw " << amount << " from " << last_balance;
          QMessageBox::information(this, "Insufficient Funds", "There is not enough money to withdraw " + format.toCurrencyString(amount));
          return;
      }
      last_balance -= amount;
      ui->labelTotal->setText("Total: " + format.toCurrencyString(last_balance));
  }
  QSqlQuery add_transaction_qry;
  add_transaction_qry.prepare("INSERT INTO transactions (description, mode, trans_amount, balance, date_added) VALUES (:desc, :mode, :trans_amount, :balance, :date);");
  add_transaction_qry.bindValue(":desc", description);
  add_transaction_qry.bindValue(":mode", mode);
  add_transaction_qry.bindValue(":trans_amount", amount);
  add_transaction_qry.bindValue(":balance", last_balance);
  add_transaction_qry.bindValue(":date", ui->dateEdit->date());
  bool result = add_transaction_qry.exec();
  qDebug() << "Add transaction status: " << add_transaction_qry.lastError();
  if(result){
      ui->statusBar->showMessage("Transaction saved", MESSAGE_DISPLAY_LENGTH);
      ui->lineEditDepWithdr->setText("");
      ui->lineEditDescription->setText("");
      ui->comboBoxMode->setCurrentIndex(-1);
      ui->pushButtonSubmit->setEnabled(false);
      QTimer::singleShot(1750, this, SLOT(reenable_submit_btn()));
      qDebug() << "Transaction saved";
    }else{
      ui->statusBar->showMessage("Error saving transaction", MESSAGE_DISPLAY_LENGTH);
      qDebug() << "Error saving transaction";
    }
  close_database();
}

void MainWindow::reenable_submit_btn(){
  ui->pushButtonSubmit->setEnabled(true);
}

void MainWindow::on_actionQuit_triggered()
{
  this->close();

}
void MainWindow::closeEvent(QCloseEvent* event){
  int result = QMessageBox::question(NULL, "Quit?", "Are you sure you want to quit?");
  if(result == QMessageBox::Yes){
      qDebug() << "Exiting, user selected yes";
      event->accept();
    }else{
      qDebug() << "Not exiting, user selected no";
      event->ignore();
    }
}

void MainWindow::on_actionExport_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Export Database"), QDir::currentPath(), tr("Sql File (*.sql)"));
    if(!filename.isEmpty()){
      open_database();
      QSqlQuery transactions_exist_qry = transaction_db.exec("SELECT count(id) FROM transactions;");
      qDebug() << "Transactions exist qry status " << transactions_exist_qry.lastError();
      if(transactions_exist_qry.next()){
          if(transactions_exist_qry.value(0) <=0){
                qDebug() << "there are no transactions to export";
                QMessageBox::information(this, "No Transactions Exist", "There are no transactions to export, export cancelled");
                return;
            }
      }else{
          qDebug() << "there are no transactions to export";
          QMessageBox::information(this, "No Transactions Exist", "There are no transactions to export, export cancelled");
          return;
        }
      qDebug() << "transactions exist, go ahead and export";
      QSqlQuery get_all_transactions_qry = transaction_db.exec("SELECT id, description, mode, trans_amount, balance, date_added FROM transactions;");
      qDebug() << "Get all transactions for export status: " << get_all_transactions_qry.lastError();
      int count = 0;
      QFile export_file(filename);
      if(!export_file.open(QFile::WriteOnly)){
          QMessageBox::information(this, "Error Opening File", "Error opening " + filename + " for export, please try again");
          return;
        }
      export_file.write("PRAGMA foreign_keys=OFF;\n");
      export_file.write("BEGIN TRANSACTION;\n");
      export_file.write("CREATE TABLE transactions(id INTEGER PRIMARY KEY AUTOINCREMENT, description TEXT, mode TEXT, trans_amount DOUBLE, balance DOUBLE, date_added DATE);\n");
      //need to add error checking for 0 transactions.
      while(get_all_transactions_qry.next()){
          QString write_str = "INSERT INTO transactions (id, description, mode, trans_amount, balance, date_added) VALUES (" + get_all_transactions_qry.value(0).toString() + ", " + get_all_transactions_qry.value(1).toString() + ", " + get_all_transactions_qry.value(2).toString() + ", " + get_all_transactions_qry.value(3).toString() + ", " + get_all_transactions_qry.value(4).toString() + ", " + get_all_transactions_qry.value(5).toString() + ");\n";
          qDebug() << "Writing " << write_str;
          export_file.write(write_str.toUtf8());
          count++;
        }
      export_file.write("COMMIT;");
      QMessageBox::information(this, "Success", QString::number(count) + " transactions exported to " + filename);
      qDebug() << count << " transactions written to " << filename;
      export_file.flush();
      export_file.close();
      close_database();
    }
}
