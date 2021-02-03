#pragma once

#include "TypeGeometry.h"

#include "GameTimer.h"
#include "UploadBuffer.h"
#include "MeshGeometry.h"
#include "FrameResource.h"
#include "SkinnedData.h"

using namespace std;
using namespace DirectX;

// 전방선언
struct IDXGIAdapter;
struct IDXGIOutput;
class Material;

// 정점 관련
static constexpr size_t VERTEX_INPUT_DESC_SIZE = 3;
const D3D12_INPUT_ELEMENT_DESC VERTEX_INPUT_LAYOUT[VERTEX_INPUT_DESC_SIZE] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};
static constexpr size_t SKINNED_VERTEX_INPUT_DESC_SIZE = 5;
const D3D12_INPUT_ELEMENT_DESC SKINNED_VERTEX_INPUT_LAYOUT[SKINNED_VERTEX_INPUT_DESC_SIZE] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"WEIGHTS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"BONEINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

enum class PSOType
{
	Normal,
	//WireFrame,
	BackSideNotCulling,
	//MirrorStencil,
	//ReflectedOpaque,
	//ReflectedTransparent,
	Transparent,
	Shadow,
	Count,
};

struct RenderItem
{
	RenderItem() noexcept;
	DirectX::XMFLOAT4X4 _worldMatrix;
	DirectX::XMFLOAT4X4 _textureTransform;

	MeshGeometry* _geometry;
	Material* _material;

	int _dirtyFrames;
	UINT _objConstantBufferIndex;

	// submesh pointer로 바꾸는것도 괜찮을지도? [1/17/2021 qwerw]
	UINT _indexCount;
	UINT _startIndexLocation;
	int _baseVertexLocation;

	D3D12_PRIMITIVE_TOPOLOGY _primitive;

	CommonIndex _skinnedConstantBufferIndex;
	SkinnedModelInstance* _skinnedModelInstance;
	HRESULT loadXML(const XMLReaderNode& rootNode) noexcept;
};

enum class RenderLayer : uint8_t
{
	Opaque,
	AlphaTested,
	Shadow,
	//MirrorStencil,
	//ReflectedOpaque,
	//ReflectedTransparent,
	Transparent,
	Count,
};
constexpr RenderLayer RenderLayers[] =
{
	RenderLayer::Opaque,
	RenderLayer::AlphaTested,
	RenderLayer::Shadow,
	//RenderLayer::MirrorStencil,
	//RenderLayer::ReflectedOpaque,
	//RenderLayer::ReflectedTransparent,
	RenderLayer::Transparent,
};
static_assert(sizeof(RenderLayers) == static_cast<int>(RenderLayer::Count), "RenderLayer 추가 시 수정해주세요.");

class D3DApp
{
public:
	typedef struct D3D12_FEATURE_DATA_FEATURE_LEVELS {
		UINT						NumFeatureLevels;
		const D3D_FEATURE_LEVEL* pFeatureLevelRequested;
		D3D_FEATURE_LEVEL			MaxSupportedLevel;
	} D3D12_FEATURE_DATA_FEATURE_LEVELS;

	bool Initialize(void);
	int Run(void);
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static D3DApp* getApp(void) noexcept;


	D3DApp(HINSTANCE hInstance);
	~D3DApp();

	template <typename VertexCont>
	void createAndAddMeshGeometry(const std::string& geoName,
		const std::vector<string>& subMeshNames,
		const VertexCont& vertices,
		const std::vector<vector<Index>>& indices)
	{
		assert(_geometries.find(geoName) == _geometries.end());
		assert(!subMeshNames.empty());
		assert(subMeshNames.size() == vertices.size());
		assert(subMeshNames.size() == indices.size());

		unique_ptr<MeshGeometry> mesh(new MeshGeometry(_device.Get(), _commandList.Get(), geoName, subMeshNames, vertices, indices));

		_geometries[geoName] = std::move(mesh);
	}
	HRESULT addMaterial(const string& materialName, const string& d);

	int loadTexture(const string& textureName, const wstring& fileName);

private:
	////////////////////////////////////////////////////////////////////////
	// 장비 정보
	void logAdapters(void) noexcept;
	void logAdapterOutput(IDXGIAdapter* adapter) noexcept;
	void logOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) noexcept;
	void checkFeatureSupport(void) noexcept;
	float aspectRatio(void) const;

	// 창 띄우기(Win32)
	bool initMainWindow(void);

	// Direct3D 초기화
	bool initDirect3D(void);
	void flushCommandQueue();
	void set4XMsaaState(bool value);
	void createCommandObjects(void);
	void createSwapChain(void);
	void createDescriptorHeaps(void);

	// 업데이트
	void OnResize(void);
	void Update(void);
	void Draw(void);

	bool isAppPaused(void) const noexcept;
	void calculateFrameStats(void);
	ID3D12Resource* getCurrentBackBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE getCurrentBackBufferView() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilView() const noexcept;

	// 마우스 입력
	void onMouseDown(WPARAM buttonState, int x, int y);
	void onMouseUp(WPARAM buttonState, int x, int y);
	void onMouseMove(WPARAM buttonState, int x, int y);

	// 키보드 입력
	void onKeyboardInput(void) noexcept;

	// 파이프라인 생성
	void buildFrameResources(void);
	void buildRootSignature(void);
	void buildShaders(void);
	void buildPipelineStateObject(void);

	// 정점 버퍼 초기화
	void buildConstantGeometry(void);
	void buildGameObjects(void);
	void buildGameObject(const std::string& meshName,
		const DirectX::XMFLOAT3& scale,
		const DirectX::XMFLOAT3& rotation, 
		const DirectX::XMFLOAT3& transition);

	void BuildObject(void);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> getStaticSampler(void) const;
	void updateCamera(void);

	void updateObjectConstantBuffer(void);
	void updateSkinnedConstantBuffer(void);
	void updatePassConstantBuffer(void);
	void updateMaterialConstantBuffer(void);

	void drawRenderItems(const RenderLayer renderLayer);
	void buildConstantBufferViews();
	
	UINT getTotalRenderItemCount(void) const noexcept;
	float getHillsHeight(const float x, const float z);
	XMFLOAT3 getHillsNormal(const float x, const float z);
	//void BuildMaterials();
	void buildShaderResourceViews();

	void animateMaterials(void);
	void initializeManagers();

	HRESULT loadXmlMaterial(const XMLReaderNode& rootNode) noexcept;
	HRESULT loadXmlGameObject(const XMLReaderNode& rootNode) noexcept;
private:
	HINSTANCE _hInstance;
	HWND _hMainWnd;
	
	int _clientWidth;
	int _clientHeight;

	bool _minimized;
	bool _maximized;
	bool _resizing;

	bool _4xMsaaState = false;
	UINT _4xMsaaQuality = 0;

	WComPtr<IDXGIFactory4> _factory;
	WComPtr<IDXGISwapChain> _swapChain;
	WComPtr<ID3D12Device> _device;

	WComPtr<ID3D12Fence> _fence;
	UINT64 _currentFence;
	WComPtr<ID3D12CommandQueue> _commandQueue;
	WComPtr<ID3D12CommandAllocator> _commandAlloc;
	WComPtr<ID3D12GraphicsCommandList> _commandList;

	UINT _rtvDescriptorSize;
	UINT _dsvDescriptorSize;
	UINT _cbvSrcUavDescriptorSize;

	WComPtr<ID3D12Resource> _swapChainBuffer[SWAP_CHAIN_BUFFER_COUNT];
	WComPtr<ID3D12Resource> _depthStencilBuffer;
	
	std::vector<std::unique_ptr<FrameResource>> _frameResources;
	int _frameIndex;
	PassConstants _passConstants;

	WComPtr<ID3D12DescriptorHeap> _rtvHeap;
	WComPtr<ID3D12DescriptorHeap> _dsvHeap;
	WComPtr<ID3D12DescriptorHeap> _srvHeap;

	int _currentBackBuffer;

	// 정점 버퍼
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> _geometries;

	std::unordered_map<std::string, std::unique_ptr<Material>> _materials;

	// 셰이더 입력 관련
	WComPtr<ID3D12RootSignature> _rootSignature;
	WComPtr<ID3DBlob> _vertexShader;
	WComPtr<ID3DBlob> _pixelShader;

	unordered_map<PSOType, WComPtr<ID3D12PipelineState>> _pipelineStateObjectMap;

	std::vector<RenderItem*> _renderItems[static_cast<int>(RenderLayer::Count)];
	std::vector<unique_ptr<RenderItem>> _renderItemsUniquePtrXXX;

	std::vector<std::unique_ptr<Texture>> _textures;

	// 마우스 입력
	POINT _mousePos;

	// 카메라 구면 좌표
	float _cameraTheta;
	float _cameraPhi;
	float _cameraRadius;
	XMFLOAT3 _cameraCenterPos;
	XMFLOAT3 _cameraPos;

	// 태양 구면좌표 (radius = 1)
	float _sunTheta;
	float _sunPhi;

	// 좌표계 변환
	XMFLOAT4X4 _viewMatrix;
	XMFLOAT4X4 _projectionMatrix;

	PSOType _psoType;
	D3D12_VIEWPORT _viewPort;
	D3D12_RECT _scissorRect;
	GameTimer _timer;

	RenderItem* _shadowItem;

	static D3DApp* _app;
	//_resourceManager : 스테이지매니저가 요청한 자료들을 로드/언로드한다. 멀티스레드 적용이 되었으면 좋겠음.
	//_stageManager : 스테이지를 불러오고, 오브젝트를 배치한다.
	//UI는 어떻게 돌아가야 하지?
	//_audioManager : 사운드쪽
	//input처리를 보통 어떻게 하지?
	//UpdateGameObject
	//UpdateUI
	//UpdateCamera

	// infoManager를 만들기전까지는 임시로 이렇게 쓴다. [1/26/2021 qwerw]
public:
	//unordered_map<string, unique_ptr<StageInfo>> _stageInfoMap;
	//unordered_map<CharacterKey, unique_ptr<CharacterInfo>> _characterInfoMap;
	unordered_map<string, unique_ptr<BoneInfo>> _boneInfoMap;
	unordered_map<string, unique_ptr<AnimationInfo>> _animationInfoMap;
	vector<unique_ptr<SkinnedModelInstance>> _skinnedInstance;
	

	HRESULT loadInfoMap(void) noexcept;
};

