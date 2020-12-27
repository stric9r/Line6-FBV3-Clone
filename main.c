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

/*prototype*/
void setup_gpio(void);
void gpio_to_fbv3_effect(void);

/// @breif Main program entry
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
          gpio_to_fbv3_effect();
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

    pinMode(DELAY_PIN, INPUT);
    pinMode(MODULATION_PIN, INPUT);
    pinMode(STOMP_PIN, INPUT);
    pinMode(VOLUME_PIN, INPUT);
    pinMode(COMPRESSOR_PIN, INPUT);
    pinMode(EQUALIZER_PIN, INPUT);
    pinMode(GATE_PIN, INPUT);
    pinMode(REVERB_PIN, INPUT);
    pinMode(WAH_PIN, INPUT);
}

/// @brief Reads GPIO and issues effect commands to the FBV3 module
void gpio_to_fbv3_effect(void)
{
    struct fbv3_state * fbv3_states = fbv3_get_states();
    int pin_state = LOW;
    
    pin_state = digitalRead(DELAY_PIN);
    if(pin_state != fbv3_states->delay_state)
    {
        fprintf(stderr, "delay pin %d local %d\n", pin_state, fbv3_states->delay_state);
    
        fbv3_update_effect_switch(DELAY, (pin_state == HIGH) ? true:false); 
        fbv3_states->delay_state = pin_state;
    }
    
    pin_state = digitalRead(MODULATION_PIN);
    if(pin_state != fbv3_states->modulation_state)
    {
        fprintf(stderr, "modulation pin %d local %d\n", pin_state, fbv3_states->modulation_state);
    
        fbv3_update_effect_switch(MODULATION, (pin_state == HIGH) ? true:false); 
        fbv3_states->modulation_state = pin_state;
    }

    pin_state = digitalRead(STOMP_PIN);
    if(pin_state != fbv3_states->stomp_state)
    {
        fprintf(stderr, "stomp pin %d local %d\n", pin_state, fbv3_states->stomp_state);
        
        fbv3_update_effect_switch(STOMP, (pin_state == HIGH) ? true:false); 
        fbv3_states->stomp_state = pin_state;
    }
    
    pin_state = digitalRead(VOLUME_PIN);
    if(pin_state != fbv3_states->volume_state)
    {
        fprintf(stderr, "volume pin %d local %d\n", pin_state, fbv3_states->volume_state);
        
        fbv3_update_effect_switch(VOLUME, (pin_state == HIGH) ? true:false); 
        fbv3_states->volume_state = pin_state;
    }
  
    pin_state = digitalRead(COMPRESSOR_PIN);
    if(pin_state != fbv3_states->compressor_state)
    {
        fprintf(stderr, "compressor pin %d local %d\n", pin_state, fbv3_states->compressor_state);
        
        fbv3_update_effect_switch(COMPRESSOR, (pin_state == HIGH) ? true:false); 
        fbv3_states->compressor_state = pin_state;
    }
    
    pin_state = digitalRead(EQUALIZER_PIN);
    if(pin_state != fbv3_states->equalizer_state)
    {
        fprintf(stderr, "equalizer pin %d local %d\n", pin_state, fbv3_states->equalizer_state);
        
        fbv3_update_effect_switch(EQUALIZER, (pin_state == HIGH) ? true:false); 
        fbv3_states->equalizer_state = pin_state;
    }

    pin_state = digitalRead(REVERB_PIN);
    if(pin_state != fbv3_states->reverb_state)
    {
        fprintf(stderr, "reverb pin %d local %d\n", pin_state, fbv3_states->reverb_state);
        
        fbv3_update_effect_switch(REVERB, (pin_state == HIGH) ? true:false); 
        fbv3_states->reverb_state = pin_state;
    }

    pin_state = digitalRead(WAH_PIN);
    if(pin_state != fbv3_states->wah_state)
    {
        fprintf(stderr, "wah pin %d local %d\n", pin_state, fbv3_states->wah_state);
        
        fbv3_update_effect_switch(WAH, (pin_state == HIGH) ? true:false); 
        fbv3_states->wah_state = pin_state;
    }

}