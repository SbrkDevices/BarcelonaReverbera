#include "barcelona_reverbera_controller.h"
#include "barcelona_reverbera_cids.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"

//////////////////////////////////////////////////////////////////////////////

using namespace Steinberg;

//////////////////////////////////////////////////////////////////////////////

tresult PLUGIN_API BarcelonaReverberaController::initialize(FUnknown* context)
{
	tresult result = EditControllerEx1::initialize(context);
	if (result != kResultOk)
		return result;
	
	setKnobMode(Vst::kLinearMode);

	if (result == kResultTrue)
	{
		parameters.addParameter(STR16("Bypass"), nullptr, 1, 0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
		                        BarcelonaReverberaParams::kBypassId);

		parameters.addParameter(STR16("Size"), STR16("%"), 0, .5, Vst::ParameterInfo::kCanAutomate,
								BarcelonaReverberaParams::kParamSizeId, 0, STR16("Size"));

		parameters.addParameter(STR16("Color"), STR16("%"), 0, .5, Vst::ParameterInfo::kCanAutomate,
								BarcelonaReverberaParams::kParamColorId, 0, STR16("Color"));

		parameters.addParameter(STR16("Dry/Wet"), STR16("%"), 0, .5, Vst::ParameterInfo::kCanAutomate,
								BarcelonaReverberaParams::kParamDryWetId, 0, STR16("Dry/Wet"));
		
		auto* selectedIRParam = new Vst::StringListParameter(USTRING("Selected IR"), BarcelonaReverberaParams::kParamSelectedIR);

		extern char m_strIRNames[CONVRVRB_IR_COUNT][128];
		for (int i=0; i<CONVRVRB_IR_COUNT; i++)
			selectedIRParam->appendString(USTRING(m_strIRNames[i]));

		parameters.addParameter(selectedIRParam);
	}

	return result;
}

tresult PLUGIN_API BarcelonaReverberaController::terminate()
{
	return EditControllerEx1::terminate();
}

tresult PLUGIN_API BarcelonaReverberaController::setComponentState(IBStream* state)
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
	
	int8 savedSelectedIR = 0;
	if (streamer.readInt8(savedSelectedIR) == false)
		return kResultFalse;

	int32 bypassState;
	if (streamer.readInt32(bypassState) == false)
		return kResultFalse;

	setParamNormalized(BarcelonaReverberaParams::kParamSizeId, savedParamSize);
	setParamNormalized(BarcelonaReverberaParams::kParamColorId, savedParamColor);
	setParamNormalized(BarcelonaReverberaParams::kParamDryWetId, savedParamDryWet);
	setParamNormalized(BarcelonaReverberaParams::kParamSelectedIR, plainParamToNormalized(BarcelonaReverberaParams::kParamSelectedIR, savedSelectedIR));
	setParamNormalized(kBypassId, bypassState ? 1 : 0);

	return kResultOk;
}

tresult PLUGIN_API BarcelonaReverberaController::setState(IBStream* state)
{
	return kResultTrue;
}

tresult PLUGIN_API BarcelonaReverberaController::getState(IBStream* state)
{
	return kResultTrue;
}

IPlugView* PLUGIN_API BarcelonaReverberaController::createView(FIDString name)
{
	if (FIDStringsEqual(name, Vst::ViewType::kEditor))
	{
		// create your editor here and return a IPlugView ptr of it
		auto* view = new VSTGUI::VST3Editor(this, "view", "barcelona_reverbera_editor.uidesc");
		return view;
	}
	return nullptr;
}

tresult PLUGIN_API BarcelonaReverberaController::setParamNormalized(Vst::ParamID tag, Vst::ParamValue value)
{
	tresult result = EditControllerEx1::setParamNormalized(tag, value);
	return result;
}

tresult PLUGIN_API BarcelonaReverberaController::getParamStringByValue(Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
	return EditControllerEx1::getParamStringByValue(tag, valueNormalized, string);
}

tresult PLUGIN_API BarcelonaReverberaController::getParamValueByString(Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
	return EditControllerEx1::getParamValueByString(tag, string, valueNormalized);
}

//////////////////////////////////////////////////////////////////////////////