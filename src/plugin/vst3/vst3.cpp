/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
 *
 * JS80P is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JS80P is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <base/source/baseiids.cpp>
#include <base/source/classfactoryhelpers.h>
#include <base/source/fbuffer.cpp>
#include <base/source/fbuffer.h>
#include <base/source/fcleanup.h>
#include <base/source/fcommandline.h>
#include <base/source/fdebug.cpp>
#include <base/source/fdebug.h>
#include <base/source/fdynlib.cpp>
#include <base/source/fdynlib.h>
#include <base/source/fobject.cpp>
#include <base/source/fobject.h>
#include <base/source/fstreamer.cpp>
#include <base/source/fstreamer.h>
#include <base/source/fstring.cpp>
#include <base/source/fstring.h>
#include <base/source/timer.cpp>
#include <base/source/timer.h>
#include <base/source/updatehandler.cpp>
#include <base/source/updatehandler.h>
#include <base/thread/include/fcondition.h>
#include <base/thread/include/flock.h>
#include <base/thread/source/fcondition.cpp>
#include <base/thread/source/flock.cpp>

#include <pluginterfaces/base/conststringtable.cpp>
#include <pluginterfaces/base/conststringtable.h>
#include <pluginterfaces/base/coreiids.cpp>
#include <pluginterfaces/base/falignpop.h>
#include <pluginterfaces/base/falignpush.h>
#include <pluginterfaces/base/fplatform.h>
#include <pluginterfaces/base/fstrdefs.h>
#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/base/funknown.cpp>
#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/funknownimpl.h>
#include <pluginterfaces/base/futils.h>
#include <pluginterfaces/base/fvariant.h>
#include <pluginterfaces/base/geoconstants.h>
#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/base/icloneable.h>
#include <pluginterfaces/base/ierrorcontext.h>
#include <pluginterfaces/base/ipersistent.h>
#include <pluginterfaces/base/ipluginbase.h>
#include <pluginterfaces/base/istringresult.h>
#include <pluginterfaces/base/iupdatehandler.h>
#include <pluginterfaces/base/keycodes.h>
#include <pluginterfaces/base/pluginbasefwd.h>
#include <pluginterfaces/base/smartpointer.h>
#include <pluginterfaces/base/typesizecheck.h>
#include <pluginterfaces/base/ucolorspec.h>
#include <pluginterfaces/base/ustring.cpp>
#include <pluginterfaces/base/ustring.h>

#include <public.sdk/source/vst/vstpresetfile.cpp>
#include <public.sdk/source/vst/vstpresetfile.h>
#include <public.sdk/source/common/commoniids.cpp>
#include <public.sdk/source/common/openurl.cpp>
#include <public.sdk/source/common/openurl.h>
#include <public.sdk/source/common/systemclipboard.h>
#include <public.sdk/source/common/systemclipboard_win32.cpp>
#include <public.sdk/source/common/threadchecker_linux.cpp>
#include <public.sdk/source/common/threadchecker_win32.cpp>
#include <public.sdk/source/common/threadchecker.h>
#include <public.sdk/source/common/pluginview.cpp>
#include <public.sdk/source/common/pluginview.h>
#include <public.sdk/source/main/pluginfactory.cpp>
#include <public.sdk/source/main/pluginfactory.h>
// #include <public.sdk/source/main/pluginfactory_constexpr.h>
#include <public.sdk/source/main/moduleinit.cpp>
#include <public.sdk/source/main/moduleinit.h>
#include <public.sdk/source/vst/utility/audiobuffers.h>
#include <public.sdk/source/vst/utility/processcontextrequirements.h>
#include <public.sdk/source/vst/utility/processdataslicer.h>
#include <public.sdk/source/vst/utility/ringbuffer.h>
#include <public.sdk/source/vst/utility/rttransfer.h>
#include <public.sdk/source/vst/utility/sampleaccurate.h>
#include <public.sdk/source/vst/utility/stringconvert.cpp>
#include <public.sdk/source/vst/utility/stringconvert.h>
#include <public.sdk/source/vst/utility/testing.cpp>
#include <public.sdk/source/vst/utility/testing.h>
#include <public.sdk/source/vst/utility/vst2persistence.h>
#include <public.sdk/source/vst/utility/vst2persistence.cpp>
#include <public.sdk/source/vst/vstaudioeffect.cpp>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vstbus.cpp>
#include <public.sdk/source/vst/vstbus.h>
#include <public.sdk/source/vst/vstbypassprocessor.h>
#include <public.sdk/source/vst/vstcomponent.cpp>
#include <public.sdk/source/vst/vstcomponent.h>
#include <public.sdk/source/vst/vstcomponentbase.cpp>
#include <public.sdk/source/vst/vstcomponentbase.h>
#include <public.sdk/source/vst/vsteditcontroller.cpp>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/vst/vsteventshelper.h>
#include <public.sdk/source/vst/vsthelpers.h>
#include <public.sdk/source/vst/vstinitiids.cpp>
#include <public.sdk/source/vst/vstnoteexpressiontypes.cpp>
#include <public.sdk/source/vst/vstnoteexpressiontypes.h>
#include <public.sdk/source/vst/vstparameters.cpp>
#include <public.sdk/source/vst/vstparameters.h>
#include <public.sdk/source/vst/vstrepresentation.cpp>
#include <public.sdk/source/vst/vstrepresentation.h>
