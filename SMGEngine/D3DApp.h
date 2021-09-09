#pragma once

#include "TypeGeometry.h"
#include "TypeUI.h"
#include "TypeD3d.h"

#include "GameTimer.h"
#include "UploadBuffer.h"
#include "MeshGeometry.h"
#include "FrameResource.h"
#include "SkinnedData.h"
#include <queue>

using namespace std;
using namespace DirectX;

// 전방선언
struct IDXGIAdapter;
struct IDXGIOutput;
class Material;
class UIManager;
class Actor;
class GameObject;
class ShadowMap;

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

enum class PSOType : uint8_t
{
	Normal,
	Skinned,
	BackSideNotCulling,
	Transparent,
	Shadow,
	ShadowSkinned,
	Effect,
	Background,
	UI,
	GameObjectDev,
	Count,
};

struct RenderItem
{
	RenderItem(const GameObject* parentObject,
				const Material* material,
				D3D12_PRIMITIVE_TOPOLOGY primitive,
				uint8_t subMeshIndex,
				RenderLayer renderLayer) noexcept;

	const GameObject* _parentObject;
	const Material* _material;

	D3D12_PRIMITIVE_TOPOLOGY _primitive;

	RenderLayer _renderLayer;
	uint8_t _subMeshIndex;
	
	const SubMeshGeometry& getSubMesh() const noexcept;
	void changeMaterial(const Material* material) noexcept;
};

class D3DApp
{
public:
	typedef struct D3D12_FEATURE_DATA_FEATURE_LEVELS {
		UINT						NumFeatureLevels;
		const D3D_FEATURE_LEVEL* pFeatureLevelRequested;
		D3D_FEATURE_LEVEL			MaxSupportedLevel;
	} D3D12_FEATURE_DATA_FEATURE_LEVELS;

	bool Initialize(void);

	D3DApp();
	~D3DApp();

	GameObject* createObjectFromXML(const std::string& fileName);

	uint16_t loadTexture(const string& textureName);

	// 업데이트
	void OnResize(void);
	void Update(void);
	void Draw(void);

	// ui 관련
	IDWriteFactory3* getWriteFactory(void) const noexcept { return _writeFactory.Get(); }
	ID2D1DeviceContext2* getD2dContext(void) const noexcept { return _d2dContext.Get(); }
	ID2D1Factory3* getD2dFactory(void) const noexcept { return _d2dFactory.Get(); }
	IDWriteTextFormat* getTextFormat(TextFormatType type) const noexcept { return _textFormats[static_cast<int>(type)].Get(); }
	ID2D1Brush* getTextBrush(TextBrushType type)const noexcept { return _textBrushes[static_cast<int>(type)].Get(); }
	ID2D1Bitmap* loadBitmapImage(const std::string& resourceName);

	void prepareCommandQueue(void);
	void executeCommandQueue(void);

	void removeRenderItem(const RenderLayer renderLayer, const RenderItem* renderItem) noexcept;
	void removeSkinnedInstance(const SkinnedModelInstance* skinnedInstance) noexcept;
	void removeGameObject(const GameObject* gameObject) noexcept;
	//const DirectX::XMFLOAT4X4& getInverseViewMatrix(void) const noexcept;
	bool XM_CALLCONV checkCulled(const DirectX::BoundingBox& box, FXMMATRIX world) const noexcept;
	void setLight(const std::vector<Light>& lights, const DirectX::XMFLOAT4& ambientLight) noexcept;
	void setBackgroundColor(const DirectX::XMFLOAT3& color) noexcept;
	void createRenderItems(GameObject* gameObject, const XMLReaderNode& node);

	void releaseItemsForStageLoad(void) noexcept;
private:
	////////////////////////////////////////////////////////////////////////
	// 장비 정보
	void logAdapters(void) noexcept;
	void logAdapterOutput(IDXGIAdapter* adapter) noexcept;
	void logOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) noexcept;
	void checkFeatureSupport(void) noexcept;

	// Direct3D 초기화
	void initDirect3D();
	void initDirect2D(void);
	void flushCommandQueue();
	void set4XMsaaState(bool value);
	void createCommandObjects(void);
	void createSwapChain();
	void createDescriptorHeaps(void);

	ID3D12Resource* getCurrentBackBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE getCurrentBackBufferView() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilView() const noexcept;


	// 파이프라인 생성
	void buildFrameResources(void);
	void buildRootSignature(void);
	void buildShaders(void);
	void buildPipelineStateObject(void);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> getStaticSampler(void) const;

	void initShadowMap();
	void updateShadowTransform(void) noexcept;
	void updateShadowPassConstantBuffer(void) noexcept;
private:
	void updateObjectConstantBuffer(void);
	void updateSkinnedConstantBuffer(void);
	void updatePassConstantBuffer(void);
	void updateMaterialConstantBuffer(void);
private:
	UINT popObjectContantBufferIndex(void);
	uint16_t popSkinnedContantBufferIndex(void);
public:
	void pushObjectContantBufferIndex(UINT index) noexcept;
	void pushSkinnedContantBufferIndex(uint16_t index) noexcept;
private:
	UINT _objectCBIndexCount;
	std::queue<UINT> _objectCBReturned;
	uint16_t _skinnedCBIndexCount;
	std::queue<uint16_t> _skinnedCBReturned;

private:
	GameObject* createGameObject(const MeshGeometry* meshGeometry, SkinnedModelInstance* skinnedInstance, uint16_t skinnedBufferIndex) noexcept;
	SkinnedModelInstance* createSkinnedInstance(uint16_t& skinnedBufferIndex, const BoneInfo* boneInfo, const AnimationInfo* animationInfo) noexcept;
public:
	const MeshGeometry* createMeshGeometry(const std::string& meshName, const GeneratedMeshData& meshData);
	const Material* getMaterial(const std::string& fileName, const std::string& materialName) const noexcept;
private:
	void drawRenderItems(const RenderLayer renderLayer, bool checkCulled);
	void drawUI(void);
	void drawSceneToShadowMap(void);
	void drawEffects(void);
	std::string getMaterialKey(const std::string& fileName, const std::string& materialName) const noexcept;

	void buildShaderResourceViews();

	Material* loadXmlMaterial(const std::string& fileName, const std::string& materialName);
	BoneInfo* loadXMLBoneInfo(const std::string& fileName);
	MeshGeometry* loadXMLMeshGeometry(const std::string& fileName);
	AnimationInfo* loadXMLAnimationInfo(const std::string& fileName);
	
private:
	
	bool _4xMsaaState = false;
	UINT _4xMsaaQuality = 0;

	WComPtr<IDXGIFactory4> _factory;
	WComPtr<IDXGISwapChain> _swapChain;
	WComPtr<ID3D12Device> _deviceD3d12;

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
	PassConstants _shadowPassConstants;

	WComPtr<ID3D12DescriptorHeap> _rtvHeap;
	WComPtr<ID3D12DescriptorHeap> _dsvHeap;
	WComPtr<ID3D12DescriptorHeap> _srvHeap;

	int _currentBackBuffer;

	// 정점 버퍼
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> _geometries;

	std::unordered_map<std::string, std::unique_ptr<Material>> _materials;

	// 셰이더 입력 관련
	WComPtr<ID3D12RootSignature> _rootSignature;
	std::unordered_map<std::string, WComPtr<ID3DBlob>> _shaders;

	unordered_map<PSOType, WComPtr<ID3D12PipelineState>> _pipelineStateObjectMap;

	std::vector<std::unique_ptr<RenderItem>> _renderItems[static_cast<int>(RenderLayer::Count)];
	std::vector<std::unique_ptr<GameObject>> _gameObjects;

	std::vector<std::unique_ptr<Texture>> _textures;
	std::unordered_map<std::string, uint16_t> _textureIndexMap;
	int _textureLoadedCount;
	static constexpr int TEXTURE_SRV_INDEX = 1;

	DirectX::BoundingFrustum _viewFrustumLocal;

	// 좌표계 변환
	XMFLOAT4X4 _projectionMatrix;

	D3D12_VIEWPORT _viewPort;
	D3D12_RECT _scissorRect;

	std::unique_ptr<ShadowMap> _shadowMap;
	BoundingSphere _sceneBounds;
	DirectX::XMFLOAT4X4 _mainLightViewMatrix;
	DirectX::XMFLOAT4X4 _mainLightProjectionMatrix;
	DirectX::XMFLOAT4X4 _shadowTransform;

	DirectX::XMFLOAT3 _backgroundColor;

	// UI & D2D
	WComPtr<ID3D11Resource> _backBufferWrapped[SWAP_CHAIN_BUFFER_COUNT];
	WComPtr<ID2D1Bitmap1> _backBufferBitmap[SWAP_CHAIN_BUFFER_COUNT];
	WComPtr<ID2D1Factory3> _d2dFactory;
	WComPtr<ID2D1Device2> _deviceD2d;
	WComPtr<ID2D1DeviceContext2> _d2dContext;

	WComPtr<ID3D11On12Device> _deviceD3d11On12;
	WComPtr<IDWriteFactory3> _writeFactory;
	std::array<WComPtr<IDWriteTextFormat>, static_cast<int>(TextFormatType::Count)> _textFormats;
	std::array<WComPtr<ID2D1Brush>, static_cast<int>(TextBrushType::Count)> _textBrushes;

	WComPtr<IWICImagingFactory> _wicImageFactory;
	std::unordered_map<std::string, WComPtr<ID2D1Bitmap>> _bitmapImages;

	WComPtr<ID3D11DeviceContext> _d3d11Context;

	unordered_map<string, unique_ptr<BoneInfo>> _boneInfoMap;
	unordered_map<string, unique_ptr<AnimationInfo>> _animationInfoMap;
	vector<unique_ptr<SkinnedModelInstance>> _skinnedInstance;

#if defined DEBUG | defined _DEBUG
public:
	void createGameObjectDev(Actor* actor);
	void createGameObjectDev(GameObject* gameObject);
private:
#endif
public:
};

