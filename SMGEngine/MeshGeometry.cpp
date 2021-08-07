#include "stdafx.h"
#include "MeshGeometry.h"
#include "D3DUtil.h"
#include "FileHelper.h"
#include "GeometryGenerator.h"

void MeshGeometry::createMeshGeometryXXX(ID3D12Device* device,
										ID3D12GraphicsCommandList* commandList,
										const void* vb,
										const void* ib)
{
	check(_subMeshList.empty() == false);
	// cpu buffer가 ID3DBlob이어야 하는 이유가 있는가? 알아보고 바꾸자. [6/9/2021 qwerw]
	ThrowIfFailed(D3DCreateBlob(_vertexBufferByteSize, &_vertexBufferCPU), "vertexBuffer 할당 실패" );
	CopyMemory(_vertexBufferCPU->GetBufferPointer(), vb, _vertexBufferByteSize);

	ThrowIfFailed(D3DCreateBlob(_indexBufferByteSize, &_indexBufferCPU), "indexBuffer 할당 실패" );
	CopyMemory(_indexBufferCPU->GetBufferPointer(), ib, _indexBufferByteSize);

	_vertexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList, vb, _vertexBufferByteSize, _vertexBufferUploader);
	_indexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList, ib, _indexBufferByteSize, _indexBufferUploader);
	check(_vertexBufferGPU != nullptr && _indexBufferGPU != nullptr, "vertex index buffer가 생성되지 않았습니다.");
}

void MeshGeometry::loadXmlSkinnedVertices(const XMLReaderNode& node, std::vector<SkinnedVertex>& skinnedVertices) const
{
	const auto& childNodes = node.getChildNodes();
	for (int i = 0; i < childNodes.size(); ++i)
	{
		SkinnedVertex vertex;

		childNodes[i].loadAttribute("Position", vertex._position);
		childNodes[i].loadAttribute("Normal", vertex._normal);
		childNodes[i].loadAttribute("TexCoord", vertex._textureCoord);
		childNodes[i].loadAttribute("Weight", vertex._boneWeights);
		childNodes[i].loadAttribute("BoneIndex", vertex._boneIndices);

		skinnedVertices.push_back(vertex);
	}
}

void MeshGeometry::loadXmlVertices(const XMLReaderNode& node, std::vector<Vertex>& vertices) const
{
	const auto& childNodes = node.getChildNodes();
	for (int i = 0; i < childNodes.size(); ++i)
	{
		Vertex vertex;

		childNodes[i].loadAttribute("Position", vertex._position);
		childNodes[i].loadAttribute("Normal", vertex._normal);
		childNodes[i].loadAttribute("TexCoord", vertex._textureCoord);

		vertices.push_back(vertex);
	}
}

void MeshGeometry::loadXmlIndices(const XMLReaderNode& node, std::vector<GeoIndex>& indices) const
{
	const auto& childNodes = node.getChildNodes();
	for (int i = 0; i < childNodes.size(); ++i)
	{
		GeoIndex index0, index1, index2;
		childNodes[i].loadAttribute("_0", index0);
		childNodes[i].loadAttribute("_1", index1);
		childNodes[i].loadAttribute("_2", index2);

		indices.push_back(index0);
		indices.push_back(index1);
		indices.push_back(index2);
	}
}

MeshGeometry::MeshGeometry(const XMLReaderNode& rootNode, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	rootNode.loadAttribute("Name", _name);
	bool isSkinned;
	rootNode.loadAttribute("IsSkinned", isSkinned);
	int totalVertexCount;
	rootNode.loadAttribute("TotalVertexCount", totalVertexCount);
	int totalIndexCount;
	rootNode.loadAttribute("TotalIndexCount", totalIndexCount);

	DirectX::XMFLOAT3 min, max;
	rootNode.loadAttribute("Min", min);
	rootNode.loadAttribute("Max", max);

	DirectX::BoundingBox::CreateFromPoints(_boundingBox, XMLoadFloat3(&min), XMLoadFloat3(&max));

	std::vector<SkinnedVertex> skinnedVertices;
	std::vector<Vertex> vertices;
	std::vector<GeoIndex> indices;
	if (isSkinned)
	{
		_vertexByteStride = sizeof(SkinnedVertex);
		_vertexBufferByteSize = totalVertexCount * sizeof(SkinnedVertex);

		skinnedVertices.reserve(totalVertexCount);
	}
	else
	{
		_vertexByteStride = sizeof(Vertex);
		_vertexBufferByteSize = totalVertexCount * sizeof(Vertex);

		vertices.reserve(totalVertexCount);
	}
	_indexBufferByteSize = totalIndexCount * sizeof(GeoIndex);
	indices.reserve(totalIndexCount);

	UINT baseIndexLocation = 0;
	UINT baseVertexLocation = 0;
	const auto& childList = rootNode.getChildNodes();
	for (int i = 0; i < childList.size(); ++i)
	{
		const std::string& nodeName = childList[i].getNodeName();
		
		if (nodeName == "SubMesh")
		{
			std::string name;
			UINT vertexCount, indexCount;
			childList[i].loadAttribute("Name", name);
			childList[i].loadAttribute("VertexCount", vertexCount);
			childList[i].loadAttribute("IndexCount", indexCount);
			SubMeshGeometry subMesh;
			subMesh._name = name;
			subMesh._baseVertexLoaction = baseVertexLocation;
			subMesh._baseIndexLoacation = baseIndexLocation;
			subMesh._indexCount = indexCount;

			baseVertexLocation += vertexCount;
			baseIndexLocation += indexCount;
			_subMeshList.emplace_back(subMesh);

			const auto& subMeshChildList = childList[i].getChildNodes();
			for (int j = 0; j < subMeshChildList.size(); ++j)
			{
				const std::string& subMeshNodeName = subMeshChildList[j].getNodeName();
				if (subMeshNodeName == "Vertices")
				{
					if (isSkinned)
					{
						loadXmlSkinnedVertices(subMeshChildList[j], skinnedVertices);
					}
					else
					{
						loadXmlVertices(subMeshChildList[j], vertices);
					}
				}
				else if (subMeshNodeName == "Indices")
				{
					loadXmlIndices(subMeshChildList[j], indices);
				}
				else
				{
					ThrowErrCode(ErrCode::NodeNameNotFound, "subMeshNodeName이 이상합니다." + subMeshNodeName);
				}
			}
		}
		else
		{
			ThrowErrCode(ErrCode::NodeNotFound, "nodeName이 이상합니다." + nodeName);
		}
	}

	if (isSkinned)
	{
		createMeshGeometryXXX(device, commandList, skinnedVertices.data(), indices.data());
	}
	else
	{
		createMeshGeometryXXX(device, commandList, vertices.data(), indices.data());
	}
}
#if defined DEBUG | defined _DEBUG
MeshGeometry::MeshGeometry(const GeneratedMeshData& meshData, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	_vertexBufferByteSize = meshData._vertices.size() * sizeof(Vertex);
	_vertexByteStride = sizeof(Vertex);
	_indexBufferByteSize = meshData._indices.size() * sizeof(GeoIndex);

	SubMeshGeometry subMesh;
	subMesh._baseIndexLoacation = 0;
	subMesh._baseVertexLoaction = 0;
	subMesh._indexCount = meshData._indices.size();
	_subMeshList.push_back(subMesh);

	createMeshGeometryXXX(device, commandList, meshData._vertices.data(), meshData._indices.data());
}
#endif

const Vertex* MeshGeometry::getVertexBufferXXX(size_t& bufferSize) const noexcept
{
	check(_vertexBufferCPU != nullptr);
	check(_vertexByteStride == sizeof(Vertex));

	bufferSize = _vertexBufferCPU->GetBufferSize() / _vertexByteStride;
	return reinterpret_cast<Vertex*>(_vertexBufferCPU->GetBufferPointer());
}

const GeoIndex* MeshGeometry::getIndexBufferXXX(void) const noexcept
{
	check(_indexBufferCPU != nullptr);
	
	return reinterpret_cast<GeoIndex*>(_indexBufferCPU->GetBufferPointer());
}

const DirectX::BoundingBox& MeshGeometry::getBoundingBox(void) const noexcept
{
	return _boundingBox;
}

void MeshGeometry::setVertexByteSizeOnlyXXXXX(UINT vertexBufferSize) noexcept
{
	_vertexBufferCPU = nullptr;
	_vertexBufferGPU = nullptr;
	_vertexBufferByteSize = vertexBufferSize;
}

void MeshGeometry::setNameXXXXX(const std::string& name) noexcept
{
	_name = name;
}

void MeshGeometry::setVertexBufferGPUXXXXX(ID3D12Resource* buffer) noexcept
{
	_vertexBufferGPU = buffer;
}

void MeshGeometry::createIndexBufferXXX(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::vector<GeoIndex>& ib)
{
	check(ib.size() != 0, "buffer size가 0입니다.");
	const UINT ibByteSize = (UINT)ib.size() * sizeof(GeoIndex);
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &_indexBufferCPU));

	CopyMemory(_indexBufferCPU->GetBufferPointer(), ib.data(), ibByteSize);
	_indexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList, ib.data(), ibByteSize, _indexBufferUploader);
	_indexBufferByteSize = ibByteSize;
}

D3D12_VERTEX_BUFFER_VIEW MeshGeometry::getVertexBufferView(void) const noexcept
{
	check(_vertexBufferGPU != nullptr, "vertexBuffer가 할당된 상태가 아닙니다.");
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = _vertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = _vertexByteStride;
	vbv.SizeInBytes = _vertexBufferByteSize;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW MeshGeometry::getIndexBufferView(void) const noexcept
{
	check(_indexBufferGPU != nullptr, "indexBuffer가 할당된 상태가 아닙니다.");
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = _indexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = _indexFormat;
	ibv.SizeInBytes = _indexBufferByteSize;

	return ibv;
}