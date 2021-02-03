#include "stdafx.h"
#include "D3DUtil.h"

DxException::DxException()
	: _errorCode(S_OK)
	, _functionName()
	, _fileName()
	, _lineNumber(-1)
{
}

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, const int lineNumber)
	: _errorCode(hr)
	, _functionName(functionName)
	, _fileName(fileName)
	, _lineNumber(lineNumber)
{
}

std::wstring DxException::to_wstring() const noexcept
{
	_com_error err(_errorCode);
	return _functionName + L" failed in " + _fileName + L": line " + std::to_wstring(_lineNumber) + L"\n" + err.ErrorMessage();
}

WComPtr<ID3D12Resource> D3DUtil::CreateDefaultBuffer(ID3D12Device* device,
													 ID3D12GraphicsCommandList* cmdList,
													 const void* initData,
													 UINT64 byteSize,
													 WComPtr<ID3D12Resource>& uploadBuffer)
{
	WComPtr<ID3D12Resource> defaultBuffer;

	const CD3DX12_RESOURCE_DESC& bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	const CD3DX12_HEAP_PROPERTIES& heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc ,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	const CD3DX12_HEAP_PROPERTIES& heapPropertyUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapPropertyUpload,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

	D3D12_SUBRESOURCE_DATA subResourceData = {initData, byteSize, byteSize};

	const CD3DX12_RESOURCE_BARRIER& transitionToCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->ResourceBarrier(1, &transitionToCopyDest);

	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	
	const CD3DX12_RESOURCE_BARRIER& transitionToReadOnly = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList->ResourceBarrier(1, &transitionToReadOnly);

	return defaultBuffer;
}

WComPtr<ID3DBlob> D3DUtil::CompileShader(const std::wstring& fileName,
										 const D3D_SHADER_MACRO* defines,
										 const std::string& entryPoint,
										 const std::string& target)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	WComPtr<ID3DBlob> byteCode = nullptr;
	WComPtr<ID3DBlob> errors = nullptr;
	HRESULT hr = D3DCompileFromFile(
		fileName.c_str(),
		defines,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint.c_str(),
		target.c_str(),
		compileFlags, 0, &byteCode, &errors);
	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	return byteCode;
}

WComPtr<ID3DBlob> D3DUtil::LoadBinaryShaer(const std::wstring& fileName)
{
	std::ifstream fin(fileName, std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	std::ifstream::pos_type size = static_cast<int>(fin.tellg());
	fin.seekg(0, std::ios_base::beg);

	WComPtr<ID3DBlob> blob;
	ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

	fin.read(static_cast<char*>(blob->GetBufferPointer()), size);
	fin.close();

	return blob;
}
