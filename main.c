//How to build
//gcc -o fbv3 main.c fbv3_store.c fbv3.c -lwiringPi -lusb-1.0


#include "fbv3.h"
#include <wiringPi.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

/*defines*/

/// Mapping to wPi from raspberry pi 4b and pi zero w pinout on header
/// Mapping can be seen by running command "gpio readall"
#define FX3_PIN         0 /*Pin 11*/
#define FX2_PIN         2 /*Pin 13*/
#define FX1_PIN         3 /*Pin 15*/
#define VOLUME_PIN     23 /*Pin 33*/
#define COMPRESSOR_PIN 24 /*Pin 35*/
#define EQUALIZER_PIN  25 /*Pin 37*/
#define GATE_PIN       27 /*Pin 36*/
#define REVERB_PIN     28 /*Pin 38*/
#define WAH_PIN        29 /*Pin 40*/
#define BANK_UP_PIN     1 /*Pin 12*/
#define BANK_DOWN_PIN   4 /*Pin 16*/

/// Used to debounce the buttons
#define DEBOUNCE_DELAY 100 /*100 ms*/

/*prototypes*/
void setup_gpio(void);
bool gpio_to_fbv3_effect(bool latching);
bool gpio_process(int pin, enum effects effect, int8_t * p_state, bool latching);

/// @brief Main program entry
int main(int argc, char *argv[])
{
    bool b_run = false;

    //setup io using wirePi
    setup_gpio();
    
    //init the foot board clone (handles USB comms too)
    b_run = fbv3_init();

    while(b_run)
    {
        if(fbv3_ready())
        {
          //poll the gpio and add to command queue
          bool b_event = gpio_to_fbv3_effect(false);

          // poor debounce
          if(b_event)
          {
              delay(DEBOUNCE_DELAY);
          }
        }
               
        //process anything in the queue
        b_run = fbv3_process();
    }
    
    fbv3_close();

    return 0;
}

/// @brief Sets up all GPIO using wringPi module
void setup_gpio()
{
    wiringPiSetup();

    // set inputs
    pinMode(FX3_PIN, INPUT);
    pinMode(FX2_PIN, INPUT);
    pinMode(FX1_PIN, INPUT);
    pinMode(VOLUME_PIN, INPUT);
    pinMode(COMPRESSOR_PIN, INPUT);
    pinMode(EQUALIZER_PIN, INPUT);
    pinMode(GATE_PIN, INPUT);
    pinMode(REVERB_PIN, INPUT);
    pinMode(WAH_PIN, INPUT);
    pinMode(BANK_UP_PIN, INPUT);
    pinMode(BANK_DOWN_PIN, INPUT);

    // use pull ups, no floating pins
    pullUpDnControl(FX3_PIN, PUD_DOWN);
    pullUpDnControl(FX2_PIN, PUD_DOWN);
    pullUpDnControl(FX1_PIN, PUD_DOWN);
    pullUpDnControl(VOLUME_PIN, PUD_DOWN);
    pullUpDnControl(COMPRESSOR_PIN, PUD_DOWN);
    pullUpDnControl(EQUALIZER_PIN, PUD_DOWN);
    pullUpDnControl(GATE_PIN, PUD_DOWN);
    pullUpDnControl(REVERB_PIN, PUD_DOWN);
    pullUpDnControl(WAH_PIN, PUD_DOWN);
    pullUpDnControl(BANK_UP_PIN, PUD_DOWN);
    pullUpDnControl(BANK_DOWN_PIN, PUD_DOWN);
}

/// @brief Handle GPIO presses 
///
/// @param pin the physical pin as defined by wirePi interface
/// @param effect the effect to update
/// @param p_state  the current state of the effect 
/// @param latching is the switch latching or momentary
///
/// @return True if an event happened
bool gpio_process(int pin, enum effects effect, int8_t * p_state, bool latching)
{
    bool ret = false; 
    int8_t pin_state = (int8_t)digitalRead(pin);
    
    if(latching  && (pin_state != *p_state))
    {
        *p_state = pin_state; //set,unset
        fbv3_update_effect_switch(effect, (*p_state == HIGH) ? true:false); 
        ret = true;
    }
    else if (!latching && (pin_state == HIGH)) //pedal pressed
    {
        *p_state = (*p_state == HIGH) ? LOW:HIGH; //toggle
        fbv3_update_effect_switch(effect, (*p_state == HIGH) ? true:false); 
        ret = true;
    }
    else
    {
        //do nothing
    }

    return ret;
}

/// @brief Reads GPIO and issues effect commands to the FBV3 module
///
/// @return True if event happened
bool gpio_to_fbv3_effect(bool latching)
{
    bool ret = false;
    struct fbv3_state * p_fbv3_states = fbv3_get_states();

    ret |= gpio_process(FX3_PIN, EFFECTS_FX3, &p_fbv3_states->fx3_state, latching);
    ret |= gpio_process(FX2_PIN, EFFECTS_FX2, &p_fbv3_states->fx2_state, latching);
    ret |= gpio_process(FX1_PIN, EFFECTS_FX1, &p_fbv3_states->fx1_state, latching);
    ret |= gpio_process(VOLUME_PIN, EFFECTS_VOLUME, &p_fbv3_states->volume_state, latching);
    ret |= gpio_process(COMPRESSOR_PIN, EFFECTS_COMPRESSOR, &p_fbv3_states->compressor_state, latching);
    ret |= gpio_process(EQUALIZER_PIN, EFFECTS_EQUALIZER, &p_fbv3_states->equalizer_state, latching);
    ret |= gpio_process(GATE_PIN, EFFECTS_GATE, &p_fbv3_states->gate_state, latching);
    ret |= gpio_process(REVERB_PIN, EFFECTS_REVERB, &p_fbv3_states->reverb_state, latching);
    ret |= gpio_process(WAH_PIN, EFFECTS_WAH, &p_fbv3_states->wah_state, latching);
    ret |= gpio_process(BANK_UP_PIN, EFFECTS_BANK_UP, &p_fbv3_states->bank_up_state, latching);
    ret |= gpio_process(BANK_DOWN_PIN, EFFECTS_BANK_DOWN, &p_fbv3_states->bank_down_state, latching);

    return ret;
}

