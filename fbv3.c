#include "fbv3.h"
#include "fbv3_store.h"
#include "fbv3_defines.h"
#include "debug.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <libusb-1.0/libusb.h>


#define PRESETS_PER_BANK 4
#define PRESET_A_OFFSET 0
#define PRESET_B_OFFSET 1
#define PRESET_C_OFFSET 2
#define PRESET_D_OFFSET 3

/*enums and structs*/

/// State machine states for commnication with L6 Spider V Amp
enum comm_state
{
    COMM_STATE_CONNECT,     /// On first connection, only used once per session
    COMM_STATE_AUTH1,       /// After first connection, authenicate 1 - 3, only used once per session
    COMM_STATE_AUTH2,
    COMM_STATE_AUTH3, 
    COMM_STATE_WAIT,        /// Wait for action from IO
    COMM_STATE_CTRL1,       /// Control hello msg sent before every control command
    COMM_STATE_CTRL2,       /// Control command then back to wait state
    COMM_STATE_BANK_PRESET1,
    COMM_STATE_BANK_PRESET2,/// Complete bank message
    COMM_STATE_MAX,
};

/// Command structure that's added to commands to process queue
struct command
{
    enum effects effect;
    bool on_off;
    int8_t preset_num;
};

// Preset number used to iterate through presets with bank command (up or down)
static int8_t preset_num_store = 0;
static int8_t bank_num_store = 0;

/*global arrays*/
static struct command commands_to_process[CMD_MAX_SZ]; /// Storage for commands set by user input


// note: Data in these arrays were populated using observations from wireshark usb tracing.
// this program augments certain indexes to create new commands.  See "Line6_Spider_V_USB_Protocol.xlsx"
static unsigned char connect_msg[CONNECT_SZ] = {0x04, 0xf0, 0x7e, 0x7f, 0x07, 0x06, 0x01, 0xf7};

static unsigned char auth_msg1[AUTH_40_SZ] = {0x04, 0xF0, 0x00, 0x01, 
                                              0x04, 0x0C, 0x22, 0x00,  
                                              0x04, 0x4D, 0x00, 0x00, 
                                              0x04, 0x00, 0x00, 0x07, 
                                              0x04, 0x00, 0x01, 0x00, 
                                              0x04, 0x00, 0x00, 0x00,  
                                              0x04, 0x00, 0x00, 0x00, 
                                              0x04, 0x00, 0x00, 0x00, 
                                              0x04, 0x00, 0x00, 0x00, 
                                              0x06, 0x00, 0xF7, 0x00};  


static unsigned char auth_msg2[AUTH_40_SZ] = {0x04, 0xF0, 0x00, 0x01, 
                                              0x04, 0x0C, 0x22, 0x00, 
                                              0x04, 0x4D, 0x00, 0x01, 
                                              0x04, 0x00, 0x00, 0x07,
                                              0x04, 0x00, 0x04, 0x00, 
                                              0x04, 0x00, 0x00, 0x04,
                                              0x04, 0x00, 0x00, 0x00, 
                                              0x04, 0x00, 0x00, 0x00,
                                              0x04, 0x00, 0x00, 0x00, 
                                              0x06, 0x00, 0xF7, 0x00};


static unsigned char auth_msg3[AUTH_28_SZ] = {0x04, 0xF0, 0x00, 0x01, 
                                              0x04, 0x0C, 0x22, 0x00,
                                              0x04, 0x4D, 0x00, 0x02, 
                                              0x04, 0x00, 0x00, 0x03,
                                              0x04, 0x08, 0x2D, 0x2E, 
                                              0x04, 0x6A, 0x50, 0x00,
                                              0x07, 0x00, 0x00, 0xF7};                                
                               
static unsigned char ctrl_msg1[CTRL_40_SZ] = {0x04, 0xf0, 0x00, 0x01, 
                                              0x04, 0x0c, 0x22, 0x00, 
                                              0x04, 0x4d, 0x00, 0x00, 
                                              0x04, 0x00, 0x00, 0x07, 
                                              0x04, 0x00, 0x0a, 0x00, 
                                              0x04, 0x00, 0x00, 0x10, 
                                              0x04, 0x00, 0x00, 0x00, 
                                              0x04, 0x00, 0x00, 0x00, 
                                              0x04, 0x00, 0x00, 0x00, 
                                              0x06, 0x00, 0xf7, 0x00};
                           
static unsigned char ctrl_msg2 [CTRL_52_SZ] = {0x04, 0xf0, 0x00, 0x01, 
                                               0x04, 0x0c, 0x22, 0x00, 
                                               0x04, 0x4d, 0x00, 0x01, 
                                               0x04, 0x00, 0x00, 0x0f, 
                                               0x04, 0x00, 0x74, 0x03,
                                               0x04, 0x00, 0x00, 0x47, 
                                               0x04, 0x03, 0x00, 0x00, 
                                               0x04, 0x00, 0x03, 0x00, 
                                               0x04, 0x00, 0x00, 0x01, 
                                               0x04, 0x00, 0x00, 0x00, 
                                               0x04, 0x00, 0x00, 0x00, 
                                               0x04, 0x00, 0x00, 0x00, 
                                               0x05, 0xf7, 0x00, 0x00};

static unsigned char preset_msg1[BANK_40_SZ] = {0x04, 0xF0, 0x00, 0x01,
                                              0x04, 0x0C, 0x22, 0x00,
                                              0x04, 0x4D, 0x01, 0x00,
                                              0x04, 0x00, 0x00, 0x0B,
                                              0x04, 0x00, 0x0D, 0x00,
                                              0x04, 0x00, 0x00, 0x04,
                                              0x04, 0x00, 0x00, 0x00,
                                              0x04, 0x00, 0x00, 0x00,
                                              0x04, 0x00, 0x00, 0x00,
                                              0x06, 0x00, 0xF7, 0x00};

static unsigned char preset_msg2[BANK_40_SZ] = {0x04, 0xF0, 0x00, 0x01,
                                              0x04, 0x0C, 0x22, 0x00,
                                              0x04, 0x4D, 0x01, 0x01,
                                              0x04, 0x00, 0x00, 0x0B,
                                              0x04, 0x00, 0x0B, 0x00,
                                              0x04, 0x00, 0x00, 0x04,
                                              0x04, 0x00, 0x00, 0x3C,
                                              0x04, 0x00, 0x7F, 0x7F,
                                              0x04, 0x7F, 0x7F, 0x00,
                                              0x06, 0x00, 0xF7, 0x00};

/*variables*/

/// Used for prints,  lines up with enum effects
static char const * const effects_strings[EFFECTS_MAX] = 
{
  "NONE",
  "FX3",
  "FX2",
  "FX1",
  "VOLUME",
  "COMPRESSOR",
  "EQUALIZER",
  "GATE",
  "REVERB",
  "WAH",
  "BANK_UP",
  "BANK_DOWN",
  "PRESET_A",
  "PRESET_B",
  "PRESET_C",
  "PRESET_D",
  "PRESET_INIT",
};

/// Used for usb enumeration / control
static libusb_device **p_list;
static libusb_device_handle *p_handle = NULL;
static libusb_context *p_context = NULL;

static struct fbv3_state fbv3_states; /// Used to keep track of on/off commands
static uint8_t command_index = 0;     /// Used to loop through commands to process array
static bool fpv_clone_ready = false;  /// Used to signal the pedal can take commands

/*function Prototypes*/
static enum comm_state fbv3_process_commands(void);
static void fbv3_print_msg_data(const uint8_t* data, const size_t size, const char * description);
static void fbv3_print_usb_error(const int16_t error);
static void fbv3_set_preset(enum effects effect);

/// @brief Initialize the fbv3 interface over USB.
///        This handles enumeration, configuration, and kernal detatchment.
///
///@return True on success
bool fbv3_init(void)
{
    // Get parameters stored in file
    fbv3_store_init();
    preset_num_store = fbv3_store_get_preset();
    bank_num_store = fbv3_store_get_bank();

    int16_t status = LIBUSB_ERROR_OTHER; 

    //clear the command array
    memset(commands_to_process, 0, sizeof(struct command) * CMD_MAX_SZ);

    //clear current states
    memset(&fbv3_states, 0, sizeof(struct fbv3_state));
    
    //init libusb, must be called first!
    status = libusb_init(&p_context);
  
     if(LIBUSB_SUCCESS == status)
     {
        //set debugging
        libusb_set_option(p_context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);

        p_handle = libusb_open_device_with_vid_pid(p_context, L6_VID, L6_PID);  
        debug_print( "fbv3 usb handle 0x%x\n", p_handle);
      
      if(NULL != p_handle)
      { 
        //reset the interface
        libusb_reset_device(p_handle);
      
        //must check all interfaces and detatch from kernel to configure
        for(int intf = 0; intf <= L6_TOTAL_INTERFACES; intf++)
        {
            debug_print( "\ninterface %d:\n", intf);
            int16_t active = libusb_kernel_driver_active(p_handle, intf);
            if(1 == active)
            {
                debug_print( "   -kernel driver attached\n");
                
                status = libusb_detach_kernel_driver(p_handle, intf);
                if(LIBUSB_SUCCESS == status)
                {
                    debug_print( "   -kernel driver detatched\n");
                }
                else
                {
                    debug_print( "   -kernel driver detatch failed\n");
                    fbv3_print_usb_error(status); 
                }
            }
            else
            {
                debug_print( "   -kernel driver not attached %d\n", active);
            }
        }

        status = libusb_set_configuration(p_handle, L6_CONFIGURATION);
        debug_print( "\nset configuration\n");
        fbv3_print_usb_error(status);

        status = libusb_claim_interface(p_handle, L6_INTERFACE);  
        debug_print( "claim interface\n");
        fbv3_print_usb_error(status);

        debug_print( "success: config %d, Interface %d\n", L6_CONFIGURATION, L6_INTERFACE);
      }
      else
      {
          status = LIBUSB_ERROR_OTHER;
          debug_print( "there was a problem getting handle ret status  %d, handle %d\n", status, p_handle);
      }
    }
    
    // Force to bank read from storage file, in command queue
    if(status == LIBUSB_SUCCESS)
    {
        //init the bank/preset
        fbv3_update_effect_switch(EFFECTS_PRESET_INIT, false);
    }

    return status == LIBUSB_SUCCESS ? true : false;
}

/// @brief Get if pedal ready for commands
///
/// @return True if FBV module has authenticated with amp
bool fbv3_ready(void)
{
    return fpv_clone_ready;
}

/// @brief Process function for module, called in main loop.
///        This is the statemachine that authaticates, and sends commands.
///
/// @return True on success
bool fbv3_process(void) 
{
    static enum comm_state state = COMM_STATE_CONNECT;
    uint32_t bytes_rx_tx = 0;
    int16_t ret = LIBUSB_SUCCESS;

    if( NULL == p_handle)
    {
        debug_print( "device handle is NULL\n");
        return LIBUSB_ERROR_OTHER;
    }

    //setup control messages
    //hard coded in arrays, just here to be explicit
    //ctrl_msg1[PACKET_NUM_IDX] = 0;  //first msg
    //ctrl_msg2[PACKET_NUM_IDX] = 1;  //second msg

    //populate tranfer structure
    //on connect message
    unsigned char buff_in[MAX_IN_SZ];

    //check for data, keep checking till it goes silent
    //if rx message ignored, L6 Spider V amp will not respond to commands
    do
    {
      bytes_rx_tx = 0;
      ret = libusb_bulk_transfer(p_handle,          //handle to device
                                 L6_ENDPOINT_IN,    //end point
                                 buff_in,           //input buffer
                                 MAX_IN_SZ,         //max input size 
                                 &bytes_rx_tx,      //actual bytes sent
                                 L6_TIMEOUT_IN);    //timeout (ms)
      
      
      //we got some data
      if(LIBUSB_SUCCESS == ret)
      {
          fbv3_print_msg_data(buff_in, bytes_rx_tx, "INPUT");  
          debug_print( "received %d\n", bytes_rx_tx);
          fbv3_print_usb_error(ret);
          
          //todo do something with data when you know what it is
      }
      
    }while (LIBUSB_SUCCESS == ret); 


    //set the buffer
    char description[16];
    unsigned char * buff_out = NULL;
    int buff_out_sz = 0;
    switch(state)
    {
        case COMM_STATE_CONNECT: //should only connect 1 time per session
            buff_out = connect_msg;
            buff_out_sz = CONNECT_SZ;
            strcpy(description, "CONNECT MSG");
            
            state = COMM_STATE_AUTH1;
            break;
        case COMM_STATE_AUTH1:
            buff_out = auth_msg1;
            buff_out_sz = AUTH_40_SZ;
            strcpy(description, "AUTH1 MSG");
            
            state = COMM_STATE_AUTH2;
            break;
        case COMM_STATE_AUTH2:
            buff_out = auth_msg2;
            buff_out_sz = AUTH_40_SZ;
            strcpy(description, "AUTH2 MSG");
            
            state = COMM_STATE_AUTH3;
            break;
        case COMM_STATE_AUTH3:
            buff_out = auth_msg3;
            buff_out_sz = AUTH_28_SZ;
            strcpy(description, "AUTH3 MSG");

            state = COMM_STATE_WAIT; //after authentication, wait for commands
            break;
        case COMM_STATE_WAIT:
            fpv_clone_ready = true; 

            buff_out = NULL;
            buff_out_sz = 0;
            
            state = fbv3_process_commands();
            break;
        case COMM_STATE_CTRL1: //CTRL1 and CTRL2 only called when action needed
            buff_out = ctrl_msg1;
            buff_out_sz = CTRL_40_SZ;
            strcpy(description, "CTRL1 MSG");

            state = COMM_STATE_CTRL2;
            break;
        case COMM_STATE_CTRL2:
            buff_out = ctrl_msg2;
            buff_out_sz = CTRL_52_SZ;
            strcpy(description, "CTRL2 MSG");

            state = COMM_STATE_WAIT; //force back into wait to process next message
            break;
        case COMM_STATE_BANK_PRESET1:
            buff_out = preset_msg1;
            buff_out_sz = BANK_40_SZ;
            strcpy(description, "BANK_PRESET MSG");

            state = COMM_STATE_BANK_PRESET2;
            break;
        case COMM_STATE_BANK_PRESET2:
          buff_out = preset_msg2;
          buff_out_sz = BANK_40_SZ;
          strcpy(description, "BANK2 MSG");
          
          state = COMM_STATE_WAIT; //force back into wait to process next message
        default:
            fpv_clone_ready = false;
            return LIBUSB_ERROR_OTHER;
            break;
    }
    
    //reset
    ret = LIBUSB_SUCCESS;
    
    //send data out
    if(NULL != buff_out)
    {
        //populate tranfer structure
        //on connect message
        fbv3_print_msg_data(buff_out, buff_out_sz, description);
        bytes_rx_tx = 0;
        ret = libusb_bulk_transfer(p_handle,         //handle to device
                                   L6_ENDPOINT_OUT,  //end point
                                   buff_out,         //data to send
                                   buff_out_sz,      //data size
                                   &bytes_rx_tx,     //actual bytes sent
                                   L6_TIMEOUT_OUT);  //timeout (ms)
    
        
        debug_print( "size %d sent %d\n", buff_out_sz, bytes_rx_tx);
        
        fbv3_print_usb_error(ret);
    }
    
    
    return ret == LIBUSB_SUCCESS ? true : false;
}

/// @brief Adds effect command to be processed
void fbv3_update_effect_switch(enum effects effect, /// effect to add 
                               bool on_off)         /// effect on/off state
{
    if( effect != EFFECTS_BANK_UP && 
        effect != EFFECTS_BANK_DOWN &&
        effect != EFFECTS_A &&
        effect != EFFECTS_B &&
        effect != EFFECTS_C &&
        effect != EFFECTS_D &&
        effect != EFFECTS_PRESET_INIT)
    {
        debug_print( "adding command effect %s state %d\n", effects_strings[effect], (int)on_off);

        commands_to_process[command_index].effect = effect;
        commands_to_process[command_index].on_off = on_off;
        commands_to_process[command_index].preset_num = 0; //don't care

        command_index = (command_index + 1) % CMD_MAX_SZ;
    }
    else
    {
        fbv3_set_preset(effect);
    }
    
}

/// @brief Adds command to change presets
static void fbv3_set_preset(enum effects effect)
{
    //get the offset
    int8_t preset_offset = 0;
    bool bank_update = false;
    
    switch(effect)
    {
      case EFFECTS_BANK_UP:
        debug_print( "BANK UP1 - %d\n", bank_num_store); 
        bank_num_store = (bank_num_store + 1) >= BANK_END ? BANK_START : (bank_num_store + 1);
        debug_print( "BANK UP2 - %d\n", bank_num_store); 
        bank_update = true;
        break; 
      case EFFECTS_BANK_DOWN:
        debug_print( "BANK DOWN1 - %d\n", bank_num_store); 
        bank_num_store = (bank_num_store - 1) < BANK_START ? (BANK_END-1) : (bank_num_store - 1); 
        debug_print( "BANK DOWN2 - %d\n", bank_num_store); 
        bank_update = true;
        break;
      case EFFECTS_A:
        preset_offset = PRESET_A_OFFSET;
        break;
      case EFFECTS_B:
        preset_offset = PRESET_B_OFFSET;
        break;
      case EFFECTS_C:
        preset_offset = PRESET_C_OFFSET;
        break;
      case EFFECTS_D:
        preset_offset = PRESET_D_OFFSET;
        break;
      default:
        preset_offset = PRESET_A_OFFSET;
        break;
    }

    //only calculate if not INIT
    if(effect != EFFECTS_PRESET_INIT)
    {
      // bank command, updat the bank and get stored preset
      if(bank_update)
      {          
          fbv3_store_set_bank(bank_num_store);
          preset_num_store = fbv3_store_get_preset();
          bank_update = true;
      }
      // Just a preset change, don't update the bank
      else 
      {
          //get the current bank and add the offset
          bank_num_store = fbv3_store_get_bank();
          
          // preset = bank * presets_per_bank + preset_offset
          preset_num_store = (bank_num_store * PRESETS_PER_BANK) + preset_offset;
          fbv3_store_set_preset(preset_num_store);
      }
    }
    
    // build the message
    commands_to_process[command_index].effect = effect;
    commands_to_process[command_index].on_off = false; //dont care
    commands_to_process[command_index].preset_num = preset_num_store;

    command_index = (command_index + 1) % CMD_MAX_SZ; //next index number

    debug_print( "setting bank %d to preset %d\n", bank_num_store, preset_num_store);
}

/// @breif Gets the structure that holds the current state of each effect pedal
struct fbv3_state * fbv3_get_states(void)
{
    return &fbv3_states;
}

/// @brief Processes commands by putting them in the CTRL packet
///
/// @return The comm state to go to next
static enum comm_state fbv3_process_commands(void)
{
    enum comm_state ret = COMM_STATE_WAIT;

    //find first command, process, remove command,
    //then exit, next round gets next command
    for(int i = 0; i < CMD_MAX_SZ; i++)
    {
        //found something to process
        if(commands_to_process[i].effect != EFFECTS_NONE)
        {
            char effect = commands_to_process[i].effect;
            char state = commands_to_process[i].on_off ? PEDAL_ON : PEDAL_OFF;
            char preset = commands_to_process[i].preset_num;
            
            //clear commmand
            commands_to_process[i].effect = EFFECTS_NONE;
            commands_to_process[i].on_off = false;
            commands_to_process[i].preset_num = 0;

            if((EFFECTS_NONE < effect) && (EFFECTS_MAX > effect))
            {
                switch(effect)
                {
                    case EFFECTS_FX2:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = PEDAL_TYPE_FX2;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        ret = COMM_STATE_CTRL1;
                        break;
                    case EFFECTS_FX3:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = PEDAL_TYPE_FX3;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        ret = COMM_STATE_CTRL1;
                        break;
                    case EFFECTS_FX1:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = PEDAL_TYPE_FX1;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        ret = COMM_STATE_CTRL1;
                        break;
                    case EFFECTS_VOLUME:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = PEDAL_TYPE_VOLUME;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        ret = COMM_STATE_CTRL1;
                        break;
                    case EFFECTS_COMPRESSOR:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = PEDAL_TYPE_COMPRESSOR;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        ret = COMM_STATE_CTRL1;
                        break;
                    case EFFECTS_EQUALIZER:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = PEDAL_TYPE_EQUALIZER;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        ret = COMM_STATE_CTRL1;
                        break;
                    case EFFECTS_GATE:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = PEDAL_TYPE_GATE;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        ret = COMM_STATE_CTRL1;
                        break;
                    case EFFECTS_REVERB:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = PEDAL_TYPE_REVERB;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        ret = COMM_STATE_CTRL1;
                        break;
                    case EFFECTS_WAH:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = PEDAL_TYPE_WAH;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        ret = COMM_STATE_CTRL1;
                        break;
                    case EFFECTS_BANK_UP:
                    case EFFECTS_BANK_DOWN:
                    case EFFECTS_A:
                    case EFFECTS_B:
                    case EFFECTS_C:
                    case EFFECTS_D:
                    case EFFECTS_PRESET_INIT:
                        preset_msg1[PRESET_IDX] = preset;
                        ret = COMM_STATE_BANK_PRESET1;
                        break;
                    default:
                        ret = COMM_STATE_WAIT;
                        break;
                }

                debug_print( "got command %d! effect %s state %d, preset %d \n", i, effects_strings[effect], (int)state, preset);
            }
            else
            {
                ret = COMM_STATE_WAIT;
                debug_print( "unknown effect type 0x%x", (int)effect);
            }

            break;
        }
    }

    return ret;
}

/// @brief Print function for message data, used for debugging.
void fbv3_print_msg_data(
    const uint8_t* data,       /// message data
    const size_t size,         /// size of message data
    const char * description)  /// string description of message data
{
    
    debug_print( "\n%s:\n", description);
    
    size_t i = 0;
    for(i = 0; i < size; i++)
    {
        //only 4 bytes per row (command/params 4 bytes each)
        if(0 == (i % 4))
        {
          debug_print( "\n");
        }
        
        debug_print( "0x%02x ", data[i]);
    }
    
    debug_print( "\n");
}


/// @brief Print out readable format of usb error return code, used for debugging.
void fbv3_print_usb_error(int16_t error) /// usb error code
{
    debug_print( "usb result code: ");
    switch(error)
    {
        case LIBUSB_SUCCESS:
            debug_print( "SUCCESS\n");
            break;
        case LIBUSB_ERROR_IO:
            debug_print( "LIBUSB_ERROR_IO\n");
            break;
        case LIBUSB_ERROR_INVALID_PARAM:
            debug_print( "LIBUSB_ERROR_INVALID_PARAM\n");
            break;
        case LIBUSB_ERROR_ACCESS:
            debug_print( "LIBUSB_ERROR_ACCESS\n");
            break;
        case LIBUSB_ERROR_NO_DEVICE:
            debug_print( "LIBUSB_ERROR_NO_DEVICE\n");
            break;
        case LIBUSB_ERROR_NOT_FOUND:
            debug_print( "LIBUSB_ERROR_NOT_FOUND\n");
            break;
        case LIBUSB_ERROR_BUSY:
            debug_print( "LIBUSB_ERROR_BUSY\n");
            break;
        case LIBUSB_ERROR_TIMEOUT:
            debug_print( "LIBUSB_ERROR_TIMEOUT\n");
            break;
        case LIBUSB_ERROR_OVERFLOW:
            debug_print( "LIBUSB_ERROR_OVERFLOW\n");
            break;
        case LIBUSB_ERROR_PIPE:
            debug_print( "LIBUSB_ERROR_PIPE\n");
            break;
        case LIBUSB_ERROR_INTERRUPTED:
            debug_print( "LIBUSB_ERROR_INTERRUPTED\n");
            break;
        case LIBUSB_ERROR_NO_MEM:
            debug_print( "LIBUSB_ERROR_NO_MEM\n");
            break;
        case LIBUSB_ERROR_NOT_SUPPORTED:
            debug_print( "LIBUSB_ERROR_NOT_SUPPORTED\n");
            break;
        case LIBUSB_ERROR_OTHER:
            debug_print( "LIBUSB_ERROR_OTHER\n");
            break;
        default:
            debug_print( "Unknown LIBUSB_ERROR %d\n", error);
            break;
    }
    
    debug_print("\n");
}

/// @brief Clean up usb handle
void fbv3_close(void)
{
    debug_print( "\n");
    if(NULL != p_handle)
    {
        libusb_release_interface(p_handle, L6_INTERFACE);
        libusb_close(p_handle);
    }
    
    libusb_exit(p_context);
}
