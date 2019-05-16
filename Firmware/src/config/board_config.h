#pragma once

#define MAX_LED_COUNT 21

#include "stddef.h"
#include "core/float3.h"
#include "stdint.h"

namespace Config
{
    namespace BoardManager
    {
        struct Board
        {
            // Measuring board type
            int boardResistorValue;

            // Talking to LEDs
            uint32_t ledDataPin;
            uint32_t ledClockPin;
            uint32_t ledPowerPin;

            // I2C Pins for accelerometer
            uint32_t i2cDataPin;
            uint32_t i2cClockPin;
            uint32_t accInterruptPin;

            // Power Management pins
            uint32_t chargingStatePin;
            uint32_t vledFaultPin;
            uint32_t vbatSensePin;

            // Magnet pin
            uint32_t magnetPin;

            // LED config
            int ledCount;
            uint8_t faceToLedLookup[MAX_LED_COUNT];
            Core::float3 faceNormal[MAX_LED_COUNT];
        };

        void init();
        const Board* getBoard();
    }
}
