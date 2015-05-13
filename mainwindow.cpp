#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
#include <iostream>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/imgs/money_management.gif"));
    setWindowTitle("Money Management");
    setFixedSize(width(), height());

    status_bar_label = new QLabel();
    ui->statusBar->addPermanentWidget(status_bar_label, width());
    status_bar_label->setText("[+] Status");

    ui->dateEdit->setDate(QDate::currentDate());

    QList<QString> options;
    options.append("Deposit");
    options.append("Withdraw");
    ui->comboBoxMode->addItems(options);
    ui->comboBoxMode->setCurrentIndex(-1);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionAbout_triggered()
{
  std::cout << "About action" << std::endl;
}
