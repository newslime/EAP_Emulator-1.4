
#include <windows.h> 
#include <wininet.h>
#include "FTPClient.h"

//#pragma comment(lib, "wininet") 
static char* s_ftp = "10.7.80.18";
static char* s_user = "klt";
static char* s_pass = "klt";

static FTPClient* s_FTPClient_Instance = NULL;

FTPClient* FTPClient::Get() 
{
	if (!s_FTPClient_Instance)
		s_FTPClient_Instance = new FTPClient();
	
	return s_FTPClient_Instance;
}

void FTPClient::Free()
{
	delete s_FTPClient_Instance;
	s_FTPClient_Instance = NULL;
}

FTPClient::FTPClient()
{	
}

FTPClient::~FTPClient()
{
}

int FTPClient::Upload(char* localFile, char* remoteFile)
{
	HINTERNET hInternet;
	HINTERNET hFtpSession;
	int result;
	
	hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if(hInternet == NULL)
		return FTP_ERROR;

	hFtpSession = InternetConnect(hInternet,(LPTSTR)s_ftp , INTERNET_DEFAULT_FTP_PORT, (LPTSTR)s_user, (LPTSTR)s_pass, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
	if(hFtpSession == NULL)
		return FTP_ERROR;

	result = FtpPutFile(hFtpSession, (LPTSTR)localFile, (LPTSTR)remoteFile, FTP_TRANSFER_TYPE_ASCII, 0);
	Sleep(200);
	
	InternetCloseHandle(hFtpSession);
	InternetCloseHandle(hInternet);
	
	return result;
}
	
int FTPClient::Download(char* localFile, char* remoteFile)
{
	HINTERNET hInternet;
	HINTERNET hFtpSession;
	int result;

	hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT,NULL, NULL, 0);
	if(hInternet == NULL)
		return FTP_ERROR;

	hFtpSession = InternetConnect(hInternet,(LPTSTR)s_ftp , INTERNET_DEFAULT_FTP_PORT, (LPTSTR)s_user, (LPTSTR)s_pass, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
	if(hFtpSession == NULL)
		return FTP_ERROR;

	result = FtpGetFile(hFtpSession, (LPTSTR)remoteFile, (LPTSTR)localFile, FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_ASCII,0);
	Sleep(200);
	
	InternetCloseHandle(hFtpSession);
	InternetCloseHandle(hInternet);
	
	return result;
}

int CreateDirectory(char* remotePath)
{
	HINTERNET hInternet;
	HINTERNET hFtpSession;
	int result;

	hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT,NULL, NULL, 0);
	if(hInternet == NULL)
		return FTP_ERROR;

	hFtpSession = InternetConnect(hInternet,(LPTSTR)s_ftp , INTERNET_DEFAULT_FTP_PORT, (LPTSTR)s_user, (LPTSTR)s_pass, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
	if(hFtpSession == NULL)
		return FTP_ERROR;

	result = FtpCreateDirectory(hFtpSession, remotePath);

	InternetCloseHandle(hFtpSession);
	InternetCloseHandle(hInternet);

	return result;
}
