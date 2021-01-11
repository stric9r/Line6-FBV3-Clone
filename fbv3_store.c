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
#define BUFFER_SIZE 256
#define PARAM_SIZE 32

/// Params in the file
#define PARAM_PRESET "preset"

/*enums and structs*/


/*variables*/
static int8_t current_preset = 0;

/*function prototypes*/
bool fbv3_store_get_data_from_file(char * p_param, int32_t * p_val);
bool fbv3_store_set_data_to_file(char * p_param, int32_t val);

/// @brief 
void fbv3_store_init(void)
{
    int32_t data = 0;
    if(fbv3_store_get_data_from_file(PARAM_PRESET, &data))
    {
        current_preset = (int8_t)data;
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


/// @brief Opens file and gets specified param
///        Each line format is like (example):
///
///        preset 5 
///        other  2
///        data   3
///
///        Just the param name, a space, then the number
///
/// @param p_param[in] String specifying the param to get
/// @param p_val  [in] Param value
///
/// @return True if param found and set in p_val
bool fbv3_store_get_data_from_file(char * p_param, int32_t * p_val)
{
    FILE *fp = NULL;
    char buffer[BUFFER_SIZE];
    char param_name[PARAM_SIZE];
    int32_t param_val = 0;
    bool ret_val = false;

    fp = fopen(FBV_STORE_FILE_NAME, "r");

    // file open, get the data
    if(fp != NULL)
    {
        while(fgets(buffer, BUFFER_SIZE, fp) != NULL)
        {
            scanf(buffer, "%s %d", param_name, &param_val);

            /// found
            if(strcmp(buffer, p_param) == 0)
            {
                *p_val = param_val;

                ret_val = true;
                break;
            }
        }
    }

    fclose(fp);

    return ret_val;
}

/// @brief Opens file and sets specified param data
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
    char buffer[BUFFER_SIZE];
    char param_name[PARAM_SIZE];
    int8_t param_val = 0;
    bool ret_val = false;

    fp = fopen(FBV_STORE_FILE_NAME, "r+");

    // file open, set the data
    if(fp != NULL)
    {
        while(fgets(buffer, BUFFER_SIZE, fp) != NULL)
        {
            scanf(buffer, "%s %d", param_name, &param_val);

            /// found, write line
            if(strcmp(buffer, p_param) == 0)
            {
                sprintf(&buffer[0], "%s %d\n", param_name, val);
                fputs(buffer, fp);

                ret_val = true;
                break;
            }
        }
    }

    fclose(fp);

    return ret_val;

}