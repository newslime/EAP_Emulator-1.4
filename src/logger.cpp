
#include <fstream>
#include <iostream>
#include <windows.h>
#include "logger.h"

//////////////////////
///////Logger/////////
//////////////////////
static Logger* s_Logger_Instance = NULL;

extern string s_ExePath;

Logger* Logger::Get()
{
	if (!s_Logger_Instance)
		s_Logger_Instance = new Logger();
	
	return s_Logger_Instance;
}

void Logger::Free()
{
	delete s_Logger_Instance;
	s_Logger_Instance = NULL;
}

Logger::Logger()
{
	char logFile[256];
	sprintf(logFile, "%s/EAPEmulator_%s.log", s_ExePath.c_str(), getCurrentDate().c_str());
	mFilename = logFile;
}

Logger::~Logger()
{
	if (mLogFile.is_open())
    {
        mLogFile.close();
    }
}

void Logger::write(const string &msg)
{
	if(getDayChanged())
	{
		char logFile[256];
		sprintf(logFile, "%s/EAPEmulator_%s.log", s_ExePath.c_str(), getCurrentDate().c_str());
		mFilename = logFile;
	}
	
	mLogFile.open(mFilename.c_str(), ios::app);
	if (!mLogFile.is_open())
	{
		throw ios::failure("unable to open " + mFilename + "for writing");
	}
	else
	{
		mLogFile.exceptions(ios::failbit | ios::badbit);
	}

	time_t now = time(0);
	struct tm  tstruct;
	tstruct = *localtime(&now);
	char timebuf[80] = {0};

	strftime(timebuf, sizeof(timebuf), "%Y/%m/%d %H:%M:%S : ", &tstruct);

	string content = string(timebuf) + msg; 

    mLogFile << content << endl;

	mLogFile.close();
}

bool Logger::getDayChanged()
{
    string dayDate = getCurrentDate();

    if (mLastCallDate != dayDate)
    {
        // Keep track of the old date.
        mOldDate = mLastCallDate;
        // Reset the current date for next call.
        mLastCallDate = dayDate;
        return true;
    }
    return false;
}

