#pragma once

class UserData
{
public:
	UserData() noexcept;
	void increaseStarBit() noexcept;
	bool decreaseStarBit() noexcept;
	void increaseCoin() noexcept;
	void increaseLife() noexcept;
	bool decreaseLife() noexcept;

private:
	static constexpr int COIN_MAX = 100;
	uint32_t _starBit;
	uint32_t _coin;
	uint32_t _life;
};