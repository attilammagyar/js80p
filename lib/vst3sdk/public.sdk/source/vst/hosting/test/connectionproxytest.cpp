//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/test/connectionproxytest.cpp
// Created by  : Steinberg, 08/2021
// Description : Test connection proxy
// Flags       : clang-format SMTGSequencer
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "public.sdk/source/main/moduleinit.h"
#include "public.sdk/source/vst/hosting/connectionproxy.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/utility/testing.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace {

//------------------------------------------------------------------------
class ConnectionPoint : public IConnectionPoint
{
public:
	tresult PLUGIN_API connect (IConnectionPoint* inOther) override
	{
		other = inOther;
		return kResultTrue;
	}

	tresult PLUGIN_API disconnect (IConnectionPoint* inOther) override
	{
		if (inOther != other)
			return kResultFalse;
		return kResultTrue;
	}

	tresult PLUGIN_API notify (IMessage*) override
	{
		messageReceived = true;
		return kResultTrue;
	}

	tresult PLUGIN_API queryInterface (const TUID, void**) override { return kNotImplemented; }
	uint32 PLUGIN_API addRef () override { return 100; }
	uint32 PLUGIN_API release () override { return 100; }

	IConnectionPoint* other {nullptr};
	bool messageReceived {false};
};

//------------------------------------------------------------------------
ModuleInitializer ConnectionProxyTests ([] () {
	constexpr auto TestSuiteName = "ConnectionProxy";
	registerTest (TestSuiteName, STR ("Connect and disconnect"), [] (ITestResult* testResult) {
		ConnectionPoint cp1;
		ConnectionPoint cp2;
		ConnectionProxy proxy (&cp1);
		EXPECT_EQ (proxy.connect (&cp2), kResultTrue);
		EXPECT_EQ (proxy.disconnect (&cp2), kResultTrue);
		return true;
	});
	registerTest (TestSuiteName, STR ("Disconnect wrong object"), [] (ITestResult* testResult) {
		ConnectionPoint cp1;
		ConnectionPoint cp2;
		ConnectionPoint cp3;
		ConnectionProxy proxy (&cp1);
		EXPECT_EQ (proxy.connect (&cp2), kResultTrue);
		EXPECT_NE (proxy.disconnect (&cp3), kResultTrue);
		return true;
	});
	registerTest (TestSuiteName, STR ("Send message on UI thread"), [] (ITestResult* testResult) {
		ConnectionPoint cp1;
		ConnectionPoint cp2;
		ConnectionProxy proxy (&cp1);
		EXPECT_EQ (proxy.connect (&cp2), kResultTrue);
		EXPECT_FALSE (cp2.messageReceived);
		HostMessage msg;
		EXPECT_EQ (proxy.notify (&msg), kResultTrue);
		EXPECT_TRUE (cp2.messageReceived);
		return true;
	});
	registerTest (TestSuiteName, STR ("Send message on 2nd thread"), [] (ITestResult* testResult) {
		ConnectionPoint cp1;
		ConnectionPoint cp2;
		ConnectionProxy proxy (&cp1);
		EXPECT_EQ (proxy.connect (&cp2), kResultTrue);
		EXPECT_FALSE (cp2.messageReceived);

		std::condition_variable cv;
		std::mutex m;
		std::optional<tresult> notifyResult;

		std::thread thread ([&] () {
			HostMessage msg;
			{
				const std::scoped_lock sl (m);
				notifyResult = proxy.notify (&msg);
			}
			cv.notify_one ();
		});

		std::unique_lock ul (m);
		cv.wait (ul, [&] { return notifyResult.has_value (); });
		EXPECT_NE (*notifyResult, kResultTrue);
		EXPECT_FALSE (cp2.messageReceived);
		thread.join ();
		return true;
	});
});

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
} // Vst
} // Steinberg