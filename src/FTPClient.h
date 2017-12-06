#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#define FTP_SUCCESS  1
#define FTP_ERROR    0

class FTPClient
{
public:
	static FTPClient*	Get();
	static void			Free();

	FTPClient();
	~FTPClient(void);

	int Upload(char* localFile, char* remoteFile);
	int Download(char* localFile, char* remoteFile);
	int CreateDirectory(char* remotePath);

protected:
	void GetFTPInfo();
};

#endif
