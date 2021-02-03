#include "stdafx.h"
#include "MeshGeometry.h"
#include "D3DUtil.h"
#include "FileHelper.h"

// MeshGeometry::MeshGeometry(ID3D12Device* device,
// 	ID3D12GraphicsCommandList* commandList,
// 	const std::string& name,
// 	const std::vector<std::string>& subMeshNames,
// 	const std::vector<std::vector<Vertex>>& vb,
// 	const std::vector<std::vector<Index>>& ib)
// 	: _vertexByteStride(sizeof(Vertex))
// 	, _name(name)
// {
// 	assert(device != nullptr);
// 	assert(commandList != nullptr);
// 	assert(!subMeshNames.empty());
// 	assert(!vb.empty());
// 	assert(!ib.empty());
// 	assert(subMeshNames.size() == vb.size());
// 	assert(subMeshNames.size() == ib.size());
// 	
// 	std::vector<Vertex> totalVertices = D3DUtil::mergeContainer(vb);
// 	std::vector<Index> totalIndices = D3DUtil::mergeContainer(ib);
// 
// 	_vertexBufferByteSize = totalVertices.size() * sizeof(Vertex);
// 	_indexBufferByteSize = totalIndices.size() * sizeof(Index);
// 	
// 	makeSubMeshMap(subMeshNames, vb, ib);
// 
// 	createMeshGeometryXXX(device, commandList, totalVertices.data(), totalIndices.data());
// }

void MeshGeometry::createMeshGeometryXXX(ID3D12Device* device,
										ID3D12GraphicsCommandList* commandList,
										const void* vb,
										const void* ib)
{
	ThrowIfFailed(D3DCreateBlob(_vertexBufferByteSize, &_vertexBufferCPU));
	CopyMemory(_vertexBufferCPU->GetBufferPointer(), vb, _vertexBufferByteSize);

	ThrowIfFailed(D3DCreateBlob(_indexBufferByteSize, &_indexBufferCPU));
	CopyMemory(_indexBufferCPU->GetBufferPointer(), ib, _indexBufferByteSize);

	_vertexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList, vb, _vertexBufferByteSize, _vertexBufferUploader);
	_indexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList, ib, _indexBufferByteSize, _indexBufferUploader);
}

HRESULT MeshGeometry::loadXmlSkinnedVertices(const XMLReaderNode& node, std::vector<SkinnedVertex>& skinnedVertices) const noexcept
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
	return S_OK;
}

HRESULT MeshGeometry::loadXmlVertices(const XMLReaderNode& node, std::vector<Vertex>& vertices) const noexcept
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
	return S_OK;
}

HRESULT MeshGeometry::loadXmlIndices(const XMLReaderNode& node, std::vector<Index>& indices) const noexcept
{
	const auto& childNodes = node.getChildNodes();
	for (int i = 0; i < childNodes.size(); ++i)
	{
		Index index0, index1, index2;
		childNodes[i].loadAttribute("_0", index0);
		childNodes[i].loadAttribute("_1", index1);
		childNodes[i].loadAttribute("_2", index2);

		indices.push_back(index0);
		indices.push_back(index1);
		indices.push_back(index2);
	}
	return S_OK;
}

HRESULT MeshGeometry::loadXml(const XMLReaderNode& rootNode, ID3D12Device* device, ID3D12GraphicsCommandList* commandList) noexcept
{
	HRESULT rv;
	rootNode.loadAttribute("Name", _name);
	bool isSkinned;
	rootNode.loadAttribute("IsSkinned", isSkinned);
	int totalVertexCount;
	rootNode.loadAttribute("TotalVertexCount", totalVertexCount);
	int totalIndexCount;
	rootNode.loadAttribute("TotalIndexCount", totalIndexCount);

	std::vector<SkinnedVertex> skinnedVertices;
	std::vector<Vertex> vertices;
	std::vector<Index> indices;
	if (isSkinned)
	{
		_vertexByteStride = sizeof(SkinnedVertex);
		_vertexBufferByteSize = totalVertexCount * sizeof(SkinnedVertex);

		skinnedVertices.reserve(totalVertexCount);
	}
	else
	{
		vertices.reserve(totalVertexCount);
		_vertexBufferByteSize = totalVertexCount * sizeof(Vertex);

		_vertexByteStride = sizeof(Vertex);
	}
	_indexBufferByteSize = totalIndexCount * sizeof(Index);
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
			subMesh._baseVertexLoaction = baseVertexLocation;
			subMesh._baseIndexLoacation = baseIndexLocation;
			subMesh._indexCount = indexCount;

			baseVertexLocation += vertexCount;
			baseIndexLocation += indexCount;
			_subMeshMap.emplace(name, subMesh);

			const auto& subMeshChildList = childList[i].getChildNodes();
			for (int j = 0; j < subMeshChildList.size(); ++j)
			{
				const std::string& subMeshNodeName = subMeshChildList[j].getNodeName();
				if (subMeshNodeName == "Vertices")
				{
					if (isSkinned)
					{
						rv = loadXmlSkinnedVertices(subMeshChildList[j], skinnedVertices);
					}
					else
					{
						rv = loadXmlVertices(subMeshChildList[j], vertices);
					}
					if (FAILED(rv))
					{
						assert(false);
						return rv;
					}
				}
				else if (subMeshNodeName == "Indices")
				{
					rv = loadXmlIndices(subMeshChildList[j], indices);
					if (FAILED(rv))
					{
						assert(false);
						return rv;
					}
				}
				else
				{
					assert(false);
					return E_FAIL;
				}
			}
		}
		else
		{
			assert(false);
			return E_FAIL;
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
	return S_OK;
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

void MeshGeometry::createIndexBufferXXX(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::vector<Index>& ib)
{
	assert(ib.size() != 0);
	const UINT ibByteSize = (UINT)ib.size() * sizeof(Index);
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &_indexBufferCPU));

	CopyMemory(_indexBufferCPU->GetBufferPointer(), ib.data(), ibByteSize);
	_indexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList, ib.data(), ibByteSize, _indexBufferUploader);
	_indexBufferByteSize = ibByteSize;
}

D3D12_VERTEX_BUFFER_VIEW MeshGeometry::getVertexBufferView(void) const noexcept
{
	assert(_vertexBufferGPU != nullptr);
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = _vertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = _vertexByteStride;
	vbv.SizeInBytes = _vertexBufferByteSize;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW MeshGeometry::getIndexBufferView(void) const noexcept
{
	assert(_indexBufferGPU != nullptr);
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = _indexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = _indexFormat;
	ibv.SizeInBytes = _indexBufferByteSize;

	return ibv;
}

// template<typename VertexCont>
// void MeshGeometry::makeSubMeshMap(const std::vector<std::string>& subMeshNames,
// 	const VertexCont& vb,
// 	const std::vector<std::vector<Index>>& ib)
// {
// 	CommonIndex startIndexLocation = 0;
// 	CommonIndex baseVertexLocation = 0;
// 	for (int i = 0; i < subMeshNames.size(); ++i)
// 	{
// 		if (ib[i].empty())
// 		{
// 			continue;
// 		}
// 
// 		const SubMeshGeometry subMesh = { ib[i].size(), startIndexLocation, baseVertexLocation };
// 		auto it = _subMeshMap.emplace(std::make_pair(subMeshNames[i], subMesh));
// 		if (it.second == false)
// 		{
// 			assert(false && L"submesh name이 중복되었음!! mesh가 생성되지 않습니다.");
// 			return;
// 		}
// 
// 		startIndexLocation += ib[i].size();
// 		baseVertexLocation += vb[i].size();
// 	}
// }