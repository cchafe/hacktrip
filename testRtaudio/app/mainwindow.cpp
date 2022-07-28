#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <lib.h>
#include <RtAudio.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    RtAudio m_adac;
    m_adac.getDeviceCount();
    Lib l;
    l.hi();
    ht = nullptr;
}

MainWindow::~MainWindow()
{
    if (ht != nullptr) delete ht;
    delete ui;
}


void MainWindow::on_connectButton_clicked()
{
    if (ht != nullptr) delete ht;
    ht = new Hapitrip();
    ht->connect(); // grab the next free client slot from server pool
}

