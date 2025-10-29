//------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : Common Base Classes
// Filename    : public.sdk/source/main/pluginfactory.h
// Created by  : Steinberg, 01/2004
// Description : Standard Plug-In Factory
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/ipluginbase.h"
#include <vector>

namespace Steinberg {

//------------------------------------------------------------------------
class IPluginFactoryInternal : public FUnknown
{
public:
	using HostContextCallbackFunc = void (*) (FUnknown*);
	virtual void PLUGIN_API addHostContextCallback (HostContextCallbackFunc func) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};
DECLARE_CLASS_IID (IPluginFactoryInternal, 0x5A6AD11A, 0x22AF40F3, 0xBCA1C147, 0x506C88D9)

//------------------------------------------------------------------------
/** Default Class Factory implementation.
\ingroup sdkBase
\see classFactoryMacros 
*/
class CPluginFactory : public IPluginFactory3, public IPluginFactoryInternal
{
public:
//------------------------------------------------------------------------
	CPluginFactory (const PFactoryInfo& info);
	virtual ~CPluginFactory ();

	//--- ---------------------------------------------------------------------
	/** Registers a plug-in class with classInfo version 1, returns true for success. */
	bool registerClass (const PClassInfo* info, FUnknown* (*createFunc) (void*),
	                    void* context = nullptr);

	/** Registers a plug-in class with classInfo version 2, returns true for success. */
	bool registerClass (const PClassInfo2* info, FUnknown* (*createFunc) (void*),
	                    void* context = nullptr);

	/** Registers a plug-in class with classInfo Unicode version, returns true for success. */
	bool registerClass (const PClassInfoW* info, FUnknown* (*createFunc) (void*),
	                    void* context = nullptr);

	/** Check if a class for a given classId is already registered. */
	bool isClassRegistered (const FUID& cid);

	/** Remove all classes (no class exported) */
	void removeAllClasses ();

//------------------------------------------------------------------------
	DECLARE_FUNKNOWN_METHODS

	//---from IPluginFactory------
	tresult PLUGIN_API getFactoryInfo (PFactoryInfo* info) SMTG_OVERRIDE;
	int32 PLUGIN_API countClasses () SMTG_OVERRIDE;
	tresult PLUGIN_API getClassInfo (int32 index, PClassInfo* info) SMTG_OVERRIDE;
	tresult PLUGIN_API createInstance (FIDString cid, FIDString _iid, void** obj) SMTG_OVERRIDE;

	//---from IPluginFactory2-----
	tresult PLUGIN_API getClassInfo2 (int32 index, PClassInfo2* info) SMTG_OVERRIDE;

	//---from IPluginFactory3-----
	tresult PLUGIN_API getClassInfoUnicode (int32 index, PClassInfoW* info) SMTG_OVERRIDE;
	tresult PLUGIN_API setHostContext (FUnknown* context) SMTG_OVERRIDE;

	//---from IPluginFactoryInternal
	void PLUGIN_API addHostContextCallback (HostContextCallbackFunc func) SMTG_OVERRIDE;

//------------------------------------------------------------------------
protected:
	/// @cond
	struct PClassEntry
	{
	//-----------------------------------
		PClassInfo2 info8;
		PClassInfoW info16;

		FUnknown* (*createFunc) (void*);
		void* context;
		bool isUnicode;
	//-----------------------------------
	};
	/// @endcond

	PFactoryInfo factoryInfo;
	PClassEntry* classes;
	int32 classCount;
	int32 maxClassCount;

	std::vector<HostContextCallbackFunc> hostContextCallbacks;

	bool growClasses ();
};

extern CPluginFactory* gPluginFactory;
//------------------------------------------------------------------------
} // namespace Steinberg

//------------------------------------------------------------------------
/** \defgroup classFactoryMacros Macros for defining the class factory
\ingroup sdkBase

\b Example - How to use the class factory macros:
\code
BEGIN_FACTORY ("Steinberg Technologies", 
			   "http://www.steinberg.de", 
			   "mailto:info@steinberg.de", 
			   PFactoryInfo::kNoFlags)

DEF_CLASS (INLINE_UID (0x00000000, 0x00000000, 0x00000000, 0x00000000),
			PClassInfo::kManyInstances,    
			"Service",
			"Test Service",
			TestService::newInstance)
END_FACTORY
\endcode 

@{*/

#define BEGIN_FACTORY_CLASS(FactoryClass,vendor,url,email,flags) using namespace Steinberg; \
	SMTG_EXPORT_SYMBOL IPluginFactory* PLUGIN_API GetPluginFactory () { \
	if (!gPluginFactory) \
	{	static PFactoryInfo factoryInfo (vendor,url,email,flags); \
	gPluginFactory = new FactoryClass (factoryInfo); \

#define BEGIN_FACTORY(vendor,url,email,flags) using namespace Steinberg; \
	SMTG_EXPORT_SYMBOL IPluginFactory* PLUGIN_API GetPluginFactory () { \
	if (!gPluginFactory) \
	{	static PFactoryInfo factoryInfo (vendor,url,email,flags); \
		gPluginFactory = new CPluginFactory (factoryInfo); \

#define DEF_CLASS(cid,cardinality,category,name,createMethod) \
	{ TUID lcid = cid; static PClassInfo componentClass (lcid,cardinality,category,name); \
	gPluginFactory->registerClass (&componentClass,createMethod); }

#define DEF_CLASS1(cid,cardinality,category,name,createMethod) \
	{ static PClassInfo componentClass (cid,cardinality,category,name); \
	gPluginFactory->registerClass (&componentClass,createMethod); }

#define DEF_CLASS2(cid,cardinality,category,name,classFlags,subCategories,version,sdkVersion,createMethod) \
	{ TUID lcid = cid; static PClassInfo2 componentClass (lcid,cardinality,category,name,classFlags,subCategories,nullptr,version,sdkVersion);\
	gPluginFactory->registerClass (&componentClass,createMethod); }

#define DEF_CLASS_W(cid,cardinality,category,name,classFlags,subCategories,version,sdkVersion,createMethod) \
	{ TUID lcid = cid; static PClassInfoW componentClass (lcid,cardinality,category,name,classFlags,subCategories,nullptr,version,sdkVersion);\
	gPluginFactory->registerClass (&componentClass,createMethod); }

#define DEF_CLASS_W2(cid,cardinality,category,name,classFlags,subCategories,vendor,version,sdkVersion,createMethod) \
	{ TUID lcid = cid; static PClassInfoW componentClass (lcid,cardinality,category,name,classFlags,subCategories,vendor,version,sdkVersion);\
	gPluginFactory->registerClass (&componentClass,createMethod); }

#define END_FACTORY	} else gPluginFactory->addRef (); \
	return gPluginFactory; }

#define DEF_VST3_CLASS(pluginName, pluginVst3Categories, classFlags, pluginVersion, processorCID, \
                       processorCreateFunc, controllerCID, controllerCreateFunc)                  \
	{                                                                                             \
		{                                                                                         \
			const Steinberg::TUID lcid = processorCID;                                            \
			static Steinberg::PClassInfo2 processorClass (                                        \
			    lcid, Steinberg::PClassInfo::kManyInstances, kVstAudioEffectClass, pluginName,    \
			    classFlags, pluginVst3Categories, 0, pluginVersion, kVstVersionString);           \
			gPluginFactory->registerClass (&processorClass, processorCreateFunc);                 \
		}                                                                                         \
		{                                                                                         \
			const Steinberg::TUID lcid = controllerCID;                                           \
			static Steinberg::PClassInfo2 controllerClass (                                       \
			    lcid, Steinberg::PClassInfo::kManyInstances, kVstComponentControllerClass,        \
			    pluginName, 0, "", 0, pluginVersion, kVstVersionString);                          \
			gPluginFactory->registerClass (&controllerClass, controllerCreateFunc);               \
		}                                                                                         \
	}

/** @} */
