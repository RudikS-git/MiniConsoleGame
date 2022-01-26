#ifndef MessagesH
#define MessagesH

#include <iostream>

#pragma pack(push, 1)
struct UdpData {
	int port;
	int clients;
	int maxClients;
};

enum OutputTypeMessage {
	CLIENT_DISCONNECT = 0,
	QUESTION = 1,
	QUESTION_RESULT = 2,
	GAME_OVER = 3
};

#pragma pack(push, 1)
struct OutputClient
{
	int idClient;
	int score;
};

#pragma pack(push, 1)
struct OutputDataQuestion // to client
{
	int totalQuestions;
	int questionNumber;
	char question[16];
};

#pragma pack(push, 1)
struct OutputDataQuestionResult // to client
{
	OutputDataQuestion outputDataQuestion;
	int respondingClient;
	int answer;
	OutputClient dataClients[5];
};

struct OutputClientDisconnect // to client
{
	int clientId;
};

#pragma pack(push, 1)
struct OutputDataGameOver // to client
{
	int winner;
	int score;
};

enum InputTypeMessage {
	INPUT_CLIENT_DISCONNECT = 0,
	READY = 1,
	ANSWER = 2,
};


#pragma pack(push, 1)
struct InputDataAnswer // from client
{
	int answer;
};

#endif MessagesH