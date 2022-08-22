#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  ht = new Hapitrip(); // APIstructure inits its params with its defaults
  updateStateFromUI(); // override params with initial UI values
  connected = false;
}

MainWindow::~MainWindow() {
  if (ht != nullptr)
    delete ht;
  delete ui;
}

void MainWindow::on_connectButton_clicked() {
    std::cout << mServer.toStdString() << std::endl;
    ht->connectToServer(mServer); // grab the next free client slot from server pool
    connected = true;
}

void MainWindow::on_runButton_clicked() { ht->run(); }

void MainWindow::on_stopButton_clicked() {
  if (connected) {
    ht->stop();
    delete ht;
    ht = nullptr;
  }
  this->close();
}

void MainWindow::on_serverComboBox_currentIndexChanged(const QString &arg1)
{
    mServer = arg1;
}
void MainWindow::on_FPPcomboBox_currentIndexChanged(const QString &arg1)
{
    ht->setFPP(arg1.toInt());
}

void MainWindow::on_channelsSpinBox_valueChanged(int arg1)
{
    ht->setChannels(arg1);
}

void MainWindow::on_srateComboBox_currentIndexChanged(const QString &arg1)
{
    ht->setSampleRate(arg1.toInt());
}

void MainWindow::updateStateFromUI() {
    mServer = ui->serverComboBox->currentText();
    ht->setFPP(ui->FPPcomboBox->currentText().toInt());
    ht->setChannels(ui->channelsSpinBox->value());
    ht->setSampleRate(ui->srateComboBox->currentText().toInt());
};




