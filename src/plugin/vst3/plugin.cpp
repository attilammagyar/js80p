/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
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
#include <cstddef>
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


#define JS80P_VST3_SEND_MSG(msg_id, attr_setter, attr_name, attr_value)     \
    do {                                                                    \
        IPtr<Vst::IMessage> message = owned(allocateMessage());             \
                                                                            \
        if (message) {                                                      \
            message->setMessageID(msg_id);                                  \
                                                                            \
            Vst::IAttributeList* attributes = message->getAttributes();     \
                                                                            \
            if (attributes) {                                               \
                attributes->attr_setter((attr_name), (attr_value));         \
                sendMessage(message);                                       \
            }                                                               \
        }                                                                   \
    } while (false)


#define JS80P_VST3_SEND_EMPTY_MSG(msg_id)                                   \
    do {                                                                    \
        IPtr<Vst::IMessage> message = owned(allocateMessage());             \
                                                                            \
        if (message) {                                                      \
            message->setMessageID(msg_id);                                  \
            sendMessage(message);                                           \
        }                                                                   \
    } while (false)


namespace JS80P
{

FUID const Vst3Plugin::Processor::ID(0x565354, 0x414d4a38, 0x6a733830, 0x70000000);

FUID const Vst3Plugin::Controller::ID(0x565345, 0x414d4a38, 0x6a733830, 0x70000000);


ViewRect const Vst3Plugin::GUI::rect(0, 0, JS80P::GUI::WIDTH, JS80P::GUI::HEIGHT);


Vst3Plugin::Event::Event()
    : time_offset(0.0),
    velocity_or_value(0.0),
    type(Type::UNDEFINED),
    note_or_ctl(0),
    channel(0)
{
}


Vst3Plugin::Event::Event(
        Type const type,
        Seconds const time_offset,
        Midi::Byte const note_or_ctl,
        Midi::Channel const channel,
        Number const velocity_or_value
) : time_offset(time_offset),
    velocity_or_value(velocity_or_value),
    type(type),
    note_or_ctl(note_or_ctl),
    channel(channel & 0x0f)
{
}


bool Vst3Plugin::Event::operator<(Event const& event) const noexcept
{
    return time_offset < event.time_offset;
}


FUnknown* Vst3Plugin::Processor::createInstance(void* unused)
{
    return (Vst::IAudioProcessor*)new Processor();
}


Vst3Plugin::Processor::Processor()
    : synth(),
    renderer(synth),
    mts_esp(synth),
    bank(NULL),
    events(4096),
    new_program(0),
    need_to_load_new_program(false)
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
    addAudioInput(STR16("AudioInput"), Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("AudioOutput"), Vst::SpeakerArr::kStereo);

    return kResultOk;
}


tresult PLUGIN_API Vst3Plugin::Processor::setBusArrangements(
    Vst::SpeakerArrangement* inputs,
    int32 number_of_inputs,
    Vst::SpeakerArrangement* outputs,
    int32 number_of_outputs
) {
    if (
            number_of_inputs == 1 && inputs[0] == Vst::SpeakerArr::kStereo
            && number_of_outputs == 1 && outputs[0] == Vst::SpeakerArr::kStereo
    )
    {
        return AudioEffect::setBusArrangements(
            inputs, number_of_inputs, outputs, number_of_outputs
        );
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

    if (FIDStringsEqual(message->getMessageID(), MSG_PROGRAM_CHANGE)) {
        double program;

        if (message->getAttributes()->getFloat(MSG_PROGRAM_CHANGE_PROGRAM, program) == kResultOk) {
            events.push_back(
                Event(Event::Type::PROGRAM_CHANGE, 0.0, 0, 0, (Number)program)
            );
        }
    } else if (FIDStringsEqual(message->getMessageID(), MSG_CTL_READY)) {
        int64 bank_ptr;

        if (message->getAttributes()->getInt(MSG_CTL_READY_BANK, bank_ptr) == kResultOk) {
            bank = (Bank const*)bank_ptr;
            share_synth();
        }
    }

    return Vst::AudioEffect::notify(message);
}


tresult PLUGIN_API Vst3Plugin::Processor::canProcessSampleSize(int32 symbolic_sample_size)
{
    if (
            symbolic_sample_size == Vst::SymbolicSampleSizes::kSample64
            || symbolic_sample_size == Vst::SymbolicSampleSizes::kSample32
    ) {
        return kResultTrue;
    }

    return kResultFalse;
}


tresult PLUGIN_API Vst3Plugin::Processor::setupProcessing(Vst::ProcessSetup& setup)
{
    synth.set_sample_rate((Frequency)setup.sampleRate);
    renderer.reset();

    return AudioEffect::setupProcessing(setup);
}


tresult PLUGIN_API Vst3Plugin::Processor::setActive(TBool state)
{
    if (state)
    {
        synth.resume();
        renderer.reset();
    }
    else
    {
        synth.suspend();
        renderer.reset();
    }

    return AudioEffect::setActive(state);
}


void Vst3Plugin::Processor::share_synth() noexcept
{
    JS80P_VST3_SEND_MSG(
        MSG_SHARE_SYNTH, setInt, MSG_SHARE_SYNTH_SYNTH, (int64)&synth
    );
}


tresult PLUGIN_API Vst3Plugin::Processor::process(Vst::ProcessData& data)
{
    collect_param_change_events(data);
    collect_note_events(data);
    std::sort(events.begin(), events.end());
    process_events();
    events.clear();

    if (bank != NULL && need_to_load_new_program) {
        need_to_load_new_program = false;
        Serializer::import_patch_in_audio_thread(
            synth,
            (*bank)[new_program].serialize()
        );
        synth.clear_dirty_flag();
    }

    if (data.numOutputs == 0 || data.numSamples < 1) {
        return kResultOk;
    }

    update_bpm(data);
    mts_esp.update_active_notes_tuning();

    generate_samples(data);

    mts_esp.update_connection_status();

    if (synth.is_dirty()) {
        synth.clear_dirty_flag();
        JS80P_VST3_SEND_EMPTY_MSG(MSG_SYNTH_DIRTY);
    }

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
        Vst::IParamValueQueue* param_queue = (
            data.inputParameterChanges->getParameterData(i)
        );

        if (!param_queue) {
            continue;
        }

        Vst::ParamID const param_id = param_queue->getParameterId();

        switch (param_id) {
            case (Vst::ParamID)PROGRAM_LIST_ID:
                collect_param_change_events_as(
                    param_queue, Event::Type::PROGRAM_CHANGE, 0
                );
                break;

            case (Vst::ParamID)Vst::ControllerNumbers::kPitchBend:
                collect_param_change_events_as(
                    param_queue, Event::Type::PITCH_WHEEL, 0
                );
                break;

            case (Vst::ParamID)Vst::ControllerNumbers::kAfterTouch:
                collect_param_change_events_as(
                    param_queue, Event::Type::CHANNEL_PRESSURE, 0
                );
                break;

            default:
                if (Synth::is_supported_midi_controller((Midi::Controller)param_id)) {
                    collect_param_change_events_as(
                        param_queue, Event::Type::CONTROL_CHANGE, (Midi::Byte)param_id
                    );
                }
                break;
        }
    }
}


void Vst3Plugin::Processor::collect_param_change_events_as(
        Vst::IParamValueQueue* const param_queue,
        Event::Type const event_type,
        Midi::Byte const midi_controller
) noexcept {
    int32 const number_of_points = param_queue->getPointCount();

    Vst::ParamValue value;
    int32 sample_offset;

    for (int32 i = 0; i != number_of_points; ++i) {
        if (param_queue->getPoint(i, sample_offset, value) != kResultTrue) {
            continue;
        }

        events.push_back(
            Event(
                event_type,
                synth.sample_count_to_time_offset(sample_offset),
                midi_controller,
                0,
                (Number)value
            )
        );
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
                        Event::Type::NOTE_ON,
                        synth.sample_count_to_time_offset(event.sampleOffset),
                        (Midi::Byte)event.noteOn.pitch,
                        (Midi::Channel)(event.noteOn.channel & 0xff),
                        (Number)event.noteOn.velocity
                    )
                );
                break;

            case Vst::Event::EventTypes::kNoteOffEvent:
                events.push_back(
                    Event(
                        Event::Type::NOTE_OFF,
                        synth.sample_count_to_time_offset(event.sampleOffset),
                        (Midi::Byte)event.noteOff.pitch,
                        (Midi::Channel)(event.noteOn.channel & 0xff),
                        (Number)event.noteOff.velocity
                    )
                );
                break;

            case Vst::Event::EventTypes::kPolyPressureEvent:
                events.push_back(
                    Event(
                        Event::Type::NOTE_PRESSURE,
                        synth.sample_count_to_time_offset(event.sampleOffset),
                        (Midi::Byte)event.polyPressure.pitch,
                        (Midi::Channel)(event.noteOn.channel & 0xff),
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
        case Event::Type::NOTE_ON: {
            mts_esp.update_note_tuning(event.channel, event.note_or_ctl);
            Midi::Byte const velocity = float_to_midi_byte(event.velocity_or_value);

            if (velocity == 0) {
                synth.note_off(event.time_offset, event.channel, event.note_or_ctl, 64);
            } else {
                synth.note_on(event.time_offset, event.channel, event.note_or_ctl, velocity);
            }

            break;
        }

        case Event::Type::NOTE_PRESSURE:
            synth.aftertouch(
                event.time_offset,
                event.channel,
                event.note_or_ctl,
                float_to_midi_byte(event.velocity_or_value)
            );
            break;

        case Event::Type::NOTE_OFF:
            synth.note_off(
                event.time_offset,
                event.channel,
                event.note_or_ctl,
                float_to_midi_byte(event.velocity_or_value)
            );
            break;

        case Event::Type::PITCH_WHEEL:
            synth.pitch_wheel_change(
                event.time_offset,
                0,
                float_to_midi_word(event.velocity_or_value)
            );
            break;

        case Event::Type::CONTROL_CHANGE:
            synth.control_change(
                event.time_offset,
                0,
                event.note_or_ctl,
                float_to_midi_byte(event.velocity_or_value)
            );
            break;

        case Event::Type::CHANNEL_PRESSURE:
            synth.channel_pressure(
                event.time_offset,
                0,
                float_to_midi_byte(event.velocity_or_value)
            );
            break;

        case Event::Type::PROGRAM_CHANGE:
            new_program = Bank::normalized_parameter_value_to_program_index(
                event.velocity_or_value
            );
            need_to_load_new_program = true;

        default:
            break;
    }
}


Midi::Byte Vst3Plugin::Processor::float_to_midi_byte(
        Number const number
) const noexcept {
    return std::min(
        (Midi::Byte)127,
        std::max((Midi::Byte)0, (Midi::Byte)std::round(number * 127.0))
    );
}


Midi::Word Vst3Plugin::Processor::float_to_midi_word(
        Number const number
) const noexcept {
    return std::min(
        (Midi::Word)16384,
        std::max((Midi::Word)0, (Midi::Word)std::round(number * 16384.0))
    );
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
        renderer.render<double>(
            (Integer)data.numSamples,
            (double**)getChannelBuffersPointer(processSetup, data.inputs[0]),
            (double**)getChannelBuffersPointer(processSetup, data.outputs[0])
        );
    } else if (processSetup.symbolicSampleSize == Vst::SymbolicSampleSizes::kSample32) {
        renderer.render<float>(
            (Integer)data.numSamples,
            (float**)getChannelBuffersPointer(processSetup, data.inputs[0]),
            (float**)getChannelBuffersPointer(processSetup, data.outputs[0])
        );
    } else {
        return;
    }
}


uint32 PLUGIN_API Vst3Plugin::Processor::getLatencySamples()
{
    return (uint32)renderer.get_latency_samples();
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

    Serializer::import_patch_in_gui_thread(synth, read_stream(state));
    synth.push_message(
        Synth::MessageType::CLEAR_DIRTY_FLAG,
        Synth::ParamId::INVALID_PARAM_ID,
        0.0,
        0
    );

    return kResultOk;
}


std::string Vst3Plugin::read_stream(IBStream* stream)
{
    /*
    Not using FStreamer::readString8(), because we need the entire string here,
    and that method stops at line breaks.
    */

    char* buffer = new char[Serializer::MAX_SIZE];
    size_t i;
    int32 bytes_read;
    char8 c;

    for (i = 0; i != Serializer::MAX_SIZE; ++i) {
        stream->read(&c, sizeof(char8), &bytes_read);

        if (bytes_read != sizeof(char8) || c == '\x00') {
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

    std::string const& serialized = Serializer::serialize(synth);
    int32 const size = serialized.size();
    int32 numBytesWritten;

    state->write((void*)serialized.c_str(), size, &numBytesWritten);

    if (numBytesWritten != size) {
        return kResultFalse;
    }

    return kResultOk;
}


Vst3Plugin::GUI::GUI(Synth& synth)
    : CPluginView(&rect),
    synth(synth),
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
    if (FIDStringsEqual(type, JS80P_VST3_GUI_PLATFORM)) {
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


void Vst3Plugin::GUI::show_if_needed()
{
    if (!isAttached()) {
        return;
    }

    initialize();
}


FUnknown* Vst3Plugin::Controller::createInstance(void* unused)
{
    return (IEditController*)new Vst3Plugin::Controller();
}


Vst3Plugin::Controller::Controller() : bank(), synth(NULL)
{
}


Vst3Plugin::Controller::~Controller()
{
}


tresult PLUGIN_API Vst3Plugin::Controller::initialize(FUnknown* context)
{
    tresult result = EditControllerEx1::initialize(context);

    if (result != kResultTrue) {
        return result;
    }

    addUnit(
        new Vst::Unit(
            STR("Root"), Vst::kRootUnitId, Vst::kNoParentUnitId, PROGRAM_LIST_ID
        )
    );

    parameters.addParameter(set_up_program_change_param());

    parameters.addParameter(
        create_midi_ctl_param(
            Synth::ControllerId::PITCH_WHEEL,
            (Vst::ParamID)Vst::ControllerNumbers::kPitchBend
        )
    );

    parameters.addParameter(
        create_midi_ctl_param(
            Synth::ControllerId::CHANNEL_PRESSURE,
            (Vst::ParamID)Vst::ControllerNumbers::kAfterTouch
        )
    );

    for (Integer cc = 0; cc != Synth::MIDI_CONTROLLERS; ++cc) {
        Midi::Controller const midi_controller = (Midi::Controller)cc;

        if (!Synth::is_supported_midi_controller(midi_controller)) {
            continue;
        }

        /*
        VST3 parameters have order-independent identifiers, so the
        backward-incompatibility problem which occurs with the sustain pedal in
        FstPlugin is unlikely to occur here. However, for the sake of
        consistency, let's put the sustain pedal at the end of the list here as
        well.
        */
        if (midi_controller == Midi::SUSTAIN_PEDAL) {
            continue;
        }

        parameters.addParameter(
            create_midi_ctl_param((Synth::ControllerId)cc, (Vst::ParamID)cc)
        );
    }

    parameters.addParameter(
        create_midi_ctl_param(
            Synth::ControllerId::SUSTAIN_PEDAL,
            (Vst::ParamID)Synth::ControllerId::SUSTAIN_PEDAL
        )
    );

    parameters.addParameter(set_up_patch_changed_param());

    return result;
}


Vst::Parameter* Vst3Plugin::Controller::set_up_program_change_param()
{
    Vst::ProgramList* program_list = new Vst::ProgramList(
        STR("Program"), PROGRAM_LIST_ID, Vst::kRootUnitId
    );

    for (size_t i = 0; i != Bank::NUMBER_OF_PROGRAMS; ++i) {
        program_list->addProgram(USTRING(bank[i].get_name().c_str()));
    }

    addProgramList(program_list);

    Vst::Parameter* program_change_param = program_list->getParameter();
    Vst::ParameterInfo& param_info = program_change_param->getInfo();

    param_info.flags &= ~Vst::ParameterInfo::kCanAutomate;
    param_info.flags |= Vst::ParameterInfo::kIsProgramChange;

    return program_change_param;
}


tresult PLUGIN_API Vst3Plugin::Controller::setParamNormalized(
        Vst::ParamID tag,
        Vst::ParamValue value
) {
    tresult result = EditControllerEx1::setParamNormalized(tag, value);

    if (result == kResultOk && tag == PROGRAM_LIST_ID) {
        JS80P_VST3_SEND_MSG(
            MSG_PROGRAM_CHANGE, setFloat, MSG_PROGRAM_CHANGE_PROGRAM, (double)value
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


Vst::RangeParameter* Vst3Plugin::Controller::set_up_patch_changed_param() const
{
    Vst::RangeParameter* param = new Vst::RangeParameter(
        USTRING("Patch Changed"),
        PATCH_CHANGED_PARAM_ID,
        USTRING("%"),
        0.0,
        100.0,
        0.0,
        0,
        Vst::ParameterInfo::kIsReadOnly,
        Vst::kRootUnitId,
        USTRING("Changed")
    );
    param->setPrecision(1);

    return param;
}


tresult PLUGIN_API Vst3Plugin::Controller::getMidiControllerAssignment(
        int32 bus_index,
        int16 channel,
        Vst::CtrlNumber midi_controller_number,
        Vst::ParamID& id
) {
    if (bus_index == 0 && midi_controller_number < Vst::kCountCtrlNumber) {
        if (
                Synth::is_supported_midi_controller((Midi::Controller)midi_controller_number)
                || midi_controller_number == Vst::ControllerNumbers::kPitchBend
                || midi_controller_number == Vst::ControllerNumbers::kAfterTouch
        ) {
            id = midi_controller_number;

            return kResultTrue;
        }
    }

    return kResultFalse;
}


tresult PLUGIN_API Vst3Plugin::Controller::connect(IConnectionPoint* other)
{
    tresult result = EditControllerEx1::connect(other);

    JS80P_VST3_SEND_MSG(MSG_CTL_READY, setInt, MSG_CTL_READY_BANK, (int64)&bank);

    return result;
}


tresult PLUGIN_API Vst3Plugin::Controller::notify(Vst::IMessage* message)
{
    if (!message) {
        return kInvalidArgument;
    }

    if (FIDStringsEqual(message->getMessageID(), MSG_SHARE_SYNTH)) {
        int64 synth_ptr;

        if (message->getAttributes()->getInt(MSG_SHARE_SYNTH_SYNTH, synth_ptr) == kResultOk) {
            synth = (Synth*)synth_ptr;

            return kResultOk;
        }
    } else if (FIDStringsEqual(message->getMessageID(), MSG_SYNTH_DIRTY)) {
        /*
        Calling setDirty(true) would suffice, the dummy parameter dance is done
        only to keep parameter behaviour in sync with the FST plugin.
        */
        Vst::ParamValue const new_value = getParamNormalized(PATCH_CHANGED_PARAM_ID) + 0.01;

        setParamNormalized(PATCH_CHANGED_PARAM_ID, new_value < 1.0 ? new_value : 0.0);
        setDirty(true);

        return kResultOk;
    }

    return EditControllerEx1::notify(message);
}


IPlugView* PLUGIN_API Vst3Plugin::Controller::createView(FIDString name)
{
    if (FIDStringsEqual(name, Vst::ViewType::kEditor)) {
        if (synth == NULL) {
            return NULL;
        }

        GUI* gui = new GUI(*synth);

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
