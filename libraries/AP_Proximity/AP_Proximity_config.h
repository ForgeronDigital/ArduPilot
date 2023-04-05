#pragma once

#include <AP_HAL/AP_HAL_Boards.h>
#include <GCS_MAVLink/GCS_config.h>
#include <AP_RangeFinder/AP_RangeFinder_config.h>

#ifndef HAL_PROXIMITY_ENABLED
#define HAL_PROXIMITY_ENABLED (!HAL_MINIMIZE_FEATURES && BOARD_FLASH_SIZE > 1024)
#endif

#ifndef AP_PROXIMITY_BACKEND_DEFAULT_ENABLED
#define AP_PROXIMITY_BACKEND_DEFAULT_ENABLED HAL_PROXIMITY_ENABLED
#endif

#ifndef AP_PROXIMITY_AIRSIMSITL_ENABLED
#define AP_PROXIMITY_AIRSIMSITL_ENABLED AP_PROXIMITY_BACKEND_DEFAULT_ENABLED && (CONFIG_HAL_BOARD == HAL_BOARD_SITL)
#endif

#ifndef AP_PROXIMITY_CYGBOT_ENABLED
#define AP_PROXIMITY_CYGBOT_ENABLED AP_PROXIMITY_BACKEND_DEFAULT_ENABLED
#endif

#ifndef AP_PROXIMITY_DRONECAN_ENABLED
#define AP_PROXIMITY_DRONECAN_ENABLED AP_PROXIMITY_BACKEND_DEFAULT_ENABLED && HAL_ENABLE_DRONECAN_DRIVERS
#endif

#ifndef AP_PROXIMITY_LIGHTWARE_SF40C_ENABLED
#define AP_PROXIMITY_LIGHTWARE_SF40C_ENABLED AP_PROXIMITY_BACKEND_DEFAULT_ENABLED
#endif

#ifndef AP_PROXIMITY_LIGHTWARE_SF45B_ENABLED
#define AP_PROXIMITY_LIGHTWARE_SF45B_ENABLED AP_PROXIMITY_BACKEND_DEFAULT_ENABLED
#endif

#ifndef AP_PROXIMITY_MAV_ENABLED
#define AP_PROXIMITY_MAV_ENABLED AP_PROXIMITY_BACKEND_DEFAULT_ENABLED && HAL_GCS_ENABLED
#endif

#ifndef AP_PROXIMITY_RANGEFINDER_ENABLED
#define AP_PROXIMITY_RANGEFINDER_ENABLED AP_RANGEFINDER_ENABLED && AP_PROXIMITY_BACKEND_DEFAULT_ENABLED
#endif

#ifndef AP_PROXIMITY_RPLIDARA2_ENABLED
#define AP_PROXIMITY_RPLIDARA2_ENABLED AP_PROXIMITY_BACKEND_DEFAULT_ENABLED
#endif

#ifndef AP_PROXIMITY_SITL_ENABLED
#define AP_PROXIMITY_SITL_ENABLED AP_PROXIMITY_BACKEND_DEFAULT_ENABLED && (CONFIG_HAL_BOARD == HAL_BOARD_SITL)
#endif

#ifndef AP_PROXIMITY_TERARANGERTOWER_ENABLED
#define AP_PROXIMITY_TERARANGERTOWER_ENABLED AP_PROXIMITY_BACKEND_DEFAULT_ENABLED
#endif

#ifndef AP_PROXIMITY_TERARANGERTOWEREVO_ENABLED
#define AP_PROXIMITY_TERARANGERTOWEREVO_ENABLED AP_PROXIMITY_BACKEND_DEFAULT_ENABLED
#endif
