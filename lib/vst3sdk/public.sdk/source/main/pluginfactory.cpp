//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : Common Base Classes
// Filename    : public.sdk/source/main/pluginfactory.cpp
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

#include "pluginfactory.h"

#if SMTG_OS_LINUX
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/gui/iplugview.h"
#include "base/source/timer.h"
#endif

#include <algorithm>
#include <cstdlib>

namespace Steinberg {

DEF_CLASS_IID (IPluginFactoryInternal);

CPluginFactory* gPluginFactory = nullptr;

//------------------------------------------------------------------------
//  CPluginFactory implementation
//------------------------------------------------------------------------
CPluginFactory::CPluginFactory (const PFactoryInfo& info)
: classes (nullptr), classCount (0), maxClassCount (0)
{
	FUNKNOWN_CTOR

	factoryInfo = info;
}

//------------------------------------------------------------------------
CPluginFactory::~CPluginFactory ()
{
	if (gPluginFactory == this)
		gPluginFactory = nullptr;

	if (classes)
		free (classes);

	FUNKNOWN_DTOR
}

//------------------------------------------------------------------------
IMPLEMENT_REFCOUNT (CPluginFactory)

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::queryInterface (FIDString _iid, void** obj)
{
	QUERY_INTERFACE (_iid, obj, IPluginFactory::iid, IPluginFactory)
	QUERY_INTERFACE (_iid, obj, IPluginFactory2::iid, IPluginFactory2)
	QUERY_INTERFACE (_iid, obj, IPluginFactory3::iid, IPluginFactory3)
	QUERY_INTERFACE (_iid, obj, IPluginFactoryInternal::iid, IPluginFactoryInternal)
	QUERY_INTERFACE (_iid, obj, FUnknown::iid, IPluginFactory)
	*obj = nullptr;
	return kNoInterface;
}

//------------------------------------------------------------------------
bool CPluginFactory::registerClass (const PClassInfo* info, FUnknown* (*createFunc) (void*),
                                    void* context)
{
	if (!info || !createFunc)
		return false;

	PClassInfo2 info2;
	memcpy (&info2, info, sizeof (PClassInfo));
	return registerClass (&info2, createFunc, context);
}

//------------------------------------------------------------------------
bool CPluginFactory::registerClass (const PClassInfo2* info, FUnknown* (*createFunc) (void*),
                                    void* context)
{
	if (!info || !createFunc)
		return false;

	if (classCount >= maxClassCount)
	{
		if (!growClasses ())
			return false;
	}

	PClassEntry& entry = classes[classCount];
	entry.info8 = *info;
	entry.info16.fromAscii (*info);
	entry.createFunc = createFunc;
	entry.context = context;
	entry.isUnicode = false;

	classCount++;
	return true;
}

//------------------------------------------------------------------------
bool CPluginFactory::registerClass (const PClassInfoW* info, FUnknown* (*createFunc) (void*),
                                    void* context)
{
	if (!info || !createFunc)
		return false;

	if (classCount >= maxClassCount)
	{
		if (!growClasses ())
			return false;
	}

	PClassEntry& entry = classes[classCount];
	entry.info16 = *info;
	entry.createFunc = createFunc;
	entry.context = context;
	entry.isUnicode = true;

	classCount++;
	return true;
}

//------------------------------------------------------------------------
bool CPluginFactory::growClasses ()
{
	static const int32 delta = 10;

	size_t size = (maxClassCount + delta) * sizeof (PClassEntry);
	void* memory = classes;

	if (!memory)
		memory = malloc (size);
	else
		memory = realloc (memory, size);

	if (!memory)
		return false;

	classes = static_cast<PClassEntry*> (memory);
	maxClassCount += delta;
	return true;
}

//------------------------------------------------------------------------
bool CPluginFactory::isClassRegistered (const FUID& cid)
{
	for (int32 i = 0; i < classCount; i++)
	{
		if (FUnknownPrivate::iidEqual (cid, classes[i].info16.cid))
			return true;
	}
	return false;
}

//------------------------------------------------------------------------
void CPluginFactory::removeAllClasses ()
{
	classCount = 0;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::getFactoryInfo (PFactoryInfo* info)
{
	if (info)
		memcpy (info, &factoryInfo, sizeof (PFactoryInfo));
	return kResultOk;
}

//------------------------------------------------------------------------
int32 PLUGIN_API CPluginFactory::countClasses ()
{
	return classCount;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::getClassInfo (int32 index, PClassInfo* info)
{
	if (info && (index >= 0 && index < classCount))
	{
		if (classes[index].isUnicode)
		{
			memset (info, 0, sizeof (PClassInfo));
			return kResultFalse;
		}

		memcpy (info, &classes[index].info8, sizeof (PClassInfo));
		return kResultOk;
	}
	return kInvalidArgument;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::getClassInfo2 (int32 index, PClassInfo2* info)
{
	if (info && (index >= 0 && index < classCount))
	{
		if (classes[index].isUnicode)
		{
			memset (info, 0, sizeof (PClassInfo2));
			return kResultFalse;
		}

		memcpy (info, &classes[index].info8, sizeof (PClassInfo2));
		return kResultOk;
	}
	return kInvalidArgument;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::getClassInfoUnicode (int32 index, PClassInfoW* info)
{
	if (info && (index >= 0 && index < classCount))
	{
		memcpy (info, &classes[index].info16, sizeof (PClassInfoW));
		return kResultOk;
	}
	return kInvalidArgument;
}

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::createInstance (FIDString cid, FIDString _iid, void** obj)
{
	for (int32 i = 0; i < classCount; i++)
	{
		if (memcmp (classes[i].info16.cid, cid, sizeof (TUID)) == 0)
		{
			FUnknown* instance = classes[i].createFunc (classes[i].context);
			if (instance)
			{
				if (instance->queryInterface (_iid, obj) == kResultOk)
				{
					instance->release ();
					return kResultOk;
				}
				instance->release ();
			}
			break;
		}
	}

	*obj = nullptr;
	return kNoInterface;
}

#if SMTG_OS_LINUX
//------------------------------------------------------------------------
namespace /*anonymous*/ {

//------------------------------------------------------------------------
class LinuxPlatformTimer : public U::Extends<Timer, U::Directly<Linux::ITimerHandler>>
{
public:
	~LinuxPlatformTimer () noexcept override { stop (); }

	tresult init (ITimerCallback* cb, uint32 timeout)
	{
		if (!runLoop || cb == nullptr || timeout == 0)
			return kResultFalse;
		auto result = runLoop->registerTimer (this, timeout);
		if (result == kResultTrue)
		{
			callback = cb;
			timerRegistered = true;
		}
		return result;
	}

	void PLUGIN_API onTimer () override { callback->onTimer (this); }

	void stop () override
	{
		if (timerRegistered)
		{
			if (runLoop)
				runLoop->unregisterTimer (this);
			timerRegistered = false;
		}
	}

	bool timerRegistered {false};
	ITimerCallback* callback;

	static IPtr<Linux::IRunLoop> runLoop;
};
IPtr<Linux::IRunLoop> LinuxPlatformTimer::runLoop;

//------------------------------------------------------------------------
Timer* createLinuxTimer (ITimerCallback* cb, uint32 milliseconds)
{
	if (!LinuxPlatformTimer::runLoop)
		return nullptr;
	auto timer = NEW LinuxPlatformTimer;
	if (timer->init (cb, milliseconds) == kResultTrue)
		return timer;
	timer->release ();
	return nullptr;
}

} // anonymous
#endif // SMTG_OS_LINUX

//------------------------------------------------------------------------
tresult PLUGIN_API CPluginFactory::setHostContext (FUnknown* context)
{
	std::for_each (hostContextCallbacks.begin (), hostContextCallbacks.end (),
	               [context] (const auto& cb) { cb (context); });
#if SMTG_OS_LINUX
	if (auto runLoop = U::cast<Linux::IRunLoop> (context))
	{
		LinuxPlatformTimer::runLoop = runLoop;
		InjectCreateTimerFunction (createLinuxTimer);
	}
	else
	{
		LinuxPlatformTimer::runLoop.reset ();
		InjectCreateTimerFunction (nullptr);
	}
	return kResultTrue;
#else
	(void) context;
	return kNotImplemented;
#endif
}

//------------------------------------------------------------------------
void PLUGIN_API CPluginFactory::addHostContextCallback (HostContextCallbackFunc func)
{
	hostContextCallbacks.push_back (func);
}

//------------------------------------------------------------------------
} // namespace Steinberg
