#ifndef UdpServerH
#define UdpServerH

#include <WinSock2.h>
#include <exception>
#include "Server.h"

class UdpServer : Server {

public:
	struct UdpResponse {
		SOCKADDR_IN clientAddress;
		int result;
	};

private:
	void createSocket();

public:
	UdpServer(u_short port);

	void start();

	int getMessage(UdpResponse* udpResponse);

	void sendMessage(sockaddr_in* clientAddress, char *message, int length);
};

#endif UdpServerH
