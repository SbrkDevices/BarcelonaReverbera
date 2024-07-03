#include "barcelona_reverbera_processor.h"
#include "barcelona_reverbera_controller.h"
#include "barcelona_reverbera_cids.h"
#include "barcelona_reverbera_version.h"

#include "public.sdk/source/main/pluginfactory.h"

//////////////////////////////////////////////////////////////////////////////

#define STRING_PLUGIN_NAME 		"Barcelona Reverbera"

//////////////////////////////////////////////////////////////////////////////

using namespace Steinberg;

//////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------
//  VST Plug-in Entry
//------------------------------------------------------------------------
// Windows: do not forget to include a .def file in your project to export
// GetPluginFactory function!
//------------------------------------------------------------------------

BEGIN_FACTORY_DEF("sbrk devices", 
			      "-", 
			      "mailto:dani@sbrkdevices.com")

	//---First Plug-in included in this factory-------
	// its kVstAudioEffectClass component
	DEF_CLASS2(INLINE_UID_FROM_FUID(kBarcelonaReverberaProcessorUID),
			   PClassInfo::kManyInstances,	// cardinality
			   kVstAudioEffectClass,	// the component category (do not changed this)
			   STRING_PLUGIN_NAME,		// here the Plug-in name (to be changed)
			   Vst::kDistributable,	// means that component and controller could be distributed on different computers
			   BarcelonaReverberaVST3Category, // Subcategory for this Plug-in (to be changed)
			   FULL_VERSION_STR,		// Plug-in version (to be changed)
			   kVstVersionString,		// the VST 3 SDK version (do not changed this, use always this define)
			   BarcelonaReverberaProcessor::createInstance)	// function pointer called when this component should be instantiated

	// its kVstComponentControllerClass component
	DEF_CLASS2(INLINE_UID_FROM_FUID (kBarcelonaReverberaControllerUID),
			   PClassInfo::kManyInstances, // cardinality
			   kVstComponentControllerClass,// the Controller category (do not changed this)
			   STRING_PLUGIN_NAME "Controller",	// controller name (could be the same than component name)
			   0,						// not used here
			   "",						// not used here
			   FULL_VERSION_STR,		// Plug-in version (to be changed)
			   kVstVersionString,		// the VST 3 SDK version (do not changed this, use always this define)
			   BarcelonaReverberaController::createInstance)// function pointer called when this component should be instantiated

	//----for others Plug-ins contained in this factory, put like for the first Plug-in different DEF_CLASS2---

END_FACTORY

//////////////////////////////////////////////////////////////////////////////
