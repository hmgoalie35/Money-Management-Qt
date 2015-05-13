#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
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

private:
    Ui::MainWindow *ui;
    QLabel* status_bar_label;
};

#endif // MAINWINDOW_H
