#ifndef TcpServerH
#define TcpServerH

#include <WinSock2.h>
#include <exception>
#include <map>
#include "Server.h"

class TcpServer : Server {
private:
	std::map<int, Client*>* _clients = new std::map<int, Client*>;

	void createSocket();

public:

	TcpServer(u_short port);
	~TcpServer();

	std::map<int, Client*>* getClients();

	void start();

	Client* acceptMessage();

	bool getMessage(Client* client, char *message, int length);

	void sendMessage(Client* client, char *message, int length);

	void sendMessageToEveryone(char *message, int length, int* ignoreClients = NULL, int ignoreClientsLength = -1);

	bool hasClient(SOCKADDR_IN& clientAddress);

	void disconnectClient(int id);

	static int findClientId(int id, int* array, int length)
	{
		for (int j = 0; j < length; j++)
		{
			if (array[j] == id)
			{
				return j;
			}
		}

		return -1;
	}
};

#endif TcpServerH