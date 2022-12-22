#ifndef TMC5130_H
#define TMC5130_H

/* Arduino libraries */
#include <Arduino.h>
#include <SPI.h>

/* C/C++ libraries */
#include <errno.h>

/**
 *
 * @note Use -Wno-packed-bitfield-compat
 */
class tmc5130 {

   public:
    /* Register addresses */
    enum reg {

        /* General configuration registers */
        GCONF = 0x00,            // Global configuration flags
        GSTAT = 0x01,            // Global status flags
        IFCNT = 0x02,            // UART transmission counter
        SLAVECONF = 0x03,        // UART slave configuration
        IO_INPUT_OUTPUT = 0x04,  // Read input / write output pins
        X_COMPARE = 0x05,        // Position  comparison  register

        /* Velocity dependent driver feature control registers */
        IHOLD_IRUN = 0x10,  // Driver current control
        TPOWERDOWN = 0x11,  // Delay before power down
        TSTEP = 0x12,       // Actual time between microsteps
        TPWMTHRS = 0x13,    // Upper velocity for stealthChop voltage PWM mode
        TCOOLTHRS = 0x14,   // Lower threshold velocity for switching on smart energy coolStep and stallGuard feature
        THIGH = 0x15,       // Velocity threshold for switching into a different chopper mode and fullstepping

        /* Ramp generator motion control registers */
        RAMPMODE = 0x20,   // Driving mode (Velocity, Positioning, Hold)
        XACTUAL = 0x21,    // Actual motor position
        VACTUAL = 0x22,    // Actual motor velocity from ramp generator
        VSTART = 0x23,     // Motor start velocity (unsigned).
        A_1 = 0x24,        // First acceleration between VSTART and V1
        V_1 = 0x25,        // First acceleration/deceleration phase target velocity
        AMAX = 0x26,       // Second acceleration between V1 and VMAX
        VMAX = 0x27,       // Target velocity in velocity mode. It can be changed any time during a motion.
        DMAX = 0x28,       // Deceleration between VMAX and V1
        D_1 = 0x2A,        // Deceleration between V1 and VSTOP. Attention: Do not set 0 in positioning mode, even if V1=0!
        VSTOP = 0x2B,      // Motor stop velocity (unsigned). Attention: Set VSTOP > VSTART! Attention: Do not set 0 in positioning mode, minimum 10 recommend!
        TZEROWAIT = 0x2C,  // Waiting time after ramping down to zero velocity before next movement or direction inversion can start. Time range is about 0 to 2 seconds.
        XTARGET = 0x2D,    // Target position for ramp mode

        /* Ramp generator driver feature control registers */
        VDCMIN = 0x33,     // Velocity threshold for enabling automatic commutation dcStep
        SW_MODE = 0x34,    // Switch mode configuration
        RAMP_STAT = 0x35,  // Ramp status and switch event status
        XLATCH = 0x36,     // Ramp generator latch position upon programmable switch event

        /* Motor driver registers */
        CHOPCONF = 0x6C,  // Chopper and driver configuration
        PWMCONF = 0x70,   // Voltage PWM mode chopper configuration
    };

    /* Register description */
    union reg_gconf {
        uint32_t raw;
        struct {
            uint8_t i_scale_analog : 1;
            uint8_t internal_rsense : 1;
            uint8_t en_pwm_mode : 1;
            uint8_t enc_commutation : 1;
            uint8_t shaft : 1;
            uint8_t diag0_error : 1;
            uint8_t diag0_otpw : 1;
            uint8_t diag0_stall_diag0_step : 1;
            uint8_t diag1_stall_diag1_dir : 1;
            uint8_t diag1_index : 1;
            uint8_t diag1_onstate : 1;
            uint8_t diag1_steps_skipped : 1;
            uint8_t diag0_int_pushpull : 1;
            uint8_t diag1_poscomp_pushpull : 1;
            uint8_t small_hysteresis : 1;
            uint8_t stop_enable : 1;
            uint8_t direct_mode : 1;
            uint8_t test_mode : 1;
            uint8_t : 6;
            uint8_t : 8;
        } __attribute__((packed)) fields;
    };
    union reg_gstat {
        uint32_t raw;
        struct {
            uint8_t reset : 1;
            uint8_t drv_err : 1;
            uint8_t uv_cp : 1;
            uint8_t : 5;
            uint8_t : 8;
            uint8_t : 8;
            uint8_t : 8;
        } __attribute__((packed)) fields;
    };
    union reg_io_input_output {
        uint32_t raw;
        struct {
            uint8_t refl_step : 1;
            uint8_t refr_dir : 1;
            uint8_t encb_dcen_cfg4 : 1;
            uint8_t enca_dcin_cfg5 : 1;
            uint8_t drv_enn_cfg6 : 1;
            uint8_t enc_n_dco : 1;
            uint8_t sd_mode : 1;
            uint8_t swcomp_in : 1;
            uint16_t : 16;
            uint8_t version : 8;
        } __attribute__((packed)) fields;
    };
    union reg_tpowerdown {
        uint32_t raw;
        struct {
            uint8_t tpowerdown : 8;
            uint8_t : 8;
            uint8_t : 8;
            uint8_t : 8;
        } __attribute__((packed)) fields;
    };
    union reg_tpwmthrs {
        uint32_t raw;
        struct {
            uint32_t tpwmthrs : 20;
            uint8_t : 4;
            uint8_t : 8;
        } __attribute__((packed)) fields;
    };
    union reg_ihold_irun {
        uint32_t raw;
        struct {
            uint8_t ihold : 5;
            uint8_t : 3;
            uint8_t irun : 5;
            uint8_t : 3;
            uint8_t iholddelay : 4;
            uint8_t : 4;
            uint8_t : 8;
        } __attribute__((packed)) fields;
    };
    union reg_chopconf {
        uint32_t raw;
        struct {
            uint8_t toff : 4;
            uint8_t hstrt : 3;
            uint8_t hend : 4;
            uint8_t fd3 : 1;
            uint8_t disfdcc : 1;
            uint8_t rndtf : 1;
            uint8_t chm : 1;
            uint8_t tbl : 2;
            uint8_t vsense : 1;
            uint8_t vhighfs : 1;
            uint8_t vhighchm : 1;
            uint8_t sync : 4;
            uint8_t mres : 4;
            uint8_t intpol : 1;
            uint8_t dedge : 1;
            uint8_t diss2g : 1;
            uint8_t : 1;
        } __attribute__((packed)) fields;
    };
    union reg_coolconf {
        uint32_t raw;
        struct {
            uint8_t semin : 4;
            uint8_t : 1;
            uint8_t seup : 2;
            uint8_t : 1;
            uint8_t semax : 4;
            uint8_t : 1;
            uint8_t seudn : 2;
            uint8_t seimin : 1;
            uint8_t sgt : 7;
            uint8_t : 1;
            uint8_t sfilt : 1;
            uint8_t : 7;
        } __attribute__((packed)) fields;
    };
    union reg_drv_status {
        uint32_t raw;
        struct {
            uint16_t sg_result : 10;
            uint8_t : 5;
            uint8_t fsactive : 1;
            uint8_t cs_actual : 5;
            uint8_t : 3;
            uint8_t StallGuard : 1;
            uint8_t ot : 1;
            uint8_t otpw : 1;
            uint8_t s2ga : 1;
            uint8_t s2gb : 1;
            uint8_t ola : 1;
            uint8_t olb : 1;
            uint8_t stst : 1;
        } __attribute__((packed)) fields;
    };
    union reg_pwmconf {
        uint32_t raw;
        struct {
            uint8_t pwm_ampl : 8;
            uint8_t pwm_grad : 8;
            uint8_t pwm_freq : 2;
            uint8_t pwm_autoscale : 1;
            uint8_t pwm_symmetric : 1;
            uint8_t freewheel : 2;
            uint8_t : 2;
            uint8_t : 8;
        } __attribute__((packed)) fields;
    };

    /* Register access */
    virtual int status_read(uint8_t &status) = 0;
    virtual int register_read(const uint8_t address, uint32_t &data) = 0;
    virtual int register_write(const uint8_t address, const uint32_t data) = 0;

    /* Setup */
    struct config {
        union reg_gconf reg_gconf = {.raw = 0x00000004};            // EN_PWM_MODE=1 enables StealthChop (with default PWMCONF)
        union reg_ihold_irun reg_ihold_irun = {.raw = 0x00061F0A};  // IHOLD_IRUN: IHOLD=10, IRUN=31 (max. current), IHOLDDELAY=6
        union reg_chopconf reg_chopconf = {.raw = 0x000100C3};      // CHOPCONF: TOFF=3, HSTRT=4, HEND=1, TBL=2, CHM=0 (SpreadCycle)
        union reg_tpowerdown reg_tpowerdown = {.raw = 0x0000000A};  // TPOWERDOWN=10: Delay before power down in stand still
        union reg_tpwmthrs reg_tpwmthrs = {.raw = 0x000001F4};      // TPWM_THRS=500 yields a switching velocity about 35000 = ca. 30RPM
        union reg_pwmconf reg_pwmconf = {.raw = 0x000401C8};        // PWMCONF: AUTO=1, 2/1024 Fclk, Switch amplitude limit=200, Grad=1
    };
    int setup(struct config &config);
    int speed_ramp_set(const float vstart, const float vstop, const float vtrans);
    int speed_limit_set(const float speed);
    int acceleration_limit_set(const float acceleration);

    /* Movement start or stop */
    int move_to_position(const float position);
    int move_at_velocity(const float velocity);
    int move_stop(void);

    /* Position */
    int position_current_get(float &position);
    int position_latched_get(float &position);

    /* Target reached or not */
    int target_position_reached_is(void);
    int target_velocity_reached_is(void);

   protected:
    uint32_t convert_velocity_to_tmc(const float velocity);
    uint32_t convert_acceleration_to_tmc(const float acceleration);
    uint8_t m_status_byte = 0x00;
    uint32_t m_fclk = 13200000;       //!< Frenquency at which the driver is running in Hz
    uint16_t m_ustep_per_step = 256;  //!< Number of microsteps per step
};

/**
 *
 */
class tmc5130_spi : public tmc5130 {

   public:
    int setup(struct config &config, SPIClass &spi_library, const int spi_pin_cs, const int spi_speed = 4000000);
    int status_read(uint8_t &status);
    int register_read(const uint8_t address, uint32_t &data);
    int register_write(const uint8_t address, const uint32_t data);

   protected:
    SPIClass *m_spi_library = NULL;
    uint8_t m_spi_pin_cs;
    SPISettings m_spi_settings;
};

#endif
