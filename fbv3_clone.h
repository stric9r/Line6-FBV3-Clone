#ifndef FBV3_CLONE_H
#define FBV3_CLONE_H

#include <stdbool.h>

/*enums and defines*/

/// Enum of all effects
enum effects
{
    NO_EFFECT,
    MODULATION,
    DELAY,
    STOMP,
    VOLUME,
    COMPRESSOR,
    EQUALIZER,
    GATE,
    REVERB,
    WAH,
    EFFECTS_MAX
};

/// Keeps the current state of the effects on/off
struct fbv3_state
{
  int delay_state;
  int modulation_state;
  int stomp_state;
  int volume_state;
  int compressor_state;
  int equalizer_state;
  int gate_state;
  int reverb_state;
  int wah_state;
};

bool fbv3_init(void);
bool fbv3_ready(void);
bool fbv3_process(void);
void fbv3_close(void);
void fbv3_update_effect_switch(enum effects effect, bool on_off); 
struct fbv3_state * fbv3_get_states(void);

#endif //FBV3_CLONE_H
