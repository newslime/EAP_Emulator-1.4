
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

class HttpRequest
{
public:
	static HttpRequest*	Get();
	static void			Free();

	HttpRequest();
	virtual ~HttpRequest();

	void Post(std::string ceid, std::string workorder);
};



#endif // HTTP_REQUEST_H
