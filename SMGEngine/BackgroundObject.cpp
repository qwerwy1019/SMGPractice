#include "stdafx.h"
#include "BackgroundObject.h"
#include "GameObject.h"
#include "StageInfo.h"
#include "D3DApp.h"
#include "SMGFramework.h"
#include "ObjectInfo.h"

BackgroundObject::BackgroundObject(const BackgroundObjectInfo& backgroundObjectInfo)
{
	_gameObject = SMGFramework::getD3DApp()->createObjectFromXML(backgroundObjectInfo.getObjectFileName());

	_gameObject->setWorldMatrix(backgroundObjectInfo.getPosition(),
		backgroundObjectInfo.getDirection(),
		backgroundObjectInfo.getUpVector(),
		backgroundObjectInfo.getSize());
}

BackgroundObject::~BackgroundObject()
{
	SMGFramework::getD3DApp()->removeGameObject(_gameObject);
}

void BackgroundObject::setCulled() noexcept
{
	_gameObject->setCulledBackground();
}
