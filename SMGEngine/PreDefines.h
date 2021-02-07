#pragma once
#include <atlconv.h>


enum class ErrCode : UINT32
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
};

#define ErrCodeSuccess(_val) (_val == ErrCode::Success)
#define ErrCodeFailed(_val) (_val != ErrCode::Success)
#define ErrCodeString(_val) std::to_string(static_cast<int>(_val))
#define ErrCodeWString(_val) std::to_wstring(static_cast<int>(_val))