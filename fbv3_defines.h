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
#define L6_TIMEOUT_OUT      2000 /// ms
#define L6_TIMEOUT_IN       250  /// ms
#define L6_TOTAL_INTERFACES 5    /// Max number of interfaces, observed using USB Device Tree

/// Packet sizes
#define CONNECT_SZ 8
#define AUTH_40_SZ 40
#define AUTH_28_SZ 28
#define CTRL_40_SZ 40
#define CTRL_52_SZ 52
#define MAX_IN_SZ  64

/// Index locations in packets
#define PACKET_NUM_IDX 11
#define PEDAL_TYPE_IDX 23
#define PEDAL_ON_IDX   35

/// Pedal types, assigned at index PEDAL_TYPE_IDX
#define _DELAY      0x46
#define _MODULATION 0x47
#define _STOMP      0x48
#define _VOLUME     0x49
#define _COMPRESSOR 0x4A
#define _EQUALIZER  0x4B
#define _UNKNOWN_1  0x4C
#define _GATE       0x4D
#define _UNKNOWN_2  0x4E
#define _UNKNOWN_3  0x4F
#define _REVERB     0x50
#define _WAH        0x51

/// On off codes, assigned at index PEDAL_ON_IDX
#define ON  1
#define OFF 0

/// Command array size 
#define CMD_MAX_SZ 10

#endif //FBV3_DEFINES_H