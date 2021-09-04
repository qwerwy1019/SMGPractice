#pragma once
#include "stdafx.h"
#include <sstream>
#include "TypeD3d.h"
#include "TypeGeometry.h"
#include "Exception.h"
#include <iomanip>
#include "TypeCommon.h"

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
			if (newCursor != cursor)
			{
				rv.emplace_back(input.substr(cursor, newCursor - cursor));
			}
			cursor = newCursor + 1;
		}
		return rv;
	}
	static bool parseComparison(const std::string& input,
								std::string& name, 
								OperatorType& operatorType, 
								int& value)
	{
		int operatorPos = input.find_first_of("<=>");
		if (operatorPos == std::string::npos)
		{
			return false;
		}
		if (operatorPos == 0 || operatorPos == (input.length() - 1))
		{
			return false;
		}
		if (input.at(operatorPos) == '<')
		{
			operatorType = OperatorType::Less;
		}
		else if (input.at(operatorPos) == '=')
		{
			operatorType = OperatorType::Equal;
		}
		else if (input.at(operatorPos) == '>')
		{
			operatorType = OperatorType::Greater;
		}
		else
		{
			check(false, "타입 추가시 확인" + input);
			static_assert(static_cast<int>(OperatorType::Count) == 3, "타입 추가시 확인.");
		}
		name = input.substr(0, operatorPos);
		value = std::stoi(input.substr(operatorPos + 1));
		return true;
	}
	static bool compare(OperatorType operatorType, int lhs, int rhs)
	{
		switch (operatorType)
		{
			case OperatorType::Less:
			{
				return lhs < rhs;
			}
			break;
			case OperatorType::Equal:
			{
				return lhs == rhs;
			}
			break;
			case OperatorType::Greater:
			{
				return lhs > rhs;
			}
			break;
			case OperatorType::Count:
			default:
			{
				check(false, "타입 추가시 확인");
				static_assert(static_cast<int>(OperatorType::Count) == 3, "타입 추가시 확인.");
				return false;
			}
			break;
		}
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
			ThrowErrCode(ErrCode::Overflow, str + "가 boneIndex 범위를 벗어납니다.");
		}
		return value;
	}

	static std::string toString(float value, int n)
	{
		std::stringstream stream;
		stream << std::fixed << std::setprecision(n) << value;
		std::string s = stream.str();

		return s;
	}

	static std::string toString(const DirectX::XMFLOAT3& value, int n)
	{
		return toString(value.x, n) + " " + toString(value.y, n) + " " + toString(value.z, n);
	}
};

