#ifndef VBA_WIN32_INPUT_H
#define VBA_WIN32_INPUT_H

#pragma once

#include "../common/inputGlobal.h"

#define JOYCONFIG_MESSAGE (WM_USER + 1000)

// Toggle the auto-fire state of a single button.  Used by the Auto-Fire menu
// commands (Alt+0-9) and by the configurable Auto-Fire keys in DirectInput.
extern void VBAToggleAutoFireButton(u32 mask);

class Input
{
public:
	Input() {};
	virtual ~Input() {};

	virtual bool initialize() = 0;

	// Re-reads the attached devices.  The list is built once at startup, so a
	// controller that woke up, was plugged in, or was paired afterwards is not
	// in it; the setup dialogs call this before they start capturing.
	virtual void refreshDevices() {}

	// Reports controllers that were found since the last call, for the ports
	// that tell the user about it.  Called once the main window exists: the
	// devices are enumerated before there is anything to show a message on.
	virtual void announceDevices() {}

	// The controllers that are attached right now, for the list the setup
	// dialog shows.  n counts from 0; deviceIndex is the number the bindings
	// use, the one getKeyName() prints as "Joy <n>", and name is what the
	// device calls itself.
	virtual int  gamepadCount() { return 0; }
	virtual bool gamepadInfo(int n, int &deviceIndex, CString &name) { return false; }

	// How far a stick has to travel before it counts as pushed, in percent of
	// the way from the centre to the end.  DirectInput's own DIPROP_DEADZONE
	// is ignored by HID pads, so the comparison happens here.
	virtual int  getDeadzone() { return 0; }
	virtual void setDeadzone(int percent) {}

	virtual bool readDevices() = 0;
	virtual u32 readDevice(int which, bool sensor) = 0;
	virtual CString getKeyName(LONG_PTR key) = 0;
	virtual void checkKeys()    = 0;
	virtual void checkDevices() = 0;
	virtual void activate()     = 0;
	virtual void loadSettings() = 0;
	virtual void saveSettings() = 0;
};

#endif
