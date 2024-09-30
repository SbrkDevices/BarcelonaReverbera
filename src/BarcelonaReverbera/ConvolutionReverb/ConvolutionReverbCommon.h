// BarcelonaReverbera - A Non-Uniform Partitioned Convolution Reverb VST3 Plugin
// Copyright (C) 2024 sbrk devices
//
// This file is part of BarcelonaReverbera.
//
// BarcelonaReverbera is free software: you can use it and/or modify it for
// educational and non-commercial purposes only under the terms of the
// Custom Non-Commercial License.
//
// You should have received a copy of the Custom Non-Commercial License
// along with this program. If not, see https://github.com/SbrkDevices/BarcelonaReverbera.
//
// For more information, please contact dani@sbrkdevices.com.

///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <cstring>
#include <cmath>
#include <atomic>

///////////////////////////////////////////////////////////////////////////////

#define BCNRVRB_DEFAULT_IR_SAMPLERATE							(48000)
#define BCNRVRB_MAX_SAMPLERATE									(48000*8)

#define BCNRVRB_MAX_BLOCK_SIZE									(8*1024)
#define BCNRVRB_MIN_BLOCK_SIZE									(16)

#define BCNRVRB_MAX_IR_LEN_SECONDS								(10)
#define BCNRVRB_IR_MAX_LEN_SAMPLES								(BCNRVRB_MAX_SAMPLERATE*BCNRVRB_MAX_IR_LEN_SECONDS)
#define BCNRVRB_IR_MIN_LEN_SAMPLES								(3*BCNRVRB_MAX_BLOCK_SIZE) // requirement due to the algorithm used

#define BCNRVRB_SMALLEST_STAGE_SIZE								(64)
#define BCNRVRB_LONGEST_STAGE_SIZE								(16*1024)
#define BCNRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE						(128)

#define BCNRVRB_PARAM_INTERPOL_ARRAY_LEN						(1024)

#define BCNRVRB_DRYWET_SMOOTH_LEN_MS							(5.0f)

#define BCNRVRB_COLOR_LPF_FREQ_MIN								(220.0) // 220 Hz
#define BCNRVRB_COLOR_HPF_FREQ_MAX								(3000.0) // 3 kHz
#define BCNRVRB_COLOR_LPF_FREQ_LOGMIN							(std::log(BCNRVRB_COLOR_LPF_FREQ_MIN))
#define BCNRVRB_COLOR_LPF_FREQ_RANGE							(std::log(20000.0) - BCNRVRB_COLOR_LPF_FREQ_LOGMIN)
#define BCNRVRB_COLOR_HPF_FREQ_LOGMIN							(std::log(20.0))
#define BCNRVRB_COLOR_HPF_FREQ_RANGE							(std::log(BCNRVRB_COLOR_HPF_FREQ_MAX) - BCNRVRB_COLOR_HPF_FREQ_LOGMIN)

#define BCNRVRB_DECAY_COLOR_SMOOTH_LEN_MS						(80.0f)

#define BCNRVRB_DECAY_MIN										(0.015f) // just 1.5% of the full IR
#define	BCNRVRB_DECAY_KNOB_DECADES								(2.15f) // knob behavior
#define BCNRVRB_DECAY_ENVELOPE_PERCENTAGE						(2.3f) // exp. decaying part is 230% of the full gain part

#define BCNRVRB_MIN_DB											(-120.0f)

///////////////////////////////////////////////////////////////////////////////

#define BCNRVRB_COLOR_BLACK										(0xFF000000)
#define BCNRVRB_COLOR_WHITE										(0xFFFFFFFF)
#define BCNRVRB_COLOR_PURPLE									(0xFF777FE2)
#define BCNRVRB_COLOR_GREY_LIGHT								(0xFF6D6D6D)
#define BCNRVRB_COLOR_GREY_DARK									(0xFF494949)

///////////////////////////////////////////////////////////////////////////////

//#define USE_DEBUG_ASSERT      				1 // allows enabling asserts manually

#if !defined(USE_DEBUG_ASSERT) && defined(_DEBUG)
#	define USE_DEBUG_ASSERT       				1
#endif

#if USE_DEBUG_ASSERT
#	include <cassert>
#	define DEBUG_ASSERT(x)						do { assert(x); } while (0)
#	define DEBUG_VERIFY(x)						DEBUG_ASSERT(x)
#else
#	define DEBUG_ASSERT(x)						do { } while (0)
#	define DEBUG_VERIFY(x)						((void)(x))
#endif

///////////////////////////////////////////////////////////////////////////////

class DspUtils
{
public:
	static inline bool isPowOf2(int x)
	{
		return (((x) != 0) && (((x) & ((x) - 1)) == 0));
	}

	static inline float dB2Lin(float x, float mindB)
	{
		return (x > mindB) ? std::pow(10.0f, (x) * (1.0f / 20.0f)) : 0.0f;
	}

	static inline float getTimeConstantSamples(float samples)
	{
        return std::exp(-2.2f / samples);
	}

	static inline float getTimeConstantMs(float ms, float fs)
	{
		return std::exp(-2200.0f / (ms * fs));
	}

	static inline float expSmoothing(float target, float current, float rate)
	{
		return target - target*rate + current*rate;
		// return current + (target - current)*(1.0f - rate);
	}

	// linear interpolation between y0 and y1:
	static inline float linearInterpolate(float y0, float y1, float mu)
	{
		DEBUG_ASSERT((mu >= 0.0f) && (mu <= 1.0f));
		return y0 * (1.0f - mu) + (y1 * mu);
	}

	static inline void smoothParameter(const float target, float& futureCurrent, float& current, float& incr, const float smoothingFactor, uint32_t blockSize)
	{
		current = futureCurrent;
		futureCurrent = expSmoothing(target, current, smoothingFactor);
		if (futureCurrent == current) // we have reached a point where diff between target and current is only due to FP precission
			futureCurrent = target;
		incr = (futureCurrent - current)/blockSize;
	}
};

///////////////////////////////////////////////////////////////////////////////
