#include <cassert>

#include "barcelona_reverbera_processor.h"

#if CONVRVRB_USE_EXAMPLE_IR
#	include "impulse_responses/IR_example.h"
#else
#	include "impulse_responses/IR_20LUFS_MNAC.h"
#	include "impulse_responses/IR_20LUFS_MODELO.h"
#	include "impulse_responses/IR_20LUFS_PALAU.h"
#	include "impulse_responses/IR_20LUFS_PARANINF.h"
#	include "impulse_responses/IR_20LUFS_REFUGI_307.h"
#	include "impulse_responses/IR_20LUFS_UB_LOBBY.h"
#endif

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

//////////////////////////////////////////////////////////////////////////////

using namespace Steinberg;

//////////////////////////////////////////////////////////////////////////////

char m_strIRNames[CONVRVRB_IR_COUNT][128];

//////////////////////////////////////////////////////////////////////////////

BarcelonaReverberaProcessor::BarcelonaReverberaProcessor()
{
	setControllerClass(kBarcelonaReverberaControllerUID);

	initIRsData();

	m_convolutionReverb.init(16); // XXX can we know block size already?
}

BarcelonaReverberaProcessor::~BarcelonaReverberaProcessor()
{
}

//////////////////////////////////////////////////////////////////////////////

tresult PLUGIN_API BarcelonaReverberaProcessor::initialize(FUnknown* context)
{
	tresult result = AudioEffect::initialize(context);
	if (result != kResultOk)
		return result;

	//--- create Audio IO ------
	addAudioInput(STR16("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

	/* If you don't need an event bus, you can remove the next line */
	addEventInput(STR16("Event In"), 1);
	
	return kResultOk;
}

tresult PLUGIN_API BarcelonaReverberaProcessor::terminate()
{
	m_convolutionReverb.exit();

	return AudioEffect::terminate();
}

tresult PLUGIN_API BarcelonaReverberaProcessor::setActive(TBool state)
{
	return AudioEffect::setActive(state);
}

//////////////////////////////////////////////////////////////////////////////

tresult PLUGIN_API BarcelonaReverberaProcessor::process(Vst::ProcessData& data)
{
    if (data.inputParameterChanges)
    {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount();

        for (int32 index=0; index<numParamsChanged; index++)
        {
            Vst::IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData(index);
            
			if (paramQueue)
            {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();

                switch (paramQueue->getParameterId())
                {
                    case BarcelonaReverberaParams::kParamSizeId:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                            paramSize = value;
                        break;

                    case BarcelonaReverberaParams::kParamColorId:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                            paramColor = value;
                        break;

                    case BarcelonaReverberaParams::kParamDryWetId:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                            paramDryWet = value;
                        break;
					
					case BarcelonaReverberaParams::kParamSelectedIR:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
							paramSelectedIR = std::min<int>(CONVRVRB_IR_COUNT*value, CONVRVRB_IR_COUNT - 1);
						break;

					case BarcelonaReverberaParams::kBypassId:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                            paramBypass = (value > 0.5f);
                        break;
                }
            }
        }
    }

    if (data.numInputs == 0 || data.numOutputs == 0)
        return kResultOk; // nothing to do

	const int32 numChannels = data.inputs[0].numChannels;
	const int32 blockSizeSamples = data.numSamples;
	//const int32 samplerate = data.sampleRate; // XXX TODO

	bool hostParamsChange = false;
	bool selectedIrChange = false;

	if (paramSelectedIR != m_ir_index)
		selectedIrChange = true;
	if (blockSizeSamples != m_blockSize)
		hostParamsChange = true;
	//if (samplerate != m_samplerate)
	//	hostParamsChange = true;
	
	m_ir_index = paramSelectedIR;
	m_blockSize = blockSizeSamples;
	//m_samplerate = samplerate;

	if ((m_blockSize == 0) || (numChannels < 1))
		return kResultOk;

	assert(m_blockSize >= BARCELONA_REVERBERA_MIN_BLOCK_SIZE);
	assert(m_blockSize <= BARCELONA_REVERBERA_MAX_BLOCK_SIZE);

	if (hostParamsChange)
		m_convolutionReverb.init(m_blockSize);

	if (selectedIrChange)
	{
		m_ir_len = m_irs_len[m_ir_index] + m_blockSize - (m_irs_len[m_ir_index] % m_blockSize);
		assert(m_ir_len <= m_irs_len_with_zeros[m_ir_index]);
		assert(double(m_ir_len)/double(m_blockSize) == double(m_ir_len/m_blockSize));

		m_convolutionReverb.setupIR(m_irs_ptr[m_ir_index][0], m_irs_ptr[m_ir_index][1], m_ir_len);
	}

	Vst::Sample32** inChannels = data.inputs[0].channelBuffers32;
	Vst::Sample32** outChannels = data.outputs[0].channelBuffers32;

	bool silenceFlag = (data.inputs[0].silenceFlags != 0);

	if (silenceFlag && !m_silenceFlag)
		m_silenceFlagCounter = 0;

	m_silenceFlag = silenceFlag;

	if (m_silenceFlag)
	{
		if (m_silenceFlagCounter < m_ir_len)
			m_silenceFlagCounter += m_blockSize;
		else
		{
		    data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

			for (int ch=0; ch<numChannels; ch++)
				memset(outChannels[ch], 0, m_blockSize);
			
			return kResultOk;
		}
	}

	data.outputs[0].silenceFlags = 0;

	assert((paramDryWet >= 0.0f) && (paramDryWet <= 1.0f));
	const float dry = (paramDryWet < 0.4) ? 1.0f : (1.0f - paramDryWet)/0.6f;
	const float wet = (paramDryWet > 0.6) ? 1.0f : paramDryWet/0.6f;

	const uint32_t convolutionChannels = (numChannels > 2) ? 2 : numChannels;
	m_convolutionReverb.process(inChannels, outChannels, convolutionChannels);

	for (int ch=0; ch<numChannels; ch++)
	{	
		Vst::Sample32* in = inChannels[ch];
		Vst::Sample32* out = outChannels[ch];

		if (ch >= 2)
		{
			for (int i=0; i<m_blockSize; i++)
				out[i] = in[i];
		
			continue;
		}

		for (uint32_t i=0; i<m_blockSize; i++)
			out[i] = dry*in[i] + wet*out[i]*0.1f; // XXX for now, using constant gain of 0.1
	}

    return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////

tresult PLUGIN_API BarcelonaReverberaProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
{
	//--- called before any processing ----
	return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API BarcelonaReverberaProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
	// by default kSample32 is supported
	if (symbolicSampleSize == Vst::kSample32)
		return kResultTrue;

	// disable the following comment if your processing support kSample64
	/* if (symbolicSampleSize == Vst::kSample64)
		return kResultTrue; */

	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////

tresult PLUGIN_API BarcelonaReverberaProcessor::setState(IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer(state, kLittleEndian);

	float savedParamSize = 0.f;
	if (streamer.readFloat(savedParamSize) == false)
		return kResultFalse;

	float savedParamColor = 0.f;
	if (streamer.readFloat(savedParamColor) == false)
		return kResultFalse;

	float savedParamDryWet = 0.f;
	if (streamer.readFloat(savedParamDryWet) == false)
		return kResultFalse;
	
	int32 savedParamSelectedIR = 0;
	if (streamer.readInt32(savedParamSelectedIR) == false)
		return kResultFalse;

	int32 savedBypass = 0;
	if (streamer.readInt32(savedBypass) == false)
		return kResultFalse;

	paramSize = savedParamSize;
	paramColor = savedParamColor;
	paramDryWet = savedParamDryWet;
	paramSelectedIR = savedParamSelectedIR;
	paramBypass = savedBypass > 0;

	return kResultOk;
}

tresult PLUGIN_API BarcelonaReverberaProcessor::getState(IBStream* state)
{
	float toSaveParamSize = paramSize;
	float toSaveParamColor = paramColor;
	float toSaveParamDryWet = paramDryWet;
	int32 toSaveParamSelectedIR = paramSelectedIR;
	int32 toSaveBypass = paramBypass ? 1 : 0;

	IBStreamer streamer(state, kLittleEndian);
	streamer.writeFloat(toSaveParamSize);
	streamer.writeFloat(toSaveParamColor);
	streamer.writeFloat(toSaveParamDryWet);
	streamer.writeInt32(toSaveParamSelectedIR);
	streamer.writeInt32(toSaveBypass);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////

void BarcelonaReverberaProcessor::initIRsData(void)
{
# if CONVRVRB_USE_EXAMPLE_IR
	strcpy(m_strIRNames[0], "Example IR - Delta Function");
	m_irs_len_with_zeros[0] = sizeof(__IR_EXAMPLE__)/(2*sizeof(float));
	m_irs_len[0] =  m_irs_len_with_zeros[0] - __IR_EXAMPLE_EXTRA_ZEROS__;
	for (int ch=0; ch<2; ch++)
		m_irs_ptr[0][ch] = __IR_EXAMPLE__[ch];
# else
	strcpy(m_strIRNames[0], "MNAC - Museu Nacional d'Art de Catalynya");
	m_irs_len_with_zeros[0] = sizeof(__IR_20LUFS_MNAC__)/(2*sizeof(float));
	m_irs_len[0] =  m_irs_len_with_zeros[0] - __IR_20LUFS_MNAC_EXTRA_ZEROS__;
	for (int ch=0; ch<2; ch++)
		m_irs_ptr[0][ch] = __IR_20LUFS_MNAC__[ch];

	strcpy(m_strIRNames[1], "Centro Cultural La Modelo");
	m_irs_len_with_zeros[1] = sizeof(__IR_20LUFS_MODELO__)/(2*sizeof(float));
	m_irs_len[1] =  m_irs_len_with_zeros[1] - __IR_20LUFS_MODELO_EXTRA_ZEROS__;
	for (int ch=0; ch<2; ch++)
		m_irs_ptr[1][ch] = __IR_20LUFS_MODELO__[ch];

	strcpy(m_strIRNames[2], "Palau de la Musica Catalana");
	m_irs_len_with_zeros[2] = sizeof(__IR_20LUFS_PALAU__)/(2*sizeof(float));
	m_irs_len[2] =  m_irs_len_with_zeros[2] - __IR_20LUFS_PALAU_EXTRA_ZEROS__;
	for (int ch=0; ch<2; ch++)
		m_irs_ptr[2][ch] = __IR_20LUFS_PALAU__[ch];

	strcpy(m_strIRNames[3], "Paraninfo del Edificio Historico de la UB");
	m_irs_len_with_zeros[3] = sizeof(__IR_20LUFS_PARANINF__)/(2*sizeof(float));
	m_irs_len[3] =  m_irs_len_with_zeros[3] - __IR_20LUFS_PARANINF_EXTRA_ZEROS__;
	for (int ch=0; ch<2; ch++)
		m_irs_ptr[3][ch] = __IR_20LUFS_PARANINF__[ch];

	strcpy(m_strIRNames[4], "Refugi 307");
	m_irs_len_with_zeros[4] = sizeof(__IR_20LUFS_REFUGI_307__)/(2*sizeof(float));
	m_irs_len[4] =  m_irs_len_with_zeros[4] - __IR_20LUFS_REFUGI_307_EXTRA_ZEROS__;
	for (int ch=0; ch<2; ch++)
		m_irs_ptr[4][ch] = __IR_20LUFS_REFUGI_307__[ch];

	strcpy(m_strIRNames[5], "Universitat de Barcelona - Lobby");
	m_irs_len_with_zeros[5] = sizeof(__IR_20LUFS_UB_LOBBY__)/(2*sizeof(float));
	m_irs_len[5] =  m_irs_len_with_zeros[5] - __IR_20LUFS_UB_LOBBY_EXTRA_ZEROS__;
	for (int ch=0; ch<2; ch++)
		m_irs_ptr[5][ch] = __IR_20LUFS_UB_LOBBY__[ch];
# endif
}

//////////////////////////////////////////////////////////////////////////////