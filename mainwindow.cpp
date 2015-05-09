#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/imgs/mainwindow_logo.gif"));
    setWindowTitle("Money Management");
}

MainWindow::~MainWindow()
{
    delete ui;
}
