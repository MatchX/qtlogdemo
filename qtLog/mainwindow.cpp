#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	QDir::setCurrent(QCoreApplication::applicationDirPath());//设置当前工作路径 set current workspace 

	m_Timer10ms = new QTimer();

	connect(m_Timer10ms, SIGNAL(timeout()), this, SLOT(onTimerUpdate()));

	m_Timer10ms->start(10);
	m_work01TD = std::thread(&MainWindow::processwork01TD, this);
	m_work01TD.detach();
	m_work02TD = std::thread(&MainWindow::processwork02TD, this);
}

MainWindow::~MainWindow()
{
	m_exist = true;
	if (m_Timer10ms)
	{
		m_Timer10ms->stop();
		SAFEDELETE(m_Timer10ms);
	}
	
	if (m_work01TD.joinable())
	{
		m_work01TD.join();
	}

	if (m_work02TD.joinable())
	{
		m_work02TD.join();
	}

	delete ui;
}

void MainWindow::onTimerUpdate()
{
	static int mark{ 0 };
	logDebug() << "onTimerUpdate " << QString::number(mark++);
}

void MainWindow::processwork01TD()
{
	int mark{ 0 };
	while (true)
	{
		if (m_exist)
		{
			break;
		}
		
		logWarning() << "processwork01TD " << mark++;
		QThread::msleep(1);
	}
}

void MainWindow::processwork02TD()
{
	int mark{ 0 };
	while (true)
	{
		if (m_exist)
		{
			break;
		}

		logCritical() << "processwork02TD " << mark++;
		QThread::msleep(100);
	}
}
