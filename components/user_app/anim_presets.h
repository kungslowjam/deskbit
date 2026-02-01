#ifndef ANIM_PRESETS_H
#define ANIM_PRESETS_H

// Forward declarations if needed, but we keep the include for convenience
#include "anim_manager.h"

// Initialize and register all built-in animation presets
void anim_presets_init(void);

// List of available preset animation names for reference
#define ANIM_PRESET_WINK "wink"
#define ANIM_PRESET_SURPRISE "surprise"
#define ANIM_PRESET_SKEPTIC "skeptic"
#define ANIM_PRESET_ANGRY "angry_glare"
#define ANIM_PRESET_THINKING "thinking"

#endif // ANIM_PRESETS_H
