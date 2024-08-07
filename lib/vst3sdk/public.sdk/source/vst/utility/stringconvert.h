//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/stringconvert.h
// Created by  : Steinberg, 11/2014
// Description : c++11 unicode string convert functions
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2024, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this
//     software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/vsttypes.h"
#include <string>

namespace Steinberg {
namespace Vst {
namespace StringConvert {

//------------------------------------------------------------------------
/**
 *	Forward to Steinberg::StringConvert::convert (...)
 */
std::u16string convert (const std::string& utf8Str);

//------------------------------------------------------------------------
/**
 *	Forward to Steinberg::StringConvert::convert (...)
 */
std::string convert (const std::u16string& str);

//------------------------------------------------------------------------
/**
 *  Forward to Steinberg::StringConvert::convert (...)
 */
std::string convert (const char* str, uint32_t max);

//------------------------------------------------------------------------
/**
 *  convert an UTF-8 string to an UTF-16 string buffer with max 127 characters
 *
 *  @param utf8Str UTF-8 string
 *  @param str     UTF-16 string buffer
 *
 *  @return true on success
 */
bool convert (const std::string& utf8Str, Steinberg::Vst::String128 str);

//------------------------------------------------------------------------
/**
 *  convert an UTF-8 string to an UTF-16 string buffer
 *
 *  @param utf8Str       UTF-8 string
 *  @param str           UTF-16 string buffer
 *  @param maxCharacters max characters that fit into str
 *
 *  @return true on success
 */
bool convert (const std::string& utf8Str, Steinberg::Vst::TChar* str,
                     uint32_t maxCharacters);

//------------------------------------------------------------------------
/**
 *  convert an UTF-16 string buffer to an UTF-8 string
 *
 *  @param str UTF-16 string buffer
 *
 *  @return UTF-8 string
 */
std::string convert (const Steinberg::Vst::TChar* str);

//------------------------------------------------------------------------
/**
 *  convert an UTF-16 string buffer to an UTF-8 string
 *
 *  @param str UTF-16 string buffer
 *	@param max maximum characters in str
 *
 *  @return UTF-8 string
 */
std::string convert (const Steinberg::Vst::TChar* str, uint32_t max);

//------------------------------------------------------------------------
} // StringConvert

//------------------------------------------------------------------------
inline const Steinberg::Vst::TChar* toTChar (const std::u16string& str)
{
	return reinterpret_cast<const Steinberg::Vst::TChar*> (str.data ());
}

//------------------------------------------------------------------------
/**
 *	convert a number to an UTF-16 string
 *
 *	@param value number
 *
 *	@return UTF-16 string
 */
template <typename NumberT>
std::u16string toString (NumberT value)
{
	auto u8str = std::to_string (value);
	return StringConvert::convert (u8str);
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

//------------------------------------------------------------------------
// Deprecated VST3 namespace
//------------------------------------------------------------------------
namespace VST3 {
namespace StringConvert {

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline std::u16string convert (const std::string& utf8Str)
{
	return Steinberg::Vst::StringConvert::convert (utf8Str);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline std::string convert (const std::u16string& str)
{
	return Steinberg::Vst::StringConvert::convert (str);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline std::string convert (const char* str, uint32_t max)
{
	return Steinberg::Vst::StringConvert::convert (str, max);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline bool convert (const std::string& utf8Str, Steinberg::Vst::String128 str)
{
	return Steinberg::Vst::StringConvert::convert (utf8Str, str);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline bool convert (const std::string& utf8Str, Steinberg::Vst::TChar* str, uint32_t maxCharacters)
{
	return Steinberg::Vst::StringConvert::convert (utf8Str, str, maxCharacters);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline std::string convert (const Steinberg::Vst::TChar* str)
{
	return Steinberg::Vst::StringConvert::convert (str);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline std::string convert (const Steinberg::Vst::TChar* str, uint32_t max)
{
	return Steinberg::Vst::StringConvert::convert (str, max);
}

//------------------------------------------------------------------------
} // StringConvert

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::toTChar (...)")
inline const Steinberg::Vst::TChar* toTChar (const std::u16string& str)
{
	return Steinberg::Vst::toTChar (str);
}

//------------------------------------------------------------------------
template <typename NumberT>
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::toString (...)")
std::u16string toString (NumberT value)
{
	return Steinberg::Vst::toString (value);
}

//------------------------------------------------------------------------
} // VST3
