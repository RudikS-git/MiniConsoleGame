#include <exception>
#include "Server.h"

void Server::freeResources()
{
	closesocket(_socket);
}

void Server::initAddress(u_short port)
{
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(port);
}

void Server::bindSocket()
{
	if (bind(_socket, (SOCKADDR*)&_address, addressLength) == SOCKET_ERROR)
	{
		freeResources();
		throw std::exception("Не удалось привязать адрес к сокету: " + WSAGetLastError());
	}
}

Server::Server(u_short port)
{
	initAddress(port);
}

Server::~Server()
{
	freeResources();
}