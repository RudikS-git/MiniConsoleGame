#include "UdpServer.h"

void UdpServer::createSocket()
{
	_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (_socket == INVALID_SOCKET)
	{
		throw std::exception("Не удалось создать socket: " + WSAGetLastError());
	}
}

UdpServer::UdpServer(u_short port) : Server(port) { }

void UdpServer::start()
{
	createSocket();
	bindSocket();
}

int UdpServer::getMessage(UdpResponse* udpResponse)
{
	int result;
	SOCKADDR_IN clientAddress;
	int clientAddressLength = sizeof(clientAddress);

	if (recvfrom(_socket, (char*)&result, sizeof(result), NULL, (sockaddr*)&clientAddress, &clientAddressLength) == SOCKET_ERROR)
	{
		throw std::exception("Произошла ошибка при попытке получить сообщение по UDP");
	}

	udpResponse->clientAddress = clientAddress;
	udpResponse->result = result;
}

void UdpServer::sendMessage(sockaddr_in* clientAddress, char *message, int length)
{
	sendto(_socket, message, length, NULL, (sockaddr*)clientAddress, sizeof(*clientAddress));
}
