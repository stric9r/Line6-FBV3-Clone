#include "max7219.h"

/// Minimum clock width for high and low is 50ns
/// To be very conservative, using the current value would 
/// take 128 ms to write all 64 segments individually
#define CLK_WIDTH_US 1000


// packet size 16 bits
#define MAX_PACKET_SIZE 2

// data clocked in using MSB first
#define START_IDX 1
#define END_IDX 0

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
        default:
            break;
    }
}

/// Setup the pins and write function pointer
void init(void(*f_write)(int,int),
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
}

/// Update a digit using BCD 
/// Note that the mode must be decode
void set_digit_bcd(enum max7219_digits const digit, uint8_t const bcd)
{
    uint8_t const addr = get_digit_addr(digit);
    write(addr, bcd);
}

/// Clear all digits
void clear_all(void)
{
    write(ADDR_DIG_0, MAX7219_DECODE_BLANK);
    write(ADDR_DIG_1, MAX7219_DECODE_BLANK);
    write(ADDR_DIG_2, MAX7219_DECODE_BLANK);
    write(ADDR_DIG_3, MAX7219_DECODE_BLANK);
    write(ADDR_DIG_4, MAX7219_DECODE_BLANK);
    write(ADDR_DIG_5, MAX7219_DECODE_BLANK);
    write(ADDR_DIG_6, MAX7219_DECODE_BLANK);
    write(ADDR_DIG_7, MAX7219_DECODE_BLANK);
}

/// Clear single digit
void clear_digit(enum max7219_digits const digit)
{
    uint8_t const addr = get_digit_addr(digit);
    write(addr, MAX7219_DECODE_BLANK);
}

/// Set the decode mode
void set_decode_mode(uint8_t const mode)
{
    write(ADDR_DECODE_MODE, mode);
}

/// Set the LED intensity
void set_intensity(enum max7219_intensity const intensity)
{
    write(ADDR_INTENSITY, (uint8_t)intensity);
}

/// Set the scan limit
void set_scan_limit(enum max7219_scan_limit const scan_limit)
{
    write(ADDR_SCAN_LIMIT, (uint8_t)scan_limit);
}

/// Set the operating mode
void set_mode(enum max7219_modes const mode)
{
    write(ADDR_MODE, (uint8_t)mode);
}

/// Set the test mode
void set_display_test(enum max7219_display_test const mode)
{
    write(ADDR_DISPLAY_TEST, (uint8_t)mode);
}

/// General write command for all operations using the MAX7219
/// This is blocking
void write(uint8_t const  addr, uint8_t const  data)
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
