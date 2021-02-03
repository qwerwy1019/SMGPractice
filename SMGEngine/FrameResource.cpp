#include "stdafx.h"
#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT skinnedCount)
	: _fence(0)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(_commandListAlloc.GetAddressOf())));

	_passConstantBuffer = std::make_unique<UploadBufferWrapper<PassConstants>>(device, passCount, true);
	_objectConstantBuffer = std::make_unique<UploadBufferWrapper<ObjectConstants>>(device, objectCount, true);
	_materialConstantBuffer = std::make_unique<UploadBufferWrapper<MaterialConstants>>(device, materialCount, true);
	_skinnedConstantBuffer = std::make_unique<UploadBufferWrapper<SkinnedConstants>>(device, skinnedCount, true);
}

FrameResource::~FrameResource()
{


}