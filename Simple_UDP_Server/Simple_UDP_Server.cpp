#include <WinSock2.h>
#include <iostream>
#include <vector>
#include <thread>
#include <map>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <list>
#include <mutex>

#include "Client.h"
#include "Server.h"
#include "WindowsSocketHelper.h"
#include "Messages.h"
#include "UdpServer.h"
#include "TcpServer.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma warning (disable : 4996)

#define PORT 27015

std::mutex mtx;
std::mutex disconnectClientMutex;

class GameServer {

private:
	static const int MAX_SLOTS = 5;
	static const int TOTAL_QUESTIONS = 5;

	u_short _port;

	UdpServer *_udpServer;
	TcpServer *_tcpServer;

	struct Player {
		int score = 0;
		bool hasAnswered = false;
	};

	struct ConnectionCommonData
	{
		TcpServer* tcpServer;
		volatile  bool state = true;
		volatile bool hasGame = false;

		std::string question;
		volatile  int answer = 0;
		volatile  int totalQuestions = TOTAL_QUESTIONS;
		volatile  int currentQuestion = 0;

		std::map<int, Player> players;
	};

	struct ConnectionData
	{
		int clientId;
		ConnectionCommonData *connectionCommonData;
	};

	struct Question {
		std::string strQuestion;
		int answer;
	};


private:

	static void ClientConnection(ConnectionData *connectionData)
	{
		ConnectionCommonData *commonData = connectionData->connectionCommonData;
		TcpServer *tcpServer = commonData->tcpServer;
		std::map<int, Client*> *clients = tcpServer->getClients();
		std::map<int, Player> *players = &commonData->players;
		int id = connectionData->clientId;
		int ignoreClients[] = { id };
		int typeMessage;

		while (commonData->state)
		{
			if (!tcpServer->getMessage((*clients)[id], (char*)&typeMessage, sizeof(typeMessage)))
			{
				disconnectClientMutex.lock();
				clientDisconnectHandler(id, commonData, ignoreClients);
				disconnectClientMutex.unlock();
				break;
			}
			else
			{
				if (typeMessage == InputTypeMessage::READY)
				{
					readyHandler(id, commonData);
				}
				else if (typeMessage == InputTypeMessage::ANSWER)
				{
					if (!commonData->hasGame)
					{
						continue;
					}

					if ((*players)[id].hasAnswered)
					{
						continue;
					}

					InputDataAnswer inputDataAnswer;
					tcpServer->getMessage((*clients)[id], (char*)&inputDataAnswer, sizeof(InputDataAnswer));
					(*players)[id].hasAnswered = true;

					if (inputDataAnswer.answer == commonData->answer) // right answer
					{
						mtx.lock(); // sync

						if ((*players)[id].hasAnswered == false) // only one winner
						{
							mtx.unlock();
							continue;
						}

						commonData->currentQuestion++;
						(*players)[id].score++;

						if (commonData->currentQuestion == commonData->totalQuestions) // game over
						{
							gameOverHandler(commonData);
							mtx.unlock();
							continue;
						}

						OutputTypeMessage type = OutputTypeMessage::QUESTION_RESULT;
						tcpServer->sendMessageToEveryone((char*)&type, sizeof(OutputTypeMessage));

						OutputDataQuestionResult questionResult;
						questionResult.respondingClient = id;
						strcpy_s(questionResult.outputDataQuestion.question, sizeof(questionResult.outputDataQuestion.question), commonData->question.c_str());
						questionResult.outputDataQuestion.totalQuestions = commonData->totalQuestions;
						questionResult.outputDataQuestion.questionNumber = commonData->currentQuestion;
						questionResult.answer = commonData->answer;

						for (int i = 0; i < clients->size(); i++)
						{
							if ((*players).find(i) != (*players).end())
							{
								questionResult.dataClients[i].idClient = i;
								questionResult.dataClients[i].score = (*players)[i].score;
							}
							else
							{
								questionResult.dataClients[i].idClient = -1;
							}
						}

						tcpServer->sendMessageToEveryone((char*)&questionResult, sizeof(OutputDataQuestionResult));
						mtx.unlock();

						// prepare new question and send to all clients
						OutputTypeMessage typeMessage = OutputTypeMessage::QUESTION;
						tcpServer->sendMessageToEveryone((char*)&typeMessage, sizeof(OutputTypeMessage));

						OutputDataQuestion outputDataQuestion;
						CreateQuestionWithPacket(commonData, outputDataQuestion);
						tcpServer->sendMessageToEveryone((char*)&outputDataQuestion, sizeof(OutputDataQuestion));

						for (auto& pair : *players) {
							pair.second.hasAnswered = false;
						}
					}
					else
					{
						incorrectAnswerHandler(id, commonData);
					}
				}
			}
		}

		
		commonData->hasGame = false;
		commonData->currentQuestion = 0;

		std::cout << "Client " << id << " disconnected" << std::endl;
		tcpServer->disconnectClient(id);
		if (connectionData != NULL)
		{
			delete connectionData;
		}
	}

	// TCP прослушка
	static void Connecting(ConnectionCommonData* connectionCommonData)
	{
		TcpServer *tcpServer = connectionCommonData->tcpServer;
		std::map<int, Client*>* clients = tcpServer->getClients();

		while (true)
		{
			Client* client = tcpServer->acceptMessage();

			if (client != NULL)
			{
				std::cout << "Client " << clients->size() << " connect..." << std::endl;
				ConnectionData* privateData = new ConnectionData();
				privateData->clientId = connectionCommonData->tcpServer->getClients()->size() - 1;
				privateData->connectionCommonData = connectionCommonData;
				CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientConnection, (LPVOID)privateData, NULL, NULL);
			}
		}
		
	}

	static void gameOverHandler(ConnectionCommonData *commonData)
	{
		auto players = &commonData->players;
		auto tcpServer = commonData->tcpServer;
		auto clients = tcpServer->getClients();

		OutputTypeMessage type = OutputTypeMessage::GAME_OVER;
		tcpServer->sendMessageToEveryone((char*)&type, sizeof(OutputTypeMessage));

		auto player = std::max_element(players->begin(), players->end(),
			[](const std::pair<int, Player>& pf, const std::pair<int, Player>& ps) {
				return pf.second.score < ps.second.score;
			});

		OutputDataGameOver dataGameOver;
		dataGameOver.winner = player->first;
		dataGameOver.score = player->second.score;
		tcpServer->sendMessageToEveryone((char*)&dataGameOver, sizeof(OutputDataGameOver));

		commonData->players.clear();
		commonData->currentQuestion = 0;
		commonData->hasGame = false;
	}

	static void CreateQuestion(Question &question)
	{
		srand(time(0));
		const char operations[] = { '+', '-', '*' };

		int operationIndex = rand() % 3; // 0-2
		int firstNumber = rand() % 33; // 0-32
		int secondNumber = firstNumber - rand() % 16; // 2 число всегда было меньше
		int answer;

		switch (operationIndex)
		{
			case 0:
				answer = firstNumber + secondNumber;
				break;
			case 1:
				answer = firstNumber - secondNumber;
				break;
			case 2:
				answer = firstNumber * secondNumber;
				break;
		}

		question.strQuestion = std::to_string(firstNumber) + operations[operationIndex] + std::to_string(secondNumber);
		question.answer = answer;
	}

	static void CreateQuestionWithPacket(ConnectionCommonData *commonData, OutputDataQuestion &outputDataQuestion)
	{
		Question question;
		CreateQuestion(question);

		const char* strQuestion = question.strQuestion.c_str();
		strcpy_s(outputDataQuestion.question, sizeof(outputDataQuestion.question), strQuestion);
		outputDataQuestion.totalQuestions = commonData->totalQuestions;
		outputDataQuestion.questionNumber = commonData->currentQuestion;

		commonData->question = strQuestion;
		commonData->answer = question.answer;
	}

	static void clientDisconnectHandler(int id, ConnectionCommonData* commonData, int *ignoreClients)
	{
		auto players = &commonData->players;
		auto tcpServer = commonData->tcpServer;
		auto clients = tcpServer->getClients();

		players->erase(id);

		// some client in this thread has disconnected from the server
		OutputTypeMessage type = OutputTypeMessage::CLIENT_DISCONNECT;
		tcpServer->sendMessageToEveryone((char*)&type, sizeof(OutputTypeMessage), ignoreClients, 1);

		OutputClientDisconnect clientDisconnect;
		clientDisconnect.clientId = id;
		tcpServer->sendMessageToEveryone((char*)&clientDisconnect, sizeof(OutputClientDisconnect), ignoreClients, 1);
	}

	static void readyHandler(int id, ConnectionCommonData *commonData)
	{
		auto players = &commonData->players;
		auto tcpServer = commonData->tcpServer;
		auto clients = tcpServer->getClients();

		if (!commonData->hasGame)
		{
			if (players->find(id) == players->end()) // If this player is not yet ready
			{
				Player player;
				players->insert(std::make_pair(id, player));
			}

			if (players->size() == clients->size()) // All players are ready
			{
				commonData->hasGame = true;
				OutputTypeMessage type = OutputTypeMessage::QUESTION;
				tcpServer->sendMessageToEveryone((char*)&type, sizeof(OutputTypeMessage));

				OutputDataQuestion outputDataQuestion;
				CreateQuestionWithPacket(commonData, outputDataQuestion);
				tcpServer->sendMessageToEveryone((char*)&outputDataQuestion, sizeof(OutputDataQuestion));
			}
		}
	}

	static void incorrectAnswerHandler(int id, ConnectionCommonData *connectionCommonData)
	{
		auto tcpServer = connectionCommonData->tcpServer;
		auto clients = tcpServer->getClients();
		auto players = &connectionCommonData->players;

		auto it = std::find_if(players->begin(), players->end(), [](const std::pair<int, Player>& player) {
			return !player.second.hasAnswered;
			});

		if (it == players->end())
		{
			connectionCommonData->currentQuestion++;

			OutputTypeMessage type = OutputTypeMessage::QUESTION_RESULT;
			tcpServer->sendMessageToEveryone((char*)&type, sizeof(OutputTypeMessage));

			OutputDataQuestionResult questionResult;
			questionResult.respondingClient = -1;
			questionResult.answer = connectionCommonData->answer;

			for (int i = 0; i < clients->size(); i++)
			{
				if (players->find(i) != players->end())
				{
					questionResult.dataClients[i].idClient = i;
					questionResult.dataClients[i].score = (*players)[i].score;
				}
				else
				{
					questionResult.dataClients[i].idClient = -1;
				}
			}

			tcpServer->sendMessageToEveryone((char*)&questionResult, sizeof(OutputDataQuestionResult));

			if (connectionCommonData->currentQuestion == connectionCommonData->totalQuestions) // game over
			{
				gameOverHandler(connectionCommonData);
				return;
			}

			// prepare new question and send to all clients
			OutputTypeMessage typeMessage = OutputTypeMessage::QUESTION;
			tcpServer->sendMessageToEveryone((char*)&typeMessage, sizeof(OutputTypeMessage));

			OutputDataQuestion outputDataQuestion;
			CreateQuestionWithPacket(connectionCommonData, outputDataQuestion);
			tcpServer->sendMessageToEveryone((char*)&outputDataQuestion, sizeof(OutputDataQuestion));

			for (auto& pair : *players) {
				pair.second.hasAnswered = false;
			}
		}
		else
		{
			OutputTypeMessage type = OutputTypeMessage::QUESTION_RESULT;
			tcpServer->sendMessage((*clients)[id], (char*)&type, sizeof(OutputTypeMessage));

			OutputDataQuestionResult questionResult;
			questionResult.answer = -1;
			tcpServer->sendMessage((*clients)[id], (char*)&questionResult, sizeof(OutputDataQuestionResult));
		}
	}


public:

	GameServer(u_short port)
	{
		_port = port;

		_udpServer = new UdpServer(_port);
		_tcpServer = new TcpServer(_port);
	}

	~GameServer()
	{
		delete _udpServer;
		delete _tcpServer;
	}

	void start()
	{
		_udpServer->start();
		_tcpServer->start();
		std::cout << " Game Server starting... \n";
		std::map<int, Client*> *clients = _tcpServer->getClients();

		ConnectionCommonData *connectionData = new ConnectionCommonData();
		connectionData->state = true;
		connectionData->tcpServer = _tcpServer;
		connectionData->answer = 0;
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Connecting, (LPVOID)connectionData, NULL, NULL);  // поток на прослушку tcp

		while (true)
		{
			UdpServer::UdpResponse udpResponse;
			_udpServer->getMessage(&udpResponse); // блокирующий метод

			if (udpResponse.result < 0)
			{
				UdpData udpData;
				udpData.port = PORT;
				udpData.maxClients = MAX_SLOTS;
				udpData.clients = clients->size();

				_udpServer->sendMessage(&udpResponse.clientAddress, (char*)&udpData, sizeof(UdpData)); // отправляем информацию о tcp сервере
			}
			else // клиент отправит значение 0, чтобы выполнить подключение по TCP
			{
				int message;
				if (udpResponse.result == 0 && clients->size() < MAX_SLOTS && !_tcpServer->hasClient(udpResponse.clientAddress))
				{
					message = 1;
					_udpServer->sendMessage(&udpResponse.clientAddress, (char*)&message, sizeof(message)); // отправляем разрешение					
				}
				else
				{
					message = -1;
					_udpServer->sendMessage(&udpResponse.clientAddress, (char*)&message, sizeof(message)); // отправляем запрет
				}
			}
		}

		delete connectionData;
	}
};

int main()
{
	try 
	{ 
		int port;
		std::cout << "Server port:" << std::endl;
		std::cin >> port;

		WindowsSocketHelper windowsSocketHelper(2, 2);
		GameServer gameServer = GameServer(port);
		gameServer.start();
	}
	catch (std::exception e)
	{
		std::cout << "Произошла ошибка: " << e.what();
	}

	return 0;
}
