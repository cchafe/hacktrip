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
}

MainWindow::~MainWindow()
{
    delete ui;
}

