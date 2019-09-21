#pragma once

#if defined(_WIN32)

#define ENTRY_POINT(AppName)																				\
																											\
int WINAPI wWinMain(_In_ HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)		\
{																											\
	AppName app;																							\
	app.Run();																								\
																											\
	return 0;																								\
}																											\
																											\
int __cdecl main(int inArgC, char **inArgV)																	\
{																											\
	AppName app;																							\
	app.Run();																								\
																											\
	return 0;																								\
}

#else
#error Undefined
#endif
