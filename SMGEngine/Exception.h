#pragma once
#include <atlconv.h>

enum class ErrCode : uint32_t
{
	Success,

	HRESULTFail,
	InitFail,
	FileNotFound,
	NodeNotFound,
	NodeNameNotFound,
	MeshNotFound,
	SubMeshNotFound,
	MaterialNotFound,
	MsXmlError,
	CreateD3dBufferFail,
	CreateD3dUploadBufferFail,
	InvalidTextureInfo,
	RootSignatureBuildFail,
	CommandListResetFail,
	CommandAllocResetFail,
	CommandListCloseFail,
	CreatePSOFail,
	SetFenceFail,
	SwapChainFail,
	InvalidVertexInfo,
	InvalidIndexInfo,
	InvalidFbxData,
	InvalidNormalData,
	UndefinedType,
	TypeIsDifferent,
	PathNotFound,
	TriangulateFail,
	Overflow,
	KeyDuplicated,
	InvalidTimeInfo,
	InvalidAnimationData,
	VertexShaderCompileError,
	PixelShaderCompileError,
	TokenizeError,
	InvalidXmlData,
	AnimationNotFound,
	ActionChartLoadFail,
};

#define ErrCodeSuccess(_val) (_val == ErrCode::Success)
#define ErrCodeFailed(_val) (_val != ErrCode::Success)
#define ErrCodeString(_val) std::to_string(static_cast<int>(_val))
#define ErrCodeWString(_val) std::to_wstring(static_cast<int>(_val))

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

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

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
