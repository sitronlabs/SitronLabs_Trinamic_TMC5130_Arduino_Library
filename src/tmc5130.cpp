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
 *
 * @return 1 if the target position has been reached, 0 if it has not, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::target_position_reached_is(void) {

    /* Read bit 9 position_reached of RAMP_STAT
     * 1: Signals, that the target position is reached. This flag becomes set while XACTUAL and XTARGET match */
    uint32_t reg_ramp_status;
    if (register_read(reg::RAMP_STAT, reg_ramp_status) < 0) {
        return -EIO;
    }

    /* Remember flags that are cleared upon reading */
    m_reference_l_latched = (reg_ramp_status & (1 << 2)) ? true : m_reference_l_latched;
    m_reference_r_latched = (reg_ramp_status & (1 << 3)) ? true : m_reference_r_latched;

    /* Return wether the target is reached */
    if (reg_ramp_status & (1 << 9)) {
        return 1;
    } else {
        return 0;
    }
}

/**
 *
 * @return 1 if the target velocity has been reached, 0 if it has not, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::target_velocity_reached_is(void) {

    /* Read bit 8 velocity_reached of RAMP_STAT
     * 1: Signals, that the target velocity is reached. This flag becomes set while VACTUAL and VMAX match. */
    uint32_t reg_ramp_status;
    if (register_read(reg::RAMP_STAT, reg_ramp_status) < 0) {
        return -EIO;
    }

    /* Remember flags that are cleared upon reading */
    m_reference_l_latched = (reg_ramp_status & (1 << 2)) ? true : m_reference_l_latched;
    m_reference_r_latched = (reg_ramp_status & (1 << 3)) ? true : m_reference_r_latched;

    /* Return wether the target is reached */
    if (reg_ramp_status & (1 << 8)) {
        return 1;
    } else {
        return 0;
    }
}

/**
 *
 * @param[in] swap
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::reference_swap(bool swap) {

    /* Set bit 4 of SW_MODE
     * 1: Swap the left and the right reference switch input REFL and REFR */
    uint32_t reg_sw_mode;
    if (register_read(reg::SW_MODE, reg_sw_mode) < 0) {
        return -EIO;
    }
    if (swap) {
        reg_sw_mode |= (1 << 4);
    } else {
        reg_sw_mode &= ~(1 << 4);
    }
    if (register_write(reg::SW_MODE, reg_sw_mode) < 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] active_high
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::reference_l_polarity_set(bool active_high) {

    /* Set bit 2 of SW_MODE
     * Sets the active polarity of the left reference switch input
     * 0=non-inverted, high active: a high level on REFL stops the motor
     * 1=inverted, low active: a low level on REFL stops the motor */
    uint32_t reg_sw_mode;
    if (register_read(reg::SW_MODE, reg_sw_mode) < 0) {
        return -EIO;
    }
    if (active_high) {
        reg_sw_mode &= ~(1 << 2);
    } else {
        reg_sw_mode |= (1 << 2);
    }
    if (register_write(reg::SW_MODE, reg_sw_mode) < 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] active_high
 * @return 0 in case of success, or a negative error code otherwise, in particular:
 *  -EIO If there was an error communicating with the device
 */
int tmc5130::reference_r_polarity_set(bool active_high) {

    /* Set bit 3 of SW_MODE
     * Sets the active polarity of the right reference switch input
     * 0=non-inverted, high active: a high level on REFR stops the motor
     * 1=inverted, low active: a low level on REFR stops the motor */
    uint32_t reg_sw_mode;
    if (register_read(reg::SW_MODE, reg_sw_mode) < 0) {
        return -EIO;
    }
    if (active_high) {
        reg_sw_mode &= ~(1 << 3);
    } else {
        reg_sw_mode |= (1 << 3);
    }
    if (register_write(reg::SW_MODE, reg_sw_mode) < 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 */
int tmc5130::reference_l_active_get(void) {

    /* Read bit 0 status_stop_l of RAMP_STAT
     * Reference switch left status (1=active) */
    uint32_t reg_ramp_status;
    if (register_read(reg::RAMP_STAT, reg_ramp_status) < 0) {
        return -EIO;
    }

    /* Remember flags that are cleared upon reading */
    m_reference_l_latched = (reg_ramp_status & (1 << 2)) ? true : m_reference_l_latched;
    m_reference_r_latched = (reg_ramp_status & (1 << 3)) ? true : m_reference_r_latched;

    /* Return wether the switch is active */
    return (reg_ramp_status & (1 << 0)) ? 1 : 0;
}

/**
 *
 */
int tmc5130::reference_r_active_get(void) {

    /* Read bit 1 status_stop_r of RAMP_STAT
     * Reference switch right status (1=active) */
    uint32_t reg_ramp_status;
    if (register_read(reg::RAMP_STAT, reg_ramp_status) < 0) {
        return -EIO;
    }

    /* Remember flags that are cleared upon reading */
    m_reference_l_latched = (reg_ramp_status & (1 << 2)) ? true : m_reference_l_latched;
    m_reference_r_latched = (reg_ramp_status & (1 << 3)) ? true : m_reference_r_latched;

    /* Return wether the switch is active */
    return (reg_ramp_status & (1 << 1)) ? 1 : 0;
}

/**
 *
 * @param[in] polarity If true the position will be latched when the reference switch goes active, and conversely.
 */
int tmc5130::reference_l_latch_enable(bool polarity) {

    /* Reset flag */
    m_reference_l_latched = false;

    /* Set bit 6 and 5 of SW_MODE */
    uint32_t reg_sw_mode;
    if (register_read(reg::SW_MODE, reg_sw_mode) < 0) {
        return -EIO;
    }
    if (polarity) {
        reg_sw_mode &= ~(1 << 6);  // latch_l_inactive = 0
        reg_sw_mode |= (1 << 5);   // latch_l_active = 1
    } else {
        reg_sw_mode |= (1 << 6);   // latch_l_inactive = 1
        reg_sw_mode &= ~(1 << 5);  // latch_l_active = 0
    }
    if (register_write(reg::SW_MODE, reg_sw_mode) < 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] polarity If true the position will be latched when the reference switch goes active, and conversely.
 */
int tmc5130::reference_r_latch_enable(bool polarity) {

    /* Reset flag */
    m_reference_r_latched = false;

    /* Set bit 8 and 7 of SW_MODE */
    uint32_t reg_sw_mode;
    if (register_read(reg::SW_MODE, reg_sw_mode) < 0) {
        return -EIO;
    }
    if (polarity) {
        reg_sw_mode &= ~(1 << 8);  // latch_r_inactive = 0
        reg_sw_mode |= (1 << 7);   // latch_r_active = 1
    } else {
        reg_sw_mode |= (1 << 8);   // latch_r_inactive = 1
        reg_sw_mode &= ~(1 << 7);  // latch_r_active = 0
    }
    if (register_write(reg::SW_MODE, reg_sw_mode) < 0) {
        return -EIO;
    }

    /* Return success */
    return 0;
}

/**
 *
 * @param[out] position
 * @return 1 if the latched position is available, 0 if it is not, or a negative error code otherwise.
 */
int tmc5130::reference_l_latch_get(float &position) {

    /* Read bit 2 status_latch_l of RAMP_STAT
     * 1: Latch left ready */
    uint32_t reg_ramp_status;
    if (register_read(reg::RAMP_STAT, reg_ramp_status) < 0) {
        return -EIO;
    }

    /* Remember flags that are cleared upon reading */
    m_reference_l_latched = (reg_ramp_status & (1 << 2)) ? true : m_reference_l_latched;
    m_reference_r_latched = (reg_ramp_status & (1 << 3)) ? true : m_reference_r_latched;

    /* If latched position is available */
    if (m_reference_l_latched) {

        /* Retrieve position */
        uint32_t reg_xlatch;
        if (register_read(reg::XLATCH, reg_xlatch) < 0) {
            return -EIO;
        }
        position = (int32_t)reg_xlatch;
        position /= m_ustep_per_step;

        /* Reset flag */
        m_reference_l_latched = false;

        /* Return success */
        return 1;
    } else {
        return 0;
    }
}

/**
 *
 * @param[out] position
 * @return 1 if the latched position is available, 0 if it is not, or a negative error code otherwise.
 */
int tmc5130::reference_r_latch_get(float &position) {

    /* Read bit 3 status_latch_r of RAMP_STAT
     * 1: Latch right ready */
    uint32_t reg_ramp_status;
    if (register_read(reg::RAMP_STAT, reg_ramp_status) < 0) {
        return -EIO;
    }

    /* Remember flags that are cleared upon reading */
    m_reference_l_latched = (reg_ramp_status & (1 << 2)) ? true : m_reference_l_latched;
    m_reference_r_latched = (reg_ramp_status & (1 << 3)) ? true : m_reference_r_latched;

    /* If latched position is available */
    if (m_reference_r_latched) {

        /* Retrieve position */
        uint32_t reg_xlatch;
        if (register_read(reg::XLATCH, reg_xlatch) < 0) {
            return -EIO;
        }
        position = (int32_t)reg_xlatch;
        position /= m_ustep_per_step;

        /* Reset flag */
        m_reference_r_latched = false;

        /* Return success */
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
