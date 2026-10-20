#include "CoreOptions.h"

// Zero-initialised during static initialisation (see the note in the header).
// The frontend's constructor applies its own defaults and the persisted
// settings afterwards, exactly as it did when these were its own fields.
CoreOptions g_core = {};
