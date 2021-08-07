#pragma once
#include "TypeGeometry.h"
#include "TypeD3d.h"
#include "D3DUtil.h"

class XMLReaderNode;

struct SubMeshGeometry
{
	bool operator==(const std::string rhs) const noexcept
	{
		return _name == rhs;
	}
	std::string _name;
	UINT _indexCount = 0;
	UINT _baseIndexLoacation = 0;
	UINT _baseVertexLoaction = 0;
};

class MeshGeometry
{
public:
	MeshGeometry(const MeshGeometry& mesh) = delete;
	MeshGeometry(const XMLReaderNode& rootElement, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
#if defined DEBUG | defined _DEBUG
	MeshGeometry(const GeneratedMeshData& meshData, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
#endif
	// vertex buffer�� �������� ���� ���ؼ� ��������� ���� or ��ü�� �� ������ ������ [1/18/2021 qwerw]
	void setVertexByteSizeOnlyXXXXX(UINT vertexBufferSize) noexcept;
	void setNameXXXXX(const std::string& name) noexcept;
	void setVertexBufferGPUXXXXX(ID3D12Resource* buffer) noexcept;
	void createIndexBufferXXX(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const std::vector<GeoIndex>& ib);

	D3D12_VERTEX_BUFFER_VIEW getVertexBufferView(void) const noexcept;

	D3D12_INDEX_BUFFER_VIEW getIndexBufferView(void) const noexcept;
	const Vertex* getVertexBufferXXX(size_t& bufferSize) const noexcept;
	const GeoIndex* getIndexBufferXXX(void) const noexcept;
	std::string getName(void) const noexcept { return _name; }
	std::vector<SubMeshGeometry> _subMeshList;// todo private���� [1/14/2021 qwerw]
	const DirectX::BoundingBox& getBoundingBox(void) const noexcept;
	
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

	DirectX::BoundingBox _boundingBox;
	void loadXmlSkinnedVertices(const  XMLReaderNode& node, std::vector<SkinnedVertex>& skinnedVertices) const;
	void loadXmlVertices(const  XMLReaderNode& node, std::vector<Vertex>& vertices) const;
	void loadXmlIndices(const  XMLReaderNode& node, std::vector<GeoIndex>& indices) const;
};

