#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "hapitrip.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_connectButton_clicked();

  void on_runButton_clicked();

  void on_stopButton_clicked();

private:
  Ui::MainWindow *ui;
  Hapitrip *ht;
};
#endif // MAINWINDOW_H
