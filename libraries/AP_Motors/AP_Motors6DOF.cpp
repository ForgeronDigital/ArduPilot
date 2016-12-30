// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *       AP_Motors6DOF.cpp - ArduSub motors library
 *
 *
 */

#include <AP_HAL/AP_HAL.h>
#include "AP_Motors6DOF.h"

extern const AP_HAL::HAL& hal;

// parameters for the motor class
const AP_Param::GroupInfo AP_Motors6DOF::var_info[] = {
	AP_NESTEDGROUPINFO(AP_MotorsMulticopter, 0),
    // @Param: 1_DIRECTION
    // @DisplayName: Motor normal or reverse
    // @Description: Used to change motor rotation directions without changing wires
    // @Values: 1:normal,-1:reverse
    // @User: Standard
    AP_GROUPINFO("1_DIRECTION", 1, AP_Motors6DOF, _motor_reverse[0], 1),

	// @Param: 2_DIRECTION
	// @DisplayName: Motor normal or reverse
	// @Description: Used to change motor rotation directions without changing wires
	// @Values: 1:normal,-1:reverse
	// @User: Standard
	AP_GROUPINFO("2_DIRECTION", 2, AP_Motors6DOF, _motor_reverse[1], 1),

	// @Param: 3_DIRECTION
	// @DisplayName: Motor normal or reverse
	// @Description: Used to change motor rotation directions without changing wires
	// @Values: 1:normal,-1:reverse
	// @User: Standard
	AP_GROUPINFO("3_DIRECTION", 3, AP_Motors6DOF, _motor_reverse[2], 1),

	// @Param: 4_DIRECTION
	// @DisplayName: Motor normal or reverse
	// @Description: Used to change motor rotation directions without changing wires
	// @Values: 1:normal,-1:reverse
	// @User: Standard
	AP_GROUPINFO("4_DIRECTION", 4, AP_Motors6DOF, _motor_reverse[3], 1),

	// @Param: 5_DIRECTION
	// @DisplayName: Motor normal or reverse
	// @Description: Used to change motor rotation directions without changing wires
	// @Values: 1:normal,-1:reverse
	// @User: Standard
	AP_GROUPINFO("5_DIRECTION", 5, AP_Motors6DOF, _motor_reverse[4], 1),

	// @Param: 6_DIRECTION
	// @DisplayName: Motor normal or reverse
	// @Description: Used to change motor rotation directions without changing wires
	// @Values: 1:normal,-1:reverse
	// @User: Standard
	AP_GROUPINFO("6_DIRECTION", 6, AP_Motors6DOF, _motor_reverse[5], 1),

	// @Param: 7_DIRECTION
	// @DisplayName: Motor normal or reverse
	// @Description: Used to change motor rotation directions without changing wires
	// @Values: 1:normal,-1:reverse
	// @User: Standard
	AP_GROUPINFO("7_DIRECTION", 7, AP_Motors6DOF, _motor_reverse[6], 1),

	// @Param: 8_DIRECTION
	// @DisplayName: Motor normal or reverse
	// @Description: Used to change motor rotation directions without changing wires
	// @Values: 1:normal,-1:reverse
	// @User: Standard
	AP_GROUPINFO("8_DIRECTION", 8, AP_Motors6DOF, _motor_reverse[7], 1),

	// @Param: FV_CPLNG_K
	// @DisplayName: Forward/vertical to pitch decoupling factor
	// @Description: Used to decouple pitch from forward/vertical motion. 0 to disable, 1.2 normal
    // @Range: 0.0 1.5
    // @Increment: 0.1
	// @User: Standard
	AP_GROUPINFO("FV_CPLNG_K", 9, AP_Motors6DOF, _forwardVerticalCouplingFactor, 1.0),

    AP_GROUPEND
};

void AP_Motors6DOF::setup_motors() {
	    // call parent
	    AP_MotorsMatrix::setup_motors();

	    // hard coded config for supported frames
	    switch(_flags.frame_orientation) {
    	//				   Motor #				Roll Factor		Pitch Factor	Yaw Factor		Throttle Factor		Forward Factor		Lateral Factor	Testing Order
	    case AS_MOTORS_BLUEROV1_FRAME:
	    	add_motor_raw_6dof(AP_MOTORS_MOT_1,		0,				0,				-1.0f,			0,					1.0f,				0,				1);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_2,		0,				0,				1.0f,			0,					1.0f,				0,				2);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_3,		-0.5f,			0.5f,			0,				0.45f,				0,					0, 				3);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_4,		0.5f,			0.5f,			0,				0.45f,				0,					0,				4);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_5,		0,				-1.0f,			0,				1.0f,				0,					0,				5);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_6,		-0.25f,			0,				0,				0,					0,					1.0f,			6);
	    	break;

	    case AS_MOTORS_VECTORED_6DOF_90DEG_FRAME:
	    	add_motor_raw_6dof(AP_MOTORS_MOT_1,		1.0f,			1.0f,			0,				1.0f,				0,					0,				1);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_2,		0,				0,				1.0f,			0,					1.0f,				0,				2);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_3,		1.0f,			-1.0f,			0,				1.0f,				0,					0, 				3);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_4,		0,				0,				0,				0,					0,					1.0f,			4);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_5,		0,				0,				0,				0,					0,					1.0f,			5);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_6,		-1.0f,			1.0f,			0,				1.0f,				0,					0,				6);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_7,		0,				0,				-1.0f,			0,					1.0f,				0,				7);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_8,		-1.0f,			-1.0f,			0,				1.0f,				0,					0,				8);
	    	break;

	    case AS_MOTORS_VECTORED_6DOF_FRAME:
	    	add_motor_raw_6dof(AP_MOTORS_MOT_1,		0,				0,				1.0f,			0,					1.0f,				-1.0f,			1);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_2,		0,				0,				-1.0f,			0,					1.0f,				1.0f,			2);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_3,		0,				0,				-1.0f,			0,					-1.0f,				-1.0f, 			3);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_4,		0,				0,				1.0f,			0,					-1.0f,				1.0f,			4);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_5,		-1.0f,			1.0f,			0,				-1.0f,				0,					0,				5);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_6,		1.0f,			1.0f,			0,				-1.0f,				0,					0,				6);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_7,		-1.0f,			-1.0f,			0,				-1.0f,				0,					0,				7);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_8,		1.0f,			-1.0f,			0,				-1.0f,				0,					0,				8);
	    	break;

	    case AS_MOTORS_VECTORED_FRAME:
	    	add_motor_raw_6dof(AP_MOTORS_MOT_1,		0,				0,				1.0f,			0,					-1.0f,				1.0f,			1);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_2,		0,				0,				-1.0f,			0,					-1.0f,				-1.0f,			2);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_3,		0,				0,				-1.0f,			0,					1.0f,				1.0f, 			3);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_4,		0,				0,				1.0f,			0,					1.0f,				-1.0f,			4);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_5,		1.0f,			0,				0,				-1.0f,				0,					0,				5);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_6,		-1.0f,			0,				0,				-1.0f,				0,					0,				6);
	    	break;

	    case AS_MOTORS_CUSTOM_FRAME:
	    	// Put your custom motor setup here
	    	//break;

	    case AS_MOTORS_SIMPLEROV_3_FRAME:
	    case AS_MOTORS_SIMPLEROV_4_FRAME:
	    case AS_MOTORS_SIMPLEROV_5_FRAME:
	    default:
	    	add_motor_raw_6dof(AP_MOTORS_MOT_1,		0,				0,				-1.0f,			0,					1.0f,				0,				1);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_2,		0,				0,				1.0f,			0,					1.0f,				0,				2);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_3,		0,				0,				0,				-1.0f,				0,					0, 				3);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_4,		0,				0,				0,				-1.0f,				0,					0,				4);
	    	add_motor_raw_6dof(AP_MOTORS_MOT_5,		0,				0,				0,				0,					0,					1.0f,			5);
	    	break;
	    }
}

void AP_Motors6DOF::add_motor_raw_6dof(int8_t motor_num, float roll_fac, float pitch_fac, float yaw_fac, float throttle_fac, float forward_fac, float lat_fac, uint8_t testing_order) {
	//Parent takes care of enabling output and setting up masks
	add_motor_raw(motor_num, roll_fac, pitch_fac, yaw_fac, testing_order);

	//These are additional parameters for an ROV
	_throttle_factor[motor_num] = throttle_fac;
	_forward_factor[motor_num] = forward_fac;
	_lateral_factor[motor_num] = lat_fac;
}

// output_min - sends minimum values out to the motors
void AP_Motors6DOF::output_min()
{
    int8_t i;

    // set limits flags
    limit.roll_pitch = true;
    limit.yaw = true;
    limit.throttle_lower = false;
    limit.throttle_upper = false;

    // fill the motor_out[] array for HIL use and send minimum value to each motor
    // ToDo find a field to store the minimum pwm instead of hard coding 1500
    hal.rcout->cork();
    for( i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++ ) {
        if( motor_enabled[i] ) {
            rc_write(i, 1500);
        }
    }
    hal.rcout->push();
}

int16_t AP_Motors6DOF::calc_thrust_to_pwm(float thrust_in) const
{
    return constrain_int16(1500 + thrust_in * 400, _throttle_radio_min, _throttle_radio_max);
}

void AP_Motors6DOF::output_to_motors()
{
    int8_t i;
    int16_t motor_out[AP_MOTORS_MAX_NUM_MOTORS];    // final pwm values sent to the motor

    switch (_spool_mode) {
        case SHUT_DOWN:
            // sends minimum values out to the motors
            // set motor output based on thrust requests
            for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
                if (motor_enabled[i]) {
                    motor_out[i] = 1500;
                }
            }
            break;
        case SPIN_WHEN_ARMED:
            // sends output to motors when armed but not flying
            for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
                if (motor_enabled[i]) {
                    motor_out[i] = 1500;
                }
            }
            break;
        case SPOOL_UP:
        case THROTTLE_UNLIMITED:
        case SPOOL_DOWN:
            // set motor output based on thrust requests
            for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
                if (motor_enabled[i]) {
                    motor_out[i] = calc_thrust_to_pwm(_thrust_rpyt_out[i]);
                }
            }
            break;
    }

    // send output to each motor
    hal.rcout->cork();
    for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
        if (motor_enabled[i]) {
            rc_write(i, motor_out[i]);
        }
    }
    hal.rcout->push();
}

// output_armed - sends commands to the motors
// includes new scaling stability patch
// TODO pull code that is common to output_armed_not_stabilizing into helper functions
// ToDo calculate headroom for rpy to be added for stabilization during full throttle/forward/lateral commands
void AP_Motors6DOF::output_armed_stabilizing()
{
	if(_flags.frame_orientation == AS_MOTORS_VECTORED_FRAME) {
		output_armed_stabilizing_vectored();
	} else if(_flags.frame_orientation == AS_MOTORS_VECTORED_6DOF_FRAME) {
		output_armed_stabilizing_vectored_6dof();
	} else {
		uint8_t i;                          // general purpose counter
		float   roll_thrust;                // roll thrust input value, +/- 1.0
		float   pitch_thrust;               // pitch thrust input value, +/- 1.0
		float   yaw_thrust;                 // yaw thrust input value, +/- 1.0
		float   throttle_thrust;            // throttle thrust input value, +/- 1.0
		float   forward_thrust;             // forward thrust input value, +/- 1.0
		float   lateral_thrust;             // lateral thrust input value, +/- 1.0

		roll_thrust = _roll_in;
		pitch_thrust = _pitch_in;
		yaw_thrust = _yaw_in;
		throttle_thrust = get_throttle_bidirectional();
		forward_thrust = _forward_in;
		lateral_thrust = _lateral_in;

		float rpy_out[AP_MOTORS_MAX_NUM_MOTORS]; // buffer so we don't have to multiply coefficients multiple times.
		float linear_out[AP_MOTORS_MAX_NUM_MOTORS]; // 3 linear DOF mix for each motor

		// initialize limits flags
		limit.roll_pitch = false;
		limit.yaw = false;
		limit.throttle_lower = false;
		limit.throttle_upper = false;

		// sanity check throttle is above zero and below current limited throttle
		if (throttle_thrust <= -_throttle_thrust_max) {
			throttle_thrust = -_throttle_thrust_max;
			limit.throttle_lower = true;
		}
		if (throttle_thrust >= _throttle_thrust_max) {
			throttle_thrust = _throttle_thrust_max;
			limit.throttle_upper = true;
		}

		// calculate roll, pitch and yaw for each motor
		for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
			if (motor_enabled[i]) {
				rpy_out[i] = roll_thrust * _roll_factor[i] +
							 pitch_thrust * _pitch_factor[i] +
							 yaw_thrust * _yaw_factor[i];

			}
		}

		// calculate linear command for each motor
		// linear factors should be 0.0 or 1.0 for now
		for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
			if (motor_enabled[i]) {
				linear_out[i] = throttle_thrust * _throttle_factor[i] +
								forward_thrust * _forward_factor[i] +
								lateral_thrust * _lateral_factor[i];
			}
		}

		// Calculate final output for each motor
		for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
			if (motor_enabled[i]) {
				_thrust_rpyt_out[i] = constrain_float(_motor_reverse[i]*(rpy_out[i] + linear_out[i]),-1.0f,1.0f);
			}
		}
	}
}

// output_armed - sends commands to the motors
// includes new scaling stability patch
// TODO pull code that is common to output_armed_not_stabilizing into helper functions
// ToDo calculate headroom for rpy to be added for stabilization during full throttle/forward/lateral commands
void AP_Motors6DOF::output_armed_stabilizing_vectored()
{
	uint8_t i;                          // general purpose counter
	float   roll_thrust;                // roll thrust input value, +/- 1.0
	float   pitch_thrust;               // pitch thrust input value, +/- 1.0
	float   yaw_thrust;                 // yaw thrust input value, +/- 1.0
	float   throttle_thrust;            // throttle thrust input value, +/- 1.0
	float   forward_thrust;             // forward thrust input value, +/- 1.0
	float   lateral_thrust;             // lateral thrust input value, +/- 1.0

	roll_thrust = _roll_in;
	pitch_thrust = _pitch_in;
	yaw_thrust = _yaw_in;
	throttle_thrust = get_throttle_bidirectional();
	forward_thrust = _forward_in;
	lateral_thrust = _lateral_in;

	float rpy_out[AP_MOTORS_MAX_NUM_MOTORS]; // buffer so we don't have to multiply coefficients multiple times.
	float linear_out[AP_MOTORS_MAX_NUM_MOTORS]; // 3 linear DOF mix for each motor

    // initialize limits flags
    limit.roll_pitch = false;
    limit.yaw = false;
    limit.throttle_lower = false;
    limit.throttle_upper = false;

    // sanity check throttle is above zero and below current limited throttle
	if (throttle_thrust <= -_throttle_thrust_max) {
		throttle_thrust = -_throttle_thrust_max;
		limit.throttle_lower = true;
	}
	if (throttle_thrust >= _throttle_thrust_max) {
		throttle_thrust = _throttle_thrust_max;
		limit.throttle_upper = true;
	}

    // calculate roll, pitch and yaw for each motor
    for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
        if (motor_enabled[i]) {
        	rpy_out[i] = roll_thrust * _roll_factor[i] +
                         pitch_thrust * _pitch_factor[i] +
                         yaw_thrust * _yaw_factor[i];

        }
    }

    float forward_coupling_limit = 1-_forwardVerticalCouplingFactor*float(fabs(throttle_thrust));
    if ( forward_coupling_limit < 0 ) {
    	forward_coupling_limit = 0;
    }
    int8_t forward_coupling_direction[] = {-1,-1,1,1,0,0,0,0};

    // calculate linear command for each motor
    // linear factors should be 0.0 or 1.0 for now
    for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
        if (motor_enabled[i]) {

        	float forward_thrust_limited = forward_thrust;

        	// The following statements decouple forward/vertical hydrodynamic coupling on
        	// vectored ROVs. This is done by limiting the maximum output of the "rear" vectored
        	// thruster (where "rear" depends on direction of travel).
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define sign(x) ((x>0)-(x<0))
        	if ( sign(forward_thrust) == sign(forward_coupling_direction[i]) && forward_coupling_direction[i] != 0 ) {
        		forward_thrust_limited = constrain(forward_thrust,-forward_coupling_limit,forward_coupling_limit);
        	}
#undef constrain

        	linear_out[i] = throttle_thrust * _throttle_factor[i] +
        					forward_thrust_limited * _forward_factor[i] +
							lateral_thrust * _lateral_factor[i];

        }
    }

    // Calculate final output for each motor
	for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
		if (motor_enabled[i]) {
			_thrust_rpyt_out[i] = constrain_float(_motor_reverse[i]*(rpy_out[i] + linear_out[i]),-1.0f,1.0f);
		}
	}
}

// Band Aid fix for motor normalization issues.
// TODO: find a global solution for managing saturation that works for all vehicles
void AP_Motors6DOF::output_armed_stabilizing_vectored_6dof()
{
    uint8_t i;                          // general purpose counter
    float   roll_thrust;                // roll thrust input value, +/- 1.0
    float   pitch_thrust;               // pitch thrust input value, +/- 1.0
    float   yaw_thrust;                 // yaw thrust input value, +/- 1.0
    float   throttle_thrust;            // throttle thrust input value, +/- 1.0
    float   forward_thrust;             // forward thrust input value, +/- 1.0
    float   lateral_thrust;             // lateral thrust input value, +/- 1.0

	roll_thrust = _roll_in;
	pitch_thrust = _pitch_in;
	yaw_thrust = _yaw_in;
	throttle_thrust = get_throttle_bidirectional();
	forward_thrust = _forward_in;
	lateral_thrust = _lateral_in;

    float rpt_out[AP_MOTORS_MAX_NUM_MOTORS]; // buffer so we don't have to multiply coefficients multiple times.
    float yfl_out[AP_MOTORS_MAX_NUM_MOTORS]; // 3 linear DOF mix for each motor
    float rpt_max;
    float yfl_max;

    // initialize limits flags
    limit.roll_pitch = false;
    limit.yaw = false;
    limit.throttle_lower = false;
    limit.throttle_upper = false;

    // sanity check throttle is above zero and below current limited throttle
    if (throttle_thrust <= -_throttle_thrust_max) {
        throttle_thrust = -_throttle_thrust_max;
        limit.throttle_lower = true;
    }
    if (throttle_thrust >= _throttle_thrust_max) {
        throttle_thrust = _throttle_thrust_max;
        limit.throttle_upper = true;
    }

    // calculate roll, pitch and Throttle for each motor (only used by vertical thrusters)
    rpt_max = 1; //Initialized to 1 so that normalization will only occur if value is saturated
    for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
        if (motor_enabled[i]) {
        	rpt_out[i] = roll_thrust * _roll_factor[i] +
                         pitch_thrust * _pitch_factor[i] +
                         throttle_thrust * _throttle_factor[i];
            if (fabs(rpt_out[i]) > rpt_max) {
                rpt_max = fabs(rpt_out[i]);
            }
        }
    }

    // calculate linear/yaw command for each motor (only used for translational thrusters)
    // linear factors should be 0.0 or 1.0 for now
    yfl_max = 1; //Initialized to 1 so that normalization will only occur if value is saturated
    for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
        if (motor_enabled[i]) {
        	yfl_out[i] = yaw_thrust * _yaw_factor[i] +
        					forward_thrust * _forward_factor[i] +
							lateral_thrust * _lateral_factor[i];
            if (fabs(yfl_out[i]) > yfl_max) {
                yfl_max = fabs(yfl_out[i]);
            }
        }
    }

    // Calculate final output for each motor and normalize if necessary
    for (i=0; i<AP_MOTORS_MAX_NUM_MOTORS; i++) {
        if (motor_enabled[i]) {
        	_thrust_rpyt_out[i] = constrain_float(_motor_reverse[i]*(rpt_out[i]/rpt_max + yfl_out[i]/yfl_max),-1.0f,1.0f);
        }
    }
}
