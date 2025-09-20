#ifndef APOLLO_TIME_KEEPER_H
#define APOLLO_TIME_KEEPER_H

#include <stdint.h>
#include <stdbool.h>

void time_keeper_initialize(void);

void time_keeper_get_datetime(int* year, int* month, int* day, int* hour, int* minute, int* second);

uint64_t time_keeper_get_uptime_seconds(void);

#endif