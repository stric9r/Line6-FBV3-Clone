//How to build
//gcc -o fbv3 main.c fbv3_clone.c -lwiringPi -lusb-1.0


#include "fbv3_clone.h"
#include <wiringPi.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

/*defines*/

/// Mapping to wPi from raspberry pi 4b pinout on header
/// Mapping can be seen by running command "gpio readall"
#define DELAY_PIN       0 /*Pin 11*/
#define MODULATION_PIN  2 /*Pin 13*/
#define STOMP_PIN       3 /*Pin 15*/
#define VOLUME_PIN     23 /*Pin 33*/
#define COMPRESSOR_PIN 24 /*Pin 35*/
#define EQUALIZER_PIN  25 /*Pin 37*/
#define GATE_PIN       27 /*Pin 36*/
#define REVERB_PIN     28 /*Pin 38*/
#define WAH_PIN        29 /*Pin 40*/
#define BANK_UP_PIN     1 /*Pin 12*/
#define BANK_DOWN_PIN   4 /*Pin 16*/

/*prototypes*/
void setup_gpio(void);
bool gpio_to_fbv3_effect(bool latching);
bool gpio_process(int pin, enum effects effect, int * state, bool latching);

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
          bool event_trigger = gpio_to_fbv3_effect(false);

          // poor debounce
          if(event_trigger)
          {
              delay(50); //50 milliseconds
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
    pinMode(DELAY_PIN, INPUT);
    pinMode(MODULATION_PIN, INPUT);
    pinMode(STOMP_PIN, INPUT);
    pinMode(VOLUME_PIN, INPUT);
    pinMode(COMPRESSOR_PIN, INPUT);
    pinMode(EQUALIZER_PIN, INPUT);
    pinMode(GATE_PIN, INPUT);
    pinMode(REVERB_PIN, INPUT);
    pinMode(WAH_PIN, INPUT);
    pinMode(BANK_UP_PIN, INPUT);
    pinMode(BANK_DOWN_PIN, INPUT);

    // use pull ups, no floating pins
    pullUpDnControl(DELAY_PIN, PUD_DOWN);
    pullUpDnControl(MODULATION_PIN, PUD_DOWN);
    pullUpDnControl(STOMP_PIN, PUD_DOWN);
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
/// @return True if an event happened
bool gpio_process(int pin, enum effects effect, int * state, bool latching)
{
    bool ret = false; 
    int pin_state = digitalRead(pin);
    
    if(latching  && (pin_state != *state))
    {
        *state = pin_state; //set,unset
        fbv3_update_effect_switch(effect, (*state == HIGH) ? true:false); 
        ret = true;
    }
    else if (!latching && (pin_state == HIGH)) //pedal pressed
    {
        *state = (*state == HIGH) ? LOW:HIGH; //toggle
        fbv3_update_effect_switch(effect, (*state == HIGH) ? true:false); 
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
    struct fbv3_state * fbv3_states = fbv3_get_states();

    ret |= gpio_process(DELAY_PIN, EFFECTS_DELAY, &fbv3_states->delay_state, latching);
    ret |= gpio_process(MODULATION_PIN, EFFECTS_MODULATION, &fbv3_states->modulation_state, latching);
    ret |= gpio_process(STOMP_PIN, EFFECTS_STOMP, &fbv3_states->stomp_state, latching);
    ret |= gpio_process(VOLUME_PIN, EFFECTS_VOLUME, &fbv3_states->volume_state, latching);
    ret |= gpio_process(COMPRESSOR_PIN, EFFECTS_COMPRESSOR, &fbv3_states->compressor_state, latching);
    ret |= gpio_process(EQUALIZER_PIN, EFFECTS_EQUALIZER, &fbv3_states->equalizer_state, latching);
    ret |= gpio_process(GATE_PIN, EFFECTS_GATE, &fbv3_states->gate_state, latching);
    ret |= gpio_process(REVERB_PIN, EFFECTS_REVERB, &fbv3_states->reverb_state, latching);
    ret |= gpio_process(WAH_PIN, EFFECTS_WAH, &fbv3_states->wah_state, latching);
    ret |= gpio_process(BANK_UP_PIN, EFFECTS_BANK_UP, &fbv3_states->bank_up_state, latching);
    ret |= gpio_process(BANK_DOWN_PIN, EFFECTS_BANK_DOWN, &fbv3_states->bank_down_state, latching);

    return ret;
}

