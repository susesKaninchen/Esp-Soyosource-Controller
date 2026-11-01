#pragma once

#include <Arduino.h>

/**
 * Soyosource GTN Inverter RS485 Protocol Handler
 * 
 * Communication parameters:
 * - Baud rate: 4800 Baud
 * - Data bits: 8, Parity: None, Stop bits: 1 (8N1)
 * 
 * Frame Format (8 Bytes):
 * Byte 0: 0x24 (ASCII '$')
 * Byte 1: 0x56 (ASCII 'V')
 * Byte 2: 0x00 (Fixed)
 * Byte 3: 0x21 (Command ID)
 * Byte 4: Power High Byte (Big Endian)
 * Byte 5: Power Low Byte  (Big Endian)
 * Byte 6: 0x80 (Command flag)
 * Byte 7: Checksum = (0xFF - sum(Bytes 1..6)) & 0xFF
 */

class SoyosourceProtocol {
public:
    static const uint8_t HEADER_0 = 0x24;
    static const uint8_t HEADER_1 = 0x56;
    static const uint8_t FIXED_2  = 0x00;
    static const uint8_t CMD_ID   = 0x21;
    static const uint8_t CMD_FLAG = 0x80;

    /**
     * Calculate Soyosource packet checksum
     */
    static uint8_t calculateChecksum(const uint8_t* frame) {
        uint8_t sum = frame[1] + frame[2] + frame[3] + frame[4] + frame[5] + frame[6];
        return (uint8_t)(0xFF - sum);
    }

    /**
     * Build an 8-byte command frame for the requested power in Watts.
     */
    static void buildFrame(uint16_t watts, uint8_t* outFrame) {
        outFrame[0] = HEADER_0;
        outFrame[1] = HEADER_1;
        outFrame[2] = FIXED_2;
        outFrame[3] = CMD_ID;
        outFrame[4] = (uint8_t)((watts >> 8) & 0xFF);
        outFrame[5] = (uint8_t)(watts & 0xFF);
        outFrame[6] = CMD_FLAG;
        outFrame[7] = calculateChecksum(outFrame);
    }

    /**
     * Validate a received frame (8 bytes)
     */
    static bool validateFrame(const uint8_t* frame, size_t len) {
        if (len < 8) return false;
        if (frame[0] != HEADER_0 || frame[1] != HEADER_1) return false;
        return (frame[7] == calculateChecksum(frame));
    }

    /**
     * Send an 8-byte frame over HardwareSerial with optional RS485 DE/RE control.
     */
    static void sendFrame(HardwareSerial& serial, uint16_t watts, int derePin = -1) {
        uint8_t frame[8];
        buildFrame(watts, frame);

        if (derePin >= 0) {
            digitalWrite(derePin, HIGH);
            delayMicroseconds(60);
        }

        serial.write(frame, 8);
        serial.flush();

        if (derePin >= 0) {
            delayMicroseconds(60);
            digitalWrite(derePin, LOW);
        }
    }

    /**
     * Parse an incoming status frame from Soyosource inverter.
     * Standard status frame is 10 bytes:
     * - frame[0]: operation status
     * - frame[1..2]: raw battery voltage (0.1V resolution)
     * - frame[3..4]: raw battery current (0.1A resolution)
     * - frame[5..6]: AC grid voltage
     * - frame[7..8]: temperature
     * - frame[9]: checksum (0xFF - sum(frame[1..8]))
     */
    static bool parseStatusFrame(const uint8_t* frame, size_t len, float& outVolts, float& outAmps, uint8_t& outStatus) {
        if (len < 10) return false;

        uint8_t sum = 0;
        for (size_t i = 1; i < 9; i++) {
            sum += frame[i];
        }
        uint8_t expectedCks = (uint8_t)(0xFF - sum);
        if (frame[9] != expectedCks) return false;

        outStatus = frame[0];
        uint16_t rawVolts = ((uint16_t)frame[1] << 8) | frame[2];
        uint16_t rawAmps  = ((uint16_t)frame[3] << 8) | frame[4];

        outVolts = rawVolts * 0.1f;
        outAmps  = rawAmps * 0.1f;
        return true;
    }
};
