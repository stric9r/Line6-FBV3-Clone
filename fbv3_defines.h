#ifndef FBV3_DEFINES_H
#define FBV3_DEFINES_H
/*defines*/

/// Usb comm parameters
#define L6_VID              0x0E41
#define L6_PID              0x5068
#define L6_CONFIGURATION    0x01 /// 1 Config number to set
#define L6_INTERFACE        0x02 /// 0 - 5 Interface number to claim (2 is for control)
#define L6_ENDPOINT_OUT     0x02 /// OUT pipe from USB Host
#define L6_ENDPOINT_IN      0x82 /// IN pipe from USB Host
#define L6_TIMEOUT_OUT      100  /// ms
#define L6_TIMEOUT_IN       100  /// ms
#define L6_TOTAL_INTERFACES 5    /// Max number of interfaces, observed using USB Device Tree

/// Packet sizes
#define CONNECT_SZ 8
#define AUTH_40_SZ 40
#define AUTH_28_SZ 28
#define CTRL_40_SZ 40
#define CTRL_52_SZ 52
#define BANK_40_SZ 40
#define MAX_IN_SZ  64

/// Index locations in packets
#define PACKET_NUM_IDX   11
#define PEDAL_TYPE_IDX   23
#define PEDAL_ON_IDX     35
#define BANK_UP_DOWN_IDX 30

/// Pedal types, assigned at index PEDAL_TYPE_IDX
#define PEDAL_TYPE_DELAY      0x46
#define PEDAL_TYPE_MODULATION 0x47
#define PEDAL_TYPE_STOMP      0x48
#define PEDAL_TYPE_VOLUME     0x49
#define PEDAL_TYPE_COMPRESSOR 0x4A
#define PEDAL_TYPE_EQUALIZER  0x4B
#define PEDAL_TYPE_UNKNOWN_1  0x4C
#define PEDAL_TYPE_GATE       0x4D
#define PEDAL_TYPE_UNKNOWN_2  0x4E
#define PEDAL_TYPE_UNKNOWN_3  0x4F
#define PEDAL_TYPE_REVERB     0x50
#define PEDAL_TYPE_WAH        0x51

/// On off codes, assigned at index PEDAL_ON_IDX
#define PEDAL_ON  1
#define PEDAL_OFF 0

/// Max min preset number 
#define PRESET_START 0x00
#define PRESET_END 0x7E

/// Command array size 
#define CMD_MAX_SZ 10

#endif //FBV3_DEFINES_H
