#pragma once
#include "UploadBuffer.h"
#include "TypeGeometry.h"

class FrameResource
{
public:
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT skinnedCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;

	D3D12_GPU_VIRTUAL_ADDRESS getPassCBVirtualAddress() const noexcept;
	D3D12_GPU_VIRTUAL_ADDRESS getObjectCBVirtualAddress() const noexcept;
	D3D12_GPU_VIRTUAL_ADDRESS getMaterialCBVirtualAddress() const noexcept;
	D3D12_GPU_VIRTUAL_ADDRESS getSkinnedCBVirtualAddress() const noexcept;
	ID3D12CommandAllocator* getCommandListAlloc() const noexcept;

	void setPassCB(UINT index, const PassConstants& passContants);
	void setObjectCB(UINT index, const ObjectConstants& objectContants);
	void setMaterialCB(UINT index, const MaterialConstants& materialConstants);
	void setSkinnedCB(UINT index, const SkinnedConstants& skinnedConstants);

	void setFence(UINT fence) noexcept;
	UINT getFence(void) const noexcept;
private:
	std::unique_ptr<UploadBufferWrapper<PassConstants>> _passConstantBuffer;
	std::unique_ptr<UploadBufferWrapper<ObjectConstants>> _objectConstantBuffer;
	std::unique_ptr<UploadBufferWrapper<MaterialConstants>> _materialConstantBuffer;
	std::unique_ptr<UploadBufferWrapper<SkinnedConstants>> _skinnedConstantBuffer;
	WComPtr<ID3D12CommandAllocator> _commandListAlloc;

	UINT64 _fence;
};

