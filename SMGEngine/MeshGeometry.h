#pragma once
#include "TypeGeometry.h"
#include "TypeD3d.h"
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
	MeshGeometry() noexcept;
	void loadXml(const XMLReaderNode& rootElement, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

	// vertex buffer만 동적으로 쓰기 위해서 만든거지만 삭제 or 대체할 수 있으면 좋겠음 [1/18/2021 qwerw]
	void setVertexByteSizeOnlyXXXXX(UINT vertexBufferSize) noexcept;
	void setNameXXXXX(const std::string& name) noexcept;
	void setVertexBufferGPUXXXXX(ID3D12Resource* buffer) noexcept;
	void createIndexBufferXXX(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const std::vector<GeoIndex>& ib);

	D3D12_VERTEX_BUFFER_VIEW getVertexBufferView(void) const noexcept;

	D3D12_INDEX_BUFFER_VIEW getIndexBufferView(void) const noexcept;

	std::string getName(void) const noexcept { return _name; }
	std::unordered_map<std::string, SubMeshGeometry> _subMeshMap;// todo private으로 [1/14/2021 qwerw]
private:
	void createMeshGeometryXXX(ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const void* vb,
		const void* ib);

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
	void loadXmlSkinnedVertices(const  XMLReaderNode& node, std::vector<SkinnedVertex>& skinnedVertices) const;
	void loadXmlVertices(const  XMLReaderNode& node, std::vector<Vertex>& vertices) const;
	void loadXmlIndices(const  XMLReaderNode& node, std::vector<GeoIndex>& indices) const;
};

