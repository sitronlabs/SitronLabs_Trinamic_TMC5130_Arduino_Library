/* Self header */
#include "tmc5130.h"

/**
 *
 * @param[in] config
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 *  -ENODEV If the device was not detected
 */
int tmc5130::setup(struct config &config) {
    int res;

    /* Ensure driver is dectected and has the expected version */
    union reg_io_input_output reg_io_input_output = {0};
    res = register_read(reg::IO_INPUT_OUTPUT, reg_io_input_output.raw);
    if (res < 0) {
        return -EIO;
    }
    if (reg_io_input_output.fields.version != 0x11) {
        return -ENODEV;
    }

    /* Clear the reset and charge pump undervoltage flags */
    union reg_gstat reg_gstat = {0};
    reg_gstat.fields.reset = 1;
    reg_gstat.fields.uv_cp = 1;
    res = register_write(reg::GSTAT, reg_gstat.raw);
    if (res < 0) {
        return -EIO;
    }

    /* Write configuration registers */
    res = 0;
    res |= register_write(reg::CHOPCONF, config.reg_chopconf.raw);
    res |= register_write(reg::IHOLD_IRUN, config.reg_ihold_irun.raw);
    res |= register_write(reg::TPOWERDOWN, config.reg_tpowerdown.raw);
    res |= register_write(reg::GCONF, config.reg_gconf.raw);
    res |= register_write(reg::TPWMTHRS, config.reg_tpwmthrs.raw);
    res |= register_write(reg::PWMCONF, config.reg_pwmconf.raw);
    if (res < 0) {
        return -EIO;
    }

    /* Set default speeds
     * This is done at least here because the datasheet explicitely says that D1 and VSTOP should not be set to 0 */
    res = 0;
    res |= register_write(reg::RAMPMODE, 0);
    res |= register_write(reg::VSTART, 0);
    res |= register_write(reg::V_1, 0);
    res |= register_write(reg::VSTOP, 10);
    res |= register_write(reg::VMAX, 100);
    res |= register_write(reg::AMAX, 10000);
    res |= register_write(reg::DMAX, 10000);
    res |= register_write(reg::A_1, 10000);
    res |= register_write(reg::D_1, 10000);
    if (res < 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] vstart
 * @param[in] vstop
 * @param[in] vtrans
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::speed_ramp_set(const float vstart, const float vstop, const float vtrans) {
    int res;

    /*  */
    res = 0;
    res |= register_write(reg::VSTART, convert_velocity_to_tmc(fabs(vstart)));
    res |= register_write(reg::VSTOP, convert_velocity_to_tmc(fabs(vstop)));
    res |= register_write(reg::V_1, convert_velocity_to_tmc(fabs(vtrans)));
    if (res < 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] speed
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::speed_limit_set(const float speed) {
    int res;

    /* Ensure speed is positive */
    if (speed < 0) {
        return -EINVAL;
    }

    /* Write register */
    res = 0;
    res |= register_write(reg::VMAX, convert_velocity_to_tmc(speed));
    if (res < 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] acceleration
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::acceleration_limit_set(const float acceleration) {
    int res;

    /* Ensure acceleration is positive */
    if (acceleration < 0) {
        return -EINVAL;
    }

    /* Write registers  */
    res = 0;
    res |= register_write(reg::AMAX, convert_acceleration_to_tmc(acceleration));
    res |= register_write(reg::DMAX, convert_acceleration_to_tmc(acceleration));
    res |= register_write(reg::A_1, convert_acceleration_to_tmc(acceleration));
    res |= register_write(reg::D_1, convert_acceleration_to_tmc(acceleration));
    if (res < 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] position
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::move_to_position(const float position) {
    int res;

    /* Set RAMPMODE to Positioning mode */
    res = register_write(reg::RAMPMODE, 0x00);
    if (res < 0) {
        return -EIO;
    }

    /* Set XTARGET */
    int32_t reg_xtarget = roundf(position * m_ustep_per_step);
    res = register_write(reg::XTARGET, (uint32_t)reg_xtarget);
    if (res < 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] velocity
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::move_at_velocity(const float velocity) {
    int res;

    /* */
    res |= register_write(reg::VMAX, convert_velocity_to_tmc(fabs(velocity)));
    res |= register_write(reg::RAMPMODE, velocity < 0.0f ? 2 : 1);
    if (res < 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::move_stop(void) {

    /* For a stop in positioning mode, set VSTART=0 and VMAX=0 */
    int res = 0;
    res |= register_write(reg::VSTART, 0);
    res |= register_write(reg::VMAX, 0);
    if (res != 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[out] position
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::position_current_get(float &position) {
    uint32_t reg_xactual;
    if (register_read(reg::XACTUAL, reg_xactual) < 0) {
        return -EIO;
    }
    position = (int32_t)reg_xactual;
    position /= m_ustep_per_step;
    return 0;
}

/**
 *
 * @param[out] position
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::position_latched_get(float &position) {
    int32_t position_ustep;
    uint32_t reg_xlatch;
    if (register_read(reg::XLATCH, reg_xlatch) < 0) {
        return -EIO;
    }
    position = (int32_t)reg_xlatch;
    position /= m_ustep_per_step;
    return 0;
}

/**
 * @return 1 if the target position has been reached, 0 if it has not, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::target_position_reached_is(void) {

    /* Read bit 9 position_reached of RAMP_STAT
     * 1: Signals, that the target position is reached. This flag becomes set while XACTUAL and XTARGET match */
    uint32_t reg_ramp_status;
    if (register_read(reg::RAMP_STAT, reg_ramp_status) < 0) {
        return -EIO;
    } else if (reg_ramp_status & (1 << 9)) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * @return 1 if the target velocity has been reached, 0 if it has not, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::target_velocity_reached_is(void) {

    /* Read bit 8 velocity_reached of RAMP_STAT
     * 1: Signals, that the target velocity is reached. This flag becomes set while VACTUAL and VMAX match. */
    uint32_t reg_ramp_status;
    if (register_read(reg::RAMP_STAT, reg_ramp_status) < 0) {
        return -EIO;
    } else if (reg_ramp_status & (1 << 8)) {
        return 1;
    } else {
        return 0;
    }
}

/**
 *
 * @see Datasheet, section 14.1 Real World Unit Conversion
 */
uint32_t tmc5130::convert_velocity_to_tmc(const float velocity) {
    return (int32_t)(velocity / ((float)m_fclk / (float)(1ul << 24)) * (float)m_ustep_per_step);
}

/**
 *
 * @see Datasheet, section 14.1 Real World Unit Conversion
 */
uint32_t tmc5130::convert_acceleration_to_tmc(const float acceleration) {
    return (int32_t)(acceleration / ((float)m_fclk * (float)m_fclk / (512.0 * 256.0) / (float)(1ul << 24)) * (float)m_ustep_per_step);
}
