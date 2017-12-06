
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _WIN32
	#include <winsock2.h>
	#include <windows.h>
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netdb.h>
#endif

#include "GemSDK.h"
#include "HttpRequest.h"
#include "logger.h"


using namespace std;

//////////////////////
///////HttpRequest/////////
//////////////////////
static HttpRequest* s_HttpRequest_Instance = NULL;

HttpRequest* HttpRequest::Get()
{
	if (!s_HttpRequest_Instance)
		s_HttpRequest_Instance = new HttpRequest();
	
	return s_HttpRequest_Instance;
}

void HttpRequest::Free()
{
	delete s_HttpRequest_Instance;
	s_HttpRequest_Instance = NULL;
}

HttpRequest::HttpRequest()
{
}

HttpRequest::~HttpRequest()
{
}

/*void HttpRequest::GetMethod()
{
	WSADATA wsaData;
	SOCKET Socket;
	SOCKADDR_IN SockAddr;
	struct hostent *host;
	char buffer[10000];
	int nDataLength;
	string website_HTML;

	// website url
	string url = "www.google.com";

	//HTTP GET
	string get_http = "GET / HTTP/1.1\r\nHost: " + url + "\r\nConnection: close\r\n\r\n";

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
	{
        cout << "WSAStartup failed.\n";
        system("pause");
        //return 1;
    }

    Socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    host = gethostbyname(url.c_str());

    SockAddr.sin_port=htons(80);
    SockAddr.sin_family=AF_INET;
    SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

    if(connect(Socket,(SOCKADDR*)(&SockAddr),sizeof(SockAddr)) != 0)
	{
        cout << "Could not connect";
        system("pause");
        //return 1;
    }

    // send GET / HTTP
    send(Socket,get_http.c_str(), strlen(get_http.c_str()),0 );

    // recieve html
    while ((nDataLength = recv(Socket,buffer,10000,0)) > 0)
	{        
        int i = 0;
        while (buffer[i] >= 32 || buffer[i] == '\n' || buffer[i] == '\r')
		{

            website_HTML+=buffer[i];
            i += 1;
        }               
    }

    closesocket(Socket);
    WSACleanup();
}*/

void HttpRequest::Post(string ceid, string workorder)
{
	WSADATA			wsaData;
	SOCKET			Socket;
	struct hostent*	host;
	SOCKADDR_IN		SockAddr;
	ostringstream	FormBuffer;
	char			FormAction[1024];
	char			buffer[10000];
	int				nDataLength;
	const string	token = "";

	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
	{
        cout << "WSAStartup failed.\n";
        return;
    }

	Socket	= socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	host	= gethostbyname("192.168.0.160");

	if(host)
	{
		SockAddr.sin_port = htons(3000);
		SockAddr.sin_family = AF_INET;
		SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

		if (connect(Socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0)
		{
			MessageBox(NULL, "Could not connect to MES", "Error", MB_OK); 
			Logger::Get()->write("Could not connect to MES");
			return;
		}	

		if(ceid.compare("D03003") == 0)
		{
			sprintf(FormAction, "/api/process/workorder/saa-in/%s", workorder.c_str());
		}
		else
		{
			sprintf(FormAction, "/api/process/workorder/saa-out/%s", workorder.c_str());
		}

		//const char* data = text.c_str();
		
		// header
		FormBuffer << "POST " << "/equipment" << " HTTP/1.1\r\n";
		//FormBuffer << "Content-Type: application/x-www-form-urlencoded\r\n";
		FormBuffer << "Content-Type: application/json;charset=UTF-8\r\n";
		FormBuffer << "Authorization: " << token.c_str() << "\r\n";
		FormBuffer << "Host: 192.168.0.160:3000 \r\n";
		//FormBuffer << "Content-Length: " << content.length() << "\r\n";
		FormBuffer << "Connection: close\r\n\r\n";
		//FormBuffer << "Connection: Keep-Alive\r\n\r\n";
		//FormBuffer << data;
		
		const string str = FormBuffer.str();

		send(Socket, str.data(), str.size(), NULL);
			
		nDataLength = recv(Socket, buffer, 10000, 0);
		/*while ((nDataLength = recv(Socket, buffer, 10000, 0)) > 0)
		{
			int i = 0;
			while (buffer[i] >= 32 || buffer[i] == '\n' || buffer[i] == '\r') 
			{
				cout << buffer[i];
				i += 1;
			}
		}*/

		closesocket(Socket);
	}
	WSACleanup();
}