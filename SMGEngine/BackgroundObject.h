#pragma once
class GameObject;
class BackgroundObjectInfo;

class BackgroundObject
{
public:
	BackgroundObject(const BackgroundObjectInfo& backgroundObjectInfo);
	~BackgroundObject();
	void setCulled() noexcept;
private:
	GameObject* _gameObject;
};