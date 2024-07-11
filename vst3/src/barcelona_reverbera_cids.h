#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

#include "impulse_responses/impulse_responses.h"

//////////////////////////////////////////////////////////////////////////////

#define BARCELONA_REVERBERA_MAX_BLOCK_SIZE						(8*1024)
#define BARCELONA_REVERBERA_MIN_BLOCK_SIZE						(8)
#define BARCELONA_REVERBERA_IR_MAX_LEN_SAMPLES					(48000*16) // 16 seconds of IR...

#define CONVRVRB_SMALLEST_STAGE_SIZE							(64)
#define CONVRVRB_LONGEST_STAGE_SIZE								(8*1024)
#define CONVRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE					(128)

//////////////////////////////////////////////////////////////////////////////

enum BarcelonaReverberaParams : Steinberg::Vst::ParamID
{
	kBypassId = 100,

	kParamSizeId = 102,
	kParamColorId = 103,
	kParamDryWetId = 104,
	kParamSelectedIR = 105
};

//////////////////////////////////////////////////////////////////////////////

static const Steinberg::FUID kBarcelonaReverberaProcessorUID(0x32C50013, 0xFF5F5CB4, 0x871C312D, 0xB4F42368);
static const Steinberg::FUID kBarcelonaReverberaControllerUID(0xAE34DD83, 0x308259DF, 0xA0D88E2F, 0xB1C1CB8B);

//////////////////////////////////////////////////////////////////////////////

#define BarcelonaReverberaVST3Category "Fx"

//////////////////////////////////////////////////////////////////////////////
