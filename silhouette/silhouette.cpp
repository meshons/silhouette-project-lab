#include <iostream>
#include <windows.h>

#include "App.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	AllocConsole();

	FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;
	freopen_s(&fpstdin, "CONIN$", "r", stdin);
	freopen_s(&fpstdout, "CONOUT$", "w", stdout);
	freopen_s(&fpstderr, "CONOUT$", "w", stderr);

	App app = App(hInstance, nCmdShow);

	app.start();

	return 0;
}
