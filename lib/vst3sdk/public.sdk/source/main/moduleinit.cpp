//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : Common Base Classes
// Filename    : public.sdk/source/main/moduleinit.cpp
// Created by  : Steinberg, 11/2020
// Description : Module Initializers/Terminators
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "moduleinit.h"
#include <atomic>
#include <vector>

extern void* moduleHandle; // from dllmain.cpp, linuxmain.cpp or macmain.cpp

//------------------------------------------------------------------------
namespace Steinberg {
namespace {

//------------------------------------------------------------------------
using FunctionVector = std::vector<std::pair<ModuleInitPriority, ModuleInitFunction>>;

//------------------------------------------------------------------------
FunctionVector& getInitFunctions ()
{
	static FunctionVector gInitVector;
	return gInitVector;
}

//------------------------------------------------------------------------
FunctionVector& getTermFunctions ()
{
	static FunctionVector gTermVector;
	return gTermVector;
}

//------------------------------------------------------------------------
void addInitFunction (ModuleInitFunction&& func, ModuleInitPriority prio)
{
	getInitFunctions ().emplace_back (prio, std::move (func));
}

//------------------------------------------------------------------------
void addTerminateFunction (ModuleInitFunction&& func, ModuleInitPriority prio)
{
	getTermFunctions ().emplace_back (prio, std::move (func));
}

//------------------------------------------------------------------------
void sortAndRunFunctions (FunctionVector& array)
{
	std::sort (array.begin (), array.end (),
	           [] (const FunctionVector::value_type& v1, const FunctionVector::value_type& v2) {
		           return v1.first < v2.first;
	           });
	for (auto& entry : array)
		entry.second ();
}

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
ModuleInitializer::ModuleInitializer (ModuleInitFunction&& func, ModuleInitPriority prio)
{
	addInitFunction (std::move (func), prio);
}

//------------------------------------------------------------------------
ModuleTerminator::ModuleTerminator (ModuleInitFunction&& func, ModuleInitPriority prio)
{
	addTerminateFunction (std::move (func), prio);
}

//------------------------------------------------------------------------
PlatformModuleHandle getPlatformModuleHandle ()
{
	return reinterpret_cast<PlatformModuleHandle> (moduleHandle);
}

//------------------------------------------------------------------------
} // Steinberg

//------------------------------------------------------------------------
bool InitModule ()
{
	Steinberg::sortAndRunFunctions (Steinberg::getInitFunctions ());
	return true;
}

//------------------------------------------------------------------------
bool DeinitModule ()
{
	Steinberg::sortAndRunFunctions (Steinberg::getTermFunctions ());
	return true;
}
