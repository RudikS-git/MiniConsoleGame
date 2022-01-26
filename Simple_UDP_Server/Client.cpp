#include "Client.h"

Client::Client(SOCKET socket, sockaddr_in address)
{
	_socket = socket;
	address = address;
}

void Client::setSocket(SOCKET socket)
{
	_socket = socket;
}

SOCKET Client::getSocket()
{
	return _socket;
}

void Client::setAddress(sockaddr_in address)
{
	_address = address;
}

sockaddr_in Client::getAddress()
{
	return _address;
}

Client::~Client()
{
	this->disconnect();
}

void Client::disconnect()
{
	closesocket(_socket);
}