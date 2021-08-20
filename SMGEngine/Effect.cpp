#include "stdafx.h"
#include "Effect.h"
#include "SMGFramework.h"
#include "D3DApp.h"
#include "MathHelper.h"
#include "FileHelper.h"
#include "MeshGeometry.h"

Effect::Effect(const XMLReaderNode& node)
{
	std::string textureName;
	node.loadAttribute("DiffuseTexture", textureName);
	_textureIndex = SMGFramework::getD3DApp()->loadTexture(textureName);
	node.loadAttribute("TotalFrame", _totalFrame);
	node.loadAttribute("TickPerFrame", _tickPerFrame);
	node.loadAttribute("Size", _size);
}

void Effect::addInstance(const DirectX::XMFLOAT3& position, float size) noexcept
{
	TickCount64 expireTick = SMGFramework::Get().getTimer().getCurrentTickCount() + (_tickPerFrame * _totalFrame);
	_instances.emplace_back(position, expireTick, size);
}

void Effect::updateEffectInstanceData(FrameResource* frameResource, uint32_t& instanceCount) const noexcept
{
	check(frameResource != nullptr);
	if (_instances.empty())
	{
		return;
	}
	TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();
	check(currentTick < _instances.front()._expireTick);

	float radius = std::sqrt(_size.x * _size.x + _size.y + _size.y);
	XMMATRIX identityMat = XMLoadFloat4x4(&MathHelper::Identity4x4);
	EffectInstanceData instanceData;
	instanceData._totalFrame = _totalFrame;
	instanceData._textureIndex = _textureIndex;
	instanceData._size = _size;
	instanceData._totalFrame = _totalFrame;

	for (const auto& instance : _instances)
	{
		BoundingBox box(instance._position, XMFLOAT3(radius, radius, radius));
		if (SMGFramework::getD3DApp()->checkCulled(box, identityMat))
		{
			continue;
		}

		instanceData._position = instance._position;
		instanceData._frame = getCurrentFrame(instance._expireTick - currentTick);
		instanceData._size = XMFLOAT2(_size.x * instance._size, _size.y * instance._size);

		frameResource->setEffectBuffer(instanceCount++, instanceData);
	}
}

void Effect::update(void) noexcept
{
	TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();
	while (!_instances.empty() && _instances.front()._expireTick <= currentTick)
	{
		_instances.pop_front();
	}
}

uint32_t Effect::getCurrentFrame(const TickCount64& leftTick) const noexcept
{
	check(leftTick > 0);
	return _totalFrame - 1 - (leftTick - 1) / _tickPerFrame;
}

EffectInstance::EffectInstance(const DirectX::XMFLOAT3& position, const TickCount64& expireTick, float size) noexcept
	: _position(position)
	, _expireTick(expireTick)
	, _size(size)
{

}

EffectManager::EffectManager()
	: _instanceCount(0)
{
}

void EffectManager::createEffectMeshGeometry(void)
{
	GeneratedMeshData pointMeshData;
	pointMeshData._vertices.push_back(Vertex(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0, 1, 0), DirectX::XMFLOAT2(0, 0)));
	pointMeshData._indices.push_back(0);

	_mesh = SMGFramework::getD3DApp()->createMeshGeometry("onePoint", pointMeshData);
}

void EffectManager::loadXML(const std::string& fileName)
{
	const std::string filePath = "../Resources/XmlFiles/Effect/" + fileName + ".xml";

	XMLReader xmlEffect;
	xmlEffect.loadXMLFile(filePath);
	const auto& childNodes = xmlEffect.getRootNode().getChildNodes();
	_effects.reserve(childNodes.size());
	for (const auto& childNode : childNodes)
	{
		std::string name;
		childNode.loadAttribute("Name", name);
		auto it = _effects.emplace(name, std::make_unique<Effect>(childNode));
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::KeyDuplicated, name);
		}
	}
}

void EffectManager::addEffectInstance(const std::string& effectName, const DirectX::XMFLOAT3& position, float size) noexcept
{
	auto it = _effects.find(effectName);
	if (it == _effects.end())
	{
		check(false, effectName + "가 이펙트 목록에 없습니다.");
		return;
	}

	it->second->addInstance(position, size);
}

void EffectManager::updateEffectInstanceData(FrameResource* frameResource) noexcept
{
	check(frameResource != nullptr);

	_instanceCount = 0;
	
	for (const auto& effect : _effects)
	{
		effect.second->updateEffectInstanceData(frameResource, _instanceCount);
	}
}

void EffectManager::update(void) noexcept
{
	for (auto& effect : _effects)
	{
		effect.second->update();
	}
}

bool EffectManager::hasEffect(const std::string& effectName) const noexcept
{
	auto it = _effects.find(effectName);
	if (it == _effects.end())
	{
		return false;
	}
	return true;
}

const MeshGeometry* EffectManager::getMeshGeometry(void) const noexcept
{
	return _mesh;
}

uint32_t EffectManager::getInstanceCount(void) const noexcept
{
	return _instanceCount;
}
