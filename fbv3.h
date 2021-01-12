#ifndef FBV3_H
#define FBV3_H

#include <stdbool.h>
#include <stdint.h>

/*enums and defines*/

/// Enum of all effects
enum effects
{
    EFFECTS_NONE,
    EFFECTS_FX3,
    EFFECTS_FX2,
    EFFECTS_FX1,
    EFFECTS_VOLUME,
    EFFECTS_COMPRESSOR,
    EFFECTS_EQUALIZER,
    EFFECTS_GATE,
    EFFECTS_REVERB,
    EFFECTS_WAH,
    EFFECTS_BANK_UP,
    EFFECTS_BANK_DOWN,
    EFFECTS_A,
    EFFECTS_B,
    EFFECTS_C,
    EFFECTS_D,
    EFFECTS_MAX
};

/// Keeps the current state of the effects on/off
struct fbv3_state
{
  int8_t fx3_state;
  int8_t fx2_state;
  int8_t fx1_state;
  int8_t volume_state;
  int8_t compressor_state;
  int8_t equalizer_state;
  int8_t gate_state;
  int8_t reverb_state;
  int8_t wah_state;
  int8_t bank_up_state;
  int8_t bank_down_state;
  int8_t a_state;
  int8_t b_state;
  int8_t c_state;
  int8_t d_state;
};

bool fbv3_init(void);
bool fbv3_ready(void);
bool fbv3_process(void);
void fbv3_close(void);
void fbv3_update_effect_switch(enum effects effect, bool on_off); 
void fbv3_increment_preset(void);
void fbv3_decrement_preset(void);
struct fbv3_state * fbv3_get_states(void);

#endif //FBV3_H
