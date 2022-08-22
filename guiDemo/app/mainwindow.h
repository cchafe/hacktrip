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

  void on_FPPcomboBox_currentIndexChanged(const QString &arg1);

  void on_channelsSpinBox_valueChanged(int arg1);

  void updateStateFromUI();

  void on_serverComboBox_currentIndexChanged(const QString &arg1);

  void on_srateComboBox_currentIndexChanged(const QString &arg1);

  void on_audioComboBox_currentIndexChanged(const int arg1);

private:
  Ui::MainWindow *ui;
  Hapitrip *ht;
  bool connected;
  QString mServer;
};
#endif // MAINWINDOW_H
