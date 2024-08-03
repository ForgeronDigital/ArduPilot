#include "AP_Quicktune.h"

#if AP_QUICKTUNE_ENABLED

#define UPDATE_RATE_HZ 40
#define UPDATE_PERIOD_MS (1000U/UPDATE_RATE_HZ)
#define STAGE_DELAY 4000
#define PILOT_INPUT_DELAY 4000
#define YAW_FLTE_MAX 2.0
#define FLTD_MUL 0.5
#define FLTT_MUL 0.5
#define DEFAULT_SMAX 50.0
#define OPTIONS_TWO_POSITION (1<<0)

/*
  if while tuning the attitude error goes over 25 degrees then abort
  the tune
 */
#define MAX_ATTITUDE_ERROR 25.0

#include <AP_Common/AP_Common.h>
#include <AP_Vehicle/AP_Vehicle.h>
#include <AP_RCMapper/AP_RCMapper.h>
#include <AP_Logger/AP_Logger.h>
#include <AP_Arming/AP_Arming.h>
#include <GCS_MAVLink/GCS.h>

const AP_Param::GroupInfo AP_Quicktune::var_info[] = {
    // @Param: ENABLE
    // @DisplayName: Quicktune enable
    // @Description: Enable quicktune system
    // @Values: 0:Disabled,1:Enabled
    // @User: Standard
    AP_GROUPINFO_FLAGS("ENABLE", 1, AP_Quicktune, enable, 0, AP_PARAM_FLAG_ENABLE),

    // @Param: AXES
    // @DisplayName: Quicktune axes
    // @Description: Axes to tune
    // @Bitmask: 0:Roll,1:Pitch,2:Yaw
    // @User: Standard
    AP_GROUPINFO("AXES", 2, AP_Quicktune, axes_enabled, 7),

    // @Param: DOUBLE_TIME
    // @DisplayName: Quicktune doubling time
    // @Description: Time to double a tuning parameter. Raise this for a slower tune.
    // @Range: 5 20
    // @Units: s
    // @User: Standard
    AP_GROUPINFO("DOUBLE_TIME", 3, AP_Quicktune, double_time, 10),

    // @Param: GAIN_MARGIN
    // @DisplayName: Quicktune gain margin
    // @Description: Reduction in gain after oscillation detected. Raise this number to get a more conservative tune
    // @Range: 20 80
    // @Units: %
    // @User: Standard
    AP_GROUPINFO("GAIN_MARGIN", 4, AP_Quicktune, gain_margin, 60),

    // @Param: OSC_SMAX
    // @DisplayName: Quicktune oscillation rate threshold
    // @Description: Threshold for oscillation detection. A lower value will lead to a more conservative tune.
    // @Range: 1 10
    // @User: Standard
    AP_GROUPINFO("OSC_SMAX", 5, AP_Quicktune, osc_smax, 5),

    // @Param: YAW_P_MAX
    // @DisplayName: Quicktune Yaw P max
    // @Description: Maximum value for yaw P gain
    // @Range: 0.1 3
    // @User: Standard
    AP_GROUPINFO("YAW_P_MAX", 6, AP_Quicktune, yaw_p_max, 0.5),

    // @Param: YAW_D_MAX
    // @DisplayName: Quicktune Yaw D max
    // @Description: Maximum value for yaw D gain
    // @Range: 0.001 1
    // @User: Standard
    AP_GROUPINFO("YAW_D_MAX", 7, AP_Quicktune, yaw_d_max, 0.01),

    // @Param: RP_PI_RATIO
    // @DisplayName: Quicktune roll/pitch PI ratio
    // @Description: Ratio between P and I gains for roll and pitch. Raise this to get a lower I gain
    // @Range: 0.5 1.0
    // @User: Standard
    AP_GROUPINFO("RP_PI_RATIO", 8, AP_Quicktune, rp_pi_ratio, 1.0),

    // @Param: Y_PI_RATIO
    // @DisplayName: Quicktune Yaw PI ratio
    // @Description: Ratio between P and I gains for yaw. Raise this to get a lower I gain
    // @Range: 0.5 20
    // @User: Standard
    AP_GROUPINFO("Y_PI_RATIO", 9, AP_Quicktune, y_pi_ratio, 10),

    // @Param: AUTO_FILTER
    // @DisplayName: Quicktune auto filter enable
    // @Description: When enabled the PID filter settings are automatically set based on INS_GYRO_FILTER
    // @Values: 0:Disabled,1:Enabled
    // @User: Standard
    AP_GROUPINFO("AUTO_FILTER", 10, AP_Quicktune, auto_filter, 1),

    // @Param: AUTO_SAVE
    // @DisplayName: Quicktune auto save
    // @Description: Number of seconds after completion of tune to auto-save. This is useful when using a 2 position switch for quicktune
    // @Units: s
    // @User: Standard
    AP_GROUPINFO("AUTO_SAVE", 11, AP_Quicktune, auto_save, 0),

    // @Param: REDUCE_MAX
    // @DisplayName: Quicktune maximum gain reduction
    // @Description: This controls how much quicktune is allowed to lower gains from the original gains. If the vehicle already has a reasonable tune and is not oscillating then you can set this to zero to prevent gain reductions. The default of 20% is reasonable for most vehicles. Using a maximum gain reduction lowers the chance of an angle P oscillation happening if quicktune gets a false positive oscillation at a low gain, which can result in very low rate gains and a dangerous angle P oscillation.
    // @Units: %
    // @Range: 0 100
    // @User: Standard
    AP_GROUPINFO("REDUCE_MAX", 12, AP_Quicktune, reduce_max, 20),

    // @Param: OPTIONS
    // @DisplayName: Quicktune options
    // @Description: Additional options. When the Two Position Switch option is enabled then a high switch position will start the tune, low will disable the tune. you should also set a QUIK_AUTO_SAVE time so that you will be able to save the tune.
    // @Bitmask: 0:UseTwoPositionSwitch
    // @User: Standard
    AP_GROUPINFO("OPTIONS", 13, AP_Quicktune, options, 0),

    AP_GROUPEND
};

// Call at loop rate
void AP_Quicktune::update(bool mode_supports_quicktune)
{
    if (enable < 1) {
        if (need_restore) {
            GCS_SEND_TEXT(MAV_SEVERITY_EMERGENCY, "QuickTune disabled");
            abort_tune();
        }
        return;
    }
    const uint32_t now = AP_HAL::millis();

    if (!mode_supports_quicktune) {
        /*
          user has switched to a non-quicktune mode. If we have
          pending parameter changes then revert
         */
        if (need_restore) {
            GCS_SEND_TEXT(MAV_SEVERITY_EMERGENCY, "QuickTune aborted");
        }
        abort_tune();
        return;
    }

    if (need_restore) {
        const float att_error = AC_AttitudeControl::get_singleton()->get_att_error_angle_deg();
        if (att_error > MAX_ATTITUDE_ERROR) {
            GCS_SEND_TEXT(MAV_SEVERITY_EMERGENCY, "Tuning: attitude error %.1fdeg - ABORTING", att_error);
            abort_tune();
            return;
        }
    }

    const auto &vehicle = *AP::vehicle();

    if (vehicle.have_pilot_input()) {
        last_pilot_input = now;
    }

    SwitchPos sw_pos_tune = SwitchPos::MID;
    SwitchPos sw_pos_save = SwitchPos::HIGH;
    if ((options & OPTIONS_TWO_POSITION) != 0) {
        sw_pos_tune = SwitchPos::HIGH;
        sw_pos_save = SwitchPos::NONE;
    }

    if (sw_pos == sw_pos_tune && (!hal.util->get_soft_armed() || !vehicle.get_likely_flying()) && now > last_warning + 5000) {
        GCS_SEND_TEXT(MAV_SEVERITY_EMERGENCY, "Tuning: Must be flying to tune");
        last_warning = now;
        return;
    }
    if (sw_pos == SwitchPos::LOW || !hal.util->get_soft_armed() || !vehicle.get_likely_flying()) {
        // Abort, revert parameters
        if (need_restore) {
            need_restore = false;
            restore_all_params();
            GCS_SEND_TEXT(MAV_SEVERITY_EMERGENCY, "Tuning: Reverted");
            tune_done_time = 0;
        }
        reset_axes_done();
        return;
    }
    if (sw_pos == sw_pos_save) {
        // Save all params
        if (need_restore) {
            need_restore = false;
            save_all_params();
            GCS_SEND_TEXT(MAV_SEVERITY_NOTICE, "Tuning: Saved");
        }
    }
    if (sw_pos != sw_pos_tune) {
        return;
    }

    if (now - last_stage_change < STAGE_DELAY) {
        // Update slew gain
        if (slew_parm != Param::END) {
            float P = get_param_value(slew_parm);
            AxisName axis = get_axis(slew_parm);
            // local ax_stage = string.sub(slew_parm, -1)
            adjust_gain(slew_parm, P+slew_delta);
            slew_steps = slew_steps - 1;
            Write_QUIK(get_slew_rate(axis), P, slew_parm);
            if (slew_steps == 0) {
                GCS_SEND_TEXT(MAV_SEVERITY_INFO, "%s %.4f", get_param_name(slew_parm), P);
                slew_parm = Param::END;
                if (get_current_axis() == AxisName::DONE) {
                    GCS_SEND_TEXT(MAV_SEVERITY_NOTICE, "Tuning: DONE");
                    tune_done_time = now;
                }
            }
        }
        return;
    }

    const AxisName axis = get_current_axis();

    if (axis == AxisName::DONE) {
        // Nothing left to do, check autosave time
        if (tune_done_time != 0 && auto_save > 0) {
            if (now - tune_done_time > (auto_save*1000)) {
                need_restore = false;
                save_all_params();
                GCS_SEND_TEXT(MAV_SEVERITY_NOTICE, "Tuning: Saved");
                tune_done_time = 0;
            }
        }
        return;
    }

    if (!need_restore) {
        need_restore = true;
        // We are just starting tuning, get current values
        GCS_SEND_TEXT(MAV_SEVERITY_NOTICE, "Tuning: Starting tune");
        // Get all params
        for (int8_t pname = 0; pname < uint8_t(Param::END); pname++) {
            param_saved[pname] = get_param_value(Param(pname));
        }
        // Set up SMAX
        Param is[3];
        is[0] = Param::RLL_SMAX;
        is[1] = Param::PIT_SMAX;
        is[2] = Param::YAW_SMAX;
        for (uint8_t i = 0; i < 3; i++) {
            float smax = get_param_value(is[i]);
            if (smax <= 0) {
                adjust_gain(is[i], DEFAULT_SMAX); 
            }
        }
    }

    if (now - last_pilot_input < PILOT_INPUT_DELAY) {
        return;
    }

    if (!BIT_IS_SET(filters_done, uint8_t(axis))) {
        GCS_SEND_TEXT(MAV_SEVERITY_INFO, "Starting %s tune", get_axis_name(axis));
        setup_filters(axis);
    }

    Param pname = get_pname(axis, current_stage);
    float pval = get_param_value(pname);
    float limit = gain_limit(pname);
    bool limited = (limit > 0.0 && pval >= limit);
    float srate = get_slew_rate(axis);
    bool oscillating = srate > osc_smax;
    
    // Check if reached limit
    if (limited || oscillating) {
        float reduction = (100.0-gain_margin)*0.01;
        if (!oscillating) {
            reduction = 1.0;
        }
        float new_gain = pval * reduction;
        if (limit > 0.0 && new_gain > limit) {
            new_gain = limit;
        }
        float old_gain = param_saved[uint8_t(pname)];
        if (new_gain < old_gain && (pname == Param::PIT_D || pname == Param::RLL_D)) {
            // We are lowering a D gain from the original gain. Also lower the P gain by the same amount so that we don't trigger P oscillation. We don't drop P by more than a factor of 2
            float ratio = fmaxf(new_gain / old_gain, 0.5);
            Param P_name = Param(uint8_t(pname)-2); //from D to P
            float old_pval = get_param_value(P_name);;
            float new_pval = old_pval * ratio;
            GCS_SEND_TEXT(MAV_SEVERITY_INFO, "Adjusting %s %.3f -> %.3f", get_param_name(P_name), old_pval, new_pval);
            adjust_gain_limited(P_name, new_pval);
        }
        // Set up slew gain
        slew_parm = pname;
        slew_target = limit_gain(pname, new_gain);
        slew_steps = UPDATE_RATE_HZ/2;
        slew_delta = (slew_target - get_param_value(pname)) / slew_steps;

        Write_QUIK(srate, pval, pname);
        GCS_SEND_TEXT(MAV_SEVERITY_INFO, "Tuning: %s done", get_param_name(pname));
        advance_stage(axis);
        last_stage_change = now;
    } else {
        float new_gain = pval*get_gain_mul();
        if (new_gain <= 0.0001) {
            new_gain = 0.001;
        }
        adjust_gain_limited(pname, new_gain);
        Write_QUIK(srate, pval, pname);
        if (now - last_gain_report > 3000) {
            last_gain_report = now;
            GCS_SEND_TEXT(MAV_SEVERITY_INFO, "%s %.4f sr:%.2f", get_param_name(pname), new_gain, srate);
        }
    }
}

/*
  abort the tune if it has started
 */
void AP_Quicktune::abort_tune()
{
    if (need_restore) {
        need_restore = false;
        restore_all_params();
    }
    tune_done_time = 0;
    reset_axes_done();
    sw_pos = SwitchPos::LOW;
}

void AP_Quicktune::update_switch_pos(const  RC_Channel::AuxSwitchPos ch_flag) 
{
    sw_pos = SwitchPos(ch_flag);
}

void AP_Quicktune::reset_axes_done()
{
    axes_done = 0;
    filters_done = 0;
    current_stage = Stage::D;
}

void AP_Quicktune::setup_filters(AP_Quicktune::AxisName axis)
{
    if (auto_filter <= 0) {
        BIT_SET(filters_done, uint8_t(axis));
    }
    AP_InertialSensor *imu = AP_InertialSensor::get_singleton();
    if (imu == nullptr) {
        GCS_SEND_TEXT(MAV_SEVERITY_EMERGENCY, "Quicktune: can't find IMU.");
        return;
    }
    float gyro_filter = imu->get_gyro_filter_hz();
    adjust_gain(get_pname(axis, Stage::FLTT), gyro_filter * FLTT_MUL);
    adjust_gain(get_pname(axis, Stage::FLTD), gyro_filter * FLTT_MUL);

    if (axis == AxisName::YAW) {
        float FLTE = get_param_value(Param::YAW_FLTE);
        if (FLTE < 0.0 || FLTE > YAW_FLTE_MAX) {
            adjust_gain(Param::YAW_FLTE, YAW_FLTE_MAX);
        }
    }
    BIT_SET(filters_done, uint8_t(axis));
}

// Get the axis name we are working on, or DONE for all done 
AP_Quicktune::AxisName AP_Quicktune::get_current_axis()
{
    for (int8_t i = 0; i < int8_t(AxisName::DONE); i++) {
        if (BIT_IS_SET(axes_enabled, i) == true && BIT_IS_SET(axes_done, i) == false) {
            return AxisName(i);
        }
    }
    return AxisName::DONE;
}

float AP_Quicktune::get_slew_rate(AP_Quicktune::AxisName axis)
{
    auto &attitude_control = *AC_AttitudeControl::get_singleton();
    switch(axis) {
    case AxisName::RLL:
        return attitude_control.get_rate_roll_pid().get_pid_info().slew_rate;
    case AxisName::PIT:
        return attitude_control.get_rate_pitch_pid().get_pid_info().slew_rate;
    case AxisName::YAW:
        return attitude_control.get_rate_yaw_pid().get_pid_info().slew_rate;
    default:
        INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
        return 0.0;
    }
}

// Move to next stage of tune
void AP_Quicktune::advance_stage(AP_Quicktune::AxisName axis)
{
    if (current_stage == Stage::D) {
        current_stage = Stage::P;
    } else {
        BIT_SET(axes_done, uint8_t(axis));
        GCS_SEND_TEXT(MAV_SEVERITY_NOTICE, "Tuning: %s done", get_axis_name(axis));
        current_stage = Stage::D;
    }
}

void AP_Quicktune::adjust_gain(AP_Quicktune::Param param, float value)
{
    need_restore = true;
    BIT_SET(param_changed, uint8_t(param));
    set_param_value(param, value);

    if (get_stage(param) == Stage::P) {
        // Also change I gain
        Param iname = Param(uint8_t(param)+1);
        Param ffname = Param(uint8_t(param)+7);
        float FF = get_param_value(ffname);
        if (FF > 0) {
            // If we have any FF on an axis then we don't couple I to P,
            // usually we want I = FF for a one second time constant for trim
            return;
        }
        BIT_SET(param_changed, uint8_t(iname));

        // Work out ratio of P to I that we want
        float pi_ratio = rp_pi_ratio;
        if (get_axis(param) == AxisName::YAW) {
            pi_ratio = y_pi_ratio;
        }
        if (pi_ratio >= 1) {
            set_param_value(iname, value/pi_ratio);
        }
    }

}

void AP_Quicktune::adjust_gain_limited(AP_Quicktune::Param param, float value)
{
    adjust_gain(param, limit_gain(param, value));
}

float AP_Quicktune::limit_gain(AP_Quicktune::Param param, float value)
{
    float saved_value = param_saved[uint8_t(param)];
    if (reduce_max >= 0 && reduce_max < 100 && saved_value > 0) {
        // Check if we exceeded gain reduction
        float reduction_pct = 100.0 * (saved_value - value) / saved_value;
        if (reduction_pct > reduce_max) {
            float new_value = saved_value * (100 - reduce_max) * 0.01;
            GCS_SEND_TEXT(MAV_SEVERITY_INFO, "Limiting %s %.3f -> %.3f", get_param_name(param), value, new_value);
            value = new_value;
        }
    }
   return value;
}

const char* AP_Quicktune::get_param_name(AP_Quicktune::Param param)
{
    switch (param)
    {
        case Param::RLL_P:
            return "Roll P";
        case Param::RLL_I:
            return "Roll I";
        case Param::RLL_D:
            return "Roll D";
        case Param::PIT_P:
            return "Pitch P";
        case Param::PIT_I:
            return "Pitch I";
        case Param::PIT_D:
            return "Pitch D";
        case Param::YAW_P:
            return "Yaw P";
        case Param::YAW_I:
            return "Yaw I";
        case Param::YAW_D:
            return "Yaw D";
        default:
            INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
            return "UNK";
    }
}

float AP_Quicktune::get_gain_mul()
{
   return expf(logf(2.0)/(UPDATE_RATE_HZ*double_time));
}

void AP_Quicktune::restore_all_params()
{
    for (int8_t pname = 0; pname < uint8_t(Param::END); pname++) {
        if (BIT_IS_SET(param_changed, pname)) {
            set_param_value(Param(pname), param_saved[pname]);
            BIT_CLEAR(param_changed, pname);
        }
    }
}

void AP_Quicktune::save_all_params()
{
    // for pname in pairs(params) do
    for (int8_t pname = 0; pname < uint8_t(Param::END); pname++) {
        if (BIT_IS_SET(param_changed, pname)) {
            set_and_save_param_value(Param(pname), get_param_value(Param(pname)));
            param_saved[pname] = get_param_value(Param(pname));
            BIT_CLEAR(param_changed, pname);
        }
    }
}

AP_Quicktune::Param AP_Quicktune::get_pname(AP_Quicktune::AxisName axis, AP_Quicktune::Stage stage)
{
    switch (axis)
    {
        case AxisName::RLL:
            switch (stage)
            {
                case Stage::P:
                    return Param::RLL_P;
                case Stage::D:
                    return Param::RLL_D;
                case Stage::FLTT:
                    return Param::RLL_FLTT;
                case Stage::FLTD:
                    return Param::RLL_FLTD;
                default:
                    INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
                    return Param::END;
            }
        case AxisName::PIT:
            switch (stage)
            {
                case Stage::P:
                    return Param::PIT_P;
                case Stage::D:
                    return Param::PIT_D;
                case Stage::FLTT:
                    return Param::PIT_FLTT;
                case Stage::FLTD:
                    return Param::PIT_FLTD;
                default:
                    INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
                    return Param::END;
            }
        case AxisName::YAW:
            switch (stage)
            {
                case Stage::P:
                    return Param::YAW_P;
                case Stage::D:
                    return Param::YAW_D;
                case Stage::FLTT:
                    return Param::YAW_FLTT;
                case Stage::FLTD:
                    return Param::YAW_FLTD;
                default:
                    INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
                    return Param::END;
            }
        default:
            INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
            return Param::END;
    }
}

AP_Quicktune::Stage AP_Quicktune::get_stage(AP_Quicktune::Param param)
{
    if (param == Param::RLL_P || param == Param::PIT_P || param == Param::YAW_P) {
        return Stage::P;
    } else if (param == Param::RLL_I || param == Param::PIT_I || param == Param::YAW_I) {
        return Stage::I;
    } else if (param == Param::RLL_D || param == Param::PIT_D || param == Param::YAW_D) {
        return Stage::D;
    } else if (param == Param::RLL_SMAX || param == Param::PIT_SMAX || param == Param::YAW_SMAX) {
        return Stage::SMAX;
    } else if (param == Param::RLL_FLTT || param == Param::PIT_FLTT || param == Param::YAW_FLTT) {
        return Stage::FLTT;
    } else if (param == Param::RLL_FLTD || param == Param::PIT_FLTD || param == Param::YAW_FLTD) {
        return Stage::FLTD;
    } else if (param == Param::RLL_FLTE || param == Param::PIT_FLTE || param == Param::YAW_FLTE) {
        return Stage::FLTE;
    } else if (param == Param::RLL_FF || param == Param::PIT_FF || param == Param::YAW_FF) {
        return Stage::FF;
    } else {
        INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
        return Stage::END;
    }
}

AP_Float *AP_Quicktune::get_param_pointer(AP_Quicktune::Param param)
{
    auto &attitude_control = *AC_AttitudeControl::get_singleton();
    AC_PID* pid_ptr;

    AxisName axis = get_axis(param);
    switch (axis)
    {
        case AxisName::RLL:
            pid_ptr = &attitude_control.get_rate_roll_pid();
            break;
        case AxisName::PIT:
            pid_ptr =  &attitude_control.get_rate_pitch_pid();
            break;
        case AxisName::YAW:
            pid_ptr =  &attitude_control.get_rate_yaw_pid();
            break;
        default:
            INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
            return nullptr;
    }
    
    Stage stage = get_stage(param);
    switch (stage)
    {
        case Stage::P:
            return &pid_ptr->kP();
        case Stage::I:
            return &pid_ptr->kI();
        case Stage::D:
            return &pid_ptr->kD();
        case Stage::SMAX:
            return &pid_ptr->slew_limit();
        case Stage::FLTT:
            return &pid_ptr->filt_T_hz();
        case Stage::FLTD:
            return &pid_ptr->filt_D_hz();
        case Stage::FLTE:
            return &pid_ptr->filt_E_hz();
        case Stage::FF:
            return &pid_ptr->ff();
        default:
            INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
            return nullptr;
    }
}

float AP_Quicktune::get_param_value(AP_Quicktune::Param param)
{
    AP_Float *ptr = get_param_pointer(param);
    if (ptr != nullptr) {
        return ptr->get();
    }
    INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
    return 0.0;
}

void AP_Quicktune::set_param_value(AP_Quicktune::Param param, float value)
{
    AP_Float *ptr = get_param_pointer(param);
    if (ptr != nullptr) {
       ptr->set(value);
       return;
    }
    INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
    return;
}

void AP_Quicktune::set_and_save_param_value(AP_Quicktune::Param param, float value)
{
    AP_Float *ptr = get_param_pointer(param);
    if (ptr != nullptr) {
       ptr->set_and_save(value);
       return;
    }
    INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
    return;
}

AP_Quicktune::AxisName AP_Quicktune::get_axis(AP_Quicktune::Param param)
{
    if (param < Param::PIT_P) {
        return AxisName::RLL;
    } else if (param < Param::YAW_P) {
        return AxisName::PIT;
    } else if (param < Param::END) {
        return AxisName::YAW;
    } else {
        return AxisName::END;
    }
}

const char* AP_Quicktune::get_axis_name(AP_Quicktune::AxisName axis)
{
    switch (axis)
    {
        case AxisName::RLL:
            return "Roll";
        case AxisName::PIT:
            return "Pitch";
        case AxisName::YAW:
            return "Yaw";
        default:
            INTERNAL_ERROR(AP_InternalError::error_t::flow_of_control);
            return "UNK";
    }
}

float AP_Quicktune::gain_limit(AP_Quicktune::Param param)
{
    if (get_axis(param) == AxisName::YAW) {
        if (param == Param::YAW_P) {
            return yaw_p_max;
        }
        if (param == Param::YAW_D) {
            return yaw_d_max;
        }
    }
   return 0.0;
}


// @LoggerMessage: QUIK
// @Description: Quicktune
// @Field: TimeUS: Time since system startup
// @Field: SRate: slew rate
// @Field: Gain: test gain for current axis and PID element
// @Field: Param: name of parameter being being tuned
// @Field: ParamNo: number of parameter being tuned
void AP_Quicktune::Write_QUIK(float srate, float gain, AP_Quicktune::Param param)
{
#if HAL_LOGGING_ENABLED
    AP::logger().WriteStreaming("QUIK","TimeUS,SRate,Gain,Param,ParamNo", "QffNI",
                                AP_HAL::micros64(),
                                srate,
                                gain,
                                get_param_name(param),
                                unsigned(param));
#endif
}

#endif //AP_QUICKTUNE_ENABLED
