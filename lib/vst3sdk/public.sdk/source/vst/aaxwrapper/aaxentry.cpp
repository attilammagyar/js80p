//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/aaxwrapper/aaxentry.h
// Created by  : Steinberg, 08/2017
// Description : VST 3 -> AAX Wrapper
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

/// \cond ignore

/**
 *	plugin entry from AAX_Exports.cpp
 */
//-----------------------------------------------------------------------------

#include "pluginterfaces/base/fplatform.h"

// change names to avoid different linkage
#define ACFRegisterPlugin ACFRegisterPlugin_
#define ACFRegisterComponent ACFRegisterComponent_
#define ACFGetClassFactory ACFGetClassFactory_

#define ACFCanUnloadNow ACFCanUnloadNow_
#define ACFStartup ACFStartup_
#define ACFShutdown ACFShutdown_

//#define INITACFIDS // Make sure all of the AVX2 uids are defined.

#include "AAX.h"
#include "AAX_Init.h"
#include "acfresult.h"
#include "acfunknown.h"

#undef ACFRegisterPlugin
#undef ACFRegisterComponent
#undef ACFGetClassFactory

#undef ACFCanUnloadNow
#undef ACFStartup
#undef ACFShutdown

// defined in basewrapper.cpp
extern bool _InitModule ();
extern bool _DeinitModule ();

// reference this in the plugin to force inclusion of the wrapper in the link
int AAXWrapper_linkAnchor;

//------------------------------------------------------------------------
#if defined(__GNUC__)
#define AAX_EXPORT extern "C" __attribute__ ((visibility ("default"))) ACFRESULT
#else
#define AAX_EXPORT extern "C" __declspec (dllexport) ACFRESULT __stdcall
#endif

AAX_EXPORT ACFRegisterPlugin (IACFUnknown* pUnkHost, IACFPluginDefinition** ppPluginDefinition);
AAX_EXPORT ACFRegisterComponent (IACFUnknown* pUnkHost, acfUInt32 index,
                                 IACFComponentDefinition** ppComponentDefinition);
AAX_EXPORT ACFGetClassFactory (IACFUnknown* pUnkHost, const acfCLSID& clsid, const acfIID& iid,
                               void** ppOut);

AAX_EXPORT ACFCanUnloadNow (IACFUnknown* pUnkHost);
AAX_EXPORT ACFStartup (IACFUnknown* pUnkHost);
AAX_EXPORT ACFShutdown (IACFUnknown* pUnkHost);
AAX_EXPORT ACFGetSDKVersion (acfUInt64* oSDKVersion);

//------------------------------------------------------------------------
// \func ACFRegisterPlugin
// \brief Determines the number of components defined in the dll.
//
ACFAPI ACFRegisterPlugin (IACFUnknown* pUnkHostVoid, IACFPluginDefinition** ppPluginDefinitionVoid)
{
	ACFRESULT result = ACF_OK;

	try
	{
		result = AAXRegisterPlugin (pUnkHostVoid, ppPluginDefinitionVoid);
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

//------------------------------------------------------------------------
// \func ACFRegisterComponent
// \brief Registers a specific component in the DLL.
//
ACFAPI ACFRegisterComponent (IACFUnknown* pUnkHost, acfUInt32 index,
                             IACFComponentDefinition** ppComponentDefinition)
{
	ACFRESULT result = ACF_OK;

	try
	{
		result = AAXRegisterComponent (pUnkHost, index, ppComponentDefinition);
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

//------------------------------------------------------------------------
// \func ACFGetClassFactory
// \brief Gets the factory for a given class ID.
//
ACFAPI ACFGetClassFactory (IACFUnknown* pUnkHost, const acfCLSID& clsid, const acfIID& iid,
                           void** ppOut)
{
	ACFRESULT result = ACF_OK;

	try
	{
		result = AAXGetClassFactory (pUnkHost, clsid, iid, ppOut);
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

//------------------------------------------------------------------------
// \func ACFCanUnloadNow
// \brief Figures out if all objects are released so we can unload.
//
ACFAPI ACFCanUnloadNow (IACFUnknown* pUnkHost)
{
	ACFRESULT result = ACF_OK;

	try
	{
		result = AAXCanUnloadNow (pUnkHost);
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

//------------------------------------------------------------------------
// \func ACFStartup
// \brief Called once at init time.
//
ACFAPI ACFStartup (IACFUnknown* pUnkHost)
{
	ACFRESULT result = ACF_OK;

	try
	{
		result = AAXStartup (pUnkHost);
		if (result == ACF_OK)
		{
			if (!_InitModule ())
			{
				AAXShutdown (pUnkHost);
				result = ACF_E_UNEXPECTED;
			}
		}
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

//------------------------------------------------------------------------
// \func ACFShutdown
// \brief Called once at termination of dll.
//
ACFAPI ACFShutdown (IACFUnknown* pUnkHost)
{
	ACFRESULT result = ACF_OK;

	try
	{
		_DeinitModule ();
		result = AAXShutdown (pUnkHost);
	}
	catch (...)
	{
		result = ACF_E_UNEXPECTED;
	}

	return result;
}

//------------------------------------------------------------------------
ACFAPI ACFGetSDKVersion (acfUInt64* oSDKVersion)
{
	return AAXGetSDKVersion (oSDKVersion);
}

/// \endcond
