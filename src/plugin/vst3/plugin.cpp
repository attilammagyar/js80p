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

#include <algorithm>
#include <string>

#include <vst3sdk/base/source/fstreamer.h>
#include <vst3sdk/base/source/fstring.h>
#include <vst3sdk/pluginterfaces/base/fstrdefs.h>
#include <vst3sdk/pluginterfaces/base/futils.h>
#include <vst3sdk/pluginterfaces/base/ibstream.h>
#include <vst3sdk/pluginterfaces/base/ustring.h>
#include <vst3sdk/pluginterfaces/vst/ivstparameterchanges.h>
#include <vst3sdk/public.sdk/source/vst/vstaudioprocessoralgo.h>

#include "plugin/vst3/plugin.hpp"

#include "gui/gui.hpp"

#if SMTG_OS_LINUX
#include "plugin/vst3/plugin-xcb.cpp"
#elif SMTG_OS_WINDOWS
#include "plugin/vst3/plugin-win32.cpp"
#else
#error "Unsupported OS, currently JS80P can be compiled only for Linux and Windows. (Or did something go wrong with the SMTG_OS_LINUX and SMTG_OS_WINDOWS macros?)"
#endif

#include "midi.hpp"
#include "serializer.hpp"


using namespace Steinberg;


namespace JS80P
{

FUID const Vst3Plugin::Processor::ID(0x565354, 0x414d4a38, 0x6a733830, 0x70000000);

FUID const Vst3Plugin::Controller::ID(0x565345, 0x414d4a38, 0x6a733830, 0x70000000);


ViewRect const Vst3Plugin::GUI::rect(0, 0, JS80P::GUI::WIDTH, JS80P::GUI::HEIGHT);


Vst3Plugin::Event::Event()
    : time_offset(0.0),
    velocity_or_value(0.0),
    type(Type::UNDEFINED),
    note_or_ctl(0)
{
}


Vst3Plugin::Event::Event(
        Type const type,
        Seconds const time_offset,
        Midi::Byte const note_or_ctl,
        Number const velocity_or_value
) : time_offset(time_offset),
    velocity_or_value(velocity_or_value),
    type(type),
    note_or_ctl(note_or_ctl)
{
}


Vst3Plugin::Event::Event(Event const& event)
    : time_offset(event.time_offset),
    velocity_or_value(event.velocity_or_value),
    type(event.type),
    note_or_ctl(event.note_or_ctl)
{
}


Vst3Plugin::Event::Event(Event const&& event)
    : time_offset(event.time_offset),
    velocity_or_value(event.velocity_or_value),
    type(event.type),
    note_or_ctl(event.note_or_ctl)
{
}


Vst3Plugin::Event& Vst3Plugin::Event::operator=(Event const& event) noexcept
{
    time_offset = event.time_offset;
    velocity_or_value = event.velocity_or_value;
    type = event.type;
    note_or_ctl = event.note_or_ctl;

    return *this;
}


Vst3Plugin::Event& Vst3Plugin::Event::operator=(Event const&& event) noexcept
{
    time_offset = event.time_offset;
    velocity_or_value = event.velocity_or_value;
    type = event.type;
    note_or_ctl = event.note_or_ctl;

    return *this;
}


bool Vst3Plugin::Event::operator<(Event const& event) const noexcept
{
    return time_offset < event.time_offset;
}


FUnknown* Vst3Plugin::Processor::createInstance(void* unused)
{
    return (Vst::IAudioProcessor*)new Processor();
}


Vst3Plugin::Processor::Processor() : synth(), round(-1), events(4096)
{
    setControllerClass(Controller::ID);
    processContextRequirements.needTempo();
}


tresult PLUGIN_API Vst3Plugin::Processor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);

    if (result != kResultTrue) {
        return kResultFalse;
    }

    addEventInput(STR16("Event Input"), 1);
    addAudioOutput(STR16("AudioOutput"), Vst::SpeakerArr::kStereo);

    return kResultOk;
}


tresult PLUGIN_API Vst3Plugin::Processor::setBusArrangements(
    Vst::SpeakerArrangement* inputs,
    int32 numIns,
    Vst::SpeakerArrangement* outputs,
    int32 numOuts
) {
    if (numIns == 0 && numOuts == 1 && outputs[0] == Vst::SpeakerArr::kStereo)
    {
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    }

    return kResultFalse;
}


tresult PLUGIN_API Vst3Plugin::Processor::connect(IConnectionPoint* other)
{
    tresult result = Vst::AudioEffect::connect(other);
    share_synth();

    return result;
}


tresult PLUGIN_API Vst3Plugin::Processor::notify(Vst::IMessage* message)
{
    if (!message) {
        return kInvalidArgument;
    }

    if (FIDStringsEqual(message->getMessageID(), MSG_CTL_READY)) {
        share_synth();
    }

    return Vst::AudioEffect::notify(message);
}


tresult PLUGIN_API Vst3Plugin::Processor::canProcessSampleSize(int32 symbolicSampleSize)
{
    if (
            symbolicSampleSize == Vst::SymbolicSampleSizes::kSample64
            || symbolicSampleSize == Vst::SymbolicSampleSizes::kSample32
    ) {
        return kResultTrue;
    }

    return kResultFalse;
}


tresult PLUGIN_API Vst3Plugin::Processor::setupProcessing(Vst::ProcessSetup& setup)
{
    synth.set_block_size((Integer)setup.maxSamplesPerBlock);
    synth.set_sample_rate((Frequency)setup.sampleRate);

    return AudioEffect::setupProcessing(setup);
}


tresult PLUGIN_API Vst3Plugin::Processor::setActive(TBool state)
{
    if (state)
    {
        synth.resume();
    }
    else
    {
        synth.suspend();
    }

    return AudioEffect::setActive(state);
}


void Vst3Plugin::Processor::share_synth() noexcept
{
    IPtr<Vst::IMessage> message = owned(allocateMessage());

    if (!message) {
        return;
    }

    message->setMessageID(MSG_SHARE_SYNTH);

    Vst::IAttributeList* attributes = message->getAttributes();

    if (attributes) {
        attributes->setInt(MSG_SHARE_SYNTH_ATTR, (int64)&synth);
        sendMessage(message);
    }
}


tresult PLUGIN_API Vst3Plugin::Processor::process(Vst::ProcessData& data)
{
    events.clear();
    collect_param_change_events(data);
    collect_note_events(data);
    std::sort(events.begin(), events.end());
    process_events();

    if (data.numOutputs == 0 || data.numSamples < 1) {
        return kResultOk;
    }

    update_bpm(data);
    generate_samples(data);

    return kResultOk;
}


void Vst3Plugin::Processor::collect_param_change_events(
        Vst::ProcessData& data
) noexcept {
    if (!data.inputParameterChanges) {
        return;
    }

    int32 numParamsChanged = data.inputParameterChanges->getParameterCount();

    for (int32 i = 0; i != numParamsChanged; i++) {
        Vst::IParamValueQueue* paramQueue = (
            data.inputParameterChanges->getParameterData(i)
        );

        if (!paramQueue) {
            continue;
        }

        Vst::ParamID const param_id = paramQueue->getParameterId();
        Vst::ParamValue value;
        int32 sampleOffset;
        int32 numPoints = paramQueue->getPointCount();

        if (param_id == (Vst::ParamID)Vst::ControllerNumbers::kPitchBend) {
            for (int32 i = 0; i != numPoints; ++i) {
                if (paramQueue->getPoint(i, sampleOffset, value) != kResultTrue) {
                    continue;
                }

                events.push_back(
                    Event(
                        Event::PITCH_WHEEL,
                        synth.sample_count_to_time_offset(sampleOffset),
                        0,
                        value
                    )
                );
            }
        } else if (Synth::is_supported_midi_controller((Midi::Controller)param_id)) {
            for (int32 i = 0; i != numPoints; ++i) {
                if (paramQueue->getPoint(i, sampleOffset, value) != kResultTrue) {
                    continue;
                }

                events.push_back(
                    Event(
                        Event::CONTROL_CHANGE,
                        synth.sample_count_to_time_offset(sampleOffset),
                        (Midi::Byte)param_id,
                        value
                    )
                );
            }
        }
    }
}


void Vst3Plugin::Processor::collect_note_events(Vst::ProcessData& data) noexcept
{
    Vst::IEventList* input_events = data.inputEvents;

    if (!input_events) {
        return;
    }

    int32 count = input_events->getEventCount();
    Vst::Event event;

    for (int32 i = 0; i != count; ++i) {
        if (input_events->getEvent(i, event) != kResultTrue) {
            continue;
        }

        switch (event.type) {
            case Vst::Event::EventTypes::kNoteOnEvent:
                events.push_back(
                    Event(
                        Event::NOTE_ON,
                        synth.sample_count_to_time_offset(event.sampleOffset),
                        (Midi::Byte)event.noteOn.pitch,
                        (Number)event.noteOn.velocity
                    )
                );
                break;

            case Vst::Event::EventTypes::kNoteOffEvent:
                events.push_back(
                    Event(
                        Event::NOTE_OFF,
                        synth.sample_count_to_time_offset(event.sampleOffset),
                        (Midi::Byte)event.noteOff.pitch,
                        (Number)event.noteOff.velocity
                    )
                );
                break;

            case Vst::Event::EventTypes::kPolyPressureEvent:
                events.push_back(
                    Event(
                        Event::NOTE_PRESSURE,
                        synth.sample_count_to_time_offset(event.sampleOffset),
                        (Midi::Byte)event.polyPressure.pitch,
                        (Number)event.polyPressure.pressure
                    )
                );
                break;

            default:
                break;
        }
    }
}


void Vst3Plugin::Processor::process_events() noexcept
{
    for (std::vector<Event>::const_iterator it = events.begin(); it != events.end(); ++it) {
        process_event(*it);
    }
}


void Vst3Plugin::Processor::process_event(Event const event) noexcept
{
    switch (event.type) {
        case Event::Type::NOTE_ON:
            synth.note_on(
                event.time_offset, 0, event.note_or_ctl, event.velocity_or_value
            );
            break;

        case Event::Type::NOTE_PRESSURE:
            synth.aftertouch(
                event.time_offset, 0, event.note_or_ctl, event.velocity_or_value
            );
            break;

        case Event::Type::NOTE_OFF:
            synth.note_off(
                event.time_offset, 0, event.note_or_ctl, event.velocity_or_value
            );
            break;

        case Event::Type::PITCH_WHEEL:
            synth.pitch_wheel_change(
                event.time_offset, 0, event.velocity_or_value
            );
            break;

        case Event::Type::CONTROL_CHANGE:
            synth.control_change(
                event.time_offset, 0, event.note_or_ctl, event.velocity_or_value
            );
            break;

        default:
            break;
    }
}


void Vst3Plugin::Processor::update_bpm(Vst::ProcessData& data) noexcept
{
    if (
            data.processContext == NULL
            || (data.processContext->state & Vst::ProcessContext::StatesAndFlags::kTempoValid) == 0
    ) {
        return;
    }

    synth.set_bpm((Number)data.processContext->tempo);
}


void Vst3Plugin::Processor::generate_samples(Vst::ProcessData& data) noexcept
{
    if (processSetup.symbolicSampleSize == Vst::SymbolicSampleSizes::kSample64) {
        generate_samples<double>(
            data.numSamples,
            (double**)getChannelBuffersPointer(processSetup, data.outputs[0])
        );
    } else if (processSetup.symbolicSampleSize == Vst::SymbolicSampleSizes::kSample32) {
        generate_samples<float>(
            data.numSamples,
            (float**)getChannelBuffersPointer(processSetup, data.outputs[0])
        );
    } else {
        return;
    }
}


template<typename NumberType>
void Vst3Plugin::Processor::generate_samples(
        Integer const sample_count,
        NumberType** samples
) noexcept {
    Sample const* const* buffer = render_next_round(sample_count);

    for (Integer c = 0; c != Synth::OUT_CHANNELS; ++c) {
        for (Integer i = 0; i != sample_count; ++i) {
            samples[c][i] = (NumberType)buffer[c][i];
        }
    }
}


Sample const* const* Vst3Plugin::Processor::render_next_round(
        Integer const sample_count
) noexcept {
    round = (round + 1) & ROUND_MASK;

    return synth.generate_samples(round, (Integer)sample_count);
}


uint32 PLUGIN_API Vst3Plugin::Processor::getTailSamples()
{
    return Vst::kInfiniteTail;
}


tresult PLUGIN_API Vst3Plugin::Processor::setState(IBStream* state)
{
    if (state == NULL) {
        return kResultFalse;
    }

    std::string const serialized = read_serialized_patch(state);

    synth.process_messages();
    Serializer::import(&synth, serialized);
    synth.process_messages();

    return kResultOk;
}


std::string Vst3Plugin::Processor::read_serialized_patch(IBStream* stream) const
{
    /*
    Not using FStreamer::readString8(), because we need the entire string here,
    and that method stops at line breaks, handles Unix-style line breaks
    inconsistenly, and calls strlen() for a buffer that it had just done
    terminating with a zero byte, knowing its position perfectly well:

    https://github.com/steinbergmedia/vst3_base/pull/5
    */

    char* buffer = new char[Serializer::MAX_SIZE];
    Integer i;
    int32 numBytesRead;
    char8 c;

    for (i = 0; i != Serializer::MAX_SIZE; ++i) {
        stream->read(&c, sizeof(char8), &numBytesRead);

        if (numBytesRead != sizeof(char8) || c == '\x00') {
            break;
        }

        buffer[i] = c;
    }

    if (i >= Serializer::MAX_SIZE) {
        i = Serializer::MAX_SIZE - 1;
    }

    buffer[i] = '\x00';

    std::string const result(buffer, (std::string::size_type)i);

    delete[] buffer;

    return result;
}


tresult PLUGIN_API Vst3Plugin::Processor::getState(IBStream* state)
{
    if (state == NULL) {
        return kResultFalse;
    }

    std::string const serialized = Serializer::serialize(&synth);
    int32 const size = serialized.size();
    int32 numBytesWritten;

    state->write((void*)serialized.c_str(), size, &numBytesWritten);

    if (numBytesWritten != size) {
        return kResultFalse;
    }

    return kResultOk;
}


Vst3Plugin::GUI::GUI(Controller* controller)
    : CPluginView(&rect),
    controller(controller),
    synth(NULL),
    gui(NULL),
    run_loop(NULL),
    event_handler(NULL),
    timer_handler(NULL)
{
}


Vst3Plugin::GUI::~GUI()
{
    if (gui != NULL) {
        delete gui;

        gui = NULL;
    }
}


tresult PLUGIN_API Vst3Plugin::GUI::isPlatformTypeSupported(FIDString type)
{
    if (type && strcmp(type, JS80P_VST3_GUI_PLATFORM) == 0) {
        return kResultTrue;
    }

    return kResultFalse;
}


tresult PLUGIN_API Vst3Plugin::GUI::canResize()
{
    return kResultFalse;
}


void Vst3Plugin::GUI::attachedToParent()
{
    show_if_needed();
}


void Vst3Plugin::GUI::set_synth(Synth* synth)
{
    this->synth = synth;
    show_if_needed();
}


void Vst3Plugin::GUI::show_if_needed()
{
    if (synth == NULL || !isAttached()) {
        return;
    }

    initialize();
}


FUnknown* Vst3Plugin::Controller::createInstance(void* unused)
{
    return (IEditController*)new Vst3Plugin::Controller();
}


Vst3Plugin::Controller::Controller() : synth(NULL)
{
}


Vst3Plugin::Controller::~Controller()
{
}


tresult PLUGIN_API Vst3Plugin::Controller::initialize(FUnknown* context)
{
    tresult result = EditController::initialize(context);

    if (result != kResultTrue) {
        return result;
    }

    parameters.addParameter(
        create_midi_ctl_param(
            Synth::ControllerId::PITCH_WHEEL,
            (Vst::ParamID)Vst::ControllerNumbers::kPitchBend
        )
    );

    for (Integer cc = 0; cc != Synth::MIDI_CONTROLLERS; ++cc) {
        if (!Synth::is_supported_midi_controller((Midi::Controller)cc)) {
            continue;
        }

        parameters.addParameter(
            create_midi_ctl_param((Synth::ControllerId)cc, (Vst::ParamID)cc)
        );
    }

    return result;
}


Vst::RangeParameter* Vst3Plugin::Controller::create_midi_ctl_param(
        Synth::ControllerId const controller_id,
        Vst::ParamID const param_id
) const {
    JS80P::GUI::Controller const* const controller = JS80P::GUI::get_controller(controller_id);
    Vst::RangeParameter* param = new Vst::RangeParameter(
        USTRING(controller->long_name),
        param_id,
        USTRING("%"),
        0.0,
        100.0,
        0.0,
        0,
        Vst::ParameterInfo::kCanAutomate,
        Vst::kRootUnitId,
        USTRING(controller->short_name)
    );
    param->setPrecision(1);

    return param;
}


tresult PLUGIN_API Vst3Plugin::Controller::getMidiControllerAssignment(
        int32 busIndex,
        int16 channel,
        Vst::CtrlNumber midiControllerNumber,
        Vst::ParamID& id
) {
    if (busIndex == 0 && midiControllerNumber < Vst::kCountCtrlNumber) {
        if (
                Synth::is_supported_midi_controller((Midi::Controller)midiControllerNumber)
                || midiControllerNumber == Vst::ControllerNumbers::kPitchBend
        ) {
            id = midiControllerNumber;

            return kResultTrue;
        }
    }

    return kResultFalse;
}


tresult PLUGIN_API Vst3Plugin::Controller::connect(IConnectionPoint* other)
{
    tresult result = EditController::connect(other);

    IPtr<Vst::IMessage> message = owned(allocateMessage());

    if (!message) {
        return result;
    }

    message->setMessageID(MSG_CTL_READY);
    sendMessage(message);

    return result;
}


tresult PLUGIN_API Vst3Plugin::Controller::notify(Vst::IMessage* message)
{
    if (!message) {
        return kInvalidArgument;
    }

    if (FIDStringsEqual(message->getMessageID(), MSG_SHARE_SYNTH)) {
        int64 synth_ptr;
        if (message->getAttributes()->getInt(MSG_SHARE_SYNTH_ATTR, synth_ptr) == kResultOk) {
            synth = (Synth*)synth_ptr;

            return kResultOk;
        }
    }

    return EditController::notify(message);
}


IPlugView* PLUGIN_API Vst3Plugin::Controller::createView(FIDString name)
{
    if (name && strcmp(name, Vst::ViewType::kEditor) == 0) {
        if (synth == NULL) {
            return NULL;
        }

        GUI* gui = new GUI(this);
        gui->set_synth(synth);

        return gui;
    }

    return NULL;
}


tresult PLUGIN_API Vst3Plugin::Controller::setComponentState(IBStream* state)
{
    return kResultOk;
}

}


BEGIN_FACTORY_DEF(
    JS80P::Constants::COMPANY_NAME,
    JS80P::Constants::COMPANY_WEB,
    JS80P::Constants::COMPANY_EMAIL
)

    DEF_CLASS2(
        INLINE_UID_FROM_FUID(JS80P::Vst3Plugin::Processor::ID),
        PClassInfo::kManyInstances,
        kVstAudioEffectClass,
        JS80P::Constants::PLUGIN_NAME,
        0,
        Vst::PlugType::kInstrumentSynth,
        JS80P::Constants::PLUGIN_VERSION_STR,
        kVstVersionString,
        JS80P::Vst3Plugin::Processor::createInstance
    )

    DEF_CLASS2(
        INLINE_UID_FROM_FUID(JS80P::Vst3Plugin::Controller::ID),
        PClassInfo::kManyInstances,
        kVstComponentControllerClass,
        "JS80PController",
        0,
        "",
        JS80P::Constants::PLUGIN_VERSION_STR,
        kVstVersionString,
        JS80P::Vst3Plugin::Controller::createInstance
    )

END_FACTORY
