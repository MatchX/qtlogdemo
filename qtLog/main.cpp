#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	LogMsg::Create(LogMsg::LogMsgType::ALL);//创建日志系统 creat log system
	LogMsg::SetExtMsg("extinfo");
	qInstallMessageHandler(LogMsg::outputMessage);//系统日志重定向 system log redirect
	logInfo() << "APP Start";
	
    MainWindow w;
    w.show();
	const auto rval = a.exec();

	logInfo() << "APP Stop";
	LogMsg::Destroy();
	return rval;
}
