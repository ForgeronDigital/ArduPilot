#pragma once

#include <AP_AHRS/AP_AHRS.h>
#include <AP_InertialSensor/AP_InertialSensor.h>          // ArduPilot Mega IMU Library
#include <AP_Baro/AP_Baro.h>                    // ArduPilot Mega Barometer Library
#include <AP_Buffer/AP_Buffer.h>                  // FIFO buffer library
#include <AP_NavEKF/AP_Nav_Common.h> // definitions shared by inertial and ekf nav filters

/*
 * AP_InertialNav blends accelerometer data with gps and barometer data to improve altitude and position hold.
 *
 * Most of the functions have to be called at 100Hz. (see defines above)
 *
 * The accelerometer values are integrated over time to approximate velocity and position.
 * The inaccurcy of these estimates grows over time due to noisy sensor data.
 * To improve the accuracy, baro and gps readings are used:
 *      An error value is calculated as the difference between the sensor's measurement and the last position estimation.
 *   	This value is weighted with a gain factor and incorporated into the new estimation
 *
 * Special thanks to Tony Lambregts (FAA) for advice which contributed to the development of this filter.
 *
 */
class AP_InertialNav
{
public:

    // Constructor
    AP_InertialNav() {}

    /**
     * update - updates velocity and position estimates using latest info from accelerometers
     * augmented with gps and baro readings
     *
     * @param dt : time since last update in seconds
     */
    virtual void update(float dt) = 0;

    //
    // XY Axis specific methods
    //

    /**
     * get_position - returns the current position relative to the home location in cm.
     *
     * @return
     */
    virtual const Vector3f&    get_position() const = 0;

    /**
     * get_velocity - returns the current velocity in cm/s
     *
     * @return velocity vector:
     *      		.x : latitude  velocity in cm/s
     * 				.y : longitude velocity in cm/s
     * 				.z : vertical  velocity in cm/s
     */
    virtual const Vector3f&    get_velocity() const = 0;

    /**
     * get_velocity_xy - returns the current horizontal velocity in cm/s
     *
     * @returns the current horizontal velocity in cm/s
     */
    virtual float get_velocity_xy() const = 0;

    //
    // Z Axis methods
    //

    /**
     * get_altitude - get latest altitude estimate in cm above the
     * reference position
     * @return
     */
    virtual float       get_altitude() const = 0;

    /**
     * get_velocity_z - returns the current climbrate.
     *
     * @see get_velocity().z
     *
     * @return climbrate in cm/s (positive up)
     */
    virtual float       get_velocity_z() const = 0;
};

#if AP_AHRS_NAVEKF_AVAILABLE
#include "AP_InertialNav_NavEKF.h"
#endif
