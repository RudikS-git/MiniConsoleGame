#ifndef ClientH
#define ClientH

#include <WinSock2.h>

class Client {
	SOCKET _socket;
	struct sockaddr_in _address;

public:
	Client(SOCKET socket, sockaddr_in address);
	void setSocket(SOCKET socket);
	SOCKET getSocket();
	void setAddress(sockaddr_in address);
	sockaddr_in getAddress();
	void disconnect();
	~Client();
};;

#endif ClientH