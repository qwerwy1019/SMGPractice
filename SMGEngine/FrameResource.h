#pragma once
#include "UploadBuffer.h"
#include "TypeGeometry.h"

struct FrameResource
{
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT skinnedCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();
	ID3D12CommandAllocator* getCommandListAlloc() const noexcept { return _commandListAlloc.Get(); }

	std::unique_ptr<UploadBufferWrapper<PassConstants>> _passConstantBuffer;
	std::unique_ptr<UploadBufferWrapper<ObjectConstants>> _objectConstantBuffer;
	std::unique_ptr<UploadBufferWrapper<MaterialConstants>> _materialConstantBuffer;
	std::unique_ptr<UploadBufferWrapper<SkinnedConstants>> _skinnedConstantBuffer;
	WComPtr<ID3D12CommandAllocator> _commandListAlloc;

	UINT64 _fence;
};

