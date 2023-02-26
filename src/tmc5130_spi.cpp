/* Self header */
#include "tmc5130.h"

/**
 *
 * @param[in] config
 * @param[in] spi_library
 * @param[in] spi_cs_pin
 * @param[in] spi_speed
 * @return 0 in case of success, or a negative error code otherwise.
 */
int tmc5130_spi::setup(struct config &config, SPIClass &spi_library, const int spi_cs_pin, const int spi_speed) {

    /* Ensure spi speed is within supported range */
    if (spi_speed > 8000000) {
        return -EINVAL;
    }

    /* Save spi settings */
    m_spi_library = &spi_library;
    m_spi_settings = SPISettings(spi_speed, MSBFIRST, SPI_MODE3);
    m_spi_cs_pin = spi_cs_pin;

    /* Configure cs pin */
    pinMode(m_spi_cs_pin, OUTPUT);
    digitalWrite(m_spi_cs_pin, HIGH);

    /* Configure registers */
    return tmc5130::setup(config);
}

/**
 *
 * @param[out] status
 * @return 0 in case of success, or a negative error code otherwise.
 */
int tmc5130_spi::status_read(uint8_t &status) {

    /* Ensure setup has been done */
    if (m_spi_library == NULL) {
        return -EINVAL;
    }

    /* Read any register to extract the status byte */
    m_spi_library->beginTransaction(m_spi_settings);
    digitalWrite(m_spi_cs_pin, LOW);
    status = m_spi_library->transfer(GCONF & 0x7F);
    m_spi_library->transfer(0x00);
    m_spi_library->transfer(0x00);
    m_spi_library->transfer(0x00);
    m_spi_library->transfer(0x00);
    digitalWrite(m_spi_cs_pin, HIGH);
    m_spi_library->endTransaction();

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] address
 * @param[out] data
 * @return 0 in case of success, or a negative error code otherwise.
 */
int tmc5130_spi::register_read(const uint8_t address, uint32_t &data) {

    /* Ensure setup has been done */
    if (m_spi_library == NULL) {
        return -EINVAL;
    }

    /* Send address with dummy data bytes */
    m_spi_library->beginTransaction(m_spi_settings);
    digitalWrite(m_spi_cs_pin, LOW);
    m_status_byte = m_spi_library->transfer(address & 0x7F);
    if (m_status_byte == 0xFF) {
        digitalWrite(m_spi_cs_pin, HIGH);
        m_spi_library->endTransaction();
        return -EIO;
    }
    m_spi_library->transfer(0x00);
    m_spi_library->transfer(0x00);
    m_spi_library->transfer(0x00);
    m_spi_library->transfer(0x00);
    digitalWrite(m_spi_cs_pin, HIGH);

    /* Wait */
    delayMicroseconds(10);

    /* Read data from previously selected address */
    digitalWrite(m_spi_cs_pin, LOW);
    m_status_byte = m_spi_library->transfer(address & 0x7F);
    if (m_status_byte == 0xFF) {
        digitalWrite(m_spi_cs_pin, HIGH);
        m_spi_library->endTransaction();
        return -EIO;
    }
    data = m_spi_library->transfer(0x00);
    data <<= 8;
    data |= m_spi_library->transfer(0x00);
    data <<= 8;
    data |= m_spi_library->transfer(0x00);
    data <<= 8;
    data |= m_spi_library->transfer(0x00);
    digitalWrite(m_spi_cs_pin, HIGH);
    m_spi_library->endTransaction();

    /* Return success */
    return 0;
}

/**
 *
 * @param[in] address
 * @param[in] data
 * @return 0 in case of success, or a negative error code otherwise.
 */
int tmc5130_spi::register_write(const uint8_t address, const uint32_t data) {

    /* Ensure setup has been done */
    if (m_spi_library == NULL) {
        return -EINVAL;
    }

    /* Send address */
    m_spi_library->beginTransaction(m_spi_settings);
    digitalWrite(m_spi_cs_pin, LOW);
    m_status_byte = m_spi_library->transfer(address | 0x80);
    if (m_status_byte == 0xFF) {
        digitalWrite(m_spi_cs_pin, HIGH);
        m_spi_library->endTransaction();
        return -EIO;
    }

    /* Send data */
    m_spi_library->transfer(data >> 24);
    m_spi_library->transfer(data >> 16);
    m_spi_library->transfer(data >> 8);
    m_spi_library->transfer(data);
    digitalWrite(m_spi_cs_pin, HIGH);
    m_spi_library->endTransaction();

    /* Return success */
    return 0;
}
