#include "stdafx.h"
#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT skinnedCount, UINT effectCount)
	: _fence(0)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(_commandListAlloc.GetAddressOf())), "commandAllocator 할당 실패");

	_passConstantBuffer = std::make_unique<UploadBufferWrapper<PassConstants>>(device, passCount, true);
	_objectConstantBuffer = std::make_unique<UploadBufferWrapper<ObjectConstants>>(device, objectCount, true);
	_materialConstantBuffer = std::make_unique<UploadBufferWrapper<MaterialConstants>>(device, materialCount, true);
	_skinnedConstantBuffer = std::make_unique<UploadBufferWrapper<SkinnedConstants>>(device, skinnedCount, true);
	_effectInstanceBuffer = std::make_unique<UploadBufferWrapper<EffectInstanceData>>(device, effectCount, false);
}

D3D12_GPU_VIRTUAL_ADDRESS FrameResource::getPassCBVirtualAddress() const noexcept
{
	return _passConstantBuffer->getResource()->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS FrameResource::getObjectCBVirtualAddress() const noexcept
{
	return _objectConstantBuffer->getResource()->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS FrameResource::getMaterialCBVirtualAddress() const noexcept
{

	return _materialConstantBuffer->getResource()->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS FrameResource::getSkinnedCBVirtualAddress() const noexcept
{

	return _skinnedConstantBuffer->getResource()->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS FrameResource::getEffectBufferVirtualAddress() const noexcept
{
	return _effectInstanceBuffer->getResource()->GetGPUVirtualAddress();
}

ID3D12CommandAllocator* FrameResource::getCommandListAlloc() const noexcept
{

	return _commandListAlloc.Get();
}

void FrameResource::setPassCB(UINT index, const PassConstants& passContants)
{
	_passConstantBuffer->copyData(index, passContants);
}

void FrameResource::setObjectCB(UINT index, const ObjectConstants& objectContants)
{
	_objectConstantBuffer->copyData(index, objectContants);
}

void FrameResource::setMaterialCB(UINT index, const MaterialConstants& materialConstants)
{
	_materialConstantBuffer->copyData(index, materialConstants);
}

void FrameResource::setSkinnedCB(UINT index, const SkinnedConstants& skinnedConstants)
{
	_skinnedConstantBuffer->copyData(index, skinnedConstants);
}

void FrameResource::setEffectBuffer(UINT index, const EffectInstanceData& effectInstance)
{
	_effectInstanceBuffer->copyData(index, effectInstance);
}

void FrameResource::setFence(UINT fence) noexcept
{
	_fence = fence;
}

UINT FrameResource::getFence(void) const noexcept
{
	return _fence;
}
