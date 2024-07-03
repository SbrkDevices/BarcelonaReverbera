#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

//////////////////////////////////////////////////////////////////////////////

class BarcelonaReverberaController : public Steinberg::Vst::EditControllerEx1
{
public:
	BarcelonaReverberaController() = default;
	~BarcelonaReverberaController() SMTG_OVERRIDE = default;

	static Steinberg::FUnknown* createInstance(void* /*context*/)
	{
		return (Steinberg::Vst::IEditController*) new BarcelonaReverberaController;
	}

	// IPluginBase
	Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

	// EditController
	Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag,
                                                     Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getParamStringByValue(Steinberg::Vst::ParamID tag,
                                                        Steinberg::Vst::ParamValue valueNormalized,
                                                        Steinberg::Vst::String128 string) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getParamValueByString(Steinberg::Vst::ParamID tag,
                                                        Steinberg::Vst::TChar* string,
                                                        Steinberg::Vst::ParamValue& valueNormalized) SMTG_OVERRIDE;

 	//---Interface---------
	DEFINE_INTERFACES
		// Here you can add more supported VST3 interfaces
		// DEF_INTERFACE (Vst::IXXX)
	END_DEFINE_INTERFACES(EditController)
    DELEGATE_REFCOUNT(EditController)

protected:
};

//////////////////////////////////////////////////////////////////////////////
