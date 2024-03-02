#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QShortcut>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , mUi(new Ui::MainWindow)
{
    mUi->setupUi(this);
    mHt = nullptr;
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this, SLOT(close()));
}

MainWindow::~MainWindow() {
    if (mHt != nullptr)
        delete mHt;
    delete mUi;
}

void MainWindow::on_connectButton_clicked() {
    mHt = new Hapitrip(); // APIstructure inits its params with its defaults
    updateStateFromUI(); // override params with initial UI values
    mConnected = false;
    mConnected = mHt->connectToServer(mServer); // grab the next free client slot from server pool
    if (!mConnected) {
        QMessageBox msgBox;
        msgBox.setText("connection failed for " + mServer + "\n try another" );
        msgBox.exec();
    }
}

void MainWindow::on_runButton_clicked() {
    if (mConnected) mHt->run();
}

void MainWindow::on_stopButton_clicked() {
    if (mConnected) {
        mHt->stop();
    }
    delete mHt;
    mHt = nullptr;
    //  this->close();
}

void MainWindow::on_serverComboBox_currentIndexChanged(const QString &arg1)
{
    mServer = arg1; // takes effect after stop
}

void MainWindow::on_FPPcomboBox_currentIndexChanged(const QString &arg1)
{
    if (!mConnected) mHt->setFPP(arg1.toInt());
}

void MainWindow::on_channelsSpinBox_valueChanged(int arg1)
{
    if (!mConnected) mHt->setChannels(arg1);
}

void MainWindow::on_srateComboBox_currentIndexChanged(const QString &arg1)
{
    mHt->setSampleRate(arg1.toInt()); // takes effect after stop
}

void MainWindow::on_audioComboBox_currentIndexChanged(int arg1)
{
    mHt->setRtAudioAPI(arg1); // takes effect after stop
}

void MainWindow::on_plcCheckBox_stateChanged(int arg1)
{
    mHt->setUsePLC(arg1);
}

void MainWindow::updateStateFromUI() {
    mServer = mUi->serverComboBox->currentText();
    mHt->setFPP(mUi->FPPcomboBox->currentText().toInt());
    mHt->setChannels(mUi->channelsSpinBox->value());
    mHt->setSampleRate(mUi->srateComboBox->currentText().toInt());
    mHt->setUsePLC(mUi->plcCheckBox->checkState());
};

