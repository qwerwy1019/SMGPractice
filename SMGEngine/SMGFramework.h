#pragma once
#include "GameTimer.h"
#include "TypeAction.h"

class D3DApp;
class UIManager;
class StageManager;

class SMGFramework
{
public:
	~SMGFramework();
	static void Create(HINSTANCE hInstance);
	static SMGFramework& Get(void);
	UIManager* getUIManager(void) const noexcept { return _uiManager.get(); }
	D3DApp* getD3DApp(void) const noexcept { return _d3dApp.get(); }
	int Run(void);
	// �� �̷��� ȣ���ؾ��ϳ�? Ȯ���ʿ� [2/24/2021 qwerwy]
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

	// ���콺 �Է�
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
	// �ֿ��� �Ŵ���
	std::unique_ptr<D3DApp> _d3dApp;
	std::unique_ptr<UIManager> _uiManager;
	//std::unique_ptr<GameManager> _gameManager;
	//std::unique_ptr<SoundManager> _soundManager;
	std::unique_ptr<StageManager> _stageManager;

	// ������
	uint32_t _clientWidth;
	uint32_t _clientHeight;
	bool _minimized;
	bool _maximized;
	bool _resizing;

	HINSTANCE _hInstance;
	HWND _hMainWnd;

	// Ÿ�̸�
	GameTimer _timer;

	// �÷��̾�
	DirectX::XMFLOAT3 _playerPos;
	DirectX::XMFLOAT4 _playerUpVector;
	float _playerRotation;

	
	enum class DevStringMode
	{
		Fps,
		PlayerAnimation,
		PlayerPosition,
		Camera,

		Count,
	};

	// Ű����/���콺 �Է�
	
public:
	ButtonState getButtonInput(const ButtonInputType type) const noexcept;
private:
	void setButtonInput(const ButtonInputType type, bool pressed) noexcept;
	ButtonState _buttonInput[static_cast<int>(ButtonInputType::Count)];
	void setStickInput(const StickInputType type, float dx, float dy) noexcept;
	void resetStickInput(const StickInputType type) noexcept;
	DirectX::XMFLOAT2 _stickInput[static_cast<int>(StickInputType::Count)];

	// ���콺 �Է�
	POINT _mousePos;
};