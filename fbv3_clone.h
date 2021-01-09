#ifndef FBV3_CLONE_H
#define FBV3_CLONE_H

#include <stdbool.h>

/*enums and defines*/

/// Enum of all effects
enum effects
{
    EFFECTS_NONE,
    EFFECTS_MODULATION,
    EFFECTS_DELAY,
    EFFECTS_STOMP,
    EFFECTS_VOLUME,
    EFFECTS_COMPRESSOR,
    EFFECTS_EQUALIZER,
    EFFECTS_GATE,
    EFFECTS_REVERB,
    EFFECTS_WAH,
    EFFECTS_BANK_UP,
    EFFECTS_BANK_DOWN,
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
  int bank_up_state;
  int bank_down_state;
};

bool fbv3_init(void);
bool fbv3_ready(void);
bool fbv3_process(void);
void fbv3_close(void);
void fbv3_update_effect_switch(enum effects effect, bool on_off); 
void fbv3_increment_preset(void);
void fbv3_decrement_preset(void);
struct fbv3_state * fbv3_get_states(void);

#endif //FBV3_CLONE_H
