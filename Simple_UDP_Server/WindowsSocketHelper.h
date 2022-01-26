#ifndef WindowsSocketHelperH
#define WindowsSocketHelperH

#include <WinSock2.h>
#include <iostream>

class WindowsSocketHelper {
	WSAData _wsaData;

public:

	WindowsSocketHelper(u_short first, u_short second);
	~WindowsSocketHelper();
	WSAData getWSAData();

private:

	void initLibrary(u_short first, u_short second);
	void freeLibrary();
};

#endif WindowsSocketHelperH