/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef AUDIO_MIDIDRV_H
#define AUDIO_MIDIDRV_H

#include "common/scummsys.h"
#include "common/str.h"
#include "common/stream.h"
#include "common/timer.h"
#include "common/array.h"

class MidiChannel;

/**
 * Music types that music drivers can implement and engines can rely on.
 */
enum MusicType {
	MT_INVALID = -1,	// Invalid output
	MT_AUTO = 0,		// Auto
	MT_NULL,			// Null
	MT_PCSPK,			// PC Speaker
	MT_PCJR,			// PCjr
	MT_CMS,				// CMS
	MT_ADLIB,			// AdLib
	MT_C64,				// C64
	MT_AMIGA,			// Amiga
	MT_APPLEIIGS,		// Apple IIGS
	MT_TOWNS,			// FM-TOWNS
	MT_PC98,			// PC98
	MT_GM,				// General MIDI
	MT_MT32,			// MT-32
	MT_GS				// Roland GS
};

/**
 * A set of flags to be passed to detectDevice() which can be used to
 * specify what kind of music driver is preferred / accepted.
 *
 * The flags (except for MDT_PREFER_MT32 and MDT_PREFER_GM) indicate whether a
 * given driver type is acceptable. E.g. the TOWNS music driver could be
 * returned by detectDevice if and only if MDT_TOWNS is specified.
 *
 * MDT_PREFER_MT32 and MDT_PREFER_GM indicate the MIDI device type to use when
 * no device is selected in the music options, or when the MIDI device selected
 * does not match the requirements of a game engine. With these flags, more
 * priority is given to an MT-32 device, or a GM device respectively.
 *
 * @todo Rename MidiDriverFlags to MusicDriverFlags
 */
enum MidiDriverFlags {
	MDT_NONE        = 0,
	MDT_PCSPK       = 1 << 0,		// PC Speaker: Maps to MT_PCSPK and MT_PCJR
	MDT_CMS         = 1 << 1,		// Creative Music System / Gameblaster: Maps to MT_CMS
	MDT_PCJR        = 1 << 2,		// Tandy/PC Junior driver
	MDT_ADLIB       = 1 << 3,		// AdLib: Maps to MT_ADLIB
	MDT_C64         = 1 << 4,
	MDT_AMIGA       = 1 << 5,
	MDT_APPLEIIGS   = 1 << 6,
	MDT_TOWNS       = 1 << 7,		// FM-TOWNS: Maps to MT_TOWNS
	MDT_PC98        = 1 << 8,		// FM-TOWNS: Maps to MT_PC98
	MDT_MIDI        = 1 << 9,		// Real MIDI
	MDT_PREFER_MT32 = 1 << 10,		// MT-32 output is preferred
	MDT_PREFER_GM   = 1 << 11,		// GM output is preferred
	MDT_PREFER_FLUID= 1 << 12		// FluidSynth driver is preferred
};

/**
 * TODO: Document this, give it a better name.
 */
class MidiDriver_BASE {
public:
	MidiDriver_BASE();

	virtual ~MidiDriver_BASE();

	/**
	 * Output a packed midi command to the midi stream.
	 * The 'lowest' byte (i.e. b & 0xFF) is the status
	 * code, then come (if used) the first and second
	 * opcode.
	 */
	virtual void send(uint32 b) = 0;

	/**
	 * Send a MIDI command from a specific source. If the MIDI driver
	 * does not support multiple sources, the source parameter is
	 * ignored.
	 */
	virtual void send(int8 source, uint32 b) { send(b); }

	/**
	 * Output a midi command to the midi stream. Convenience wrapper
	 * around the usual 'packed' send method.
	 *
	 * Do NOT use this for sysEx transmission; instead, use the sysEx()
	 * method below.
	 */
	void send(byte status, byte firstOp, byte secondOp);
	
	/**
	 * Send a MIDI command from a specific source. If the MIDI driver
	 * does not support multiple sources, the source parameter is
	 * ignored.
	 */
	void send(int8 source, byte status, byte firstOp, byte secondOp);

	/**
	 * Transmit a SysEx to the MIDI device.
	 *
	 * The given msg MUST NOT contain the usual SysEx frame, i.e.
	 * do NOT include the leading 0xF0 and the trailing 0xF7.
	 *
	 * Furthermore, the maximal supported length of a SysEx
	 * is 264 bytes. Passing longer buffers can lead to
	 * undefined behavior (most likely, a crash).
	 */
	virtual void sysEx(const byte *msg, uint16 length) { }

	/**
	 * Transmit a SysEx to the MIDI device and return the necessary
	 * delay until the next SysEx event in milliseconds.
	 *
	 * This can be used to implement an alternate delay method than the
	 * OSystem::delayMillis function used by most sysEx implementations.
	 * Note that not every driver needs a delay, or supports this method.
	 * In this case, 0 is returned and the driver itself will do a delay 
	 * if necessary.
	 *
	 * For information on the SysEx data requirements, see the sysEx method.
	 */
	virtual uint16 sysExNoDelay(const byte *msg, uint16 length) { sysEx(msg, length); return 0; }

	// TODO: Document this.
	virtual void metaEvent(byte type, byte *data, uint16 length) { }

	/**
	 * Send a meta event from a specific source. If the MIDI driver
	 * does not support multiple sources, the source parameter is
	 * ignored.
	 */
	virtual void metaEvent(int8 source, byte type, byte *data, uint16 length) { metaEvent(type, data, length); }
protected:

	/**
	 * Enables midi dumping to a 'dump.mid' file and to debug messages on screen
	 * It's set by '--dump-midi' command line parameter
	 */
	bool _midiDumpEnable;

	/** Used for MIDI dumping delta calculation */
	uint32 _prevMillis;

	/** Stores all MIDI events, will be written to disk after an engine quits */
	Common::Array<byte> _midiDumpCache;

	/** Initialize midi dumping mechanism, called only if enabled */
	void midiDumpInit();

	/** Handles MIDI file variable length dumping */
	int midiDumpVarLength(const uint32 &delta);

	/** Handles MIDI file time delta dumping */
	void midiDumpDelta();

	/** Performs dumping of MIDI commands, called only if enabled */
	void midiDumpDo(uint32 b);

	/** Performs dumping of MIDI SysEx commands, called only if enabled */
	void midiDumpSysEx(const byte *msg, uint16 length);

	/** Writes the captured MIDI events to disk, called only if enabled */
	void midiDumpFinish();

};

/**
 * Abstract MIDI Driver Class
 *
 * @todo Rename MidiDriver to MusicDriver
 */
class MidiDriver : public MidiDriver_BASE {
public:
	/**
	 * The device handle.
	 *
	 * The value 0 is reserved for an invalid device for now.
	 * TODO: Maybe we should use -1 (i.e. 0xFFFFFFFF) as
	 * invalid device?
	 */
	typedef uint32 DeviceHandle;

	enum DeviceStringType {
		kDriverName,
		kDriverId,
		kDeviceName,
		kDeviceId
	};

	static Common::String musicType2GUIO(uint32 musicType);

	/** Create music driver matching the given device handle, or NULL if there is no match. */
	static MidiDriver *createMidi(DeviceHandle handle);

	/** Returns device handle based on the present devices and the flags parameter. */
	static DeviceHandle detectDevice(int flags);

	/** Find the music driver matching the given driver name/description. */
	static DeviceHandle getDeviceHandle(const Common::String &identifier);

	/** Check whether the device with the given handle is available. */
	static bool checkDevice(DeviceHandle handle);

	/** Get the music type matching the given device handle, or MT_AUTO if there is no match. */
	static MusicType getMusicType(DeviceHandle handle);

	/** Get the device description string matching the given device handle and the given type. */
	static Common::String getDeviceString(DeviceHandle handle, DeviceStringType type);

	/** Common operations to be done by all drivers on start of send */
	void midiDriverCommonSend(uint32 b);

	/** Common operations to be done by all drivers on start of sysEx */
	void midiDriverCommonSysEx(const byte *msg, uint16 length);

protected:
	// True if stereo panning should be reversed.
	bool _reversePanning;
	// True if GS percussion channel volume should be scaled to match MT-32 volume.
	bool _scaleGSPercussionVolumeToMT32;
	// The currently selected GS instrument bank / variation for each channel.
	byte _gsBank[16];

private:
	// If detectDevice() detects MT32 and we have a preferred MT32 device
	// we use this to force getMusicType() to return MT_MT32 so that we don't
	// have to rely on the 'True Roland MT-32' config manager setting (since nobody
	// would possibly think about activating 'True Roland MT-32' when he has set
	// 'Music Driver' to '<default>')
	static bool _forceTypeMT32;

public:
	MidiDriver() : _reversePanning(false),
					_scaleGSPercussionVolumeToMT32(false) {
		memset(_gsBank, 0, sizeof(_gsBank));
	}
	virtual ~MidiDriver() { }

	static const byte _mt32ToGm[128];
	static const byte _gmToMt32[128];
	static const byte _mt32DefaultInstruments[8];
	static const byte _mt32DefaultPanning[8];
	// Map for correcting Roland GS drumkit numbers.
	static const uint8 _gsDrumkitFallbackMap[128];

	/**
	 * Error codes returned by open.
	 * Can be converted to a string with getErrorName().
	 */
	enum {
		MERR_CANNOT_CONNECT = 1,
//		MERR_STREAMING_NOT_AVAILABLE = 2,
		MERR_DEVICE_NOT_AVAILABLE = 3,
		MERR_ALREADY_OPEN = 4
	};

	enum {
//		PROP_TIMEDIV = 1,
		PROP_OLD_ADLIB = 2,
		PROP_CHANNEL_MASK = 3,
		// HACK: Not so nice, but our SCUMM AdLib code is in audio/
		PROP_SCUMM_OPL3 = 4
	};

	/**
	 * Open the midi driver.
	 * @return 0 if successful, otherwise an error code.
	 */
	virtual int open() = 0;

	/**
	 * Check whether the midi driver has already been opened.
	 */
	virtual bool isOpen() const = 0;

	/** Close the midi driver. */
	virtual void close() = 0;

	/** Get or set a property. */
	virtual uint32 property(int prop, uint32 param) { return 0; }

	/** Retrieve a string representation of an error code. */
	static const char *getErrorName(int error_code);

	// HIGH-LEVEL SEMANTIC METHODS
	virtual void setPitchBendRange(byte channel, uint range) {
		send(0xB0 | channel, 101, 0);
		send(0xB0 | channel, 100, 0);
		send(0xB0 | channel,   6, range);
		send(0xB0 | channel,  38, 0);
		send(0xB0 | channel, 101, 127);
		send(0xB0 | channel, 100, 127);
	}

	/**
	 * Initializes the MT-32 MIDI device. The device will be reset and, 
	 * if the parameter is specified, set up for General MIDI data.
	 * @param initForGM True if the MT-32 should be initialized for GM mapping
	 */
	void initMT32(bool initForGM);

	/**
	 * Send a Roland MT-32 reset sysEx to the midi device.
	 */
	void sendMT32Reset();

	/**
	 * Initializes the General MIDI device. The device will be reset.
	 * If the initForMT32 parameter is specified, the device will be set up for
	 * MT-32 MIDI data. If the device supports Roland GS, the enableGS
	 * parameter can be specified for enhanced GS MT-32 compatiblity.
	 * @param initForMT32 True if the device should be initialized for MT-32 mapping
	 * @param enableGS True if the device should be initialized for GS MT-32 mapping
	 */
	void initGM(bool initForMT32, bool enableGS);

	/**
	 * Send a General MIDI reset sysEx to the midi device.
	 */
	void sendGMReset();

	virtual void sysEx_customInstrument(byte channel, uint32 type, const byte *instr) { }

	// Timing functions - MidiDriver now operates timers
	virtual void setTimerCallback(void *timer_param, Common::TimerManager::TimerProc timer_proc) = 0;

	/** The time in microseconds between invocations of the timer callback. */
	virtual uint32 getBaseTempo() = 0;

	// Channel allocation functions
	virtual MidiChannel *allocateChannel() = 0;
	virtual MidiChannel *getPercussionChannel() = 0;

	// Allow an engine to supply its own soundFont data. This stream will be destroyed after use.
	virtual void setEngineSoundFont(Common::SeekableReadStream *soundFontData) { }

	// Does this driver accept soundFont data?
	virtual bool acceptsSoundFontData() { return false; }

protected:
	/**
	 * Checks if the currently selected GS bank / instrument variation
	 * on the specified channel is valid for the specified patch.
	 * If this is not the case, the correct bank will be returned which
	 * can be set by sending a bank select message. If no correction is
	 * needed, 0xFF will be returned.
	 * This emulates the fallback functionality of the Roland SC-55 v1.2x,
	 * on which some games rely to correct wrong bank selects.
	 */
	byte correctInstrumentBank(byte outputChannel, byte patchId);
};

class MidiChannel {
public:
	virtual ~MidiChannel() {}

	virtual MidiDriver *device() = 0;
	virtual byte getNumber() = 0;
	virtual void release() = 0;

	virtual void send(uint32 b) = 0; // 4-bit channel portion is ignored

	// Regular messages
	virtual void noteOff(byte note) = 0;
	virtual void noteOn(byte note, byte velocity) = 0;
	virtual void programChange(byte program) = 0;
	virtual void pitchBend(int16 bend) = 0; // -0x2000 to +0x1FFF

	// Control Change messages
	virtual void controlChange(byte control, byte value) = 0;
	virtual void modulationWheel(byte value) { controlChange(1, value); }
	virtual void volume(byte value) { controlChange(7, value); }
	virtual void panPosition(byte value) { controlChange(10, value); }
	virtual void pitchBendFactor(byte value) = 0;
	virtual void transpose(int8 value) {}
	virtual void detune(byte value) { controlChange(17, value); }
	virtual void priority(byte value) { }
	virtual void sustain(bool value) { controlChange(64, value ? 1 : 0); }
	virtual void effectLevel(byte value) { controlChange(91, value); }
	virtual void chorusLevel(byte value) { controlChange(93, value); }
	virtual void allNotesOff() { controlChange(123, 0); }

	// SysEx messages
	virtual void sysEx_customInstrument(uint32 type, const byte *instr) = 0;
};

#endif
