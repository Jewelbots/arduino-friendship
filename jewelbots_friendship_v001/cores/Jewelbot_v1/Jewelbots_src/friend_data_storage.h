#ifndef FRIEND_DATA_STORAGE_H_
#define FRIEND_DATA_STORAGE_H_
#include <stdbool.h>
#include "jewelbot_types.h"
#include "fds.h"
#include <stdint.h>

void jwb_fds_evt_handler(fds_evt_t const *const p_fds_evt);
void create_friends_list_in_flash(friends_list_t *friends_list);
void save_friends(friends_list_t *friends_list);
void load_friends(friends_list_t *friends_list_to_load);
void delete_friends_list(void);
bool friends_list_loaded(void);
void set_friends_list_loaded(void);
void set_delete_friends_list(void);
#endif
