#include "fbv3_clone.h"
#include "fbv3_defines.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>



/*enums and structs*/

/// State machine states for commnication with L6 Spider V Amp
enum comm_state
{
    CONNECT,    /// On first connection, only used once per session
    AUTH1,      /// After first connection, authenicate 1 - 3, only used once per session
    AUTH2,
    AUTH3, 
    WAIT,       /// Wait for action from IO
    CTRL1,      /// Control hello msg sent before every control command
    CTRL2,      /// Control command then back to wait state
    BANK_UP1,   /// Bank up/down command then back to wait state
    BANK_DOWN1,
    COMM_STATE_MAX,
};

/// Command structure that's added to commands to process queue
struct command
{
    enum effects effect;
    bool on_off;
};

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

static unsigned char bank_msg1[BANK_40_SZ] = {0x04, 0xF0, 0x00, 0x01,
                                              0x04, 0x0C, 0x22, 0x00,
                                              0x04, 0x4D, 0x00, 0x00,
                                              0x04, 0x00, 0x00, 0x0B,
                                              0x04, 0x00, 0x0D, 0x00,
                                              0x04, 0x00, 0x00, 0x04,
                                              0x04, 0x00, 0x00, 0x00,
                                              0x04, 0x00, 0x01, 0x00,
                                              0x04, 0x00, 0x00, 0x00,
                                              0x06, 0x00, 0xF7, 0x00};

/*variables*/

/// Used for usb enumeration / control
static libusb_device **p_list;
static libusb_device_handle *p_handle = NULL;
static libusb_context *p_context = NULL;

static struct fbv3_state fbv3_states; /// Used to keep track of on/off commands
static uint8_t command_index = 0;     /// Used to loop through commands to process array
static bool fpv_clone_ready = false;  /// Used to signal the pedal can take commands

/*function Prototypes*/
static comm_state fbv3_process_commands(void);
static void fbv3_print_msg_data(const uint8_t* data, const size_t size, const char * description);
static void fbv3_print_usb_error(const int16_t error);


/// @brief Initialize the fbv3 interface over USB.
///        This handles enumeration, configuration, and kernal detatchment.
///
///@return True on success
bool fbv3_init(void)
{
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
        fprintf(stderr, "fbv3 usb handle 0x%x\n", p_handle);
      
      if(NULL != p_handle)
      { 
        //reset the interface
        libusb_reset_device(p_handle);
      
        //must check all interfaces and detatch from kernel to configure
        for(int intf = 0; intf <= L6_TOTAL_INTERFACES; intf++)
        {
            fprintf(stderr, "\ninterface %d:\n", intf);
            int16_t active = libusb_kernel_driver_active(p_handle, intf);
            if(1 == active)
            {
                fprintf(stderr, "   -kernel driver attached\n");
                
                status = libusb_detach_kernel_driver(p_handle, intf);
                if(LIBUSB_SUCCESS == status)
                {
                    fprintf(stderr, "   -kernel driver detatched\n");
                }
                else
                {
                    fprintf(stderr, "   -kernel driver detatch failed\n");
                    fbv3_print_usb_error(status); 
                }
            }
            else
            {
                fprintf(stderr, "   -kernel driver not attached %d\n", active);
            }
        }

        status = libusb_set_configuration(p_handle, L6_CONFIGURATION);
        fprintf(stderr, "\nset configuration\n");
        fbv3_print_usb_error(status);

        status = libusb_claim_interface(p_handle, L6_INTERFACE);  
        fprintf(stderr, "claim interface\n");
        fbv3_print_usb_error(status);

        fprintf(stderr, "success: config %d, Interface %d\n", L6_CONFIGURATION, L6_INTERFACE);
      }
      else
      {
          status = LIBUSB_ERROR_OTHER;
          fprintf(stderr, "there was a problem getting handle ret status  %d, handle %d\n", status, p_handle);
      }
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
    static enum comm_state state = CONNECT;
    uint32_t bytes_rx_tx = 0;
    int16_t ret = LIBUSB_SUCCESS;

    if( NULL == p_handle)
    {
        fprintf(stderr, "device handle is NULL\n");
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
          fprintf(stderr, "received %d\n", bytes_rx_tx);
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
        case CONNECT: //should only connect 1 time per session
            buff_out = connect_msg;
            buff_out_sz = CONNECT_SZ;
            strcpy(description, "CONNECT MSG");
            
            state = AUTH1;
            break;
        case AUTH1:
            buff_out = auth_msg1;
            buff_out_sz = AUTH_40_SZ;
            strcpy(description, "AUTH1 MSG");
            
            state = AUTH2;
            break;
        case AUTH2:
            buff_out = auth_msg2;
            buff_out_sz = AUTH_40_SZ;
            strcpy(description, "AUTH2 MSG");
            
            state = AUTH3;
            break;
        case AUTH3:
            buff_out = auth_msg3;
            buff_out_sz = AUTH_28_SZ;
            strcpy(description, "AUTH3 MSG");

            state = WAIT; //after authentication, wait for commands
            break;
        case WAIT:
            fpv_clone_ready = true; 

            buff_out = NULL;
            buff_out_sz = 0;
            
            state = fbv3_process_commands() ? CTRL1 : WAIT; //on successful process, execute command
            break;
        case CTRL1: //CTRL1 and CTRL2 only called when action needed
            buff_out = ctrl_msg1;
            buff_out_sz = CTRL_40_SZ;
            strcpy(description, "CTRL1 MSG");

            state = CTRL2;
            break;
        case CTRL2:
            buff_out = ctrl_msg2;
            buff_out_sz = CTRL_52_SZ;
            strcpy(description, "CTRL2 MSG");

            state = WAIT; //force back into wait to process next message
            break;
        case BANK_UP:
            buff_out = bank_msg1;
            buff_out_sz = BANK_40_SZ;
            strcpy(description, "BANK_UP MSG");

            state = WAIT; //force back into wait to process next message
            break;
        case BANK_DOWN:
            buff_out = bank_msg1;
            buff_out_sz = BANK_40_SZ;
            strcpy(description, "BANK_DOWN MSG");

            state = WAIT; //force back into wait to process next message
            break;
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
    
        
        fprintf(stderr, "size %d sent %d\n", buff_out_sz, bytes_rx_tx);
        
        fbv3_print_usb_error(ret);
    }
    
    
    return ret == LIBUSB_SUCCESS ? true : false;
}

/// @brief Adds effect command to be processed
void fbv3_update_effect_switch(enum effects effect, /// effect to add 
                               bool on_off)         /// effect on/off state
{
    fprintf(stderr, "adding command effect 0x%x state %d\n", (int)effect, (int)on_off);
    
    commands_to_process[command_index].effect = effect;
    commands_to_process[command_index].on_off = on_off;

    command_index = (command_index + 1) % CMD_MAX_SZ;
}

/// @breif Gets the structure that holds the current state of each effect pedal
struct fbv3_state * fbv3_get_states(void)
{
    return &fbv3_states;
}

/// @brief Processes commands by putting them in the CTRL packet
///
/// @return The comm state to go to next
static comm_state fbv3_process_commands(void)
{
    comm_state ret = CTRL1; //assume control, will change below if needed

    //find first command, process, remove command,
    //then exit, next round gets next command
    for(int i = 0; i < CMD_MAX_SZ; i++)
    {
        //found something to process
        if(commands_to_process[i].effect != NO_EFFECT)
        {
            char effect = commands_to_process[i].effect;
            char state = commands_to_process[i].on_off ? ON : OFF;
            
            //clear commmand
            commands_to_process[i].effect = NO_EFFECT;
            commands_to_process[i].on_off = false;
            
            if((NO_EFFECT < effect) && (EFFECTS_MAX > effect))
            {
                switch(effect)
                {
                    case MODULATION:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = _MODULATION;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        break;
                    case DELAY:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = _DELAY;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        break;
                    case STOMP:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = _STOMP;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        break;
                    case VOLUME:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = _VOLUME;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        break;
                    case COMPRESSOR:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = _COMPRESSOR;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        break;
                    case EQUALIZER:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = _EQUALIZER;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        break;
                    case GATE:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = _GATE;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        break;
                    case REVERB:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = _REVERB;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        break;
                    case WAH:
                        ctrl_msg2[PEDAL_TYPE_IDX]  = _WAH;
                        ctrl_msg2[PEDAL_ON_IDX]    = state;
                        break;
                    case BANK_UP:
                        bank_msg1[BANK_UP_DOWN_IDX] = UP;

                        ret = BANK_UP1;
                        break;
                    case BANK_DOWN:
                        bank_msg1[BANK_UP_DOWN_IDX] = DOWN;

                        ret = BANK_DOWN1;
                        break;
                    default:
                        ret = WAIT;
                        break;
                }

                ret = true;
                fprintf(stderr, "got command %d! effect 0x%x state %d\n", i, (int)effect, (int)state);
            }
            else
            {
                ret = false;
                fprintf(stderr, "unknown effect type 0x%x", (int)effect);
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
    
    fprintf(stderr, "\n%s:\n", description);
    
    size_t i = 0;
    for(i = 0; i < size; i++)
    {
        //only 4 bytes per row (command/params 4 bytes each)
        if(0 == (i % 4))
        {
          fprintf(stderr, "\n");
        }
        
        fprintf(stderr, "0x%02x ", data[i]);
    }
    
    fprintf(stderr, "\n");
}


/// @brief Print out readable format of usb error return code, used for debugging.
void fbv3_print_usb_error(int16_t error) /// usb error code
{
    fprintf(stderr, "usb result code: ");
    switch(error)
    {
        case LIBUSB_SUCCESS:
            fprintf(stderr, "SUCCESS\n");
            break;
        case LIBUSB_ERROR_IO:
            fprintf(stderr, "LIBUSB_ERROR_IO\n");
            break;
        case LIBUSB_ERROR_INVALID_PARAM:
            fprintf(stderr, "LIBUSB_ERROR_INVALID_PARAM\n");
            break;
        case LIBUSB_ERROR_ACCESS:
            fprintf(stderr, "LIBUSB_ERROR_ACCESS\n");
            break;
        case LIBUSB_ERROR_NO_DEVICE:
            fprintf(stderr, "LIBUSB_ERROR_NO_DEVICE\n");
            break;
        case LIBUSB_ERROR_NOT_FOUND:
            fprintf(stderr, "LIBUSB_ERROR_NOT_FOUND\n");
            break;
        case LIBUSB_ERROR_BUSY:
            fprintf(stderr, "LIBUSB_ERROR_BUSY\n");
            break;
        case LIBUSB_ERROR_TIMEOUT:
            fprintf(stderr, "LIBUSB_ERROR_TIMEOUT\n");
            break;
        case LIBUSB_ERROR_OVERFLOW:
            fprintf(stderr, "LIBUSB_ERROR_OVERFLOW\n");
            break;
        case LIBUSB_ERROR_PIPE:
            fprintf(stderr, "LIBUSB_ERROR_PIPE\n");
            break;
        case LIBUSB_ERROR_INTERRUPTED:
            fprintf(stderr, "LIBUSB_ERROR_INTERRUPTED\n");
            break;
        case LIBUSB_ERROR_NO_MEM:
            fprintf(stderr, "LIBUSB_ERROR_NO_MEM\n");
            break;
        case LIBUSB_ERROR_NOT_SUPPORTED:
            fprintf(stderr, "LIBUSB_ERROR_NOT_SUPPORTED\n");
            break;
        case LIBUSB_ERROR_OTHER:
            fprintf(stderr, "LIBUSB_ERROR_OTHER\n");
            break;
        default:
            fprintf(stderr, "Unknown LIBUSB_ERROR %d\n", error);
            break;
    }
    
    fprintf(stderr,"\n");
}

/// @brief Clean up usb handle
void fbv3_close(void)
{
    fprintf(stderr, "\n");
    if(NULL != p_handle)
    {
        libusb_release_interface(p_handle, L6_INTERFACE);
        libusb_close(p_handle);
    }
    
    libusb_exit(p_context);
}
