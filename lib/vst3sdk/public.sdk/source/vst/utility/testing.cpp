//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/source/vst/utility/testing.cpp
// Created by  : Steinberg, 04/2021
// Description : Utility classes for custom testing in the vst validator
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "public.sdk/source/vst/utility/testing.h"
#include <atomic>
#include <cassert>
#include <string>
#include <vector>

//------------------------------------------------------------------------
namespace Steinberg {

DEF_CLASS_IID (ITest)
DEF_CLASS_IID (ITestSuite)
DEF_CLASS_IID (ITestFactory)

//------------------------------------------------------------------------
namespace Vst {
namespace {

//------------------------------------------------------------------------
struct TestRegistry
{
	struct TestWithContext
	{
		std::u16string desc;
		TestFuncWithContext func;
	};
	using Tests = std::vector<std::pair<std::string, IPtr<ITest>>>;
	using TestsWithContext = std::vector<std::pair<std::string, TestWithContext>>;

	static TestRegistry& instance ()
	{
		static TestRegistry gInstance;
		return gInstance;
	}

	Tests tests;
	TestsWithContext testsWithContext;
};

//------------------------------------------------------------------------
struct TestBase : ITest
{
	TestBase (const tchar* inDesc)
	{
		if (inDesc)
			desc = reinterpret_cast<const std::u16string::value_type*> (inDesc);
	}
	TestBase (const std::u16string& inDesc) : desc (inDesc) {}

	virtual ~TestBase () = default;

	bool PLUGIN_API setup () override { return true; }
	bool PLUGIN_API teardown () override { return true; }
	const tchar* PLUGIN_API getDescription () override
	{
		return reinterpret_cast<const tchar*> (desc.data ());
	}

	tresult PLUGIN_API queryInterface (const TUID _iid, void** obj) override
	{
		QUERY_INTERFACE (_iid, obj, FUnknown::iid, FUnknown)
		QUERY_INTERFACE (_iid, obj, ITest::iid, ITest)
		*obj = nullptr;
		return kNoInterface;
	}
	uint32 PLUGIN_API addRef () override { return ++refCount; }
	uint32 PLUGIN_API release () override
	{
		if (--refCount == 0)
		{
			delete this;
			return 0;
		}
		return refCount;
	}

	std::atomic<uint32> refCount {1};
	std::u16string desc;
};

//------------------------------------------------------------------------
struct FuncTest : TestBase
{
	FuncTest (const tchar* desc, const TestFunc& func) : TestBase (desc), func (func) {}
	FuncTest (const tchar* desc, TestFunc&& func) : TestBase (desc), func (std::move (func)) {}

	bool PLUGIN_API run (ITestResult* testResult) override { return func (testResult); }

	TestFunc func;
};

//------------------------------------------------------------------------
struct FuncWithContextTest : TestBase
{
	FuncWithContextTest (FUnknown* context, const std::u16string& desc,
	                     const TestFuncWithContext& func)
	: TestBase (desc), func (func), context (context)
	{
	}

	bool PLUGIN_API run (ITestResult* testResult) override { return func (context, testResult); }

	TestFuncWithContext func;
	FUnknown* context;
};

//------------------------------------------------------------------------
struct TestFactoryImpl : ITestFactory
{
	TestFactoryImpl () = default;
	virtual ~TestFactoryImpl () = default;

	tresult PLUGIN_API createTests (FUnknown* context, ITestSuite* parentSuite) override
	{
		for (auto& t : TestRegistry::instance ().tests)
		{
			t.second->addRef ();
			parentSuite->addTest (t.first.data (), t.second);
		}
		for (auto& t : TestRegistry::instance ().testsWithContext)
			parentSuite->addTest (t.first.data (),
			                      new FuncWithContextTest (context, t.second.desc, t.second.func));
		return kResultTrue;
	}
	tresult PLUGIN_API queryInterface (const TUID _iid, void** obj) override
	{
		QUERY_INTERFACE (_iid, obj, FUnknown::iid, FUnknown)
		QUERY_INTERFACE (_iid, obj, ITestFactory::iid, ITestFactory)
		*obj = nullptr;
		return kNoInterface;
	}

	uint32 PLUGIN_API addRef () override { return ++refCount; }

	uint32 PLUGIN_API release () override
	{
		if (--refCount == 0)
		{
			delete this;
			return 0;
		}
		return refCount;
	}

private:
	std::atomic<uint32> refCount {1};
};

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
void registerTest (FIDString name, const tchar* desc, const TestFunc& func)
{
	registerTest (name, new FuncTest (desc, func));
}

//------------------------------------------------------------------------
void registerTest (FIDString name, const tchar* desc, TestFunc&& func)
{
	registerTest (name, new FuncTest (desc, std::move (func)));
}

//------------------------------------------------------------------------
void registerTest (FIDString name, ITest* test)
{
	assert (name != nullptr);
	TestRegistry::instance ().tests.push_back (std::make_pair (name, owned (test)));
}

//------------------------------------------------------------------------
void registerTest (FIDString name, const tchar* desc, const TestFuncWithContext& func)
{
	std::u16string descStr;
	if (desc)
		descStr = reinterpret_cast<const std::u16string::value_type*> (desc);
	TestRegistry::instance ().testsWithContext.push_back (
	    std::make_pair (name, TestRegistry::TestWithContext {descStr, func}));
}

//------------------------------------------------------------------------
void registerTest (FIDString name, const tchar* desc, TestFuncWithContext&& func)
{
	std::u16string descStr;
	if (desc)
		descStr = reinterpret_cast<const std::u16string::value_type*> (desc);
	TestRegistry::instance ().testsWithContext.push_back (
	    std::make_pair (name, TestRegistry::TestWithContext {descStr, std::move (func)}));
}

//------------------------------------------------------------------------
FUnknown* createTestFactoryInstance (void*)
{
	return new TestFactoryImpl;
}

//------------------------------------------------------------------------
const FUID& getTestFactoryUID ()
{
	static FUID uid = FUID::fromTUID (TestFactoryUID);
	return uid;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
