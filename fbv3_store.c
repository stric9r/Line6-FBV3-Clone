#include "fbv3_store.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/*defines*/

/// File name where data is stored 
#define FBV_STORE_FILE_NAME "fbv3_storage"

/// Sizes of buffer and params
#define BUFFER_SIZE 2048
#define PARAM_MAX 64 /*2048 buffer / 32 param size = 64 params total*/
#define PARAM_SIZE 32
#define PARAM_NAME_SIZE 16
#define PARAM_VALUE_SIZE 16

/// Params in the file
#define PARAM_PRESET  "preset"
#define PARAM_BANK    "bank"

/*enums and structs*/

/*variables*/

/// file buffer
static char buffer[BUFFER_SIZE];
static bool buffer_read = false;

/// local storage
static char storage[PARAM_MAX][PARAM_SIZE];

/// params broken out for use
static int8_t current_preset = 0;
static int8_t current_bank = 0;

/*function prototypes*/
bool fbv3_store_get_data_from_file(char * p_param, int32_t * p_val);
bool fbv3_store_set_data_to_file(char * p_param, int32_t val);

/// @brief Get configuration data from storage file.
///        Will populate pedal for use with stored config data.
void fbv3_store_init(void)
{
    buffer_read = false;

    // make sure storage is empty
    memset(storage, 0, (PARAM_MAX * PARAM_SIZE)*sizeof(char));

    int32_t data = 0;
    if(fbv3_store_get_data_from_file(PARAM_PRESET, &data))
    {
        current_preset = (int8_t)data;
    }
    
    if(fbv3_store_get_data_from_file(PARAM_BANK, &data))
    {
        current_bank = (int8_t)data;
    }
    //...
    //...
    //... todo future settings
}

/// @brief Get the preset value
///
/// @return the preset value read from file
int8_t fbv3_store_get_preset(void)
{
    return current_preset;
}

/// @brief Set the new preset value, write to file.
///
/// @param preset the preset number to write to file
void fbv3_store_set_preset(int8_t preset)
{
    current_preset = preset;

    (void)fbv3_store_set_data_to_file(PARAM_PRESET, (int32_t) preset);
}

/// @brief Get the bank value
///
/// @return the bank value read from file
int8_t fbv3_store_get_bank(void)
{
    return current_bank;
}

/// @brief Set the new bank value, write to file.
///
/// @param preset the bank number to write to file
void fbv3_store_set_bank(int8_t bank)
{
    current_bank = bank;

    (void)fbv3_store_set_data_to_file(PARAM_BANK, (int32_t) bank);
}


/// @brief Opens file on startup and gets specified params.
///        Puts them in local storage buffers.
///        Each line format is like (example):
///
///        preset 5\n
///        bank 2\n
///        data 3\n
///
///        Just the param name, a space, then the number
///
/// @param p_param[in] String specifying the param to get
/// @param p_val  [in] Param value
///
/// @return True if param found and set in p_val
bool fbv3_store_get_data_from_file(char * p_param, int32_t * p_val)
{
    char param_name[PARAM_NAME_SIZE];
    int32_t param_val = 0;
    bool ret_val = false;

    // only read the file at startup
    if(!buffer_read)
    {
        FILE *fp = NULL;

        // file open, get the data
        fp = fopen(FBV_STORE_FILE_NAME, "r");
        if(fp != NULL)
        {
            // read the whole file into
            fread(buffer, sizeof(char), BUFFER_SIZE, fp);
            fclose(fp);
            buffer_read = true;
        }
        else
        {
            fprintf(stderr, "storage file not found\n");
        }
    }

    // file open, get the data
    if(buffer_read)
    {
        // process 1st token
        char * token;
        token = strtok(buffer, "\n");

        // walk through other tokens */
        int32_t param_idx = 0;
        while(token != NULL)
        {
            bool found = false;
            //get it
            sscanf(token, "%s %d", param_name, param_val);

            // is there a match?
            if(strncmp(param_name, p_param, sizeof(param_name)/sizeof(char)) == 0)
            {
                *p_val = (int32_t)param_val;
                found = true;
            }

            //store for later use
            if(found && param_idx < PARAM_MAX)
            {
                strncpy(&storage[param_idx++][0], token, sizeof(token));
            }

            //get next token on new line
            token = strtok(NULL, "\n");
        }
    }

    return ret_val;
}

/// @brief Sets param data in local storage.
///        If storage update, file is written.
///        Each line format is like (example):
///
///        preset 5 
///        other  2
///        data   3
///
///        Just the param name, a space, then the number
///
/// @param p_param[in] String specifying the param to get
/// @param val  [in] Param value to write
///
/// @return True if param found and set in p_val
bool fbv3_store_set_data_to_file(char * p_param, int32_t val)
{
    FILE *fp = NULL;
    char param_name[PARAM_SIZE];
    int8_t param_val = 0;
    bool ret_val = false;

    // find param in storage
    bool found = false;
    for(int32_t i = 0; i < PARAM_MAX; i++)
    {
        // is there something there?
        if(storage[i][0] != 0)
        {
            // is this the param?
            sscanf(&storage[i][0], "%s %d", param_name, param_val);
            if(strncmp(param_name, p_param, sizeof(param_name)/sizeof(char)) == 0)
            {
                // clear it
                memset(&storage[i][0], 0, PARAM_SIZE * sizeof(char));

                // format it and write it
                snprintf(&storage[i][0], PARAM_SIZE * sizeof(char), "%s %d\n", p_param, val);
            
                found = true;
                break; //done
            }
        }
    }

    // only write out file if found param
    if(found)
    {
        fp = fopen(FBV_STORE_FILE_NAME, "w");

        // file open, set the data
        if(fp != NULL)
        {        
            // quicker to just write the whole thing, file is small
            fwrite(storage, sizeof(char), (PARAM_MAX * PARAM_SIZE) * sizeof(char), fp);
            fclose(fp);
            ret_val = true;
        }
        else
        {
            fprintf(stderr, "storage file not found\n");
        }
    }
    return ret_val;
}