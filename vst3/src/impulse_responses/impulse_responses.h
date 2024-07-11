#pragma

#include <stdint.h>
#include <cstring>
#include <cassert>

//////////////////////////////////////////////////////////////////////////////

#define IMPULSE_RESPONSES_COUNT						(2)

//////////////////////////////////////////////////////////////////////////////

class ImpulseResponses
{
public:
	static inline char* getIrName(uint32_t index)
	{
		assert(index < IMPULSE_RESPONSES_COUNT);

		return m_irName[index];
	}

	inline float* getIrPtr(uint32_t index, uint8_t channel)
	{
		assert(index < IMPULSE_RESPONSES_COUNT);
		assert(channel < 2);

		return m_irPtr[index][channel];
	}

	inline uint32_t getIrLen(uint32_t index)
	{
		assert(index < IMPULSE_RESPONSES_COUNT);

		return m_irLen[index];
	}

	inline uint32_t getIrLenWithZeros(uint32_t index)
	{
		assert(index < IMPULSE_RESPONSES_COUNT);

		return m_irLenWithZeros[index];
	}

public:
	void init(void);

private:
	static char m_irName[IMPULSE_RESPONSES_COUNT][128];

	float* m_irPtr[IMPULSE_RESPONSES_COUNT][2] = {};
	uint32_t m_irLen[IMPULSE_RESPONSES_COUNT] = {};
	uint32_t m_irLenWithZeros[IMPULSE_RESPONSES_COUNT] = {};
};

//////////////////////////////////////////////////////////////////////////////