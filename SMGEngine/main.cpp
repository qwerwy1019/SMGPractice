#include "stdafx.h"
#include "SMGFramework.h"
#include "Exception.h"
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <dxgidebug.h>
//#include "vld.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	int rv = 0;
	do 
	{
		try
		{
			ThrowIfFailed(::CoInitialize(nullptr));
			SMGFramework::Create(hInstance);
		}
		catch (DxException& e)
		{
			MessageBox(nullptr, e.to_wstring().c_str(), L"�ʱ�ȭ ���� !", MB_RETRYCANCEL);
			rv = 1;
			break;
		}

		try
		{
			rv = SMGFramework::Get().Run();
		}
		catch (DxException& e)
		{
			MessageBox(nullptr, e.to_wstring().c_str(), L"���� �߻� !", MB_RETRYCANCEL);
			rv = 2;
			break;
		}
	} while (false);


	SMGFramework::Destroy();
	::CoUninitialize();

#if defined(DEBUG) | defined(_DEBUG)
	{
		Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
	}
#endif
	return rv;
}
