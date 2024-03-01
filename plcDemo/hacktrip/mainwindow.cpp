#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QShortcut>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this, SLOT(close()));
}

MainWindow::~MainWindow()
{
    delete ui;
}
