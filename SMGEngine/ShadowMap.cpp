#include "stdafx.h"
#include "ShadowMap.h"
#include "Exception.h"

ShadowMap::ShadowMap(ID3D12Device* device, uint32_t width, uint32_t height)
{
	_d3dDevice = device;
	_width = width;
	_height = height;

	_viewPort = { 0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f };
	_scissorRect = { 0, 0, static_cast<int>(width), static_cast<int>(height) };

	buildResource();
}

void ShadowMap::buildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
	_hCpuSrv = hCpuSrv;
	_hGpuSrv = hGpuSrv;
	_hCpuDsv = hCpuDsv;

	buildDescriptors();
}

void ShadowMap::buildDescriptors()
{
	check(_d3dDevice != nullptr);
	check(_shadowMap.Get() != nullptr);
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Texture2D.PlaneSlice = 0;
	_d3dDevice->CreateShaderResourceView(_shadowMap.Get(), &srvDesc, _hCpuSrv);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DEPTH_STENCIL_FORMAT;
	dsvDesc.Texture2D.MipSlice = 0;
	_d3dDevice->CreateDepthStencilView(_shadowMap.Get(), &dsvDesc, _hCpuDsv);
}

void ShadowMap::onResize(uint32_t width, uint32_t height)
{
	if ((_width == width) && (_height == height))
	{
		return;
	}
	_width = width;
	_height = height;

	buildResource();
	buildDescriptors();
}

const D3D12_VIEWPORT& ShadowMap::getViewPort() const noexcept
{
	return _viewPort;
}

const D3D12_RECT& ShadowMap::getScissorRect() const noexcept
{
	return _scissorRect;
}

ID3D12Resource* ShadowMap::getResource() const noexcept
{
	return _shadowMap.Get();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::getDsv() const noexcept
{
	return _hCpuDsv;
}

uint32_t ShadowMap::getWidth() const noexcept
{
	return _width;
}

uint32_t ShadowMap::getHeight() const noexcept
{
	return _height;
}

void ShadowMap::buildResource()
{
	check(_d3dDevice != nullptr);

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = _width;
	texDesc.Height = _height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = SHADOW_MAP_FORMAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	
	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DEPTH_STENCIL_FORMAT;
	optClear.DepthStencil.Depth = 1.f;
	optClear.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES heapProperty(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(_d3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&_shadowMap)));
	
}
