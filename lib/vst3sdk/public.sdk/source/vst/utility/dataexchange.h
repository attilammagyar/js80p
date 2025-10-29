//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/dataexchange.h
// Created by  : Steinberg, 06/2023
// Description : VST Data Exchange API Helper
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstdataexchange.h"

#include <functional>
#include <memory>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Helper class to provide a single API for plug-ins to transfer data from the realtime audio
 *	process to the edit controller either via the backwards compatible message handling protocol
 *	(see IMessage) or the new IDataExchangeHandler/IDataExchangeReceiver API.
 *
 *	To use this, make an instance of DataExchangeHandler a member of your IAudioProcessor class and
 *	call onConnect, onDisconnect, onActivate and onDeactivate when the processor is (dis-)connected
 *	and (de)activated. In your IAudioProcessor::process method you call getCurrentOrNewBlock () to
 *	get a block fill it with the data you want to send and then call sendCurrentBlock.
 *	See DataExchangeReceiverHandler on how to receive that data.
 */
class DataExchangeHandler
{
public:
	struct Config
	{
		/** the size of one block in bytes */
		uint32 blockSize;
		/** the number of blocks to request */
		uint32 numBlocks;
		/** the alignment of the buffer */
		uint32 alignment {32};
		/** a user defined context ID */
		DataExchangeUserContextID userContextID {0};
	};
	/** the callback will be called on setup processing to get the required configuration for the
	 * data exchange */
	using ConfigCallback = std::function<bool (Config& config, const ProcessSetup& setup)>;

	DataExchangeHandler (IAudioProcessor* processor, ConfigCallback&& callback);
	DataExchangeHandler (IAudioProcessor* processor, const ConfigCallback& callback);
	~DataExchangeHandler () noexcept;

	/** call this in AudioEffect::connect
	 *
	 *	provide the hostContext you get via AudioEffect::initiailze to this method
	 */
	void onConnect (IConnectionPoint* other, FUnknown* hostContext);

	/** call this in AudioEffect::disconnect
	 */
	void onDisconnect (IConnectionPoint* other);

	/** call this in AudioEffect::setActive(true)
	 */
	void onActivate (const Vst::ProcessSetup& setup, bool forceUseMessageHandling = false);

	/** call this in AudioEffect::setActive(false)
	 */
	void onDeactivate ();

	//--- ---------------------------------------------------------------------
	/** Get the current or a new block
	 *
	 * 	On the first call this will always return a new block, only after sendCurrentBlock or
	 *	discardCurrentBlock is called a new block will be acquired.
	 *	This may return an invalid DataExchangeBlock (check the blockID for
	 *	InvalidDataExchangeBlockID) when the queue is full.
	 *
	 *	[call only in process call]
	 */
	DataExchangeBlock getCurrentOrNewBlock ();

	/** Send the current block to the receiver
	 *
	 *	[call only in process call]
	 */
	bool sendCurrentBlock ();

	/** Discard the current block
	 *
	 *	[call only in process call]
	 */
	bool discardCurrentBlock ();

	/** Enable or disable the acquiring of new blocks (per default it is enabled)
	 *
	 *	If you disable this then the getCurrentOrNewBlock will always return an invalid block.
	 *
	 *	[call only in process call]
	 */
	void enable (bool state);

	/** Ask if enabled
	 *
	 *	[call only in process call]
	 */
	bool isEnabled () const;
//------------------------------------------------------------------------

private:
	DataExchangeHandler (IAudioProcessor* processor);
	struct Impl;
	std::unique_ptr<Impl> impl;
};

//------------------------------------------------------------------------
/** Helper class to provide a single API for plug-ins to transfer data from the realtime audio
 *	process to the edit controller either via the message handling protocol (see IMessage) or the
 *	new IDataExchangeHandler/IDataExchangeReceiver API.
 *
 *	This is the other side of the DataExchangeHandler on the edit controller side. Make this a
 *	member of your edit controller and call onMessage for every IMessage you get via
 *	IConnectionPoint::notify. Your edit controller must implement the IDataExchangeReceiver
 *	interface.
 */
class DataExchangeReceiverHandler
{
public:
	DataExchangeReceiverHandler (IDataExchangeReceiver* receiver);
	~DataExchangeReceiverHandler () noexcept;

	/** call this for every message you receive via IConnectionPoint::notify
	 *
	 *	@return	true if the message was handled
	 */
	bool onMessage (IMessage* msg);

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};

//------------------------------------------------------------------------
inline bool operator!= (const DataExchangeBlock& lhs, const DataExchangeBlock& rhs)
{
	return lhs.data != rhs.data || lhs.size != rhs.size || lhs.blockID != rhs.blockID;
}

//------------------------------------------------------------------------
inline bool operator== (const DataExchangeBlock& lhs, const DataExchangeBlock& rhs)
{
	return lhs.data == rhs.data && lhs.size == rhs.size && lhs.blockID == rhs.blockID;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
