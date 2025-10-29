//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST3 SDK
// Filename    : public.sdk/source/vst/utility/versionparser.h
// Created by  : Steinberg, 04/2018
// Description : version parser helper
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/utility/optional.h"
#include <algorithm>
#include <array>
#include <cctype>

#if __cplusplus >= 201703L
#include <string_view>
#define SMTG_VERSIONPARSER_USE_STRINGVIEW
#else
#include <string>
#endif

//------------------------------------------------------------------------
namespace VST3 {

//------------------------------------------------------------------------
struct Version
{
private:
	enum
	{
		Major,
		Minor,
		Sub,
		BuildNumber
	};

public:
#ifdef SMTG_VERSIONPARSER_USE_STRINGVIEW
	using StringType = std::string_view;
#else
	using StringType = std::string;
#endif
	Version (uint32_t inMajor = 0, uint32_t inMinor = 0, uint32_t inSub = 0,
	         uint32_t inBuildnumber = 0)
	{
		setMajor (inMajor);
		setMinor (inMinor);
		setSub (inSub);
		setBuildnumber (inBuildnumber);
	}

	void setMajor (uint32_t v) { storage[Major] = v; }
	void setMinor (uint32_t v) { storage[Minor] = v; }
	void setSub (uint32_t v) { storage[Sub] = v; }
	void setBuildnumber (uint32_t v) { storage[BuildNumber] = v; }

	uint32_t getMajor () const { return storage[Major]; }
	uint32_t getMinor () const { return storage[Minor]; }
	uint32_t getSub () const { return storage[Sub]; }
	uint32_t getBuildnumber () const { return storage[BuildNumber]; }

	bool operator> (const Version& v) const
	{
		if (getMajor () < v.getMajor ())
			return false;
		if (getMajor () > v.getMajor ())
			return true;
		if (getMinor () < v.getMinor ())
			return false;
		if (getMinor () > v.getMinor ())
			return true;
		if (getSub () < v.getSub ())
			return false;
		if (getSub () > v.getSub ())
			return true;
		if (getBuildnumber () < v.getBuildnumber ())
			return false;
		return getBuildnumber () > v.getBuildnumber ();
	}

	static Version parse (StringType str)
	{
		// skip non digits in the front
		auto it = std::find_if (str.begin (), str.end (),
		                        [] (const auto& c) { return std::isdigit (c); });
		if (it == str.end ())
			return {};
#ifdef SMTG_VERSIONPARSER_USE_STRINGVIEW
		str = StringType (&(*it), std::distance (it, str.end ()));
#else
		str = StringType (it, str.end ());
#endif
		Version version {};
		auto part = static_cast<size_t> (Major);
		StringType::size_type index;
		while (!str.empty ())
		{
			index = str.find_first_of ('.');
			if (index == StringType::npos)
			{
				// skip non digits in the back
				auto itBack = std::find_if (str.begin (), str.end (),
				                            [] (const auto& c) { return !std::isdigit (c); });
				index = std::distance (str.begin (), itBack);
				if (index == 0)
					break;
				str = {str.data (), index};
				index = str.size ();
			}
			StringType numberStr (str.data (), index);
			if (auto n = toNumber (numberStr))
				version.storage[part] = *n;
			if (++part > BuildNumber)
				break;
			if (str.size () - index == 0)
				break;
			++index;
			str = {str.data () + index, str.size () - index};
		}

		return version;
	}

private:
	std::array<uint32_t, 4> storage {};

	static Optional<int32_t> toNumber (StringType str)
	{
		if (str.size () > 9)
			str = {str.data (), 9};
		int32_t result = 0;
		for (const auto& c : str)
		{
			if (c < 48 || c > 57)
				return {};
			result *= 10;
			result += c - 48;
		}
		return Optional<int32_t> {result};
	}
};

//------------------------------------------------------------------------
} // VST3
