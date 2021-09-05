#define NOMINMAX
#include "stdafx.h"
#include "SMGFramework.h"
#include "D3DApp.h"
#include "UIManager.h"
#include "StageManager.h"
#include <windowsx.h>
#include "CharacterInfoManager.h"
#include "Camera.h"
#include "UserData.h"
#include "Effect.h"

SMGFramework::SMGFramework(HINSTANCE hInstance)
	: _clientWidth(1280)
	, _clientHeight(720)
	, _minimized(false)
	, _maximized(false)
	, _resizing(false)
	, _hInstance(hInstance)
	, _hMainWnd(nullptr)
	, _mousePos{ 0, 0 }
	, _drawCollisionBox(true)
{
	check(hInstance != nullptr, "hInstance is null");
	for (int i = 0; i < static_cast<int>(ButtonInputType::Count); ++i)
	{
		_buttonInput[i] = ButtonState::None;
	}
	for (int i = 0; i < static_cast<int>(StickInputType::Count); ++i)
	{
		_stickInput[i] = DirectX::XMFLOAT2(0, 0);
		_stickInputState[i] = StickInputState::None;
	}
}

std::unique_ptr<SMGFramework> SMGFramework::_instance = nullptr;

SMGFramework::~SMGFramework()
{
}

void SMGFramework::Create(HINSTANCE hInstance)
{
	check(_instance == nullptr);
	_instance = std::make_unique<SMGFramework>(hInstance);
	_instance->initMainWindow();

	_instance->_d3dApp = std::make_unique<D3DApp>();
	_instance->_characterInfoManager = std::make_unique<CharacterInfoManager>();
	_instance->_stageManager = std::make_unique<StageManager>();
	_instance->_uiManager = std::make_unique<UIManager>();
	_instance->_camera = std::make_unique<Camera>();
	_instance->_userData = std::make_unique<UserData>();
	_instance->_effectManager = std::make_unique<EffectManager>();

	_instance->_stageManager->loadStage();
	_instance->_uiManager->loadUI();
}


void SMGFramework::Destroy(void)
{
	_instance->_stageManager->releaseObjects();

	_instance->_effectManager = nullptr;
	_instance->_camera = nullptr;
	_instance->_uiManager = nullptr;
	_instance->_stageManager = nullptr;
	_instance->_characterInfoManager = nullptr;
	_instance->_d3dApp = nullptr;

	_instance = nullptr;
}

SMGFramework& SMGFramework::Get(void)
{
	return *_instance;
}

UIManager* SMGFramework::getUIManager(void) noexcept
{
	check(_instance != nullptr);
	check(_instance->_uiManager != nullptr);
	return _instance->_uiManager.get();
}

D3DApp* SMGFramework::getD3DApp(void) noexcept
{
	check(_instance != nullptr);
	check(_instance->_d3dApp != nullptr);
	return _instance->_d3dApp.get();
}

CharacterInfoManager* SMGFramework::getCharacterInfoManager(void) noexcept
{
	check(_instance != nullptr);
	check(_instance->_characterInfoManager != nullptr);
	return _instance->_characterInfoManager.get();
}

StageManager* SMGFramework::getStageManager(void) noexcept
{
	check(_instance != nullptr);
	check(_instance->_stageManager != nullptr);
	return _instance->_stageManager.get();
}

Camera* SMGFramework::getCamera(void) noexcept
{
	check(_instance != nullptr);
	check(_instance->_camera != nullptr);
	return _instance->_camera.get();
}

UserData* SMGFramework::getUserData(void) noexcept
{
	check(_instance != nullptr);
	check(_instance->_userData != nullptr);
	return _instance->_userData.get();
}

EffectManager* SMGFramework::getEffectManager(void) noexcept
{
	check(_instance != nullptr);
	check(_instance->_effectManager != nullptr);
	return _instance->_effectManager.get();
}

ButtonState SMGFramework::getButtonInput(const ButtonInputType type) const noexcept
{
	return _buttonInput[static_cast<int>(type)];
}

StickInputState SMGFramework::getStickInputState(const StickInputType type) const noexcept
{
	return _stickInputState[static_cast<int>(type)];
}

DirectX::XMFLOAT2 SMGFramework::getStickInput(const StickInputType type) const noexcept
{
	return _stickInput[static_cast<int>(type)];
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
	const size_t typeIndex = static_cast<int>(type);
	float x = _stickInput[typeIndex].x + dx;
	float y = _stickInput[typeIndex].y - dy;
	float length = std::sqrt(x * x + y * y);
	if (length > 1.f)
	{
		x /= length;
		y /= length;
		length = 1.f;
	}

	if (y <= x && y <= -x)
	{
		_stickInputState[typeIndex] = StickInputState::Front;
	}
	else if (y >= x && y <= -x)
	{
		_stickInputState[typeIndex] = StickInputState::Left;
	}
	else if (y >= x && y >= -x)
	{
		_stickInputState[typeIndex] = StickInputState::Back;
	}
	else if (y <= x && y >= -x)
	{
		_stickInputState[typeIndex] = StickInputState::Right;
	}
	else
	{
		check(false, "stickInputState Error!");
	}

	if (length < 0.5f)
	{
		_stickInputState[typeIndex] = _stickInputState[typeIndex] & StickInputState::Short;
	}
	else
	{
		_stickInputState[typeIndex] = _stickInputState[typeIndex] & StickInputState::Long;
	}

	_stickInput[typeIndex].x = x;
	_stickInput[typeIndex].y = y;
}

void SMGFramework::resetStickInput(const StickInputType type) noexcept
{
	check(type < StickInputType::Count, "타입이 비정상입니다.");

	const size_t typeIndex = static_cast<size_t>(type);
	_stickInput[typeIndex] = { 0, 0 };

	_stickInputState[typeIndex] = StickInputState::None;
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
				
				if (_timer.getDeltaTickCount() != 0)
				{
					_stageManager->update();
					_camera->update();
					_effectManager->update();
					_d3dApp->Update();
					_d3dApp->Draw();
				}

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
			((MINMAXINFO*)lParam)->ptMaxTrackSize.x = 1920;
			((MINMAXINFO*)lParam)->ptMaxTrackSize.y = 1080;
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

	RECT R = { 0, 0, static_cast<LONG>(_clientWidth), static_cast<LONG>(_clientHeight) };
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

#if defined DEBUG | defined _DEBUG
		timePosStr += L"animTime: ";
		animString += L" name: ";
		const auto& skinnedInstanceList = _d3dApp->getSkinnedInstanceXXX();
		if (!skinnedInstanceList.empty())
		{
			timePosStr = std::to_wstring(skinnedInstanceList[0]->getLocalTickCount());
			USES_CONVERSION;
			animString = A2W(_d3dApp->_animationNameListDev[_d3dApp->_animationNameIndexDev].c_str());
		}
#endif

		std::wstring windowText = WINDOW_CAPTION + timePosStr + animString + L" fps: " + fpsStr + L" mspf: " + mspfStr;
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

#if defined DEBUG | defined _DEBUG
	if (GetAsyncKeyState('0') && 0x8000)
	{
		_drawCollisionBox = !_drawCollisionBox;
	}
	if (GetAsyncKeyState('9') && 0x8000)
	{
		_camera->toggleDevCam();
	}
	if (GetAsyncKeyState(VK_UP) && 0x8000)
	{
		_camera->addDevCamRadius(-2);
	}
	if (GetAsyncKeyState(VK_DOWN) && 0x8000)
	{
		_camera->addDevCamRadius(2);
	}
#endif
}

void SMGFramework::onMouseDown(WPARAM buttonState, int x, int y, ButtonInputType type) noexcept
{
	setButtonInput(type, true);
}

void SMGFramework::onMouseUp(WPARAM buttonState, int x, int y, ButtonInputType type) noexcept
{
	setButtonInput(type, false);
	if (type == ButtonInputType::LStickButton)
	{
		resetStickInput(StickInputType::LStick);
	}
	else if (type == ButtonInputType::RStickButton)
	{
		resetStickInput(StickInputType::RStick);
	}
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

	if((buttonState & MK_LBUTTON) == 0 && (buttonState & MK_RBUTTON) == 0)
	{
		setStickInput(StickInputType::Pointer, dx, dy);
	}

	_mousePos.x = x;
	_mousePos.y = y;

#if defined (DEBUG) | defined (_DEBUG)
	if ((buttonState & MK_RBUTTON) != 0)
	{
		_camera->addDevCamTheta(dx);
		_camera->addDevCamPhi(dy);
	}
#endif
}

