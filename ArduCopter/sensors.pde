// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-

// Sensors are not available in HIL_MODE_ATTITUDE
#if HIL_MODE != HIL_MODE_ATTITUDE

static void ReadSCP1000(void) {
}

#if CONFIG_SONAR == ENABLED

static ModeFilterInt16_Size3 sonar_mode_filter(1);
static RangeFinder* sonar = NULL;

//only used for I2C sonar
static AP_RangeFinder_MaxsonarI2CXL* i2c_sonar = NULL;
static bool requestedSonarReading = false;
static int16_t lastSuccessfullSonarReading = 0;

static void init_sonar(void)
{
	# if CONFIG_COLLISION_AVOIDANCE == ENABLED
	if(!is_collision_avoidance_enabled()){
	#endif
		i2c_sonar = new AP_RangeFinder_MaxsonarI2CXL(&sonar_mode_filter);

		if(i2c_sonar->test()){
			i2c_sonar->take_reading(); //take a first reading, simplifies the reading algorithm
		    sonar = i2c_sonar;
		}else{
			i2c_sonar = NULL;
			#if CONFIG_SONAR_SOURCE == SONAR_SOURCE_ADC || CONFIG_SONAR_SOURCE == SONAR_SOURCE_ANALOG_PIN
			AP_HAL::AnalogSource *sonar_analog_source;
			#endif

		 	#if CONFIG_SONAR_SOURCE == SONAR_SOURCE_ADC
			sonar_analog_source = new AP_ADC_AnalogSource(
					&adc, CONFIG_SONAR_SOURCE_ADC_CHANNEL, 0.25);
			#elif CONFIG_SONAR_SOURCE == SONAR_SOURCE_ANALOG_PIN
			sonar_analog_source = hal.analogin->channel(
					CONFIG_SONAR_SOURCE_ANALOG_PIN);
		 	#endif

			#if CONFIG_SONAR_SOURCE == SONAR_SOURCE_ADC || CONFIG_SONAR_SOURCE == SONAR_SOURCE_ANALOG_PIN
			AP_RangeFinder_MaxsonarXL* tmp_sonar = new AP_RangeFinder_MaxsonarXL(sonar_analog_source,
					&sonar_mode_filter);
			#if CONFIG_SONAR_SOURCE == SONAR_SOURCE_ADC
			tmp_sonar->calculate_scaler(g.sonar_type, 3.3f);
			#else
			tmp_sonar->calculate_scaler(g.sonar_type, 5.0f);
			#endif
			sonar = tmp_sonar;
			#endif
		}
	# if CONFIG_COLLISION_AVOIDANCE == ENABLED
	}
	#endif
}
#endif

static void init_barometer(void)
{
    barometer.calibrate();
    ahrs.set_barometer(&barometer);
    gcs_send_text_P(SEVERITY_LOW, PSTR("barometer calibration complete"));
}

// return barometric altitude in centimeters
static int32_t read_barometer(void)
{
    barometer.read();
    return baro_filter.apply(barometer.get_altitude() * 100.0f);
}

// return sonar altitude in centimeters
static int16_t read_sonar(void)
{
#if CONFIG_SONAR == ENABLED
    // exit immediately if sonar is disabled
    if( !g.sonar_enabled ) {
        sonar_alt_health = 0;
        return 0;
    }

    int16_t min_distance = 0;
    int16_t max_distance = 0;
    int16_t temp_alt = 0;

    // if sonar is NULL we are using the sensor from the collision avoidance
	# if CONFIG_COLLISION_AVOIDANCE == ENABLED
    if(sonar == NULL){
    	min_distance = collision_sensor->get_min_distance(CA_BOTTOM);
    	max_distance = collision_sensor->get_max_distance(CA_BOTTOM);
    	temp_alt = collision_sensor->get_collision_distance(CA_BOTTOM);
    }else{
	#endif
    	min_distance = sonar->min_distance;
    	max_distance = sonar->max_distance;
    	temp_alt = sonar->read();
	# if CONFIG_COLLISION_AVOIDANCE == ENABLED
    }
	#endif

    // if i2c sonar is unequal NULL we are using it
    if(i2c_sonar != NULL){
		if(i2c_sonar->healthy){
			//last reading was successful, store and request the next
			lastSuccessfullSonarReading = temp_alt;
			i2c_sonar->take_reading();
		}else{
			//not successful, we stay with the last result
			temp_alt = lastSuccessfullSonarReading;
		}
    }

    if (temp_alt >= min_distance && temp_alt <= max_distance * 0.70f) {
        if ( sonar_alt_health < SONAR_ALT_HEALTH_MAX ) {
            sonar_alt_health++;
        }
    }else{
        sonar_alt_health = 0;
    }

 #if SONAR_TILT_CORRECTION == 1
    // correct alt for angle of the sonar
    float temp = cos_pitch_x * cos_roll_x;
    temp = max(temp, 0.707f);
    temp_alt = (float)temp_alt * temp;
 #endif

    return temp_alt;
#else
    return 0;
#endif //CONFIG_SONAR
}


#endif // HIL_MODE != HIL_MODE_ATTITUDE

static void init_compass()
{
    compass.set_orientation(MAG_ORIENTATION);                                                   // set compass's orientation on aircraft
    if (!compass.init() || !compass.read()) {
        // make sure we don't pass a broken compass to DCM
        cliSerial->println_P(PSTR("COMPASS INIT ERROR"));
        Log_Write_Error(ERROR_SUBSYSTEM_COMPASS,ERROR_CODE_FAILED_TO_INITIALISE);
        return;
    }
    ahrs.set_compass(&compass);
#if SECONDARY_DMP_ENABLED == ENABLED
    ahrs2.set_compass(&compass);
#endif
}

static void init_optflow()
{
#if OPTFLOW == ENABLED
    if( optflow.init() == false ) {
        g.optflow_enabled = false;
        cliSerial->print_P(PSTR("\nFailed to Init OptFlow "));
        Log_Write_Error(ERROR_SUBSYSTEM_OPTFLOW,ERROR_CODE_FAILED_TO_INITIALISE);
    }else{
        // suspend timer while we set-up SPI communication
        hal.scheduler->suspend_timer_procs();

        optflow.set_orientation(OPTFLOW_ORIENTATION);   // set optical flow sensor's orientation on aircraft
        optflow.set_frame_rate(2000);                   // set minimum update rate (which should lead to maximum low light performance
        optflow.set_resolution(OPTFLOW_RESOLUTION);     // set optical flow sensor's resolution
        optflow.set_field_of_view(OPTFLOW_FOV);         // set optical flow sensor's field of view

        // resume timer
        hal.scheduler->resume_timer_procs();
    }
#endif      // OPTFLOW == ENABLED
}

// read_battery - check battery voltage and current and invoke failsafe if necessary
// called at 10hz
#define BATTERY_FS_COUNTER  100     // 100 iterations at 10hz is 10 seconds
static void read_battery(void)
{
    static uint8_t low_battery_counter = 0;

    if(g.battery_monitoring == BATT_MONITOR_DISABLED) {
        battery_voltage1 = 0;
        return;
    }

    if(g.battery_monitoring == BATT_MONITOR_VOLTAGE_ONLY || g.battery_monitoring == BATT_MONITOR_VOLTAGE_AND_CURRENT) {
        batt_volt_analog_source->set_pin(g.battery_volt_pin);
        battery_voltage1 = BATTERY_VOLTAGE(batt_volt_analog_source);
    }
    if(g.battery_monitoring == BATT_MONITOR_VOLTAGE_AND_CURRENT) {
        batt_curr_analog_source->set_pin(g.battery_curr_pin);
        current_amps1    = CURRENT_AMPS(batt_curr_analog_source);
        current_total1   += current_amps1 * 0.02778f;            // called at 100ms on average, .0002778 is 1/3600 (conversion to hours)

        // update compass with current value
        compass.set_current(current_amps1);
    }

    // check for low voltage or current if the low voltage check hasn't already been triggered
    if (!ap.low_battery && ( battery_voltage1 < g.low_voltage || (g.battery_monitoring == BATT_MONITOR_VOLTAGE_AND_CURRENT && current_total1 > g.pack_capacity))) {
        low_battery_counter++;
        if( low_battery_counter >= BATTERY_FS_COUNTER ) {
            low_battery_counter = BATTERY_FS_COUNTER;   // ensure counter does not overflow
            low_battery_event();
        }
    }else{
        // reset low_battery_counter in case it was a temporary voltage dip
        low_battery_counter = 0;
    }
}

// read the receiver RSSI as an 8 bit number for MAVLink
// RC_CHANNELS_SCALED message
void read_receiver_rssi(void)
{
    rssi_analog_source->set_pin(g.rssi_pin);
    float ret = rssi_analog_source->read_latest();
    receiver_rssi = constrain(ret, 0, 255);
}
