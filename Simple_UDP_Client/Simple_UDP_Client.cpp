#include <WinSock2.h>
#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <thread>
#include "Messages.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma warning (disable : 4996)

#define GAME_MESSAGE "Exit - 1, Continue - 2:"
#define PORT 27015

SOCKET UdpSocket;
SOCKET TcpSocket;
SOCKADDR_IN udpAddress;
SOCKADDR_IN tcpAddress;

int len = sizeof(udpAddress);
bool isReady = false;

struct Server
{
	std::string address;
	UdpData udpData;
};

std::vector<Server> serverList;
bool thr_stop = false;
bool messageThreadStop = false;

void onlineServers()
{
	while (true)
	{
		system("cls");
		std::cout << "Searching for servers... (Exit - 0)";

		const int check = -1;
		udpAddress.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		serverList.clear();
		
		// отправляем по широковещалке -1
		sendto(UdpSocket, (char*)&check, sizeof(int), NULL, (sockaddr*)&udpAddress, len);

		while (true)
		{
			timeval timeout = { 1, 0 }; // 1s 0ms
			fd_set s_set;
			FD_ZERO(&s_set); // Обнуляем множество
			FD_SET(UdpSocket, &s_set); // Заносим в него сокет 
			// определяем состояние сокета на возможность чтения из него
			// возвращает число сокетов, которые готовы к чтению. В нашем случае он единственный
			// Так как рабочих серверов может быть несколько, а recvfrom блокирует поток, то имеет смысл
			// проверять сначала на возможность считать и только потом считывать
			int select_res = select(NULL, &s_set, NULL, NULL, &timeout);

			// если timeout вышел или socket error
			if (select_res == 0 || select_res == SOCKET_ERROR)
			{
				break;
			}
			
			UdpData udpData;
			if (recvfrom(UdpSocket, (char*)&udpData, sizeof(UdpData), NULL, (sockaddr*)&udpAddress, &len) != SOCKET_ERROR) // если есть ответ, то сервер подходящий
			{
				Server server;
				server.address = inet_ntoa(udpAddress.sin_addr);
				server.udpData = udpData;
				serverList.push_back(server);
			}
		}

		if (thr_stop)
			break;
	}
}

void writeAnswer()
{
	std::string answer;

	while (true)
	{
		std::cin.clear();
		std::cin >> answer;

		if (messageThreadStop) // завершаем поток
			break;

		if (!isReady)
		{
			system("cls");

			if (answer[0] == '1')
			{
				int check = -1;
				send(TcpSocket, (char*)&check, sizeof(check), NULL);
				send(TcpSocket, (char*)&check, sizeof(check), NULL);
				exit(0);
			}

			OutputTypeMessage type = OutputTypeMessage::READY;
			send(TcpSocket, (char*)&type, sizeof(OutputTypeMessage), NULL);
			std::cout << "Waiting for players:" << std::endl;

			isReady = true;
		}
		else
		{
			OutputDataAnswer outputDataAnswer;

			try
			{
				outputDataAnswer.answer = std::stoi(answer);
			}
			catch (std::invalid_argument e)
			{
				std::cout << "This is not a number!" << std::endl;
				continue;
			}

			OutputTypeMessage type = OutputTypeMessage::ANSWER;
			send(TcpSocket, (char*)&type, sizeof(OutputTypeMessage), NULL);
			send(TcpSocket, (char*)&outputDataAnswer, sizeof(OutputDataAnswer), NULL);
		}
		
	}
}

void gameHandler()
{
	std::cout << GAME_MESSAGE << std::endl;
	InputTypeMessage inputType;
	InputDataQuestion inputDataQuestion;
	InputClientDisconnect clientDisconnect;

	std::thread writeAnswerThread(writeAnswer); // отдельный поток для ввода, т.к другие игроки могут быстрее ответить
	writeAnswerThread.detach(); // не дожидаемся

	while ((recv(TcpSocket, (char*)&inputType, sizeof(InputTypeMessage), NULL) != SOCKET_ERROR))
	{
		switch (inputType)
		{
		case InputTypeMessage::QUESTION:
			recv(TcpSocket, (char*)&inputDataQuestion, sizeof(InputDataQuestion), NULL);
			std::cout << inputDataQuestion.questionNumber << ") What is: " << inputDataQuestion.question << "?" << std::endl;
			break;

		case InputTypeMessage::QUESTION_RESULT:
			InputDataQuestionResult dataQuestionResult;
			recv(TcpSocket, (char*)&dataQuestionResult, sizeof(InputDataQuestionResult), NULL);

			if (dataQuestionResult.answer == -1)
			{
				std::cout << "This answer is wrong! Waiting other players..." << std::endl;
				break;
			}

			if (dataQuestionResult.respondingClient == -1)
			{
				std::cout << "No one couldn't answer this question correctly" << "(" << inputDataQuestion.question << "=" << dataQuestionResult.answer << ")" << std::endl;
				break;
			}

			std::cout << "This question (" << inputDataQuestion.question << "=" << dataQuestionResult.answer << ") has been answered first by Client " << dataQuestionResult.respondingClient << std::endl;

			for (int i = 0; i < MAX_SLOTS; i++)
			{
				if (dataQuestionResult.dataClients[i].idClient >= 0)
				{
					std::cout << "Client " << dataQuestionResult.dataClients[i].idClient << " | score - " << dataQuestionResult.dataClients[i].score << std::endl;
				}
			}

			break;

		case InputTypeMessage::GAME_OVER:
			InputDataGameOver dataGameOver;
			recv(TcpSocket, (char*)&dataGameOver, sizeof(InputDataGameOver), NULL);
			std::cout << "Client " << dataGameOver.winner << " has won this game with the score - " << dataGameOver.score << std::endl;

			isReady = false;
			std::cout << GAME_MESSAGE << std::endl;

			break;

		case InputTypeMessage::CLIENT_DISCONNECT:
			recv(TcpSocket, (char*)&clientDisconnect, sizeof(InputClientDisconnect), NULL);
			std::cout << "Client " << clientDisconnect.clientId << " has left the game" << std::endl;
			std::cout << "Game over" << std::endl;

			isReady = false;
			std::cout << GAME_MESSAGE << std::endl;

			break;
		}
	}

	messageThreadStop = true;
	std::cout << "Some error occured with your connection to this server. Enter any key to continue..." << std::endl;
}

int selectServer()
{
	int numIp;
	while (true)
	{
		system("cls");
		std::cout << "The following servers has been found :\n" << std::endl;
		for (int i = 0; i < serverList.size(); i++)
		{
			std::cout << "Server " << i + 1 << " - " << serverList[i].address << ":" << serverList[i].udpData.port << " - connected - " << serverList[i].udpData.clients << "/" << serverList[i].udpData.maxClients << std::endl;
		}

		std::cout << "\nSelect the number of the server which you would like to connect: " << std::endl;
		std::cin >> numIp;
		if (!std::cin || numIp < 1 || numIp > serverList.size())
		{
			std::cin.clear();
			std::cin.get();
			continue;
		}

		if (serverList[numIp - 1].udpData.clients == serverList[numIp - 1].udpData.maxClients)
		{
			continue;
		}

		break;
	}

	return numIp;
}

bool connectToTcpServer(int ipNum)
{
	int msgToTCP = 0;
	udpAddress.sin_addr.s_addr = inet_addr(serverList[ipNum - 1].address.c_str());
	sendto(UdpSocket, (char*)&msgToTCP, sizeof(msgToTCP), NULL, (sockaddr*)&udpAddress, len); // отправим сигнал серверу, что хотим перейти на TCP
	if (recvfrom(UdpSocket, (char*)&msgToTCP, sizeof(msgToTCP), NULL, (sockaddr*)&udpAddress, &len) != SOCKET_ERROR) // если есть ответ 1, то разрешается присоединение
	{
		if (msgToTCP == 1)
		{
			system("cls");
			std::cout << "Connecting to TCP server..." << std::endl;
			tcpAddress.sin_addr.s_addr = udpAddress.sin_addr.s_addr;
			tcpAddress.sin_port = htons(PORT);
			tcpAddress.sin_family = AF_INET;
			TcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			if (connect(TcpSocket, (SOCKADDR*)&tcpAddress, sizeof(tcpAddress)))
			{
				std::cout << "Can`t connect to TCP";
				return false;
			}

			std::cout << "Server connection" << std::endl;
			return true;
		}
	}

	return false;
}

bool loadLibrary()
{
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return false;
	}

	return true;
}

int main()
{
	if (!loadLibrary())
		return 0;

	udpAddress.sin_family = AF_INET;
	udpAddress.sin_port = htons(PORT);

	if ((UdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		std::cout << "Не удалось создать сокет";
		return -1;
	}
	
	// socket options - для изменения флагов на сокете
	// SOL_SOCKET - common option for socket
	// SO_BROADCAST - allow sending broadcast message
	// optVal должен быть ненулевым, чтобы установить флаг, иначе он сбросит указанный флаг
	int optVal = 1;
	if (setsockopt(UdpSocket, SOL_SOCKET, SO_BROADCAST, (char*)&optVal, sizeof(optVal)))
	{
		std::cout << "При установке флагов на сокет произошла ошибка";
		return -1;
	}

	int* message = new int;

	std::thread searchForServers(onlineServers);
	searchForServers.detach();

	while (true)
	{
		if (!(*message)) // выход из программы
			break;

		if (!serverList.size())
		{
			thr_stop = false;
			continue;
		}
		else
		{
			thr_stop = true;
			int numIp = selectServer();

			if (numIp > 0 && numIp <= serverList.size() && connectToTcpServer(numIp))
			{
				gameHandler(); // блокирующий
				serverList.clear();
				thr_stop = false;
				std::cin.clear();
				std::cin >> *message;
				
			}
		}
	}

	closesocket(UdpSocket);
	closesocket(TcpSocket);
	WSACleanup();

	return 0;
}
