#include "stdafx.h"
#include "SMGFramework.h"
#include "Exception.h"
#include <crtdbg.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		SMGFramework::Create(hInstance);

		return SMGFramework::Get().Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.to_wstring().c_str(), L"초기화 실패 !", MB_RETRYCANCEL);
		return 1;
	}
}
