#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
#include <QDebug>
#include <QMessageBox>
#include <QFileInfo>

#define MESSAGE_DISPLAY_LENGTH 4000

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/imgs/money_management.gif"));
    setWindowTitle("Money Management");
    setFixedSize(width(), height());
    ui->statusBar->showMessage("", MESSAGE_DISPLAY_LENGTH);
    ui->dateEdit->setDate(QDate::currentDate());

    ui->comboBoxMode->addItem("Deposit");
    ui->comboBoxMode->addItem("Withdraw");

    ui->comboBoxMode->setCurrentIndex(-1);
    ui->lineEditDepWithdr->setValidator(new QDoubleValidator(1, 10000000, 2, ui->lineEditDepWithdr));
    db_path = QDir::currentPath() + "/transaction_db.db";

    setup_database();

    open_database();
    QSqlQuery qry = transaction_db.exec("SELECT amount FROM transactions ORDER BY id DESC LIMIT 1;");
    if(qry.next()){
      ui->labelTotal->setText("Total: " + format.toCurrencyString(qry.value(0).toDouble()));
    }else{
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
  if(!status){
      //add in message boxes showing errors...
      ui->statusBar->showMessage("Error connecting to database", MESSAGE_DISPLAY_LENGTH);
      return;
  }
  ui->statusBar->showMessage("Connected", MESSAGE_DISPLAY_LENGTH);
  if(!exists){
      qDebug() << "Creating transaction table";
      QSqlQuery create_transaction_table_qry = transaction_db.exec("CREATE TABLE transactions(id INTEGER PRIMARY KEY AUTOINCREMENT, description TEXT, mode TEXT , amount DOUBLE, date_added DATE);");
      qDebug() << create_transaction_table_qry.lastError();
  }
  close_database();
}

bool MainWindow::open_database(){
  bool status = transaction_db.open();
  if(status){
      ui->statusBar->showMessage("Connected...", MESSAGE_DISPLAY_LENGTH);
  }else{
      ui->statusBar->showMessage("Error connecting to database", MESSAGE_DISPLAY_LENGTH);
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
      //error opening db.
      //notify user transaction was not saved.

      qDebug() << "Error connecting to database (submit button)";
      return;
    }
  QString description = ui->lineEditDescription->text();
  QString mode = ui->comboBoxMode->currentText();
  double amount = ui->lineEditDepWithdr->text().toDouble();

  if(amount == 0 || description == "" || mode == ""){
      QMessageBox::critical(this, "Invalid Input", "Please provide a description, amount and if this transaction is a deposit or withdrawal.");
      qDebug() << "Invalid Input";
      return;
  }
  QSqlQuery last_trans = transaction_db.exec("SELECT amount FROM transactions ORDER BY id DESC LIMIT 1;");
  double last_balance = 0;
  if(last_trans.next()){
      qDebug() << "this should print once";
      last_balance = last_trans.value(0).toDouble();
  }
  qDebug() << "balance: " << last_balance;
  if(mode == "Deposit"){
      qDebug() << "Depositing " << amount << " to " << last_balance;
      last_balance += amount;
      ui->labelTotal->setText("Total: " + format.toCurrencyString(last_balance));
  }else if(mode == "Withdraw"){
      qDebug() << "Withdrawing " << amount << " from " << last_balance;
      if(last_balance - amount < 0){
          QMessageBox::information(this, "Insufficient Funds", "There is not enough money to withdraw " + format.toCurrencyString(amount));
          return;
      }
      last_balance -= amount;
      ui->labelTotal->setText("Total: " + format.toCurrencyString(last_balance));
  }
  QSqlQuery add_transaction_qry;
  add_transaction_qry.prepare("INSERT INTO transactions (description, amount, mode, date_added) VALUES (:desc, :amount, :mode, :date);");
  add_transaction_qry.bindValue(":desc", description);
  add_transaction_qry.bindValue(":amount", last_balance);
  add_transaction_qry.bindValue(":mode", mode);
  add_transaction_qry.bindValue(":date", ui->dateEdit->date());
  bool result = add_transaction_qry.exec();
  qDebug() << add_transaction_qry.lastError();
  if(result){
      ui->statusBar->showMessage("Transaction saved", MESSAGE_DISPLAY_LENGTH);
      ui->lineEditDepWithdr->setText("");
      ui->lineEditDescription->setText("");
      ui->comboBoxMode->setCurrentIndex(-1);
      ui->pushButtonSubmit->setEnabled(false);
      QTimer::singleShot(1750, this, SLOT(reenable_submit_btn()));
    }else{
      ui->statusBar->showMessage("Error saving transaction", MESSAGE_DISPLAY_LENGTH);
    }
  close_database();
}

void MainWindow::reenable_submit_btn(){
  ui->pushButtonSubmit->setEnabled(true);
}
