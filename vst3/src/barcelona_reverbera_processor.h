#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

#include "barcelona_reverbera_cids.h"
#include "fft/fft.h"
#include "convolution_reverb/convolution_reverb.h"

//////////////////////////////////////////////////////////////////////////////

class BarcelonaReverberaProcessor : public Steinberg::Vst::AudioEffect
{
public:
	BarcelonaReverberaProcessor();
	~BarcelonaReverberaProcessor() SMTG_OVERRIDE;

    // Create function
	static Steinberg::FUnknown* createInstance (void* /*context*/) 
	{ 
		return (Steinberg::Vst::IAudioProcessor*) new BarcelonaReverberaProcessor; 
	}

	/** Called at first after constructor */
	Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
	
	/** Called at the end before destructor */
	Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;
	
	/** Switch the Plug-in on/off */
	Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;

	/** Will be called before any process call */
	Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
	
	/** Asks if a given sample size is supported see SymbolicSampleSizes. */
	Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

	/** Here we go...the process call */
	Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
		
	/** For persistence */
	Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

protected:
	Steinberg::Vst::ParamValue paramSize = 0.5;
	Steinberg::Vst::ParamValue paramColor = 0.5;
	Steinberg::Vst::ParamValue paramDryWet = 0.5;
	int paramSelectedIR = 0;
	bool paramBypass = false;

private:
	float* m_irs_ptr[CONVRVRB_IR_COUNT][2] = {};
	uint32_t m_irs_len[CONVRVRB_IR_COUNT] = {};
	uint32_t m_irs_len_with_zeros[CONVRVRB_IR_COUNT] = {};

	uint32_t m_blockSize = 0;
	float m_samplerate = 48000.0;

	int m_ir_index = -1;
	uint32_t m_ir_len = 0;

	int m_silenceFlag = 0;
	uint32_t m_silenceFlagCounter = 0;

	ConvolutionReverb m_convolutionReverb;

	void initIRsData(void);
};

//////////////////////////////////////////////////////////////////////////////
