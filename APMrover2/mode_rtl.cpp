#include "mode.h"
#include "Rover.h"

#include <stdio.h>

bool ModeRTL::ok_to_enter(char *failure_reason, uint8_t failure_reason_len) const
{
    // refuse RTL if home has not been set
    if (!AP::ahrs().home_is_set()) {
        snprintf(failure_reason, failure_reason_len, "Home not set");
        return false;
    }
    return Mode::ok_to_enter(failure_reason, failure_reason_len);
}

void ModeRTL::enter()
{
    // initialise waypoint speed
    set_desired_speed_to_default(true);

    // set target to the closest rally point or home
#if AP_RALLY == ENABLED
    set_desired_location(rover.g2.rally.calc_best_rally_or_home_location(rover.current_loc, ahrs.get_home().alt));
#else
    // set destination
    set_desired_location(rover.home);
#endif

    Mode::enter();
}

void ModeRTL::update()
{
    // calculate distance to home
    _distance_to_destination = get_distance(rover.current_loc, _destination);
    const bool near_wp = _distance_to_destination <= rover.g.waypoint_radius;
    // check if we've reached the destination
    if (!_reached_destination && (near_wp || location_passed_point(rover.current_loc, _origin, _destination))) {
        // trigger reached
        _reached_destination = true;
        gcs().send_text(MAV_SEVERITY_INFO, "Reached destination");
    }
    // determine if we should keep navigating
    if (!_reached_destination || (rover.is_boat() && !near_wp)) {
        // continue driving towards destination
        calc_steering_to_waypoint(_reached_destination ? rover.current_loc :_origin, _destination, _reversed);
        calc_throttle(calc_reduced_speed_for_turn_or_distance(_reversed ? -_desired_speed : _desired_speed), true, false);
    } else {
        // we've reached destination so stop
        stop_vehicle();
    }
}
