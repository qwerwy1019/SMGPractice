#pragma once

class UserData
{
public:
	UserData() noexcept;
	void increaseStarBit() noexcept;
	bool decreaseStarBit() noexcept;
	void increaseCoin() noexcept;
	void increaseLife(int count) noexcept;
	bool decreaseLife() noexcept;

	uint32_t getStarBit() const noexcept { return _starBit; }
	uint32_t getCoin() const noexcept { return _coin; }
	uint32_t getLife() const noexcept { return _life; }

private:
	static constexpr int COIN_MAX = 100;
	uint32_t _starBit;
	uint32_t _coin;
	uint32_t _life;
};