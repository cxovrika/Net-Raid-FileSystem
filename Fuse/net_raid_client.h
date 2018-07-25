#ifndef NET_RAID_CLIENT_H
#define NET_RAID_CLIENT_H

#include "shared_types.h"
#include "shared_constants.h"

char path_to_config[256];
int config_fd;


int storage_count;
struct storage_info storages[128];

char path_to_errorlog[256];
int cache_size;
int timeout;


#endif
