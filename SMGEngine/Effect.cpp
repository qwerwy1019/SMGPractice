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
	node.loadAttribute("AlphaDecrease", _alphaDecrease);
	node.loadAttribute("SizeDecrease", _sizeDecrease);
	node.loadAttribute("IsRepeat", _isRepeat);

	if (_tickPerFrame * _totalFrame == 0)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, "Effect 재생 시간이 0입니다.");
	}
}

void Effect::addInstance(EffectInstance&& instance) noexcept
{
	instance._startTick = SMGFramework::Get().getTimer().getCurrentTickCount();
	_instances.emplace_back(instance);
}

void Effect::updateEffectInstanceData(FrameResource* frameResource, uint32_t& instanceCount) const noexcept
{
	check(frameResource != nullptr);
	if (_instances.empty())
	{
		return;
	}
	TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();

	float radius = std::sqrt(_size.x * _size.x + _size.y + _size.y);
	XMMATRIX identityMat = XMLoadFloat4x4(&MathHelper::Identity4x4);
	EffectInstanceData instanceData;
	instanceData._totalFrame = _totalFrame;
	instanceData._textureIndex = _textureIndex;
	TickCount64 effectTick = _totalFrame * _tickPerFrame;
	for (const auto& instance : _instances)
	{
		BoundingBox box(instance._position, XMFLOAT3(radius, radius, radius));
		if (SMGFramework::getD3DApp()->checkCulled(box, identityMat))
		{
			continue;
		}

		TickCount64 effectLocalTick = (currentTick - instance._startTick) % effectTick;

		instanceData._position = instance._position;
		instanceData._frame = effectLocalTick / _tickPerFrame;
		float sizeDecreaseRate = _sizeDecrease * (static_cast<float>(effectLocalTick) / effectTick);
		float sizeRate = instance._size * (1 - sizeDecreaseRate);
		instanceData._size = XMFLOAT2(_size.x * sizeRate, _size.y * sizeRate);

		float alphaDecreaseRate = _alphaDecrease * (static_cast<float>(effectLocalTick) / effectTick);
		instanceData._alpha = 1 - alphaDecreaseRate;

		frameResource->setEffectBuffer(instanceCount++, instanceData);
	}
}

void Effect::update(void) noexcept
{
	if (_isRepeat)
	{
		return;
	}
	TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();
	currentTick -= _tickPerFrame * _totalFrame;
	while (!_instances.empty() && _instances.front()._startTick <= currentTick)
	{
		_instances.pop_front();
	}
}

EffectInstance::EffectInstance(const DirectX::XMFLOAT3& position, float size) noexcept
	: _position(position)
	, _startTick(0)
	, _size(size)
{

}

EffectManager::EffectManager()
	: _instanceCount(0)
	, _mesh(nullptr)
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

void EffectManager::addEffectInstance(const std::string& effectName, EffectInstance&& instance) noexcept
{
	auto it = _effects.find(effectName);
	if (it == _effects.end())
	{
		check(false, effectName + "가 이펙트 목록에 없습니다.");
		return;
	}

	it->second->addInstance(std::move(instance));
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
