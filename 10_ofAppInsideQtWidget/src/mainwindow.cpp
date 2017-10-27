#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	device* d = new device(ui->ofWidget);
	ui->oflayout->addWidget(d);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::loadSubWindow(QWidget* widget) {
	auto window = ui->mdiArea->addSubWindow(widget);
	window->setWindowTitle(widget->windowTitle());
	window->setWindowIcon(widget->windowIcon());
	window->show();
}
