#pragma once

#include "stdint.h"
#include "stddef.h"
#include "core/float3.h"

#define MAX_LED_COUNT 21

namespace Config
{
	struct Settings
	{
		// Indicates whether there is valid data
		uint32_t headMarker;

		// Face detector
		float jerkClamp;
		float sigmaDecay;
		float startMovingThreshold;
		float stopMovingThreshold;
		float faceThreshold;
		float fallingThreshold;
		float shockThreshold;
		float accDecay;
		float heatUpRate;
		float coolDownRate;
		int minRollTime; // ms

		// Battery
		float batteryLow;
		float batteryHigh;

		// Calibration data
		Core::float3 faceNormals[MAX_LED_COUNT];
		
		// Indicates whether there is valid data
		uint32_t tailMarker;
	};

	namespace SettingsManager
	{
		typedef void (*SettingsWrittenCallback)(bool success);

		void init(SettingsWrittenCallback callback);
		bool checkValid();
		uint32_t getSettingsStartAddress();
		uint32_t getSettingsEndAddress();
		Config::Settings const * const getSettings();

		void writeToFlash(Settings* sourceSettings, SettingsWrittenCallback callback);
		void setDefaults(Settings& outSettings);
		void programDefaults(SettingsWrittenCallback callback);
		void programDefaultParameters(SettingsWrittenCallback callback);
		void programNormals(const Core::float3* newNormals, int count, SettingsWrittenCallback callback);
	}
}

