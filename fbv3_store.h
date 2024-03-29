#ifndef FBV3_STORE_H
#define FBV3_STORE_H

#include <stdint.h>

void fbv3_store_init(void);

int8_t fbv3_store_get_preset(void);
void fbv3_store_set_preset(const int8_t preset);

int8_t fbv3_store_get_bank(void);
void fbv3_store_set_bank(const int8_t preset);

#endif //FBV3_STORE_H