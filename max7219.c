#include "max7219.h"

/// Minimum clock width for high and low is 50ns
/// To be very conservative, using the current value would 
/// take 64 ms to write all 64 segments individually
#define CLK_WIDTH_US 500


// packet size 16 bits
#define MAX_PACKET_SIZE 2

// data clocked in using MSB first
#define START_IDX 1
#define END_IDX 0

union segments
{
    uint8_t all;

    unsigned g :1;
    unsigned f :1;
    unsigned e :1;
    unsigned d :1;
    unsigned c :1;
    unsigned b :1;
    unsigned a :1;
    unsigned dp:1;
};

static union segments digits_segment_store[DIGIT_MAX];

struct comm_str
{
    void(*f_write)(int,int);
    void(*f_delay_us)(int);
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
void max7219_init(void(*f_write)(int,int),
                    void(*f_delay_us)(int),
                    uint8_t const data_out_pin,
                    uint8_t const clk_pin, 
                    uint8_t const load_pin)
{
    comm.f_write = f_write;
    comm.f_delay_us = f_delay_us;
    comm.data_out = data_out_pin;
    comm.clk = clk_pin;
    comm.load = load_pin;

    // init storage
    for(int i = 0; i < DIGIT_MAX; i++)
    {
        digits_segment_store[i].all = 0;
    }
}

/// Update a digit using BCD 
/// Note that the mode must be decode
void max7219_set_digit_bcd(enum max7219_digits const digit, uint8_t const bcd)
{
    uint8_t const addr = get_digit_addr(digit);
    max7219_write(addr, bcd);
}

void max7219_set_digit_segment(enum max7219_digits const digit, 
                               enum max7219_segments segment, 
                               bool state)
{
    if(digit < DIGIT_MAX)
    {
        uint8_t addr = get_digit_addr(digit);

        switch(segment)
        {
            case SEG_G:
                digits_segment_store[digit].g = state ? 0 : 1;
                break;
            case SEG_F:
                digits_segment_store[digit].f = state ? 0 : 1;
                break;
            case SEG_E:
                digits_segment_store[digit].e = state ? 0 : 1;
                break;
            case SEG_D:
                digits_segment_store[digit].d = state ? 0 : 1;
                break;
            case SEG_C:
                digits_segment_store[digit].c = state ? 0 : 1;
                break;
            case SEG_B:
                digits_segment_store[digit].b = state ? 0 : 1;
                break;
            case SEG_A:
                digits_segment_store[digit].a = state ? 0 : 1;
                break;
            case SEG_DP:
                digits_segment_store[digit].dp = state ? 0 : 1;
                break;
        }

        max7219_write(addr, digits_segment_store[digit].all);
    }
}

/// Clear all digits
void max7219_clear_all(void)
{
    max7219_write(ADDR_DIG_0, MAX7219_DECODE_BLANK);
    max7219_write(ADDR_DIG_1, MAX7219_DECODE_BLANK);
    max7219_write(ADDR_DIG_2, MAX7219_DECODE_BLANK);
    max7219_write(ADDR_DIG_3, MAX7219_DECODE_BLANK);
    max7219_write(ADDR_DIG_4, MAX7219_DECODE_BLANK);
    max7219_write(ADDR_DIG_5, MAX7219_DECODE_BLANK);
    max7219_write(ADDR_DIG_6, MAX7219_DECODE_BLANK);
    max7219_write(ADDR_DIG_7, MAX7219_DECODE_BLANK);

    // Clear storage
    for(int i = 0; i < DIGIT_MAX; i++)
    {
        digits_segment_store[i].all = 0;
    }
}

/// Clear single digit
void max7219_clear_digit(enum max7219_digits const digit)
{
    if(digit < DIGIT_MAX)
    {
        uint8_t const addr = get_digit_addr(digit);
        max7219_write(addr, MAX7219_DECODE_BLANK);
        
        // Clear storage
        digits_segment_store[digit].all = 0;
    }
}

/// Set the decode mode
void max7219_set_decode_mode(uint8_t const mode)
{
    max7219_write(ADDR_DECODE_MODE, mode);
}

/// Set the LED intensity
void max7219_set_intensity(enum max7219_intensities const intensity)
{
    max7219_write(ADDR_INTENSITY, (uint8_t)intensity);
}

/// Set the scan limit
void max7219_set_scan_limit(enum max7219_scan_limits const scan_limit)
{
    max7219_write(ADDR_SCAN_LIMIT, (uint8_t)scan_limit);
}

/// Set the operating mode
void max7219_set_mode(enum max7219_modes const mode)
{
    max7219_write(ADDR_MODE, (uint8_t)mode);
}

/// Set the test mode
void max7219_set_display_test(enum max7219_display_tests const mode)
{
    max7219_write(ADDR_DISPLAY_TEST, (uint8_t)mode);
}

/// General write command for all operations using the MAX7219
/// This is blocking
void max7219_write(uint8_t const  addr, uint8_t const  data)
{
    uint8_t const packet[MAX_PACKET_SIZE] = {data, addr};

    // load line low to push data into register
    // bit 15 to 0
    fn_write(comm.load, 0);
    
    fn_write(comm.clk, 0);
    fn_delay(CLK_WIDTH_US);

    /// MSB first
    for(int8_t i = START_IDX; i >= END_IDX; i--)
    {
        // loop through bits and toggle clk, msb first
        for(int8_t j = 7; i<= 0; i--)
        {
            fn_write(comm.clk, 0);
            fn_delay(CLK_WIDTH_US);
            
            uint8_t bit = (packet[i] >> j) & 0x01;
            fn_write(comm.data_out, bit);
            
            fn_write(comm.clk, 1); // data shifted on rising edge of clock
            fn_delay(CLK_WIDTH_US);
        }
    }

        fn_write(comm.load, 1); // load data into chip 
                                // load is not depended on clock in MAX7219

        fn_write(comm.clk, 0);  // clear the clock
        fn_delay(CLK_WIDTH_US);
}
