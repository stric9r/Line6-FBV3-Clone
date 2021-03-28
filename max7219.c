#include "max7219.h"
#include "debug.h"

/// Minimum clock width for high and low is 50ns
/// To be very conservative, using the current value would 
/// take 64 ms to write all 64 segments individually
#define CLK_WIDTH_US 500

static uint8_t decode_mode_store;

static uint8_t digits_segment_store[DIGIT_MAX];

struct comm_str
{
    void(*f_write)(int,int);
    void(*f_delay_us)(unsigned int);
    uint8_t data_out;
    uint8_t clk;
    uint8_t load;
};

static struct comm_str comm;

// Helpers
uint8_t get_digit_addr(enum max7219_digits const digit);


/// Translate enum to register address of digit
/// If invalid digit, then the address for NO OPERATION returned
uint8_t get_digit_addr(enum max7219_digits const digit)
{
    uint8_t addr = ADDR_NO_OP;
    switch(digit)
    {
        case DIGIT_0:
            addr = ADDR_DIG_0;
            break;
        case DIGIT_1:
            addr = ADDR_DIG_1;
            break;
        case DIGIT_2:
            addr = ADDR_DIG_2;
            break;
        case DIGIT_3:
            addr = ADDR_DIG_3;
            break;
        case DIGIT_4:
            addr = ADDR_DIG_4;
            break;
        case DIGIT_5:
            addr = ADDR_DIG_5;
            break;
        case DIGIT_6:
            addr = ADDR_DIG_6;
            break;
        case DIGIT_7:
            addr = ADDR_DIG_7;
            break;
        case DIGIT_MAX:
        default:
            break;
    }
}

/// Setup the pins and write function pointer
/// 
/// When needed to change control settings, it is easier to just call init again.
void max7219_init(void(*f_write)(int,int),
                    void(*f_delay_us)(unsigned int),
                    uint8_t const data_out_pin,
                    uint8_t const clk_pin, 
                    uint8_t const load_pin,
                    uint8_t const decode_mode,
                    enum max7219_intensities const intensity,
                    enum max7219_scan_limits const scan_limit)
{
    debug_print("MAX7219 - Init\n");

    if(f_write == NULL || f_delay_us == NULL)
    {
        debug_print("MAX7219 - invalid function pointer(s), No writes will work!\n");
    }

    comm.f_write = f_write;
    comm.f_delay_us = f_delay_us;
    comm.data_out = data_out_pin;
    comm.clk = clk_pin;
    comm.load = load_pin;

    // set it all to 0
    comm.f_write(comm.data_out, 0);
    comm.f_write(comm.clk, 0);
    comm.f_write(comm.load, 0);

    decode_mode_store = 0;

    // init storage
    for(int i = 0; i < DIGIT_MAX; i++)
    {
        digits_segment_store[i] = 0;
    }

    // Setup the display driver - order is important
    max7219_set_mode(MODE_SHUTDOWN);
    max7219_set_decode_mode(decode_mode);
    max7219_set_intensity(intensity);
    max7219_set_scan_limit(scan_limit);
    max7219_set_display_test(TEST_OFF);
    max7219_clear_all(); //clear all after mode determined
    max7219_set_mode(MODE_NORMAL);
}

/// Update a digit using BCD 
/// Note that the mode must be decode
void max7219_set_digit_bcd(enum max7219_digits const digit, uint8_t bcd, bool const decimal_point)
{
    debug_print("MAX7219 - set digit bcd, digit: 0x%x, bcd: 0x%x\n", (uint8_t)digit, bcd);
    uint8_t const addr = get_digit_addr(digit);

    // for BCD the decimal point is the last bit
    if(decimal_point)
    {
        bcd |= 0x80;
    }

    max7219_write(addr, bcd);
}

void max7219_set_digit_segment(enum max7219_digits const digit, 
                               enum max7219_segments segment, 
                               bool state)
{
    debug_print("MAX7219 - set digit segment, digit: 0x%x, seg: 0x%x, state: 0x%x\n", (uint8_t)digit, (uint8_t)segment, (uint8_t)state);

    if(digit < DIGIT_MAX)
    {
        uint8_t addr = get_digit_addr(digit);
        uint8_t seg = (uint8_t)segment;

        // set or clear segment
        if(state)
        {
            digits_segment_store[digit] |= (1 << seg);
        }
        else
        {
            digits_segment_store[digit] &= ~(1 << seg);
        }
        

        max7219_write(addr, digits_segment_store[digit]);
    }
}

/// Clear all digits
void max7219_clear_all(void)
{
    debug_print("MAX7219 - clear all digits\n");


    bool is_bcd = (decode_mode_store & MAX7219_DECODE_DIGIT_0) > 0;
    debug_print("Digit 0 is_bcd 0x%x\n", (uint8_t)is_bcd);
    max7219_write(ADDR_DIG_0, is_bcd ? MAX7219_DECODE_BCD_BLANK : 0);

    is_bcd = (decode_mode_store & MAX7219_DECODE_DIGIT_1) > 0;
    debug_print("Digit 1 is_bcd 0x%x\n", (uint8_t)is_bcd);
    max7219_write(ADDR_DIG_1, is_bcd ? MAX7219_DECODE_BCD_BLANK : 0);

    is_bcd = (decode_mode_store & MAX7219_DECODE_DIGIT_2) > 0;
    debug_print("Digit 2 is_bcd 0x%x\n", (uint8_t)is_bcd);
    max7219_write(ADDR_DIG_2, is_bcd ? MAX7219_DECODE_BCD_BLANK : 0);

    is_bcd = (decode_mode_store & MAX7219_DECODE_DIGIT_3) > 0;
    debug_print("Digit 3 is_bcd 0x%x\n", (uint8_t)is_bcd);
    max7219_write(ADDR_DIG_3, is_bcd ? MAX7219_DECODE_BCD_BLANK : 0);

    is_bcd = (decode_mode_store & MAX7219_DECODE_DIGIT_4) > 0;
    debug_print("Digit 4 is_bcd 0x%x\n", (uint8_t)is_bcd);
    max7219_write(ADDR_DIG_4, is_bcd ? MAX7219_DECODE_BCD_BLANK : 0);

    is_bcd = (decode_mode_store & MAX7219_DECODE_DIGIT_5) > 0;
    debug_print("Digit 5 is_bcd 0x%x\n", (uint8_t)is_bcd);
    max7219_write(ADDR_DIG_5, is_bcd ? MAX7219_DECODE_BCD_BLANK : 0);

    is_bcd = (decode_mode_store & MAX7219_DECODE_DIGIT_6) > 0;
    debug_print("Digit 6 is_bcd 0x%x\n", (uint8_t)is_bcd);
    max7219_write(ADDR_DIG_6, is_bcd ? MAX7219_DECODE_BCD_BLANK : 0);

    is_bcd = (decode_mode_store & MAX7219_DECODE_DIGIT_7) > 0;
    debug_print("Digit 7 is_bcd 0x%x\n", (uint8_t)is_bcd);
    max7219_write(ADDR_DIG_7, is_bcd ? MAX7219_DECODE_BCD_BLANK : 0);

    // Clear storage
    for(int i = 0; i < DIGIT_MAX; i++)
    {
        digits_segment_store[i] = 0;
    }
}

/// Clear single digit
void max7219_clear_digit(enum max7219_digits const digit)
{
    debug_print("MAX7219 - clear digit: 0x%x\n", (uint8_t)digit);

    if(digit < DIGIT_MAX)
    {
        uint8_t const addr = get_digit_addr(digit);

        //magically the digit address lines up with the decode_mode register offsets
        bool is_bcd = (decode_mode_store & (1 << addr)) > 0;
        debug_print("Digit %u is_bcd 0x%x\n", (uint8_t)digit, (uint8_t)is_bcd);
        max7219_write(addr, is_bcd ? MAX7219_DECODE_BCD_BLANK : 0);
        
        // Clear storage
        digits_segment_store[digit] = 0;
    }
}

/// Set the test mode
void max7219_set_display_test(enum max7219_display_tests const mode)
{
    debug_print("MAX7219 - set display test mode: 0x%x\n", (uint8_t)mode);
    max7219_write(ADDR_DISPLAY_TEST, (uint8_t)mode);
}

/// Set the decode mode
void max7219_set_decode_mode(uint8_t const mode)
{
    decode_mode_store = (uint8_t)mode;
    debug_print("MAX7219 - set decode mode and clear: 0x%x\n", (uint8_t)mode);
    max7219_write(ADDR_DECODE_MODE, mode);

    // clear all when mode set
    // because all off bcd is 0xFF, all off segment is 0x00
    max7219_clear_all();
}

/// Set the LED intensity
void max7219_set_intensity(enum max7219_intensities const intensity)
{
    debug_print("MAX7219 - set intensity: 0x%x\n", (uint8_t)intensity);
    max7219_write(ADDR_INTENSITY, (uint8_t)intensity);
}

/// Set the scan limit
void max7219_set_scan_limit(enum max7219_scan_limits const scan_limit)
{
    debug_print("MAX7219 - set scan limit: 0x%x\n", (uint8_t)scan_limit);
    max7219_write(ADDR_SCAN_LIMIT, (uint8_t)scan_limit);
}

/// Set the operating mode
void max7219_set_mode(enum max7219_modes const mode)
{
    debug_print("MAX7219 - set mode: 0x%x\n", (uint8_t)mode);
    max7219_write(ADDR_MODE, (uint8_t)mode);
}
/// General write command for all operations using the MAX7219
/// This is blocking
void max7219_write(uint8_t const  addr, uint8_t const  data)
{
    if(comm.f_write != NULL && comm.f_delay_us != NULL)
    {
        //only 16 bits
        uint8_t const packet[2] = {addr, data};

        debug_print("MAX7219 write - addr: 0x%x, data: 0x%x\n", addr, data );

        // load line low to push data into register
        // bit 15 to 0
        comm.f_write(comm.load, 0);
        
        comm.f_write(comm.clk, 0);
        comm.f_delay_us(CLK_WIDTH_US);

        /// MSB first which is address (just stored differently here for ease)
        //debug_print("\n    Sending...");
        //debug_print("\n    order - :  1  2  3  4  5  6  7  8");
        for(int8_t i = 0; i < 2; i++)
        {
            
            //debug_print("\n    %s  - :", i == 0 ? "addr" : "data");
            // loop through bits and toggle clk, msb first
            for(int8_t j = 7; j >= 0; j--)
            {
                comm.f_write(comm.clk, 0);
                comm.f_delay_us(CLK_WIDTH_US);
                
                uint8_t bit = (packet[i] >> j) & 0x01;
                comm.f_write(comm.data_out, bit);
                
                comm.f_delay_us(50); //give it a little extra to be set
                                      //litle buggy when the rising edges happen at same time.

                comm.f_write(comm.clk, 1); // data shifted on rising edge of clock
                comm.f_delay_us(CLK_WIDTH_US);

                //debug_print("  %d", bit );
            }
        }
        //debug_print("\n\n");

        comm.f_write(comm.load, 1); // load data into chip 
                                    // load is not depended on clock in MAX7219
        comm.f_delay_us(CLK_WIDTH_US);

        //clean up
        comm.f_write(comm.data_out, 0);
        comm.f_write(comm.load, 0);
        comm.f_write(comm.clk, 0); 
        

    }
    else
    {
        debug_print("MAX7219 - init failed or never called\n");
    }
    
}
