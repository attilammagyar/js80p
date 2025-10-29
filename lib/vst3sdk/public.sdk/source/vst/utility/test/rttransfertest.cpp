//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/test/rtstatetransfertest.cpp
// Created by  : Steinberg, 04/2021
// Description : Realtime State Transfer
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "public.sdk/source/main/moduleinit.h"
#include "public.sdk/source/vst/utility/rttransfer.h"
#include "public.sdk/source/vst/utility/testing.h"
#include "pluginterfaces/vst/vsttypes.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace {
//------------------------------------------------------------------------
using ParameterVector = std::vector<std::pair<ParamID, ParamValue>>;
using RTTransfer = RTTransferT<ParameterVector>;

//------------------------------------------------------------------------
struct RaceConditionTestObject
{
	static std::atomic<uint32> numDeletes;
//------------------------------------------------------------------------
	struct MyDeleter
	{
		void operator () (double* v) const noexcept
		{
			delete v;
			++numDeletes;
		}
	};

	RTTransferT<double, MyDeleter> transfer;
	std::thread thread;
	std::mutex m1;
	std::mutex m2;
	std::condition_variable c1;

	bool test (ITestResult* result)
	{
		numDeletes = 0;
		{
			auto obj1 = std::unique_ptr<double, MyDeleter> (new double (0.5));
			auto obj2 = std::unique_ptr<double, MyDeleter> (new double (1.));
			transfer.transferObject_ui (std::move (obj1));
			m2.lock ();
			thread = std::thread ([&] () {
				transfer.accessTransferObject_rt ([&] (const double&) {
					c1.notify_all ();
					m2.lock ();
					m2.unlock ();
				});
				transfer.accessTransferObject_rt ([&] (const double&) {});
			});
			std::unique_lock<std::mutex> lm1 (m1);
			c1.wait (lm1);
			transfer.transferObject_ui (std::move (obj2));
			m2.unlock ();

			thread.join ();
			transfer.clear_ui ();
		}
		return numDeletes == 2;
	}
};
std::atomic<uint32> RaceConditionTestObject::numDeletes {0};

//------------------------------------------------------------------------
static std::atomic<uint32> CustomDeleterCallCount;
struct CustomDeleter
{
	template <typename T>
	void operator () (T* v) const noexcept
	{
		delete v;
		++CustomDeleterCallCount;
	}
};

//------------------------------------------------------------------------
ModuleInitializer InitStateTransferTests ([] () {
	registerTest ("RTTransfer", STR ("Simple Transfer"), [] (ITestResult*) {
		RTTransfer helper;
		auto list = std::make_unique<ParameterVector> ();
		list->emplace_back (std::make_pair (0, 1.));
		helper.transferObject_ui (std::move (list));
		bool success = false;
		constexpr double one = 1.;
		helper.accessTransferObject_rt ([&success, one = one] (const auto& list) {
			if (list.size () == 1)
			{
				if (list[0].first == 0)
				{
					if (Test::equal (one, list[0].second))
					{
						success = true;
					}
				}
			}
		});

		list = std::make_unique<ParameterVector> ();
		list->emplace_back (std::make_pair (0, 1.));
		helper.transferObject_ui (std::move (list));
		helper.accessTransferObject_rt ([] (auto&) {});
		list = std::make_unique<ParameterVector> ();
		list->emplace_back (std::make_pair (0, 1.));
		helper.transferObject_ui (std::move (list));
		helper.accessTransferObject_rt ([] (auto&) {});
		return success;
	});
	registerTest ("RTTransfer", STR ("CheckRaceCondition"), [] (ITestResult* r) {
		RaceConditionTestObject obj;
		return obj.test (r);
	});
	registerTest ("RTTransfer", STR ("Custom Deleter"), [] (ITestResult* result) {
		CustomDeleterCallCount = 0;
		RTTransferT<double, CustomDeleter> transfer;
		auto obj1 = std::unique_ptr<double, CustomDeleter> (new double (1.));
		transfer.transferObject_ui (std::move (obj1));
		if (CustomDeleterCallCount != 0)
		{
			return false;
		}
		transfer.clear_ui ();
		return CustomDeleterCallCount == 1;
	});
});

//------------------------------------------------------------------------
} // Anonymous
} // Vst
} // Steinberg
