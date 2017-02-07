/* Peer management module. Wrapper for Nordic's Peer Manager */
/* Copyright (c) 2016 Jewelbots   */
/* Author: George Stocker         */
/* Date: 9/20/2016 				  */
#ifndef PEER_MANAGEMENT_H_
#define PEER_MANAGEMENT_H_
#include <stdbool.h>
#include <stdint.h>
#include "peer_manager.h"

void peer_management_init(bool erase_bonds, bool app_dfu); 
void pm_evt_handler(pm_evt_t const * p_evt);

#endif
