#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "logmsg.h"
#include <QMainWindow>
#include <QTimer>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
	QTimer *m_Timer10ms{ nullptr };
	std::thread m_work01TD;
	std::thread m_work02TD;
	bool m_exist{ false };

private:
	void processwork01TD();
	void processwork02TD();

private slots:
	void onTimerUpdate();
};
#endif // MAINWINDOW_H
