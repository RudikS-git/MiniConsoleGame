#include "WindowsSocketHelper.h"

WindowsSocketHelper::WindowsSocketHelper(u_short first, u_short second)
{
	initLibrary(first, second);
}

WindowsSocketHelper::~WindowsSocketHelper()
{
	freeLibrary();
}


WSAData WindowsSocketHelper::getWSAData()
{
	return _wsaData;
}

void WindowsSocketHelper::initLibrary(u_short first, u_short second)
{
	if (WSAStartup(MAKEWORD(first, second), &_wsaData) != 0) // загружаем версию 2.2
	{
		throw std::exception("Не удалось загрузить библиотеку");
	}
}

void WindowsSocketHelper::freeLibrary()
{
	WSACleanup();
}
