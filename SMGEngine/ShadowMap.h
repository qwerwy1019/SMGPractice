#pragma once
#include "TypeD3d.h"

using namespace DirectX;

class ShadowMap
{
public:
	ShadowMap(ID3D12Device* device, uint32_t width, uint32_t height);
	ShadowMap(const ShadowMap& rhs) = delete;
	ShadowMap& operator=(const ShadowMap& rhs) = delete;
	~ShadowMap() = default;

	void buildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
						CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
						CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);
	void onResize(uint32_t width, uint32_t height);
	const D3D12_VIEWPORT& getViewPort() const noexcept;
	const D3D12_RECT& getScissorRect() const noexcept;
	ID3D12Resource* getResource() const noexcept;
	CD3DX12_CPU_DESCRIPTOR_HANDLE getDsv() const noexcept;
	uint32_t getWidth() const noexcept;
	uint32_t getHeight() const noexcept;
private:
	void buildDescriptors();
	void buildResource();
private:
	ID3D12Device* _d3dDevice;
	D3D12_VIEWPORT _viewPort;
	D3D12_RECT _scissorRect;

	uint32_t _width;
	uint32_t _height;
	static constexpr DXGI_FORMAT SHADOW_MAP_FORMAT = DXGI_FORMAT_R24G8_TYPELESS;

	CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuDsv;

	WComPtr<ID3D12Resource> _shadowMap;
};