//How to build
//gcc -o fbv3 main.c fbv3_store.c fbv3.c max7219.c -lwiringPi -lusb-1.0


#include "fbv3.h"
#include <wiringPi.h>
#include "max7219.h"
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

/*defines*/

/// Mapping to wPi from raspberry pi 4b and pi zero w pinout on header
/// Mapping can be seen by running command "gpio readall" - ouput below
/*

 +-----+-----+---------+------+---+-Pi ZeroW-+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 |     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |
 |   2 |   8 |   SDA.1 |   IN | 1 |  3 || 4  |   |      | 5v      |     |     |
 |   3 |   9 |   SCL.1 |   IN | 1 |  5 || 6  |   |      | 0v      |     |     |
 |   4 |   7 | GPIO. 7 |   IN | 1 |  7 || 8  | 0 | IN   | TxD     | 15  | 14  |
 |     |     |      0v |      |   |  9 || 10 | 1 | IN   | RxD     | 16  | 15  |
 |  17 |   0 | GPIO. 0 |   IN | 0 | 11 || 12 | 0 | IN   | GPIO. 1 | 1   | 18  |
 |  27 |   2 | GPIO. 2 |   IN | 0 | 13 || 14 |   |      | 0v      |     |     |
 |  22 |   3 | GPIO. 3 |   IN | 0 | 15 || 16 | 0 | IN   | GPIO. 4 | 4   | 23  |
 |     |     |    3.3v |      |   | 17 || 18 | 0 | IN   | GPIO. 5 | 5   | 24  |
 |  10 |  12 |    MOSI |   IN | 0 | 19 || 20 |   |      | 0v      |     |     |
 |   9 |  13 |    MISO |   IN | 0 | 21 || 22 | 0 | IN   | GPIO. 6 | 6   | 25  |
 |  11 |  14 |    SCLK |   IN | 0 | 23 || 24 | 1 | IN   | CE0     | 10  | 8   |
 |     |     |      0v |      |   | 25 || 26 | 1 | IN   | CE1     | 11  | 7   |
 |   0 |  30 |   SDA.0 |   IN | 1 | 27 || 28 | 1 | IN   | SCL.0   | 31  | 1   |
 |   5 |  21 | GPIO.21 |   IN | 1 | 29 || 30 |   |      | 0v      |     |     |
 |   6 |  22 | GPIO.22 |   IN | 1 | 31 || 32 | 0 | IN   | GPIO.26 | 26  | 12  |
 |  13 |  23 | GPIO.23 |   IN | 0 | 33 || 34 |   |      | 0v      |     |     |
 |  19 |  24 | GPIO.24 |   IN | 0 | 35 || 36 | 0 | IN   | GPIO.27 | 27  | 16  |
 |  26 |  25 | GPIO.25 |   IN | 0 | 37 || 38 | 0 | IN   | GPIO.28 | 28  | 20  |
 |     |     |      0v |      |   | 39 || 40 | 0 | IN   | GPIO.29 | 29  | 21  |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 */

//Used inputs as lined up on board physically (top to bottom, left to right)
//Row 1
#define BANK_UP_PIN      7  /*Pin 7*/
#define BANK_DOWN_PIN    0  /*Pin 11*/
//Row 2
#define COMPRESSOR_PIN   2  /*Pin 13*/
#define FX1_PIN          3  /*Pin 15*/
#define FX2_PIN          21 /*Pin 29*/
#define FX3_PIN          22 /*Pin 31*/
#define REVERB_PIN       23 /*Pin 33*/
//Row 3
#define A_PIN            24 /*Pin 35*/
#define B_PIN            25 /*Pin 37*/
#define C_PIN            1 /*Pin 12*/
#define D_PIN            4 /*Pin 16*/
#define ALT_PIN          5 /*Pin 18*/ 

#ifdef FUTURE_MAYBE
//Not used
#define VOLUME_PIN       99/*Pin NA*/
#define EQUALIZER_PIN    99/*Pin NA*/
#define GATE_PIN         99/*Pin NA*/
#define WAH_PIN          99/*Pin NA*/
#endif

//Pins to control led's
#define MAX7219_DIN  27 /*Pin 36*/
#define MAX7219_CLK  28 /*Pin 38*/
#define MAX7219_LD   29 /*Pin 40*/


/// Used to debounce the buttons
#define DEBOUNCE_DELAY 100 /*100 ms*/

/*prototypes*/
void setup_gpio(void);
bool gpio_to_fbv3_effect(const bool latching);
void fbv3_effect_to_led_status(const bool is_event);
bool gpio_process(const int pin, const enum effects effect, int8_t * const p_state, const bool latching);

/// @brief Main program entry
int main(int argc, char *argv[])
{
    bool b_run = false;

    //setup io using wirePi
    setup_gpio();
    
    // Init the display driver
    max7219_init(digitalWrite, 
                 delayMicroseconds, 
                 MAX7219_DIN,
                 MAX7219_CLK,
                 MAX7219_LD,
                 MAX7219_DECODE_NONE,
                 INTENSITY_8,
                 SCAN_LIMIT_2);

    // Led test
    // blink the board a few times
    for(int i = 0; i < 3; i++)
    {
        max7219_set_display_test(TEST_ON);
        delay(500);
        max7219_set_display_test(TEST_OFF);
        delay(500);
    }

    //init the foot board clone (handles USB comms too)
    b_run = fbv3_init();

    while(b_run)
    {
        if(fbv3_ready())
        {
            //poll the gpio and add to command queue
            bool b_event = gpio_to_fbv3_effect(false);

            // poor man's debounce
            if(b_event)
            {
                delay(DEBOUNCE_DELAY);
            }

            // set the LED status
            fbv3_effect_to_led_status(b_event);
        }
               
        //process anything in the queue
        b_run = fbv3_process();
    }
    
    fbv3_close();

    max7219_set_mode(MODE_SHUTDOWN);

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
    pinMode(COMPRESSOR_PIN, INPUT);
    pinMode(REVERB_PIN, INPUT);
    pinMode(BANK_UP_PIN, INPUT);
    pinMode(BANK_DOWN_PIN, INPUT);
    pinMode(A_PIN, INPUT);
    pinMode(B_PIN, INPUT);
    pinMode(C_PIN, INPUT);
    pinMode(D_PIN, INPUT);
    pinMode(ALT_PIN, INPUT);

    #ifdef FUTURE_MAYBE
    pinMode(VOLUME_PIN, INPUT);
    pinMode(WAH_PIN, INPUT);
    pinMode(EQUALIZER_PIN, INPUT);
    pinMode(GATE_PIN, INPUT);
    #endif

    pinMode(MAX7219_DIN, OUTPUT);
    pinMode(MAX7219_CLK, OUTPUT);
    pinMode(MAX7219_LD, OUTPUT);

    // use pull downs, no floating pins
    pullUpDnControl(FX3_PIN, PUD_DOWN);
    pullUpDnControl(FX2_PIN, PUD_DOWN);
    pullUpDnControl(FX1_PIN, PUD_DOWN);
    pullUpDnControl(COMPRESSOR_PIN, PUD_DOWN);
    pullUpDnControl(REVERB_PIN, PUD_DOWN);
    pullUpDnControl(BANK_UP_PIN, PUD_DOWN);
    pullUpDnControl(BANK_DOWN_PIN, PUD_DOWN);
    pullUpDnControl(A_PIN, PUD_DOWN);
    pullUpDnControl(B_PIN, PUD_DOWN);
    pullUpDnControl(C_PIN, PUD_DOWN);
    pullUpDnControl(D_PIN, PUD_DOWN);
    pullUpDnControl(ALT_PIN, PUD_DOWN);

    #ifdef FUTURE_MAYBE
    pullUpDnControl(VOLUME_PIN, PUD_DOWN);
    pullUpDnControl(EQUALIZER_PIN, PUD_DOWN);
    pullUpDnControl(GATE_PIN, PUD_DOWN);
    pullUpDnControl(WAH_PIN, PUD_DOWN);
    #endif
}

/// @brief Handle GPIO presses 
///
/// @param pin the physical pin as defined by wirePi interface
/// @param effect the effect to update
/// @param p_state  the current state of the effect 
/// @param latching is the switch latching or momentary
///
/// @return True if an event happened
bool gpio_process(const int pin, const enum effects effect, int8_t * const p_state, const bool latching)
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
bool gpio_to_fbv3_effect(const bool latching)
{
    bool ret = false;
    struct fbv3_state * p_fbv3_states = fbv3_get_states();

    ret |= gpio_process(FX3_PIN, EFFECTS_FX3, &p_fbv3_states->fx3_state, latching);
    ret |= gpio_process(FX2_PIN, EFFECTS_FX2, &p_fbv3_states->fx2_state, latching);
    ret |= gpio_process(FX1_PIN, EFFECTS_FX1, &p_fbv3_states->fx1_state, latching);
    ret |= gpio_process(COMPRESSOR_PIN, EFFECTS_COMPRESSOR, &p_fbv3_states->compressor_state, latching);
    ret |= gpio_process(REVERB_PIN, EFFECTS_REVERB, &p_fbv3_states->reverb_state, latching);
    ret |= gpio_process(BANK_UP_PIN, EFFECTS_BANK_UP, &p_fbv3_states->bank_up_state, latching);
    ret |= gpio_process(BANK_DOWN_PIN, EFFECTS_BANK_DOWN, &p_fbv3_states->bank_down_state, latching);
    ret |= gpio_process(A_PIN, EFFECTS_A, &p_fbv3_states->a_state, latching);
    ret |= gpio_process(B_PIN, EFFECTS_B, &p_fbv3_states->b_state, latching);
    ret |= gpio_process(C_PIN, EFFECTS_C, &p_fbv3_states->c_state, latching);
    ret |= gpio_process(D_PIN, EFFECTS_D, &p_fbv3_states->d_state, latching);
    ret |= gpio_process(ALT_PIN, EFFECTS_ALT, &p_fbv3_states->alt_state, latching);
    
    #ifdef FUTURE_MAYBE
    ret |= gpio_process(WAH_PIN, EFFECTS_WAH, &p_fbv3_states->wah_state, latching);
    ret |= gpio_process(EQUALIZER_PIN, EFFECTS_EQUALIZER, &p_fbv3_states->equalizer_state, latching);
    ret |= gpio_process(GATE_PIN, EFFECTS_GATE, &p_fbv3_states->gate_state, latching);
    ret |= gpio_process(VOLUME_PIN, EFFECTS_VOLUME, &p_fbv3_states->volume_state, latching);
    #endif
    return ret;
}

/// @brief Updates LED status based on effect status
///
/// @note LED mapping, per schematic
///                AMP: CMP  FX1  FX2  FX3  RVB   A    B    C    D   ALT
///               CODE: FX1, FX2, FX3, FX4, FX5,  A,   B,   C,   D,  ALT
/// MAX7219(digit/seg): 0-DP 0-G  0-F  0-E  0-D   0-C  0-B  0-A  1-B  1A
void fbv3_effect_to_led_status(const bool is_event)
{
    if(is_event)
    {
        struct fbv3_state * p_fbv3_states = fbv3_get_states();

        max7219_set_digit_segment(DIGIT_0, SEG_E, p_fbv3_states->fx3_state > 0 ? true : false);
        max7219_set_digit_segment(DIGIT_0, SEG_F, p_fbv3_states->fx2_state > 0 ? true : false);
        max7219_set_digit_segment(DIGIT_0, SEG_G, p_fbv3_states->fx1_state > 0 ? true : false);
        max7219_set_digit_segment(DIGIT_0, SEG_DP, p_fbv3_states->compressor_state > 0 ? true : false);
        max7219_set_digit_segment(DIGIT_0, SEG_D, p_fbv3_states->reverb_state > 0 ? true : false);
        //bank up, no led
        //bank down, no led
        max7219_set_digit_segment(DIGIT_0, SEG_A, p_fbv3_states->a_state > 0 ? true : false);
        max7219_set_digit_segment(DIGIT_0, SEG_C, p_fbv3_states->a_state > 0 ? true : false);
        max7219_set_digit_segment(DIGIT_0, SEG_B, p_fbv3_states->b_state > 0 ? true : false);
        max7219_set_digit_segment(DIGIT_0, SEG_A, p_fbv3_states->c_state > 0 ? true : false);
        max7219_set_digit_segment(DIGIT_1, SEG_B, p_fbv3_states->d_state > 0 ? true : false);
        max7219_set_digit_segment(DIGIT_1, SEG_A, p_fbv3_states->alt_state > 0 ? true : false);

        #ifdef FUTURE_MAYBE
        max7219_set_digit_segment(SEG_?, p_fbv3_states->wah_state > 0 ? true : false);
        max7219_set_digit_segment(SEG_?, p_fbv3_states->equalizer_state > 0 ? true : false);
        max7219_set_digit_segment(SEG_?, p_fbv3_states->gate_state > 0 ? true : false);
        max7219_set_digit_segment(SEG_?, p_fbv3_states->volume_state > 0 ? true : false);
        #endif
    }
}
