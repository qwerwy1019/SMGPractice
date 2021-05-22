#pragma once
#include "stdafx.h"
#include <sstream>
#include "TypeD3d.h"
#include "TypeGeometry.h"
#include "Exception.h"

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
		if (value > std::numeric_limits<BoneIndex>::max())
		{
			ThrowErrCode(ErrCode::Overflow, str + "°¡ boneIndex ¹üÀ§¸¦ ¹þ¾î³³´Ï´Ù.");
		}
		return value;
	}
};

