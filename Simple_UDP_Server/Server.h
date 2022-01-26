#ifndef ServerH
#define ServerH

#include <WinSock2.h>
#include "Client.h"

class Server {

protected:
	SOCKET _socket;
	SOCKADDR_IN _address;
	int addressLength = sizeof(_address);

	//virtual void createSocket() = 0;
	void freeResources();
	void initAddress(u_short port);
	void bindSocket();
private:

public:
	Server(u_short port);
	~Server();

};

#endif ServerH