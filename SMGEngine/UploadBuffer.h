#pragma once

#include "D3DUtil.h"
template<typename T>
class UploadBufferWrapper
{
public:
	UploadBufferWrapper(ID3D12Device* device, UINT elementCount, bool isConstantBuffer)
		: _elementByteSize(sizeof(T))
	{
		if (isConstantBuffer)
		{
			_elementByteSize = D3DUtil::CalcConstantBufferByteSize(_elementByteSize);
		}
		CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(_elementByteSize * elementCount);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperty,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&_uploadBuffer)));

		ThrowIfFailed(_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&_mappedData)));
	}
	UploadBufferWrapper(const UploadBufferWrapper& rhs) = delete;
	UploadBufferWrapper& operator=(const UploadBufferWrapper& rhs) = delete;
	~UploadBufferWrapper(void)
	{
		if (_uploadBuffer != nullptr)
		{
			_uploadBuffer->Unmap(0, nullptr);
		}
		_mappedData = nullptr;
	}

	ID3D12Resource* getResource(void) const noexcept
	{
		return _uploadBuffer.Get();
	}

	void copyData(int elementIndex, const T& data)
	{
		memcpy(&_mappedData[elementIndex * _elementByteSize], &data, sizeof(T));
	}

private:
	UINT _elementByteSize;

	WComPtr<ID3D12Resource> _uploadBuffer;
	BYTE* _mappedData;

};
