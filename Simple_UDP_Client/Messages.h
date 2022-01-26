#ifndef MessagesH
#define MessagesH

#include <iostream>

#define MAX_SLOTS 5

#pragma pack(push, 1)
struct UdpData {
	int port = -1;
	int clients = -1;
	int maxClients = -1;
};

enum InputTypeMessage {
	CLIENT_DISCONNECT = 0,
	QUESTION = 1,
	QUESTION_RESULT = 2,
	GAME_OVER = 3
};

#pragma pack(push, 1)
struct InputClient
{
	int idClient;
	int score;
};

#pragma pack(push, 1)
struct InputDataQuestion // to client
{
	int totalQuestions;
	int questionNumber;
	char question[16];
};

#pragma pack(push, 1)
struct InputDataQuestionResult // to client
{
	InputDataQuestion outputDataQuestion;
	int respondingClient;
	int answer;
	InputClient dataClients[MAX_SLOTS];
};

struct InputClientDisconnect // to client
{
	int clientId;
};

#pragma pack(push, 1)
struct InputDataGameOver // to client
{
	int winner;
	int score;
};

enum OutputTypeMessage {
	INPUT_CLIENT_DISCONNECT = 0,
	READY = 1,
	ANSWER = 2,
};


#pragma pack(push, 1)
struct OutputDataAnswer // from client
{
	int answer;
};

#endif MessagesH