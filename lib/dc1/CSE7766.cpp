#ifdef USE_DC1

#include "CSE7766.h"

/*
void Debug(PGM_P formatP, ...)
{
}
*/

CSE7766::CSE7766(uint8_t rxPin, uint16_t baudrate)
{
    this->serial = new SoftwareSerial(rxPin, SW_SERIAL_UNUSED_PIN, false, 32);
    this->serial->enableIntTx(false);
    this->serial->begin(baudrate);
}

bool CSE7766::checkByte()
{
    uint8_t index = this->rawDataIndex;
    uint8_t byte = this->rawData[index];
    if (index == 0)
    {
        return !((byte != 0x55) && ((byte & 0xF0) != 0xF0) && (byte != 0xAA));
    }

    if (index == 1)
    {
        if (byte != 0x5A)
        {
            //Debug(PSTR("Invalid Header 2 Start: 0x%02X!"), byte);
            return false;
        }
        return true;
    }

    if (index == 23)
    {
        uint8_t checksum = 0;
        for (uint8_t i = 2; i < 23; i++)
        {
            checksum += this->rawData[i];
        }

        if (checksum != this->rawData[23])
        {
            //Debug(PSTR("Invalid checksum from CSE7766: 0x%02X != 0x%02X"), checksum, this->rawData[23]);
            return false;
        }
        return true;
    }

    return true;
}

/**
 * Hex转Str
 */
int hex2Str(uint8_t *bin, uint16_t bin_size, char *buff, bool needBlank)
{
    const char *set = "0123456789ABCDEF";
    char *nptr = buff;
    if (NULL == buff)
    {
        return -1;
    }
    int len = needBlank ? (bin_size * 2 + bin_size) : (bin_size * 2 + 1);
    while (bin_size--)
    {
        *nptr++ = set[(*bin) >> 4];
        *nptr++ = set[(*bin++) & 0xF];
        if (needBlank && bin_size > 0)
        {
            *nptr++ = ' ';
        }
    }
    *nptr = '\0';
    return len;
}

void CSE7766::parseData()
{
    //char strHex[100];
    //hex2Str(this->rawData, 23, strHex, true);
    //Debug(PSTR("CSE7766 Data: %s"), strHex);

    uint8_t header1 = this->rawData[0];
    if (header1 == 0xAA)
    {
        //Debug(PSTR("CSE7766 not calibrated!"));
        return;
    }

    if ((header1 & 0xF0) == 0xF0 && ((header1 >> 0) & 1) == 1)
    {
        //Debug(PSTR("CSE7766 reports abnormal hardware: (0x%02X)"), header1);
        //Debug(PSTR("  Coefficient storage area is abnormal."));
        return;
    }

    uint8_t adj = this->rawData[20];

    bool power_ok = true;
    bool voltage_ok = true;
    bool current_ok = true;

    if (header1 > 0xF0)
    {
        //Serial.printf("CSE7766 reports abnormal hardware: (0x%02X)", byte);
        if ((header1 >> 3) & 1)
        {
            //Debug(PSTR("  Voltage cycle exceeds range."));
            voltage_ok = false;
        }
        if ((header1 >> 2) & 1)
        {
            //Debug(PSTR("  Current cycle exceeds range."));
            current_ok = false;
        }
        if ((header1 >> 1) & 1)
        {
            //Debug(PSTR("  Power cycle exceeds range."));
            power_ok = false;
        }
        if ((header1 >> 0) & 1)
        {
            //Debug(PSTR("  Coefficient storage area is abnormal."));
            return;
        }
    }

    if ((adj & 0x40) == 0x40 && voltage_ok && current_ok)
    {
        uint32_t voltage_calib = this->get24BitUint(2);
        uint32_t voltage_cycle = this->get24BitUint(5);
        // voltage cycle of serial port outputted is a complete cycle;
        this->voltageAcc += voltage_calib / float(voltage_cycle);
        this->voltageCounts += 1;
    }

    float power = 0;
    if ((adj & 0x10) == 0x10 && voltage_ok && current_ok && power_ok)
    {
        uint32_t power_calib = this->get24BitUint(14);
        uint32_t power_cycle = this->get24BitUint(17);
        // power cycle of serial port outputted is a complete cycle;
        power = power_calib / float(power_cycle);
        this->powerAcc += power;
        this->powerCounts += 1;
    }

    if ((adj & 0x20) == 0x20 && current_ok && voltage_ok && power != 0.0)
    {
        uint32_t current_calib = this->get24BitUint(8);
        uint32_t current_cycle = this->get24BitUint(11);
        // indicates current cycle of serial port outputted is a complete cycle;
        this->currentAcc += current_calib / float(current_cycle);
        this->currentCounts += 1;
    }
}

uint32_t CSE7766::get24BitUint(uint8_t startIndex)
{
    return (uint32_t(this->rawData[startIndex]) << 16) | (uint32_t(this->rawData[startIndex + 1]) << 8) | uint32_t(this->rawData[startIndex + 2]);
}

void CSE7766::loop()
{
    const uint32_t now = millis();
    if (now - this->lastTransmission >= 500)
    {
        // last transmission too long ago. Reset RX index.
        this->rawDataIndex = 0;
    }

    if (this->serial->available() == 0)
    {
        return;
    }

    this->lastTransmission = now;
    while (this->serial->available())
    {
        this->rawData[this->rawDataIndex] = this->serial->read();
        if (!this->checkByte())
        {
            this->rawDataIndex = 0;
        }

        if (this->rawDataIndex == 23)
        {
            this->parseData();
        }
        this->rawDataIndex = (this->rawDataIndex + 1) % 24;
    }
}

void CSE7766::update()
{
    this->voltage = this->voltageCounts > 0 ? this->voltageAcc / this->voltageCounts : 0.0f;
    this->current = this->currentCounts > 0 ? this->currentAcc / this->currentCounts : 0.0f;
    this->power = this->powerCounts > 0 ? this->powerAcc / this->powerCounts : 0.0f;

    if (this->voltage < 1.0f)
    {
        this->voltage = 0.0f;
    }
    if (this->voltage == 0.0f || this->current == 0.0f || this->power == 0.0f)
    {
        this->factor = 0.0f;
    }
    else
    {
        this->factor = this->power / this->current / this->voltage;
    }

    /*
    Serial.print("Got voltage_acc=");
    Serial.print(this->voltageAcc, 2);
    Serial.print("current_acc=");
    Serial.print(this->currentAcc, 2);
    Serial.print("power_acc=");
    Serial.print(this->powerAcc, 2);
    Serial.println();
    Serial.printf("Got voltage_counts=%d current_counts=%d power_counts=%d\r\n", this->voltageCounts, this->currentCounts, this->powerCounts);

    Serial.print("Got voltage=");
    Serial.print(voltage, 1);
    Serial.print("V current=");
    Serial.print(current, 2);
    Serial.print("A power=");
    Serial.print(power, 2);
    Serial.println();
    */

    //Serial.printf("Got voltage_acc=%2f current_acc=%2f power_acc=%2f\r\n", this->voltageAcc, this->currentAcc, this->powerAcc);
    //Serial.printf("Got voltage_counts=%d current_counts=%d power_counts=%d\r\n", this->voltageCounts, this->currentCounts, this->powerCounts);
    //Serial.printf("Got voltage=%1fV current=%1fA power=%1fW\r\n", voltage, current, power);

    this->voltageAcc = 0.0f;
    this->currentAcc = 0.0f;
    this->powerAcc = 0.0f;
    this->voltageCounts = 0;
    this->powerCounts = 0;
    this->currentCounts = 0;
}

#endif