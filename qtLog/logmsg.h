#ifndef LOGMSG_H
#define LOGMSG_H

#include <iostream>
#include <mutex>
#include <set>
#include <QString>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <QDebug>

#define SAFEDELETE(Pt_t) {\
							if ((Pt_t) != nullptr)\
							{\
								delete (Pt_t);(Pt_t) = nullptr;\
							}\
						}
#define SAFEDELETEARRAY(Pt_t) {\
								if ((Pt_t) != nullptr)\
								{\
									delete[](Pt_t);(Pt_t) = nullptr;\
								}\
							}

//定义日志宏 difine log macro
#define logDebug QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).debug
#define logInfo QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).info
#define logWarning QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).warning
#define logCritical QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).critical
#define logFatal QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).fatal

class LogMsg final
{
public:
	LogMsg(const LogMsg&) = delete;
	LogMsg& operator=(const LogMsg&) = delete;
	LogMsg(const LogMsg&&) = delete;
	LogMsg& operator=(LogMsg&&) = delete;
	
	enum class LogMsgType : char {
		ALL = 1,//全部
		DebugMsg,//debug及以上
		InfoMsg,//info及以上
		WarningMsg,
		CriticalMsg,
		FatalMsg,
	};
	
public:
	static inline void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
	{
		if (LogMsgType::InfoMsg == ms_LogMstType && QtDebugMsg == type)
		{
			return;
		}
		else if (LogMsgType::WarningMsg == ms_LogMstType && (QtDebugMsg == type || QtInfoMsg == type))
		{
			return;
		}
		else if (LogMsgType::CriticalMsg== ms_LogMstType && (QtDebugMsg == type || QtInfoMsg == type || QtWarningMsg == type))
		{
			return;
		}
		else if (LogMsgType::FatalMsg == ms_LogMstType && (QtDebugMsg == type || QtInfoMsg == type || QtWarningMsg == type || QtCriticalMsg == type))
		{
			return;
		}
		
		if (!ms_IsWorking)
		{
			return;
		}
		QString pretext;
		switch (type)
		{
			case QtDebugMsg:
			{
				pretext = QString("Debug:");
				break;
			}
			case QtInfoMsg:
			{
				pretext = QString("Info:");
				break;
			}
			case QtWarningMsg:
			{
				pretext = QString("Warning:");
				break;
			}
			case QtCriticalMsg:
			{
				pretext = QString("Critical:");
				break;
			}
			case QtFatalMsg:
			{
				pretext = QString("Fatal:");
				break;
			}
			default:
			{
				pretext = QString("Unknown:");
			};
		}

		const QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
		const QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
		const QString current_date = QString("(%1)").arg(current_date_time);
		const QString message = QString("%1 %2 %3 %4 %5").arg(pretext).arg(context_info).arg(msg).arg(current_date).arg(ms_TrailMSG) + "\n";

		//write log message
		{
			std::lock_guard<std::mutex> lock2(ms_manager->m_LogMsgBufDeqLock);
			ms_manager->m_LogMsgBufVec.push_back(message);
		}
		ms_manager->m_BufferCondVar.notify_all();
	}

	static void SetExtMsg(/*const std::string& headmsg,*/ const std::string& trailmsg)
	{
		//ms_manager->m_HeadMSG = QString::fromStdString(headmsg);
		ms_TrailMSG = QString::fromStdString(trailmsg);
	}

	static int Create(const LogMsgType logmsgtype)
	{
		int retval{ -1 };
		std::call_once(ms_CreateOnce, [&]()
		{
			const QString workdir = QCoreApplication::applicationDirPath();
			ms_LogDirPath = workdir + QDir::separator() + "Log";
			ms_LogDirPath = QDir::toNativeSeparators(ms_LogDirPath);
			const QDir dir(ms_LogDirPath);
			if (!dir.exists())
			{
				if (dir.mkpath(ms_LogDirPath))
				{
					retval = 0;
				}
				else
				{
					retval = -1;
				}
			}
			else
			{
				retval = 0;
			}
			std::lock_guard<std::mutex> lock(ms_WorkStarusLock);
			ms_manager = new LogMsg(logmsgtype);
		});
		return retval;
	}

	static void Destroy()
	{
		SAFEDELETE(ms_manager);
	}

private:
	explicit LogMsg(const LogMsgType logmsgtype = LogMsgType::ALL);
	~LogMsg();

	void OpenFile();
	void CloseFile();
	void WriteBufferThread();

private:
	static LogMsg* ms_manager;
	static bool ms_IsWorking;
	static std::mutex ms_WorkStarusLock;
	static std::once_flag ms_CreateOnce;
	static QString ms_LogDirPath;
	static LogMsgType ms_LogMstType;

	//mutable QString m_HeadMSG;
	static QString ms_TrailMSG;

	QFile* m_OutFile{ nullptr };
	mutable std::vector<QString> m_LogMsgBufVec;
	mutable std::mutex m_LogMsgBufDeqLock;
	bool m_FileIsOpen{ false };
	std::thread m_WriteBufferThread;
	//std::once_flag m_onceflg;
	std::mutex m_BufferMutex;
	std::condition_variable m_BufferCondVar;
};

#endif // LOGMSG_H
