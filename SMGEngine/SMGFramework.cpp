#define NOMINMAX
#include "stdafx.h"
#include "SMGFramework.h"
#include "D3DApp.h"
#include "UIManager.h"
#include "StageManager.h"
#include <windowsx.h>
#include "CharacterInfoManager.h"

SMGFramework::SMGFramework(HINSTANCE hInstance)
	: _hInstance(hInstance)
	, _hMainWnd(nullptr)
	, _clientWidth(800)
	, _clientHeight(600)
	, _minimized(false)
	, _maximized(false)
	, _resizing(false)
{
	check(hInstance != nullptr, "hInstance is null");
}

SMGFramework* SMGFramework::_instance = nullptr;

SMGFramework::~SMGFramework()
{
}

void SMGFramework::Create(HINSTANCE hInstance)
{
	check(_instance == nullptr);
	_instance = new SMGFramework(hInstance);
	_instance->initMainWindow();

	_instance->_d3dApp = std::make_unique<D3DApp>();
	_instance->_uiManager = std::make_unique<UIManager>();
	_instance->_stageManager = std::make_unique<StageManager>();
	_instance->_characterInfoManager = std::make_unique<CharacterInfoManager>();
}


SMGFramework& SMGFramework::Get(void)
{
	return *_instance;
}

UIManager* SMGFramework::getUIManager(void) noexcept
{
	check(_instance != nullptr);
	return _instance->_uiManager.get();
}

D3DApp* SMGFramework::getD3DApp(void) noexcept
{
	check(_instance != nullptr);
	return _instance->_d3dApp.get();
}

CharacterInfoManager* SMGFramework::getCharacterInfoManager(void) noexcept
{
	check(_instance != nullptr);
	return _instance->_characterInfoManager.get();
}

StageManager* SMGFramework::getStageManager(void) noexcept
{
	check(_instance != nullptr);
	return _instance->_stageManager.get();
}

ButtonState SMGFramework::getButtonInput(const ButtonInputType type) const noexcept
{
	return _buttonInput[static_cast<int>(type)];
}

void SMGFramework::setButtonInput(const ButtonInputType type, bool pressed) noexcept
{
	check(type < ButtonInputType::Count, "buttonInputType이 이상합니다.");

	if (pressed)
	{
		switch (_buttonInput[static_cast<int>(type)])
		{
			case ButtonState::Down:
				_buttonInput[static_cast<int>(type)] = ButtonState::Press;
				break;
			case ButtonState::Press:
				_buttonInput[static_cast<int>(type)] = ButtonState::Press;
				break;
			case ButtonState::Up:
				_buttonInput[static_cast<int>(type)] = ButtonState::Down;
				break;
			case ButtonState::None:
				_buttonInput[static_cast<int>(type)] = ButtonState::Down;
				break;
			default:
				check(false, "타입이 추가되었는지 확인 필요.");
				static_assert(static_cast<int>(ButtonState::None) == 3, "타입 추가되었다면 작업되어야함.");
				break;
		}
	}
	else
	{
		switch (_buttonInput[static_cast<int>(type)])
		{
			case ButtonState::Down:
				_buttonInput[static_cast<int>(type)] = ButtonState::Up;
				break;
			case ButtonState::Press:
				_buttonInput[static_cast<int>(type)] = ButtonState::Up;
				break;
			case ButtonState::Up:
				_buttonInput[static_cast<int>(type)] = ButtonState::None;
				break;
			case ButtonState::None:
				_buttonInput[static_cast<int>(type)] = ButtonState::None;
				break;
			default:
				check(false, "타입이 추가되었는지 확인 필요.");
				static_assert(static_cast<int>(ButtonState::None) == 3, "타입 추가되었다면 작업되어야함.");
				break;
		}
	}
}

void SMGFramework::setStickInput(const StickInputType type, float dx, float dy) noexcept
{
	check(type < StickInputType::Count, "타입이 비정상입니다.");
	_stickInput[static_cast<int>(type)].x += dx;
	_stickInput[static_cast<int>(type)].y += dy;
}

void SMGFramework::resetStickInput(const StickInputType type) noexcept
{
	check(type < StickInputType::Count, "타입이 비정상입니다.");

	_stickInput[static_cast<int>(type)] = { 0, 0 };
}

int SMGFramework::Run(void)
{
	MSG msg = { 0 };
	_timer.Reset();
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (_stageManager->isLoading())
			{
				_stageManager->loadStage();
			}

			_timer.ProgressTick();
			if (!isAppPaused())
			{
				calculateFrameStats();

				onKeyboardInput();
				//UpdateGameObject();
				_stageManager->update();
				_d3dApp->Update();

				_d3dApp->Draw();

				Sleep(10);
			}
			else
			{
				Sleep(100);
			}
		}
	}
	return (int)msg.wParam;
}

LRESULT SMGFramework::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				_timer.Stop();
			}
			else
			{
				_timer.Start();
			}
			return 0;
		case WM_SIZE:

			_clientWidth = LOWORD(lParam);
			_clientHeight = HIWORD(lParam);

			if (_d3dApp != nullptr)
			{
				if (wParam == SIZE_MINIMIZED)
				{
					_timer.Stop();
					_minimized = true;
					_maximized = false;
				}
				else if (wParam == SIZE_MAXIMIZED)
				{
					_timer.Start();
					_minimized = false;
					_maximized = true;
					_d3dApp->OnResize();
				}
				else if (wParam == SIZE_RESTORED)
				{
					if (_minimized)
					{
						_timer.Start();
						_minimized = false;
						_d3dApp->OnResize();
					}
					else if (_maximized)
					{
						_timer.Start();
						_maximized = false;
						_d3dApp->OnResize();
					}
					else if (_resizing)
					{
						__noop;
					}
					else
					{
						_d3dApp->OnResize();
					}
				}
			}
			return 0;
		case WM_ENTERSIZEMOVE:
			_timer.Stop();
			_resizing = true;
			return 0;
		case WM_EXITSIZEMOVE:
			_timer.Start();
			_resizing = false;
			_d3dApp->OnResize();
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_MENUCHAR:
			return MAKELRESULT(0, MNC_CLOSE);
		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 800;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 600;
			((MINMAXINFO*)lParam)->ptMaxTrackSize.x = 800;
			((MINMAXINFO*)lParam)->ptMaxTrackSize.y = 600;
			return 0;
		case WM_LBUTTONDOWN:
			onMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), ButtonInputType::LStickButton);
			return 0;
		case WM_RBUTTONDOWN:
			onMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), ButtonInputType::RStickButton);
			return 0;
		case WM_LBUTTONUP:
			onMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), ButtonInputType::LStickButton);
			return 0;
		case WM_RBUTTONUP:
			onMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), ButtonInputType::RStickButton);
			return 0;
		case WM_MOUSEMOVE:
			onMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_KEYUP:
			if (wParam == VK_ESCAPE)
			{
				PostQuitMessage(0);
			}
			else if (wParam == VK_F2)
			{
				//set4XMsaaState(!_4xMsaaState);
			}
			return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return SMGFramework::Get().MsgProc(hwnd, msg, wParam, lParam);
}

void SMGFramework::initMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = _hInstance;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		ThrowErrCode(ErrCode::InitFail, "RegisterClass Failed.");
	}

	RECT R = { 0, 0, _clientWidth, _clientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	_hMainWnd = CreateWindow(L"MainWnd", WINDOW_CAPTION.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, _hInstance, nullptr);
	if (nullptr == _hMainWnd)
	{
		ThrowErrCode(ErrCode::InitFail, "CreateWindow Failed.");
	}

	ShowWindow(_hMainWnd, SW_SHOW);
	UpdateWindow(_hMainWnd);
}



bool SMGFramework::isAppPaused(void) const noexcept
{
	return _timer.isTimerStopped();
}

void SMGFramework::calculateFrameStats(void) noexcept
{
	check(_hMainWnd != nullptr, "_hMainWnd가 초기화되지 않음");
	static int frameCnt = 0;
	static double timeElapsed = _timer.getTotalTime();

	++frameCnt;
	if (_timer.getTotalTime() - timeElapsed > 1.f)
	{
		std::wstring fpsStr = std::to_wstring(frameCnt);
		std::wstring mspfStr = std::to_wstring(1000.f / frameCnt);

		std::wstring timePosStr = L"";
		std::wstring animString = L"";
		if (!_d3dApp->_skinnedInstance.empty())
		{
			timePosStr = std::to_wstring(_d3dApp->_skinnedInstance[0]->getLocalTickCount());
			USES_CONVERSION;
			animString = A2W(_d3dApp->_animationNameListDev[_d3dApp->_animationNameIndexDev].c_str());
		}

		std::wstring windowText = WINDOW_CAPTION + L"animTime: " + timePosStr + L" name: " + animString + L" fps: " + fpsStr + L" mspf: " + mspfStr;
		SetWindowText(_hMainWnd, windowText.c_str());

		frameCnt = 0;
		timeElapsed += 1.f;
	}
}

void SMGFramework::onKeyboardInput(void) noexcept
{
	const float dt = _timer.getDeltaTime();
	if (GetAsyncKeyState('a') && 0x8000 ||
		GetAsyncKeyState('A') && 0x8000 ||
		GetAsyncKeyState('w') && 0x8000 ||
		GetAsyncKeyState('W') && 0x8000)
	{
		setButtonInput(ButtonInputType::XY, true);
	}
	else
	{
		setButtonInput(ButtonInputType::XY, false);
	}

	if (GetAsyncKeyState('s') && 0x8000 ||
		GetAsyncKeyState('S') && 0x8000 ||
		GetAsyncKeyState('d') && 0x8000 ||
		GetAsyncKeyState('D') && 0x8000)
	{
		setButtonInput(ButtonInputType::AB, true);
	}
	else
	{
		setButtonInput(ButtonInputType::AB, false);
	}

	if (GetAsyncKeyState('q') && 0x8000 ||
		GetAsyncKeyState('Q') && 0x8000)
	{
		setButtonInput(ButtonInputType::L, true);
	}
	else
	{
		setButtonInput(ButtonInputType::L, false);
	}

	if (GetAsyncKeyState('e') && 0x8000 ||
		GetAsyncKeyState('E') && 0x8000)
	{
		setButtonInput(ButtonInputType::R, true);
		resetStickInput(StickInputType::Pointer);
	}
	else
	{
		setButtonInput(ButtonInputType::R, false);
	}

	if (GetAsyncKeyState('1') && 0x8000)
	{
		setButtonInput(ButtonInputType::ZL, true);
	}
	else
	{
		setButtonInput(ButtonInputType::ZL, false);
	}

	if (GetAsyncKeyState('3') && 0x8000)
	{
		setButtonInput(ButtonInputType::ZR, true);
	}
	else
	{
		setButtonInput(ButtonInputType::ZR, false);
	}
}

void SMGFramework::onMouseDown(WPARAM buttonState, int x, int y, ButtonInputType type) noexcept
{
	setButtonInput(type, true);
}

void SMGFramework::onMouseUp(WPARAM buttonState, int x, int y, ButtonInputType type) noexcept
{
	setButtonInput(type, false);
}

void SMGFramework::onMouseMove(WPARAM buttonState, int x, int y) noexcept
{
	float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - _mousePos.x));
	float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - _mousePos.y));

	if ((buttonState & MK_LBUTTON) != 0)
	{
		setStickInput(StickInputType::LStick, dx, dy);
	}
	else
	{
		resetStickInput(StickInputType::LStick);
	}
	
	if ((buttonState & MK_RBUTTON) != 0)
	{
		setStickInput(StickInputType::RStick, dx, dy);
	}
	else
	{
		resetStickInput(StickInputType::RStick);
	}

	if((buttonState & MK_LBUTTON) == 0 && (buttonState & MK_RBUTTON) != 0)
	{
		setStickInput(StickInputType::Pointer, dx, dy);
	}

	_mousePos.x = x;
	_mousePos.y = y;
}

