#include "stdafx.h"
#include "SMGFramework.h"
#include "Exception.h"
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
//#include "vld.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(72);
#endif
	int rv = 0;
	try
	{
		SMGFramework::Create(hInstance);

		rv = SMGFramework::Get().Run();
		SMGFramework::Destroy();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.to_wstring().c_str(), L"초기화 실패 !", MB_RETRYCANCEL);
		return 1;
	}
#if defined(DEBUG) | defined(_DEBUG)
	_CrtDumpMemoryLeaks();
#endif
	return rv;
}
