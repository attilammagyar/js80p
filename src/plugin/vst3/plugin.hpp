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

#ifndef JS80P__PLUGIN__VST3__PLUGIN_HPP
#define JS80P__PLUGIN__VST3__PLUGIN_HPP

#include <string>
#include <vector>

#include <pluginterfaces/gui/iplugview.h>
#include <pluginterfaces/vst/ivstmessage.h>
#include <pluginterfaces/vst/ivstmidicontrollers.h>
#include <public.sdk/source/common/pluginview.h>
#include <public.sdk/source/main/pluginfactory.h>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

#include "gui/gui.hpp"

#include "js80p.hpp"
#include "synth.hpp"


using namespace Steinberg;


namespace JS80P
{

extern GUI::PlatformData* platform_data;


class Vst3Plugin
{
    public:
        static constexpr char const* MSG_CTL_READY = "JS80PCtl";
        static constexpr char const* MSG_SHARE_SYNTH = "JS80PSynth";
        static constexpr char const* MSG_SHARE_SYNTH_ATTR = "Synth";

        class Event
        {
            public:
                enum Type {
                    UNDEFINED = 0,
                    NOTE_ON = 1,
                    NOTE_PRESSURE = 2,
                    NOTE_OFF = 3,
                    PITCH_WHEEL = 4,
                    CONTROL_CHANGE = 5,
                };

                Event();
                Event(
                    Type const type,
                    Seconds const time_offset,
                    Midi::Byte const note_or_ctl,
                    Number const velocity_or_value
                );
                Event(Event const& event);
                Event(Event const&& event);

                Event& operator=(Event const& event) noexcept;
                Event& operator=(Event const&& event) noexcept;
                bool operator<(Event const& event) const noexcept;

                Seconds time_offset;
                Number velocity_or_value;
                Type type;
                Midi::Byte note_or_ctl;
        };

        class Processor : public Vst::AudioEffect
        {
            public:
                static FUID const ID;

                static FUnknown* createInstance(void* unused);

                Processor();

                tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
                tresult PLUGIN_API setBusArrangements(
                    Vst::SpeakerArrangement* inputs,
                    int32 numIns,
                    Vst::SpeakerArrangement* outputs,
                    int32 numOuts
                ) SMTG_OVERRIDE;

                tresult PLUGIN_API connect(IConnectionPoint* other) SMTG_OVERRIDE;
                tresult PLUGIN_API notify(Vst::IMessage* message) SMTG_OVERRIDE;

                tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) SMTG_OVERRIDE;
                tresult PLUGIN_API setupProcessing(Vst::ProcessSetup& setup) SMTG_OVERRIDE;
                tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE;
                tresult PLUGIN_API process(Vst::ProcessData& data) SMTG_OVERRIDE;

                uint32 PLUGIN_API getTailSamples () SMTG_OVERRIDE;

                tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
                tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;

            private:
                static constexpr Integer ROUND_MASK = 0x7fff;

                void share_synth() noexcept;
                void block_rendered() noexcept;

                void collect_param_change_events(Vst::ProcessData& data) noexcept;
                void collect_note_events(Vst::ProcessData& data) noexcept;
                void process_events() noexcept;
                void process_event(Event const event) noexcept;

                void generate_samples(Vst::ProcessData& data) noexcept;

                template<typename NumberType>
                void generate_samples(
                    Integer const sample_count,
                    NumberType** samples
                ) noexcept;

                Sample const* const* render_next_round(Integer const sample_count) noexcept;

                std::string read_stream(
                    IBStream* stream,
                    Integer const max_size
                ) const;

                Synth synth;
                Integer round;
                std::vector<Event> events;
        };

        class Controller;

        class GUI : public CPluginView
        {
            public:
                GUI(Controller* controller);

                virtual ~GUI();

                tresult PLUGIN_API isPlatformTypeSupported(FIDString type) SMTG_OVERRIDE;
                tresult PLUGIN_API canResize() SMTG_OVERRIDE;

                virtual void attachedToParent() override;
                virtual void removedFromParent() override;

                void set_synth(Synth* synth);

            private:
                static ViewRect const rect;

                void show_if_needed();

                Controller* controller;
                Synth* synth;
                JS80P::GUI* gui;
        };

        class Controller : public Vst::EditController, public Vst::IMidiMapping
        {
            public:
                static FUID const ID;

                static FUnknown* createInstance(void* unused);

                Controller();

                virtual ~Controller();

                tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
                tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;

                tresult PLUGIN_API connect(IConnectionPoint* other) SMTG_OVERRIDE;
                tresult PLUGIN_API notify(Vst::IMessage* message) SMTG_OVERRIDE;

                tresult PLUGIN_API getMidiControllerAssignment(
                    int32 busIndex,
                    int16 channel,
                    Vst::CtrlNumber midiControllerNumber,
                    Vst::ParamID& id
                ) SMTG_OVERRIDE;

                IPlugView* PLUGIN_API createView(FIDString name) SMTG_OVERRIDE;

            private:
                Vst::RangeParameter* create_midi_ctl_param(
                    Synth::ControllerId const controller_id,
                    Vst::ParamID const param_id
                ) const;

                Synth* synth;

            public:
                OBJ_METHODS(Controller, EditController)
                DEFINE_INTERFACES
                    DEF_INTERFACE(IMidiMapping)
                    END_DEFINE_INTERFACES(EditController)
                REFCOUNT_METHODS(EditController)
        };
};

}

#endif
