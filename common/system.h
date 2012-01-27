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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef COMMON_SYSTEM_H
#define COMMON_SYSTEM_H

#include "common/scummsys.h"
#include "common/noncopyable.h"
#include "common/list.h" // For OSystem::getSupportedFormats()
#include "graphics/pixelformat.h"

namespace Audio {
class Mixer;
}

namespace Graphics {
struct Surface;
class PixelBuffer;
}

namespace Common {
class EventManager;
struct Rect;
class SaveFileManager;
class SearchSet;
class String;
#if defined(USE_TASKBAR)
class TaskbarManager;
#endif
#if defined(USE_UPDATES)
class UpdateManager;
#endif
class TimerManager;
class SeekableReadStream;
class WriteStream;
class HardwareKeySet;
}

class AudioCDManager;
class FilesystemFactory;
class PaletteManager;

/**
 * A structure describing time and date. This is a clone of struct tm
 * from time.h. We roll our own since not all systems provide time.h.
 * We also do not imitate all files of struct tm, only those we
 * actually need.
 *
 * @note For now, the members are named exactly as in struct tm to ease
 * the transition.
 */
struct TimeDate {
	int tm_sec;     ///< seconds (0 - 60)
	int tm_min;     ///< minutes (0 - 59)
	int tm_hour;    ///< hours (0 - 23)
	int tm_mday;    ///< day of month (1 - 31)
	int tm_mon;     ///< month of year (0 - 11)
	int tm_year;    ///< year - 1900
};

namespace LogMessageType {

enum Type {
	kInfo,
	kError,
	kWarning,
	kDebug
};

} // End of namespace LogMessageType

/**
 * Interface for ScummVM backends. If you want to port ScummVM to a system
 * which is not currently covered by any of our backends, this is the place
 * to start. ScummVM will create an instance of a subclass of this interface
 * and use it to interact with the system.
 *
 * In particular, a backend provides a video surface for ScummVM to draw in;
 * methods to create timers, to handle user input events,
 * control audio CD playback, and sound output.
 */
class OSystem : Common::NonCopyable {
protected:
	OSystem();
	virtual ~OSystem();

protected:
	/**
	 * @name Module slots
	 *
	 * For backend authors only, the following pointers (= "slots) to various
	 * subsystem managers / factories / etc. can and should be set to
	 * a suitable instance of the respective type.
	 *
	 * For some of the slots, a default instance is set if your backend
	 * does not do so. For details, please look at the documentation of
	 * each slot.
	 *
	 * A backend may setup slot values in its initBackend() method,
	 * its constructor or somewhere in between. But it must a slot's value
	 * no later than in its initBackend() implementation, because
	 * OSystem::initBackend() will create any default instances if
	 * none has been set yet (and for other slots, will verify that
	 * one has been set; if not, an error may be generated).
	 */
	//@{

	/**
	 * No default value is provided for _audiocdManager by OSystem.
	 * However, BaseBackend::initBackend() does set a default value
	 * if none has been set before.
	 *
	 * @note _audiocdManager is deleted by the OSystem destructor.
	 */
	AudioCDManager *_audiocdManager;

	/**
	 * No default value is provided for _eventManager by OSystem.
	 * However, BaseBackend::initBackend() does set a default value
	 * if none has been set before.
	 *
	 * @note _eventManager is deleted by the OSystem destructor.
	 */
	Common::EventManager *_eventManager;

	/**
	 * No default value is provided for _timerManager by OSystem.
	 *
	 * @note _timerManager is deleted by the OSystem destructor.
	 */
	Common::TimerManager *_timerManager;

	/**
	 * No default value is provided for _savefileManager by OSystem.
	 *
	 * @note _savefileManager is deleted by the OSystem destructor.
	 */
	Common::SaveFileManager *_savefileManager;

#if defined(USE_TASKBAR)
	/**
	 * No default value is provided for _taskbarManager by OSystem.
	 *
	 * @note _taskbarManager is deleted by the OSystem destructor.
	 */
	Common::TaskbarManager *_taskbarManager;
#endif

#if defined(USE_UPDATES)
	/**
	 * No default value is provided for _updateManager by OSystem.
	 *
	 * @note _updateManager is deleted by the OSystem destructor.
	 */
	Common::UpdateManager *_updateManager;
#endif

	/**
	 * No default value is provided for _fsFactory by OSystem.
	 *
	 * Note that _fsFactory is typically required very early on,
	 * so it usually should be set in the backends constructor or shortly
	 * thereafter, and before initBackend() is called.
	 *
	 * @note _fsFactory is deleted by the OSystem destructor.
	 */
	FilesystemFactory *_fsFactory;

	//@}

public:

	/**
	 * The following method is called once, from main.cpp, after all
	 * config data (including command line params etc.) are fully loaded.
	 *
	 * @note Subclasses should always invoke the implementation of their
	 *       parent class. They should do so near the end of their own
	 *       implementation.
	 */
	virtual void initBackend();

	/**
	 * Allows the backend to perform engine specific init.
	 * Called just before the engine is run.
	 */
	virtual void engineInit() { }

	/**
	 * Allows the backend to perform engine specific de-init.
	 * Called after the engine finishes.
	 */
	virtual void engineDone() { }

	/** @name Feature flags */
	//@{

	/**
	 * A feature in this context means an ability of the backend which can be
	 * either on or off. Examples include:
	 *  - fullscreen mode
	 *  - aspect ration correction
	 *  - a virtual keyboard for text entry (on PDAs)
	 *
	 * One has to distinguish between the *availability* of a feature,
	 * which can be checked using hasFeature(), and its *state*.
	 * For example, the SDL backend *has* the kFeatureFullscreenMode,
	 * so hasFeature returns true for it. On the other hand,
	 * fullscreen mode may be active or not; this can be determined
	 * by checking the state via getFeatureState(). Finally, to
	 * switch between fullscreen and windowed mode, use setFeatureState().
	 */
	enum Feature {
		/**
		 * If supported, this feature flag can be used to switch between
		 * windowed and fullscreen mode.
		 */
		kFeatureFullscreenMode,

		/**
		 * Determine whether a virtual keyboard is too be shown or not.
		 * This would mostly be implemented by backends for hand held devices,
		 * like PocketPC, Palms, Symbian phones like the P800, Zaurus, etc.
		 */
		kFeatureVirtualKeyboard,
		kFeatureOpenGL,

		/**
		 * This feature, set to true, is a hint toward the backend to disable all
		 * key filtering/mapping, in cases where it would be beneficial to do so.
		 * As an example case, this is used in the agi engine's predictive dialog.
		 * When the dialog is displayed this feature is set so that backends with
		 * phone-like keypad temporarily unmap all user actions which leads to
		 * comfortable word entry. Conversely, when the dialog exits the feature
		 * is set to false.
		 *
		 * TODO: The word 'beneficial' above is very unclear. Beneficial to
		 * whom and for what??? Just giving an example is not enough.
		 *
		 * TODO: Fingolfin suggests that the way the feature is used can be
		 * generalized in this sense: Have a keyboard mapping feature, which the
		 * engine queries for to assign keys to actions ("Here's my default key
		 * map for these actions, what do you want them set to?").
		 */
		kFeatureDisableKeyFiltering,

		/**
		 * The presence of this feature indicates whether the displayLogFile()
		 * call is supported.
		 *
		 * This feature has no associated state.
		 */
		kFeatureDisplayLogFile
	};

	/**
	 * Determine whether the backend supports the specified feature.
	 */
	virtual bool hasFeature(Feature f) { return false; }

	/**
	 * En-/disable the specified feature. For example, this may be used to
	 * enable fullscreen mode, or to deactivate aspect correction, etc.
	 */
	virtual void setFeatureState(Feature f, bool enable) {}

	/**
	 * Query the state of the specified feature. For example, test whether
	 * fullscreen mode is active or not.
	 */
	virtual bool getFeatureState(Feature f) { return false; }

	//@}



	/**
	 * @name Graphics
	 *
	 * The way graphics work in the class OSystem are meant to make
	 * it possible for game frontends to implement all they need in
	 * an efficient manner. The downside of this is that it may be
	 * rather complicated for backend authors to fully understand and
	 * implement the semantics of the OSystem interface.
	 */
	//@{

	/**
	 * Set the size of the launcher virtual screen.
	 *
	 * @param width		the new virtual screen width
	 * @param height	the new virtual screen height
	 */
	virtual void launcherInitSize(uint width, uint height) = 0;

	virtual int getScreenChangeID() const { return 0; }

	/**
	 * Set the size of the screen.

	 *
	 * @param width			the new screen width
	 * @param height		the new screen height
	 * @param fullscreen	the new screen will be displayed in fullscreeen mode
	 */
	virtual Graphics::PixelBuffer setupScreen(int screenW, int screenH, bool fullscreen, bool accel3d) = 0;

	/**
	 * Returns the currently set virtual screen height.
	 * @see initSize
	 * @return the currently set virtual screen height
	 */
	virtual int16 getHeight() = 0;

	/**
	 * Returns the currently set virtual screen width.
	 * @see initSize
	 * @return the currently set virtual screen width
	 */
	virtual int16 getWidth() = 0;

	/**
	 * Flush the whole screen, that is render the current content of the screen
	 * framebuffer to the display.
	 *
	 */
	virtual void updateScreen() = 0;

	//@}



	/**
	 * @name Overlay
	 * In order to be able to display dialogs atop the game graphics, backends
	 * must provide an overlay mode.
	 *
	 */
	//@{

	/** Activate the overlay mode. */
	virtual void showOverlay() = 0;

	/** Deactivate the overlay mode. */
	virtual void hideOverlay() = 0;

	/**
	 * Returns the pixel format description of the overlay.
	 * @see Graphics::PixelFormat
	 */
	virtual Graphics::PixelFormat getOverlayFormat() const = 0;

	/**
	 * Reset the overlay.
	 *
	 * After calling this method while the overlay mode is active, the user
	 * should be seeing only the game graphics. How this is achieved depends
	 * on how the backend implements the overlay. Either it sets all pixels of
	 * the overlay to be transparent (when alpha blending is used).
	 */
	virtual void clearOverlay() = 0;

	/**
	 * Copy the content of the overlay into a buffer provided by the caller.
	 */
	virtual void grabOverlay(OverlayColor *buf, int pitch) = 0;

	/**
	 * Blit a graphics buffer to the overlay.
	 * In a sense, this is the reverse of grabOverlay.
	 *
	 * @note The pitch parameter actually contains the 'pixel pitch', i.e.,
	 * the number of pixels per scanline, and not as usual the number of bytes
	 * per scanline.
	 *
	 * @todo Change 'pitch' to be byte and not pixel based
	 *
	 * @param buf		the buffer containing the graphics data source
	 * @param pitch		the pixel pitch of the buffer (number of pixels in a scanline)
	 * @param x			the x coordinate of the destination rectangle
	 * @param y			the y coordinate of the destination rectangle
	 * @param w			the width of the destination rectangle
	 * @param h			the height of the destination rectangle
	 *
	 * @see copyRectToScreen
	 * @see grabOverlay
	 */
	virtual void copyRectToOverlay(const OverlayColor *buf, int pitch, int x, int y, int w, int h) = 0;

	/**
	 * Return the height of the overlay.
	 * @see getHeight
	 */
	virtual int16 getOverlayHeight() = 0;

	/**
	 * Return the width of the overlay.
	 * @see getWidth
	 */
	virtual int16 getOverlayWidth() = 0;

	//@}



	/** @name Mouse
	 * This is the lower level implementation as provided by the
	 * backends. The engines should use the Graphics::CursorManager
	 * class instead of using it directly.
	 */
	//@{

	/**
	 * Show or hide the mouse cursor.
	 *
	 * Currently the backend is not required to immediately draw the
	 * mouse cursor on showMouse(true).
	 *
	 * TODO: We might want to reconsider this fact,
	 * check Graphics::CursorManager::showMouse for some details about
	 * this.
	 *
	 * @see Graphics::CursorManager::showMouse
	 */
	virtual bool showMouse(bool visible) = 0;

	/**
	 * Lock or unlock the mouse cursor within the window.
	 *
	 * ResidualVM specific method
	 */
	virtual bool lockMouse(bool lock) = 0;

	/**
	 * Move ("warp") the mouse cursor to the specified position in virtual
	 * screen coordinates.
	 * @param x		the new x position of the mouse
	 * @param y		the new y position of the mouse
	 */
	virtual void warpMouse(int x, int y) = 0;

	/**
	 * Set the bitmap used for drawing the cursor.
	 *
	 * @param buf				the pixmap data to be used
	 * @param w					width of the mouse cursor
	 * @param h					height of the mouse cursor
	 * @param hotspotX			horizontal offset from the left side to the hotspot
	 * @param hotspotY			vertical offset from the top side to the hotspot
	 * @param keycolor			transparency color value. This should not exceed the maximum color value of the specified format.
	 *                          In case it does the behavior is undefined. The backend might just error out or simply ignore the
	 *                          value. (The SDL backend will just assert to prevent abuse of this).
	 * @param cursorTargetScale	scale factor which cursor is designed for
	 * @param format			pointer to the pixel format which cursor graphic uses (0 means CLUT8)
	 */
	virtual void setMouseCursor(const byte *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor, int cursorTargetScale = 1, const Graphics::PixelFormat *format = NULL) = 0;

	//@}



	/** @name Events and Time */
	//@{

	/** Get the number of milliseconds since the program was started. */
	virtual uint32 getMillis() = 0;

	/** Delay/sleep for the specified amount of milliseconds. */
	virtual void delayMillis(uint msecs) = 0;

	/**
	 * Get the current time and date, in the local timezone.
	 * Corresponds on many systems to the combination of time()
	 * and localtime().
	 */
	virtual void getTimeAndDate(TimeDate &t) const = 0;

	/**
	 * Return the timer manager singleton. For more information, refer
	 * to the TimerManager documentation.
	 */
	inline Common::TimerManager *getTimerManager() {
		return _timerManager;
	}

	/**
	 * Return the event manager singleton. For more information, refer
	 * to the EventManager documentation.
	 */
	inline Common::EventManager *getEventManager() {
		return _eventManager;
	}

	/**
	 * Register hardware keys with keymapper
	 *
	 * @return HardwareKeySet with all keys and recommended mappings
	 *
	 * See keymapper documentation for further reference.
	 */
	virtual Common::HardwareKeySet *getHardwareKeySet() { return 0; }

	//@}



	/**
	 * @name Mutex handling
	 * Historically, the OSystem API used to have a method which allowed
	 * creating threads. Hence mutex support was needed for thread syncing.
	 * To ease portability, though, we decided to remove the threading API.
	 * Instead, we now use timers (see setTimerCallback() and Common::Timer).
	 * But since those may be implemented using threads (and in fact, that's
	 * how our primary backend, the SDL one, does it on many systems), we
	 * still have to do mutex syncing in our timer callbacks.
	 * In addition, the sound mixer uses a mutex in case the backend runs it
	 * from a dedicated thread (as e.g. the SDL backend does).
	 *
	 * Hence backends which do not use threads to implement the timers simply
	 * can use dummy implementations for these methods.
	 */
	//@{

	typedef struct OpaqueMutex *MutexRef;

	/**
	 * Create a new mutex.
	 * @return the newly created mutex, or 0 if an error occurred.
	 */
	virtual MutexRef createMutex() = 0;

	/**
	 * Lock the given mutex.
	 *
	 * @note ScummVM code assumes that the mutex implementation supports
	 * recursive locking. That is, a thread may lock a mutex twice w/o
	 * deadlocking. In case of a multilock, the mutex has to be unlocked
	 * as many times as it was locked befored it really becomes unlocked.
	 *
	 * @param mutex	the mutex to lock.
	 */
	virtual void lockMutex(MutexRef mutex) = 0;

	/**
	 * Unlock the given mutex.
	 * @param mutex	the mutex to unlock.
	 */
	virtual void unlockMutex(MutexRef mutex) = 0;

	/**
	 * Delete the given mutex. Make sure the mutex is unlocked before you delete it.
	 * If you delete a locked mutex, the behavior is undefined, in particular, your
	 * program may crash.
	 * @param mutex	the mutex to delete.
	 */
	virtual void deleteMutex(MutexRef mutex) = 0;

	//@}



	/** @name Sound */
	//@{

	/**
	 * Return the audio mixer. For more information, refer to the
	 * Audio::Mixer documentation.
	 */
	virtual Audio::Mixer *getMixer() = 0;

	//@}



	/** @name Audio CD */
	//@{

	/**
	 * Return the audio cd manager. For more information, refer to the
	 * AudioCDManager documentation.
	 */
	inline AudioCDManager *getAudioCDManager() {
		return _audiocdManager;
	}

	//@}



	/** @name Miscellaneous */
	//@{
	/** Quit (exit) the application. */
	virtual void quit() = 0;

	/**
	 * Signals that a fatal error inside the client code has happened.
	 *
	 * This should quit the application.
	 */
	virtual void fatalError();

	/**
	 * Set a window caption or any other comparable status display to the
	 * given value. The caption must be a pure ISO LATIN 1 string. Passing a
	 * string with a different encoding may lead to unexpected behavior,
	 * even crashes.
	 *
	 * @param caption	the window caption to use, as an ISO LATIN 1 string
	 */
	virtual void setWindowCaption(const char *caption) {}

	/**
	 * Display a message in an 'on screen display'. That is, display it in a
	 * fashion where it is visible on or near the screen (e.g. in a transparent
	 * rectangle over the regular screen content; or in a message box beneath
	 * it; etc.).
	 *
	 * The message is expected to be provided in the current TranslationManager
	 * charset.
	 *
	 * @note There is a default implementation in BaseBackend which uses a
	 *       TimedMessageDialog to display the message. Hence implementing
	 *       this is optional.
	 *
	 * @param msg	the message to display on screen
	 */
	virtual void displayMessageOnOSD(const char *msg) = 0;

	/**
	 * Return the SaveFileManager, used to store and load savestates
	 * and other modifiable persistent game data. For more information,
	 * refer to the SaveFileManager documentation.
	 */
	inline Common::SaveFileManager *getSavefileManager() {
		return _savefileManager;
	}

#if defined(USE_TASKBAR)
	/**
	 * Returns the TaskbarManager, used to handle progress bars,
	 * icon overlay, tasks and recent items list on the taskbar.
	 *
	 * @return the TaskbarManager for the current architecture
	 */
	virtual Common::TaskbarManager *getTaskbarManager() {
		return _taskbarManager;
	}
#endif

#if defined(USE_UPDATES)
	/**
	 * Returns the UpdateManager, used to handle auto-updating,
	 * and updating of ScummVM in general.
	 *
	 * @return the UpdateManager for the current architecture
	 */
	virtual Common::UpdateManager *getUpdateManager() {
		return _updateManager;
	}
#endif

	/**
	 * Returns the FilesystemFactory object, depending on the current architecture.
	 *
	 * @return the FSNode factory for the current architecture
	 */
	virtual FilesystemFactory *getFilesystemFactory();

	/**
	 * Add system specific Common::Archive objects to the given SearchSet.
	 * E.g. on Unix the dir corresponding to DATA_PATH (if set), or on
	 * Mac OS X the 'Resource' dir in the app bundle.
	 *
	 * @todo Come up with a better name. This one sucks.
	 *
	 * @param s		the SearchSet to which the system specific dirs, if any, are added
	 * @param priority	the priority with which those dirs are added
	 */
	virtual void addSysArchivesToSearchSet(Common::SearchSet &s, int priority = 0) {}

	/**
	 * Open the default config file for reading, by returning a suitable
	 * ReadStream instance. It is the callers responsiblity to delete
	 * the stream after use.
	 */
	virtual Common::SeekableReadStream *createConfigReadStream();

	/**
	 * Open the default config file for writing, by returning a suitable
	 * WriteStream instance. It is the callers responsiblity to delete
	 * the stream after use.
	 *
	 * May return 0 to indicate that writing to config file is not possible.
	 */
	virtual Common::WriteStream *createConfigWriteStream();

	/**
	 * Get the default file name (or even path) where the user configuration
	 * of ScummVM will be saved.
	 * Note that not all ports may use this.
	 */
	virtual Common::String getDefaultConfigFileName();

	/**
	 * Logs a given message.
	 *
	 * It is up to the backend where to log the different messages.
	 * The backend should aim at using a non-buffered output for it
	 * so that no log data is lost in case of a crash.
	 *
	 * The default implementation outputs them on stdout/stderr.
	 *
	 * @param type    the type of the message
	 * @param message the message itself
	 */
	virtual void logMessage(LogMessageType::Type type, const char *message) = 0;

	/**
	 * Open the log file in a way that allows the user to review it,
	 * and possibly email it (or parts of it) to the ScummVM team,
	 * e.g. as part of a bug report.
	 *
	 * On a desktop operating system, this would typically launch
	 * some kind of (external) text editor / viewer.
	 * On a phone, it might also cause a context switch to another
	 * application. Finally, on some ports, it might not be supported
	 * at all, and so do nothing.
	 *
	 * The kFeatureDisplayLogFile feature flag can be used to
	 * test whether this call has been implemented by the active
	 * backend.
	 *
	 * @return true if all seems to have gone fine, false if an error occurred
	 *
	 * @note An error could mean that the log file did not exist,
	 * or the editor could not launch. However, a return value of true does
	 * not guarantee that the user actually will see the log file.
	 *
	 * @note It is up to the backend to ensure that the system is in a state
	 * that allows the user to actually see the displayed log files. This
	 * might for example require leaving fullscreen mode.
	 */
	virtual bool displayLogFile() { return false; }

	/**
	 * Returns the locale of the system.
	 *
	 * This returns the currently set up locale of the system, on which
	 * ScummVM is run.
	 *
	 * The format of the locale is language_country. These should match
	 * the POSIX locale values.
	 *
	 * For information about POSIX locales read here:
	 * http://en.wikipedia.org/wiki/Locale#POSIX-type_platforms
	 *
	 * The default implementation returns "en_US".
	 *
	 * @return locale of the system
	 */
	virtual Common::String getSystemLanguage() const;

	//@}
};


/** The global OSystem instance. Initialized in main(). */
extern OSystem *g_system;

#endif
