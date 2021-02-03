#pragma once
#include "TypeGeometry.h"
#include "TypeData.h"
#include "D3DUtil.h"

class XMLReaderNode;

struct SubMeshGeometry
{
	UINT _indexCount = 0;
	UINT _baseIndexLoacation = 0;
	UINT _baseVertexLoaction = 0;
};

class MeshGeometry
{
public:
	MeshGeometry(const MeshGeometry& mesh) = delete;
	MeshGeometry() = default;
	HRESULT loadXml(const XMLReaderNode& rootElement, ID3D12Device* device, ID3D12GraphicsCommandList* commandList) noexcept;
	// 템플릿을 쓰는게 최선일까? [1/19/2021 qwerw]
	template <typename VertexCont>
	MeshGeometry(ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const std::string& name,
		const std::vector<std::string>& subMeshNames,
		const VertexCont& vb,
		const std::vector<std::vector<Index>>& ib) 
		: _vertexByteStride(sizeof(VertexCont::value_type::value_type))
		, _name(name)
	{
		assert(device != nullptr);
		assert(commandList != nullptr);
		assert(!subMeshNames.empty());
		assert(!vb.empty());
		assert(!ib.empty());
		assert(subMeshNames.size() == vb.size());
		assert(subMeshNames.size() == ib.size());

		std::vector<Vertex> totalVertices = D3DUtil::mergeContainer(vb);
		std::vector<Index> totalIndices = D3DUtil::mergeContainer(ib);

		_vertexBufferByteSize = totalVertices.size() * sizeof(VertexCont::value_type::value_type);
		_indexBufferByteSize = totalIndices.size() * sizeof(Index);

		makeSubMeshMap(subMeshNames, vb, ib);

		createMeshGeometryXXX(device, commandList, totalVertices.data(), totalIndices.data());
	}

	// vertex buffer만 동적으로 쓰기 위해서 만든거지만 삭제 or 대체할 수 있으면 좋겠음 [1/18/2021 qwerw]
	void setVertexByteSizeOnlyXXXXX(UINT vertexBufferSize) noexcept;
	void setNameXXXXX(const std::string& name) noexcept;
	void setVertexBufferGPUXXXXX(ID3D12Resource* buffer) noexcept;
	void createIndexBufferXXX(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const std::vector<Index>& ib);

	D3D12_VERTEX_BUFFER_VIEW getVertexBufferView(void) const noexcept;

	D3D12_INDEX_BUFFER_VIEW getIndexBufferView(void) const noexcept;

	std::string getName(void) const noexcept { return _name; }
	std::unordered_map<std::string, SubMeshGeometry> _subMeshMap;// todo private으로 [1/14/2021 qwerw]
private:
	void createMeshGeometryXXX(ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const void* vb,
		const void* ib);

	template<typename VertexCont>
	void makeSubMeshMap(const std::vector<std::string>& subMeshNames,
		const VertexCont& vb,
		const std::vector<std::vector<Index>>& ib)
	{
		CommonIndex startIndexLocation = 0;
		CommonIndex baseVertexLocation = 0;
		for (int i = 0; i < subMeshNames.size(); ++i)
		{
			if (ib[i].empty())
			{
				continue;
			}

			const SubMeshGeometry subMesh = { ib[i].size(), startIndexLocation, baseVertexLocation };
			auto it = _subMeshMap.emplace(std::make_pair(subMeshNames[i], subMesh));
			if (it.second == false)
			{
				assert(false && L"submesh name이 중복되었음!! mesh가 생성되지 않습니다.");
				return;
			}

			startIndexLocation += ib[i].size();
			baseVertexLocation += vb[i].size();
		}
	}

	static constexpr DXGI_FORMAT _indexFormat = DXGI_FORMAT_R16_UINT;
	UINT _vertexByteStride;
	std::string _name;
	UINT _vertexBufferByteSize;
	UINT _indexBufferByteSize;

	WComPtr<ID3DBlob> _vertexBufferCPU;
	WComPtr<ID3DBlob> _indexBufferCPU;

	WComPtr<ID3D12Resource> _vertexBufferGPU;
	WComPtr<ID3D12Resource> _indexBufferGPU;

	WComPtr<ID3D12Resource> _vertexBufferUploader;
	WComPtr<ID3D12Resource> _indexBufferUploader;
	HRESULT loadXmlSkinnedVertices(const  XMLReaderNode& node, std::vector<SkinnedVertex>& skinnedVertices) const noexcept;
	HRESULT loadXmlVertices(const  XMLReaderNode& node, std::vector<Vertex>& vertices) const noexcept;
	HRESULT loadXmlIndices(const  XMLReaderNode& node, std::vector<Index>& indices) const noexcept;
};

