#include "stdafx.h"
#include "BackgroundObject.h"
#include "GameObject.h"
#include "StageInfo.h"
#include "D3DApp.h"
#include "SMGFramework.h"

BackgroundObject::BackgroundObject(const BackgroundObjectInfo& backgroundObjectInfo)
{
	_gameObject = SMGFramework::getD3DApp()->createObjectFromXML(backgroundObjectInfo._objectFileName);

	_gameObject->setWorldMatrix(backgroundObjectInfo._position,
		backgroundObjectInfo._direction,
		backgroundObjectInfo._upVector,
		backgroundObjectInfo._size);
}

BackgroundObject::~BackgroundObject()
{
	SMGFramework::getD3DApp()->removeGameObject(_gameObject);
}

void BackgroundObject::setCulled() noexcept
{
	_gameObject->setCulledBackground();
}
