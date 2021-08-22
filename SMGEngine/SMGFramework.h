#pragma once
#include "GameTimer.h"
#include "TypeAction.h"

class D3DApp;
class UIManager;
class StageManager;
class CharacterInfoManager;
class Camera;

class SMGFramework
{
public:
	~SMGFramework();
	static void Create(HINSTANCE hInstance);
	static void Destroy(void);
	static SMGFramework& Get(void);
	static UIManager* getUIManager(void) noexcept; 
	static D3DApp* getD3DApp(void) noexcept; 
	static CharacterInfoManager* getCharacterInfoManager(void) noexcept;
	static StageManager* getStageManager(void) noexcept;
	static Camera* getCamera(void) noexcept;

	int Run(void);
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	uint32_t getClientWidth(void) const noexcept { return _clientWidth; };
	uint32_t getClientHeight(void) const noexcept { return _clientHeight; };
	HWND getHWnd(void) const noexcept { return _hMainWnd; }
	const GameTimer& getTimer(void) const noexcept { return _timer; }
private:
	void initMainWindow();
	bool isAppPaused(void) const noexcept;
	void calculateFrameStats(void) noexcept;
	void onKeyboardInput(void) noexcept;

	// 마우스 입력
	void onMouseDown(WPARAM buttonState, int x, int y, ButtonInputType type) noexcept;
	void onMouseUp(WPARAM buttonState, int x, int y, ButtonInputType type) noexcept;
	void onMouseMove(WPARAM buttonState, int x, int y) noexcept;

public:
	SMGFramework(HINSTANCE hInstance);
private:
	SMGFramework(const SMGFramework&) = delete;
	SMGFramework(SMGFramework&&) = delete;
	SMGFramework& operator=(const SMGFramework&) = delete;
	SMGFramework& operator=(SMGFramework&&) = delete;

	static SMGFramework* _instance;
	// 주요기능 매니저
	std::unique_ptr<D3DApp> _d3dApp;
	std::unique_ptr<UIManager> _uiManager;
	//std::unique_ptr<GameManager> _gameManager;
	//std::unique_ptr<SoundManager> _soundManager;
	std::unique_ptr<StageManager> _stageManager;
	std::unique_ptr<CharacterInfoManager> _characterInfoManager;
	std::unique_ptr<Camera> _camera;

	// 윈도우
	uint32_t _clientWidth;
	uint32_t _clientHeight;
	bool _minimized;
	bool _maximized;
	bool _resizing;

	HINSTANCE _hInstance;
	HWND _hMainWnd;

	// 타이머
	GameTimer _timer;
		
	enum class DevStringMode
	{
		Fps,
		PlayerAnimation,
		PlayerPosition,
		Camera,

		Count,
	};

	// 키보드/마우스 입력
	
public:
	ButtonState getButtonInput(const ButtonInputType type) const noexcept;
	StickInputState getStickInputState(const StickInputType type) const noexcept;
	DirectX::XMFLOAT2 getStickInput(const StickInputType type) const noexcept;
private:
	void setButtonInput(const ButtonInputType type, bool pressed) noexcept;
	ButtonState _buttonInput[static_cast<int>(ButtonInputType::Count)];

	void setStickInput(const StickInputType type, float dx, float dy) noexcept;
	void resetStickInput(const StickInputType type) noexcept;
	DirectX::XMFLOAT2 _stickInput[static_cast<int>(StickInputType::Count)];
	StickInputState _stickInputState[static_cast<int>(StickInputType::Count)];

	// 마우스 입력
	POINT _mousePos;
};