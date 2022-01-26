#include "TcpServer.h"

void TcpServer::createSocket()
{
	_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (_socket == INVALID_SOCKET)
	{
		throw std::exception("Не удалось создать socket: " + WSAGetLastError());
	}
}


TcpServer::TcpServer(u_short port) : Server(port) { }

TcpServer::~TcpServer()
{
	for (std::map<int, Client*>::iterator it = _clients->begin(); it != _clients->end(); ++it)
	{
		delete it->second;
	}

	delete _clients;
}


std::map<int, Client*>* TcpServer::getClients()
{
	return _clients;
}

void TcpServer::start()
{
	createSocket();
	bindSocket();
	listen(_socket, SOMAXCONN);
}

Client* TcpServer::acceptMessage()
{
	SOCKET sock = accept(_socket, (SOCKADDR*)&_address, &addressLength);

	if (sock != INVALID_SOCKET)
	{
		Client* client = new Client(sock, _address);
		(*_clients)[_clients->size()] = client;

		return client;
	}

	return NULL;
}

bool TcpServer::getMessage(Client *client, char *message, int length)
{
	if (recv(client->getSocket(), (char*)message, length, NULL) == SOCKET_ERROR)
	{
		return false;
	}

	return true;
}

void TcpServer::sendMessage(Client* client, char *message, int length)
{
	send(client->getSocket(), (char*)message, length, NULL);
}

void TcpServer::sendMessageToEveryone(char *message, int length, int* ignoreClients, int ignoreClientsLength)
{
	int clientCount = _clients->size();

	for (int i = 0; i < clientCount; i++)
	{
		if (ignoreClients != NULL && findClientId(i, ignoreClients, ignoreClientsLength) != -1)
		{
			continue;
		}

		sendMessage((*_clients)[i], message, length);
	}
}

bool TcpServer::hasClient(SOCKADDR_IN& clientAddress) {
	for (int i = 0; i < _clients->size(); i++)
	{
		SOCKADDR_IN address = (*_clients)[i]->getAddress();
		if ((clientAddress.sin_addr.S_un.S_addr == address.sin_addr.S_un.S_addr) && (clientAddress.sin_port == clientAddress.sin_port))
		{
			return true;
		}
	}

	return false;
}

void TcpServer::disconnectClient(int id)
{
	delete (*_clients)[id];
	_clients->erase(id);
}
