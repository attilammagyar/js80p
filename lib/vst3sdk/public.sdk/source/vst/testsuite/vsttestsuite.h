//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/vsttestsuite.h
// Created by  : Steinberg, 04/2005
// Description : VST Test Suite
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/testsuite/bus/busactivation.h"
#include "public.sdk/source/vst/testsuite/bus/busconsistency.h"
#include "public.sdk/source/vst/testsuite/bus/businvalidindex.h"
#include "public.sdk/source/vst/testsuite/bus/checkaudiobusarrangement.h"
#include "public.sdk/source/vst/testsuite/bus/scanbusses.h"
#include "public.sdk/source/vst/testsuite/bus/sidechainarrangement.h"
#include "public.sdk/source/vst/testsuite/general/editorclasses.h"
#include "public.sdk/source/vst/testsuite/general/midilearn.h"
#include "public.sdk/source/vst/testsuite/general/midimapping.h"
#include "public.sdk/source/vst/testsuite/general/parameterfunctionname.h"
#include "public.sdk/source/vst/testsuite/general/scanparameters.h"
#include "public.sdk/source/vst/testsuite/general/suspendresume.h"
#include "public.sdk/source/vst/testsuite/general/terminit.h"
#include "public.sdk/source/vst/testsuite/noteexpression/keyswitch.h"
#include "public.sdk/source/vst/testsuite/noteexpression/noteexpression.h"
#include "public.sdk/source/vst/testsuite/processing/automation.h"
#include "public.sdk/source/vst/testsuite/processing/process.h"
#include "public.sdk/source/vst/testsuite/processing/processcontextrequirements.h"
#include "public.sdk/source/vst/testsuite/processing/processformat.h"
#include "public.sdk/source/vst/testsuite/processing/processinputoverwriting.h"
#include "public.sdk/source/vst/testsuite/processing/processtail.h"
#include "public.sdk/source/vst/testsuite/processing/processthreaded.h"
#include "public.sdk/source/vst/testsuite/processing/silenceflags.h"
#include "public.sdk/source/vst/testsuite/processing/silenceprocessing.h"
#include "public.sdk/source/vst/testsuite/processing/speakerarrangement.h"
#include "public.sdk/source/vst/testsuite/processing/variableblocksize.h"
#include "public.sdk/source/vst/testsuite/state/bypasspersistence.h"
#include "public.sdk/source/vst/testsuite/state/invalidstatetransition.h"
#include "public.sdk/source/vst/testsuite/state/repeatidenticalstatetransition.h"
#include "public.sdk/source/vst/testsuite/state/validstatetransition.h"
#include "public.sdk/source/vst/testsuite/unit/checkunitstructure.h"
#include "public.sdk/source/vst/testsuite/unit/scanprograms.h"
#include "public.sdk/source/vst/testsuite/unit/scanunits.h"
