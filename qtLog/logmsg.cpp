#include "logmsg.h"

LogMsg* LogMsg::ms_manager{ nullptr };
bool LogMsg::ms_IsWorking{ false };
std::mutex LogMsg::ms_WorkStarusLock;
std::once_flag LogMsg::ms_CreateOnce;
QString LogMsg::ms_LogDirPath;
LogMsg::LogMsgType LogMsg::ms_LogMstType;
QString LogMsg::ms_TrailMSG;

LogMsg::LogMsg(const LogMsgType logmsgtype)
{
	ms_LogMstType = logmsgtype;
	ms_IsWorking = true;
	m_WriteBufferThread = std::thread(&LogMsg::WriteBufferThread, this);
}

LogMsg::~LogMsg()
{
	try
	{
		ms_IsWorking = false;

		std::lock(ms_WorkStarusLock, ms_manager->m_LogMsgBufDeqLock);
		std::lock_guard<std::mutex> lock1(ms_WorkStarusLock, std::adopt_lock);
		std::lock_guard<std::mutex> lock2(ms_manager->m_LogMsgBufDeqLock, std::adopt_lock);

		//close write thread
		m_BufferCondVar.notify_all();
		if (m_WriteBufferThread.joinable())
		{
			m_WriteBufferThread.join();
		}

		//write buffer
		const auto vecsize = ms_manager->m_LogMsgBufVec.size();
		if (vecsize > 0)
		{
			OpenFile();
			if (nullptr != m_OutFile)
			{
				QTextStream text_stream(ms_manager->m_OutFile);
				text_stream.setCodec("UTF-8");
				for (size_t i = 0; i < vecsize; ++i)
				{
					text_stream << ms_manager->m_LogMsgBufVec[i];
				}
				text_stream.flush();
				ms_manager->m_LogMsgBufVec.clear();
			}
			CloseFile();
		}
	}
	catch (...)
	{
		std::cout << "~LogMsg throw exception msg=" /*<< expt.what()*/ << std::endl;
	}
}

void LogMsg::OpenFile()
{
	if (m_OutFile)
	{
		return;
	}
	
	const QString LogFileName = QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";
	const QDir dir(ms_LogDirPath);
	if (!dir.exists())
	{
		int tryamount{ 0 };
		bool rval{ false };
		do
		{
			if (tryamount++ >= 3)
			{
				std::cout << "mkdir failed";
				break;
			}
			rval = dir.mkpath(ms_LogDirPath);
		} while (!rval);
	}
	QString logfilepath = ms_LogDirPath + QDir::separator() + LogFileName;
	logfilepath = QDir::toNativeSeparators(logfilepath);

	m_OutFile = new QFile(logfilepath);
	if (m_OutFile)
	{
		m_FileIsOpen = m_OutFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Unbuffered);
	}
}

void LogMsg::CloseFile()
{
	if (nullptr != m_OutFile)
	{
		m_OutFile->flush();
		m_OutFile->close();
		SAFEDELETE(m_OutFile);
	}
}

void LogMsg::WriteBufferThread()
{
	QDateTime lastcheckdatetime;
	while (ms_IsWorking)
	{
		std::unique_lock<std::mutex> ulock(m_BufferMutex);
		m_BufferCondVar.wait(ulock, [this] {
			return (!m_LogMsgBufVec.empty() || !ms_IsWorking);
		});
		if (!ms_IsWorking)
		{
			break;
		}

		if (!ms_WorkStarusLock.try_lock())
		{
			continue;
		}
		if (ms_IsWorking && !m_LogMsgBufVec.empty())
		{
			std::vector<QString> bufque;
			{
				std::lock_guard<std::mutex> veclock(m_LogMsgBufDeqLock);
				bufque.swap(m_LogMsgBufVec);
			}
			const auto nowtime = QDateTime::currentDateTime();
			if (!lastcheckdatetime.isValid() || 0 != nowtime.daysTo(lastcheckdatetime))
			{
				CloseFile();
				OpenFile();
			}
			lastcheckdatetime = nowtime;
			if (nullptr != m_OutFile && m_FileIsOpen)
			{
				const auto vecsize = bufque.size();
				QTextStream text_stream(m_OutFile);
				text_stream.setCodec("UTF-8");
				for (size_t i = 0; i < vecsize; ++i)
				{
					text_stream << bufque[i];
				}
				text_stream.flush();
			}
			else
			{
				std::cout << "open logfile failed";
			}
		}
		else if (!ms_IsWorking)
		{
			ms_WorkStarusLock.unlock();
			break;
		}
		ms_WorkStarusLock.unlock();
	}
	CloseFile();
}
