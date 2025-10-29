//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/test/ringbuffertest.cpp
// Created by  : Steinberg, 03/2018
// Description : Test ringbuffer
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
#include "public.sdk/source/vst/utility/ringbuffer.h"
#include "public.sdk/source/vst/utility/testing.h"
#include "pluginterfaces/base/fstrdefs.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
static ModuleInitializer InitRingbufferTests ([] () {
	registerTest ("RingBuffer", STR ("push until full"), [] (ITestResult*) {
		OneReaderOneWriter::RingBuffer<uint32> rb (4);
		if (!rb.push (0))
			return false;
		if (!rb.push (1))
			return false;
		if (!rb.push (2))
			return false;
		if (!rb.push (3))
			return false;
		if (!rb.push (4))
			return true;
		return false;
	});
	registerTest ("RingBuffer", STR ("pop until empty"), [] (ITestResult*) {
		OneReaderOneWriter::RingBuffer<uint32> rb (4);
		if (!rb.push (0))
			return false;
		if (!rb.push (1))
			return false;
		if (!rb.push (2))
			return false;
		if (!rb.push (3))
			return false;

		uint32 value;
		if (!rb.pop (value) || value != 0)
			return false;
		if (!rb.pop (value) || value != 1)
			return false;
		if (!rb.pop (value) || value != 2)
			return false;
		if (!rb.pop (value) || value != 3)
			return false;
		if (!rb.pop (value))
			return true;

		return false;
	});
	registerTest ("RingBuffer", STR ("roundtrip"), [] (ITestResult*) {
		OneReaderOneWriter::RingBuffer<uint32> rb (2);
		uint32 value;

		for (auto i = 0u; i < rb.size () * 2; ++i)
		{
			if (!rb.push (i))
				return false;
			if (!rb.pop (value) || value != i)
				return false;
		}
		return true;
	});

	registerTest ("RingBuffer", STR ("push multiple"), [] (ITestResult*) {
		OneReaderOneWriter::RingBuffer<uint32> rb (3);

		if (!rb.push ({32u, 64u}))
			return false;
		if (rb.push ({32u, 64u}))
			return false;
		uint32 value;
		if (!rb.pop (value))
			return false;
		if (!rb.pop (value))
			return false;
		if (rb.pop (value))
			return false;

		return true;
	});

});

//------------------------------------------------------------------------
} // Vst
} // Steinberg
