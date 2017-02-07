#ifndef JEWELBOT_SERVICE_H_
#define JEWELBOT_SERVICE_H_
#include <stdint.h>
#include "jewelbot_types.h"
#include "jwb_jcs.h"


ble_jcs_t * get_jcs(void);
void update_friends_list(friends_list_t * friends_list); 
void jewelbot_service_init(void);

#endif
