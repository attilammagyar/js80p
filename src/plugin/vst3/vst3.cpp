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

#include <vst3sdk/base/source/baseiids.cpp>
#include <vst3sdk/base/source/classfactoryhelpers.h>
#include <vst3sdk/base/source/fbuffer.cpp>
#include <vst3sdk/base/source/fbuffer.h>
#include <vst3sdk/base/source/fcleanup.h>
#include <vst3sdk/base/source/fcommandline.h>
#include <vst3sdk/base/source/fdebug.cpp>
#include <vst3sdk/base/source/fdebug.h>
#include <vst3sdk/base/source/fdynlib.cpp>
#include <vst3sdk/base/source/fdynlib.h>
#include <vst3sdk/base/source/fobject.cpp>
#include <vst3sdk/base/source/fobject.h>
#include <vst3sdk/base/source/fstreamer.cpp>
#include <vst3sdk/base/source/fstreamer.h>
#include <vst3sdk/base/source/fstring.cpp>
#include <vst3sdk/base/source/fstring.h>
#include <vst3sdk/base/source/timer.cpp>
#include <vst3sdk/base/source/timer.h>
#include <vst3sdk/base/source/updatehandler.cpp>
#include <vst3sdk/base/source/updatehandler.h>
#include <vst3sdk/base/thread/include/fcondition.h>
#include <vst3sdk/base/thread/include/flock.h>
#include <vst3sdk/base/thread/source/fcondition.cpp>
#include <vst3sdk/base/thread/source/flock.cpp>

#include <vst3sdk/pluginterfaces/base/conststringtable.cpp>
#include <vst3sdk/pluginterfaces/base/conststringtable.h>
#include <vst3sdk/pluginterfaces/base/coreiids.cpp>
#include <vst3sdk/pluginterfaces/base/falignpop.h>
#include <vst3sdk/pluginterfaces/base/falignpush.h>
#include <vst3sdk/pluginterfaces/base/fplatform.h>
#include <vst3sdk/pluginterfaces/base/fstrdefs.h>
#include <vst3sdk/pluginterfaces/base/ftypes.h>
#include <vst3sdk/pluginterfaces/base/funknown.cpp>
#include <vst3sdk/pluginterfaces/base/funknown.h>
#include <vst3sdk/pluginterfaces/base/funknownimpl.h>
#include <vst3sdk/pluginterfaces/base/futils.h>
#include <vst3sdk/pluginterfaces/base/fvariant.h>
#include <vst3sdk/pluginterfaces/base/geoconstants.h>
#include <vst3sdk/pluginterfaces/base/ibstream.h>
#include <vst3sdk/pluginterfaces/base/icloneable.h>
#include <vst3sdk/pluginterfaces/base/ierrorcontext.h>
#include <vst3sdk/pluginterfaces/base/ipersistent.h>
#include <vst3sdk/pluginterfaces/base/ipluginbase.h>
#include <vst3sdk/pluginterfaces/base/istringresult.h>
#include <vst3sdk/pluginterfaces/base/iupdatehandler.h>
#include <vst3sdk/pluginterfaces/base/keycodes.h>
#include <vst3sdk/pluginterfaces/base/pluginbasefwd.h>
#include <vst3sdk/pluginterfaces/base/smartpointer.h>
#include <vst3sdk/pluginterfaces/base/typesizecheck.h>
#include <vst3sdk/pluginterfaces/base/ucolorspec.h>
#include <vst3sdk/pluginterfaces/base/ustring.cpp>
#include <vst3sdk/pluginterfaces/base/ustring.h>

#include <vst3sdk/public.sdk/source/vst/vstpresetfile.cpp>
#include <vst3sdk/public.sdk/source/vst/vstpresetfile.h>
#include <vst3sdk/public.sdk/source/common/commoniids.cpp>
#include <vst3sdk/public.sdk/source/common/openurl.cpp>
#include <vst3sdk/public.sdk/source/common/openurl.h>
#include <vst3sdk/public.sdk/source/common/systemclipboard.h>
#include <vst3sdk/public.sdk/source/common/systemclipboard_mac.mm>
#include <vst3sdk/public.sdk/source/common/systemclipboard_win32.cpp>
#include <vst3sdk/public.sdk/source/common/threadchecker_mac.mm>
#include <vst3sdk/public.sdk/source/common/threadchecker_linux.cpp>
#include <vst3sdk/public.sdk/source/common/threadchecker_win32.cpp>
#include <vst3sdk/public.sdk/source/common/threadchecker.h>
#include <vst3sdk/public.sdk/source/common/pluginview.cpp>
#include <vst3sdk/public.sdk/source/common/pluginview.h>
#include <vst3sdk/public.sdk/source/main/pluginfactory.cpp>
#include <vst3sdk/public.sdk/source/main/pluginfactory.h>
// #include <vst3sdk/public.sdk/source/main/pluginfactory_constexpr.h>
#include <vst3sdk/public.sdk/source/main/moduleinit.cpp>
#include <vst3sdk/public.sdk/source/main/moduleinit.h>
#include <vst3sdk/public.sdk/source/vst/utility/audiobuffers.h>
#include <vst3sdk/public.sdk/source/vst/utility/processcontextrequirements.h>
#include <vst3sdk/public.sdk/source/vst/utility/processdataslicer.h>
#include <vst3sdk/public.sdk/source/vst/utility/ringbuffer.h>
#include <vst3sdk/public.sdk/source/vst/utility/rttransfer.h>
#include <vst3sdk/public.sdk/source/vst/utility/sampleaccurate.h>
#include <vst3sdk/public.sdk/source/vst/utility/stringconvert.cpp>
#include <vst3sdk/public.sdk/source/vst/utility/stringconvert.h>
#include <vst3sdk/public.sdk/source/vst/utility/testing.cpp>
#include <vst3sdk/public.sdk/source/vst/utility/testing.h>
#include <vst3sdk/public.sdk/source/vst/utility/vst2persistence.h>
#include <vst3sdk/public.sdk/source/vst/utility/vst2persistence.cpp>
#include <vst3sdk/public.sdk/source/vst/vstaudioeffect.cpp>
#include <vst3sdk/public.sdk/source/vst/vstaudioeffect.h>
#include <vst3sdk/public.sdk/source/vst/vstbus.cpp>
#include <vst3sdk/public.sdk/source/vst/vstbus.h>
#include <vst3sdk/public.sdk/source/vst/vstbypassprocessor.h>
#include <vst3sdk/public.sdk/source/vst/vstcomponent.cpp>
#include <vst3sdk/public.sdk/source/vst/vstcomponent.h>
#include <vst3sdk/public.sdk/source/vst/vstcomponentbase.cpp>
#include <vst3sdk/public.sdk/source/vst/vstcomponentbase.h>
#include <vst3sdk/public.sdk/source/vst/vsteditcontroller.cpp>
#include <vst3sdk/public.sdk/source/vst/vsteditcontroller.h>
#include <vst3sdk/public.sdk/source/vst/vsteventshelper.h>
#include <vst3sdk/public.sdk/source/vst/vsthelpers.h>
#include <vst3sdk/public.sdk/source/vst/vstinitiids.cpp>
#include <vst3sdk/public.sdk/source/vst/vstnoteexpressiontypes.cpp>
#include <vst3sdk/public.sdk/source/vst/vstnoteexpressiontypes.h>
#include <vst3sdk/public.sdk/source/vst/vstparameters.cpp>
#include <vst3sdk/public.sdk/source/vst/vstparameters.h>
#include <vst3sdk/public.sdk/source/vst/vstrepresentation.cpp>
#include <vst3sdk/public.sdk/source/vst/vstrepresentation.h>
