#pragma once
#include "stdafx.h"
#include "TypeData.h"
#include <sstream>
#include "TypeGeometry.h"

class DxException
{
public:
	DxException();
	DxException(HRESULT hr, ErrCode errorCode, const std::wstring& functionName, const std::wstring& fileName, const int lineNumber);

	std::wstring to_wstring() const noexcept;

	HRESULT			_hr;
	ErrCode			_errorCode;
	std::wstring	_functionName;
	std::wstring	_fileName;
	int				_lineNumber;
};

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

class D3DUtil
{
public:
	static WComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		WComPtr<ID3D12Resource>& uploadBuffer);
	static inline UINT CalcConstantBufferByteSize(UINT byteSize)
	{
		return (byteSize + 255) & ~255;
	}
	static WComPtr<ID3DBlob> CompileShader(
		const std::wstring& fileName,
		const D3D_SHADER_MACRO* defines,
		const std::string& entryPoint,
		const std::string& target
	);
	static WComPtr<ID3DBlob> LoadBinaryShaer(const std::wstring& fileName);

	template<typename Container>
	static auto mergeContainer(const Container& container)
	{
		typename Container::value_type rv;
		size_t containerTotalCount = 0;
		for (const auto& c : container)
		{
			containerTotalCount += c.size();
		}
		rv.reserve(containerTotalCount);

		for (const auto& c : container)
		{
			rv.insert(rv.end(), c.begin(), c.end());
		}
		return rv;
	}

	static std::vector<std::string> tokenizeString(const std::string& input, char token)
	{
		std::vector<std::string> rv;
		rv.reserve(input.length());

		size_t cursor = 0;
		while (cursor < input.length())
		{
			size_t newCursor = std::min(input.find(token, cursor), input.length());
			rv.emplace_back(input.substr(cursor, newCursor - cursor));
			cursor = newCursor + 1;
		}
		return rv;
	}

	template <typename T> 
	static T convertTo(const std::string& str)
	{
		std::istringstream ss(str);
		T value;
		ss >> value;
		return value;
	}
	template <>
	static BoneIndex convertTo(const std::string& str)
	{
		std::istringstream ss(str);
		unsigned int value;
		ss >> value;
		assert(value <= std::numeric_limits<BoneIndex>::max());
		return value;
	}
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(_expr, ...)											\
{																			\
    HRESULT hr__ = (_expr);													\
	if(hr__ != S_OK)														\
	{																		\
		std::wstring wfn = AnsiToWString(__FILE__);							\
		DxException e(hr__, ErrCode::HRESULTFail, L#_expr, wfn, __LINE__);	\
		USES_CONVERSION;													\
		std::string errorString(W2A(e.to_wstring().c_str()));				\
		errorString.append({__VA_ARGS__});									\
		OutputDebugStringA(("Error! " + errorString).c_str());				\
		MessageBoxA(NULL, errorString.c_str(), "Assert Check", MB_OK);		\
		DebugBreak();														\
		throw e;															\
	}																		\
}
#endif

#ifndef ThrowErrCode
#define ThrowErrCode(_err, ...)												\
{																			\
	{																		\
		std::wstring wfn = AnsiToWString(__FILE__);							\
		DxException e(E_FAIL, _err, L#_err, wfn, __LINE__);					\
		USES_CONVERSION;													\
		std::string errorString(W2A(e.to_wstring().c_str()));				\
		errorString.append({__VA_ARGS__});									\
		OutputDebugStringA(("Error! " + errorString).c_str());				\
		MessageBoxA(NULL, errorString.c_str(), "Assert Check", MB_OK);		\
		DebugBreak();														\
		throw e;															\
	}																		\
}
#endif

#ifndef check
#define check(_val, _msg)																										\
{																																\
	if((_val) == false)																											\
	{																															\
		static bool ignore = false;																								\
		if(!ignore)																												\
		{																														\
			std::string fileName = __FILE__;																					\
			size_t fileNameOffset = fileName.find("Codes");																		\
			fileName = fileName.substr(fileNameOffset);																			\
			std::string text = "In " + fileName + ": line " + std::to_string(__LINE__) + "\n" + #_val + "\n" + _msg + "\n";		\
			OutputDebugStringA(("Assert! " + text).c_str());																	\
			switch(MessageBoxA(NULL, text.c_str(), "Assert Check", MB_ICONERROR | MB_ABORTRETRYIGNORE))							\
			{																													\
				case IDABORT:																									\
				case IDRETRY:																									\
					DebugBreak();																								\
					break;																										\
				case IDIGNORE:																									\
					ignore = true;																								\
					break;																										\
				default:																										\
					__noop;																										\
																																\
			}																													\
		}																														\
	}																															\
}
#endif