#include "stdafx.h"
#include "UserData.h"

UserData::UserData() noexcept
	: _starBit(0)
	, _coin(0)
	, _life(5)
{

}

void UserData::increaseStarBit() noexcept
{
	++_starBit;
}

bool UserData::decreaseStarBit() noexcept
{
	if (_starBit == 0)
	{
		return false;
	}
	--_starBit;
	return true;
}

void UserData::increaseCoin() noexcept
{
	++_coin;
	if (_coin == COIN_MAX)
	{
		_coin = 0;
		++_life;
	}
}

void UserData::increaseLife() noexcept
{
	++_life;
}

bool UserData::decreaseLife() noexcept
{
	if (_life == 0)
	{
		return false;
	}
	--_life;
	return true;
}

