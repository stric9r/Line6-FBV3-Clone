#include "fbv3_store.h"
#include "fbv3_defines.h"
#include "debug.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/*defines*/
/// File name where data is stored 
#define FBV_STORE_FILE_NAME "store.bin"

/// File format and size
#define BANK_MAX 32
#define BANK_IDX 0; /*0 to 31 banks, the value is the preset # for that bank*/
#define BANK_CURRENT_IDX BANK_MAX
#define STORE_SIZE (BANK_CURRENT_IDX + 1)

/// File layout
/// Bxx = BANK## 1 byte, holds the preset to select
/// Cxx = CURRENT BANK## 1 byte
///
/// B00 B01 B02 B03
/// B04 B05 B06 B07
/// ...
/// B28 B29 B30 B31
/// C32 <- Current bank

/*enums and structs*/

/*variables*/

/// file store
static char store[STORE_SIZE];
static bool store_read = false;


/*function prototypes*/
static void print_storage(void);
static bool fbv3_read_store_from_file(void);
static bool fbv3_write_store_to_file(void);

/// @brief Get configuration data from storage file.
///        Will populate pedal for use with stored config data.
void fbv3_store_init(void)
{
    store_read = false;

    // make sure store is clear
    memset(store, 0, STORE_SIZE);
  
    fbv3_read_store_from_file();
    
    print_storage();
}

/// @brief Get the preset value of the current bank
///
/// @return the preset value read from file
int8_t fbv3_store_get_preset(void)
{
    int8_t ret_val = 0;

    //Get the correct bank index then update the bank
    uint32_t current_idx = store[BANK_CURRENT_IDX]; 
    
    if(current_idx < BANK_MAX)
    {
        ret_val = (int8_t)store[current_idx];
    }
    
    return ret_val;
}

/// @brief Set the new preset value, write to file.
///
/// @param preset the preset number to write to file
void fbv3_store_set_preset(int8_t preset)
{
    // get the correct bank index then update the bank
    uint32_t current_idx = store[BANK_CURRENT_IDX]; 
    
    // check that we're in bounds
    if(current_idx < BANK_MAX && 
       preset < PRESET_END && 
       preset >= PRESET_START)
    {
      store[current_idx] = (char)preset;
      (void)fbv3_write_store_to_file();
    }
    else
    {
      debug_print( "preset out of range \n");
    }
}

/// @brief Get the current bank
///
/// @return the bank value read from file
int8_t fbv3_store_get_bank(void)
{
   return (int8_t)store[BANK_CURRENT_IDX]; 
}

/// @brief Set the new bank value, write to file.
///
/// @param bank the bank number to write to file
void fbv3_store_set_bank(int8_t bank)
{
    //Get the correct bank index then update the bank
    if(bank < BANK_MAX)
    {
        debug_print( "STORE BANK %d\n", (char)bank);
        store[BANK_CURRENT_IDX] = (char)bank; 
        (void)fbv3_write_store_to_file();
    }
    else
    {
        debug_print( "bank out of range \n");
    }
}

/// @brief Print whats in storage
static void print_storage(void)
{
   int i = 0;
   
   for(i = 0; i < STORE_SIZE; i++)
   {
     if(store[i] != 0);
     {
         if(i == BANK_CURRENT_IDX)
         {
             debug_print( "CURRENT BANK %d BANK %d\n", i, store[i]);
         }
         else
         {
             debug_print( "BANK %d PRESET %d\n", i, store[i]);
         }
     }
   }
}

/// @brief Opens file on startup and gets specified params.
///        Puts them in storage
///
/// @return True if param found and set in p_val
static bool fbv3_read_store_from_file(void)
{ 
    // only read the file at startup
    if(!store_read)
    {
        FILE *fp = NULL;

        // file open, get the binary data
        fp = fopen(FBV_STORE_FILE_NAME, "rb");
        if(fp != NULL)
        {
	          fseek(fp, 0, SEEK_SET);

            // read the whole file into
            fread(store, sizeof(char), STORE_SIZE, fp);
            fclose(fp);
            store_read = true;
        }
        else
        {
            debug_print( "storage file not found\n");
        }
    }
    
    return store_read;
}

/// @brief Writes local storage to file
///
/// @return True if file written
static bool fbv3_write_store_to_file(void)
{
    FILE *fp = NULL;
    bool ret_val = false;

    fp = fopen(FBV_STORE_FILE_NAME, "wb");

    // file open, set the data
    if(fp != NULL)
    {        
        // quicker to just write the whole thing, file is small
        fwrite(store, STORE_SIZE, sizeof(char), fp);
        fclose(fp);
        ret_val = true;
    }
    else
    {
        debug_print( "could not open or create file \n");
    }
    
    return ret_val;
}
