#include "anim_controller.h"
#include "animations/animation.h"
#include "animations/animation_set.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/power_manager.h"
#include "utils/utils.h"
#include "utils/rainbow.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "drivers_hw/apa102.h"
#include "app_error.h"
#include "nrf_log.h"
#include "accelerometer.h"

using namespace Animations;
using namespace Modules;
using namespace Config;
using namespace DriversNRF;
using namespace DriversHW;

#define MAX_ANIMS (20)
#define TIMER2_RESOLUTION 33 //ms

namespace Modules
{
namespace AnimController
{
	/// <summary>
	/// Internal helper struct used to store a running animation instance
	/// </summary>
	struct AnimInstance
	{
		Animation const * animation;
		uint32_t specialColorPayload; // meaning varies
		int startTime; //ms
		uint8_t remapFace;
		bool loop;

		AnimInstance()
			: animation(nullptr)
			, specialColorPayload(0)
			, startTime(0)
			, remapFace(0)
			, loop(false)
		{
		}
	};

	// Our currently running animations
	AnimInstance animations[MAX_ANIMS];
	int animationCount;

	int animationLookupByEvent[AnimationEvent_Count];

	uint32_t getColorForAnim(void* token, uint32_t colorIndex);
	void onAccelFrame(void* param, const Accelerometer::AccelFrame& accelFrame);

	int currentRainbowIndex = 0;
	const int rainbowScale = 1; 
	float heat = 0.0f;

	APP_TIMER_DEF(animControllerTimer);
	// To be passed to the timer
	void animationControllerUpdate(void* param)
	{
		update(Utils::millis());
	}

	/// <summary>
	/// Kick off the animation controller, registering it with the Timer system
	/// </summary>
	void init()
	{
		animationCount = 0;
		Timers::createTimer(&animControllerTimer, APP_TIMER_MODE_REPEATED, animationControllerUpdate);
		Timers::startTimer(animControllerTimer, TIMER2_RESOLUTION, NULL);

		// Initialize the lookup table
		for (size_t i = 0; i < AnimationEvent_Count; ++i) {
			animationLookupByEvent[i] = -1;
		}
		for (uint16_t i = 0; i < AnimationSet::getAnimationCount(); ++i) {
			auto& anim = AnimationSet::getAnimation(i);
			if (anim.animationEvent > AnimationEvent_None && anim.animationEvent < AnimationEvent_Count) {
				animationLookupByEvent[anim.animationEvent] = i;
			}
		}

		Accelerometer::hookFrameData(onAccelFrame, nullptr);

		AnimationSet::setGetColorHandler(getColorForAnim);

		heat = 0.0f;
		currentRainbowIndex = 0;

		NRF_LOG_INFO("Anim Controller Initialized");
	}

	/// <summary>
	/// Update all currently running animations, and performing housekeeping when necessary
	/// </summary>
	/// <param name="ms">Current global time in milliseconds</param>
	void update(int ms)
	{
		// Update heat value (cool down)
		heat *= SettingsManager::getSettings()->coolDownRate;
		if (heat < 0.0f) {
			heat = 0.0f;
		}

		if (animationCount > 0) {
			PowerManager::feed();
			auto board = BoardManager::getBoard();
			auto& faceToLEDs = board->faceToLedLookup;

			// clear the global color array
			uint32_t allColors[MAX_LED_COUNT];
			for (int j = 0; j < BoardManager::getBoard()->ledCount; ++j) {
				allColors[j] = 0;
			}

			for (int i = 0; i < animationCount; ++i)
			{
				auto& anim = animations[i];
				int animTime = ms - anim.startTime;
				if (anim.loop && animTime > anim.animation->duration) {
					// Yes, update anim start time so next if statement updates the animation
					anim.startTime += anim.animation->duration;
					animTime = ms - anim.startTime;
				}

				if (animTime > anim.animation->duration)
				{
					// The animation is over, get rid of it!
					// Shift the other animations
					for (int j = i; j < animationCount - 1; ++j)
					{
						animations[j] = animations[j + 1];
					}

					// Reduce the count
					animationCount--;

					// Decrement loop counter since we just replaced the current anim
					i--;
				}
				else
				{
					int canonIndices[MAX_LED_COUNT * 4]; // Allow up to 4 tracks to target the same LED
					int faceIndices[MAX_LED_COUNT * 4];
					int ledIndices[MAX_LED_COUNT * 4];
					uint32_t colors[MAX_LED_COUNT * 4];

					// Update the leds
					int animTrackCount = anim.animation->updateLEDs(&anim, animTime, canonIndices, colors);


					// Gamma correct and map face index to led index
					for (int j = 0; j < animTrackCount; ++j) {
						colors[j] = Utils::gamma(colors[j]);
						faceIndices[j] = board->remapLed(anim.remapFace, canonIndices[j]);
						ledIndices[j] = faceToLEDs[faceIndices[j]];
					}

					// Update color array
					for (int j = 0; j < animTrackCount; ++j) {
						// Combine colors if necessary
						allColors[ledIndices[j]] = Utils::addColors(allColors[ledIndices[j]], colors[j]);
					}
				}
			}
			
			// And light up!
			APA102::setPixelColors(allColors);
			APA102::show();
		}
	}

	/// <summary>
	/// Stop updating animations
	/// </summary>
	void stop()
	{
		Timers::stopTimer(animControllerTimer);
	}

	bool hasAnimationForEvent(AnimationEvent evt) {
		return animationLookupByEvent[(uint16_t)evt] != -1;
	}

	/// <summary>
	/// Add an animation to the list of running animations
	/// </summary>
	void play(AnimationEvent evt, uint8_t remapFace, bool loop) {
		int evtIndex = (uint16_t)evt;
		int animIndex = animationLookupByEvent[evtIndex];
		if (animIndex == -1) {
			animIndex = 0;
		}
		NRF_LOG_INFO("Playing anim event %s (%d) on Face %d", Animations::getEventName(evt), animIndex, remapFace);
		if (animIndex < AnimationSet::getAnimationCount()) {
			auto& anim = AnimationSet::getAnimation(animIndex);
			play(&anim, remapFace, loop);
		}
	}

	/// <summary>
	/// Add an animation to the list of running animations
	/// </summary>
	void play(const Animations::Animation* anim, uint8_t remapFace, bool loop)
	{
		#if (NRF_LOG_DEFAULT_LEVEL == 4)
		NRF_LOG_DEBUG("Playing Anim!");
		NRF_LOG_DEBUG("  Track count: %d", anim->trackCount);
		for (int t = 0; t < anim->trackCount; ++t) {
			auto& track = anim->GetTrack(t);
			NRF_LOG_DEBUG("  Track %d:", t);
			NRF_LOG_DEBUG("  Track Offset %d:", anim->tracksOffset + t);
			NRF_LOG_DEBUG("  LED index %d:", track.ledIndex);
			NRF_LOG_DEBUG("  RGB Track Offset %d:", track.trackOffset);
			auto& rgbTrack = track.getTrack();
			NRF_LOG_DEBUG("  RGB Keyframe count: %d", rgbTrack.keyFrameCount);
			for (int k = 0; k < rgbTrack.keyFrameCount; ++k) {
				auto& keyframe = rgbTrack.getKeyframe(k);
				int time = keyframe.time();
				uint32_t color = keyframe.color(0);
				NRF_LOG_DEBUG("    Offset %d: %d -> %06x", (rgbTrack.keyframesOffset + k), time, color);
			}
		}
		#endif

		int prevAnimIndex = 0;
		for (; prevAnimIndex < animationCount; ++prevAnimIndex)
		{
			auto& prevAnim = animations[prevAnimIndex];
			if (prevAnim.animation == anim && prevAnim.remapFace == remapFace)
			{
				break;
			}
		}

		int ms = Utils::millis();
		if (prevAnimIndex < animationCount)
		{
			// Replace a previous animation
			stopAtIndex(prevAnimIndex);
			animations[prevAnimIndex].startTime = ms;
		}
		else if (animationCount < MAX_ANIMS)
		{
			// Add a new animation
			animations[animationCount].animation = anim;
			animations[animationCount].startTime = ms;
			animations[animationCount].remapFace = remapFace;
			animations[animationCount].loop = loop;

			switch (anim->specialColorType) {
				case SpecialColor_Face:
					// Store a color based on the face
					animations[animationCount].specialColorPayload = Rainbow::faceWheel(remapFace, BoardManager::getBoard()->ledCount);
					break;
				case SpecialColor_ColorWheel:
					// Store the face index
					animations[animationCount].specialColorPayload = remapFace;
					break;
				case SpecialColor_Heat_Start:
					{
						// Use the global heat value
						auto& trk = AnimationSet::getHeatTrack();
						int heatMs = int(heat * trk.getDuration());
						animations[animationCount].specialColorPayload = trk.evaluate(nullptr, heatMs);
					}
					break;
				default:
					// Other cases don't need any per-instance payload
					animations[animationCount].specialColorPayload = 0;
					break;
			}

			animationCount++;
		}
		// Else there is no more room
	}

	/// <summary>
	/// Forcibly stop a currently running animation
	/// </summary>
	void stop(const Animations::Animation* anim, uint8_t remapFace)
	{
		int prevAnimIndex = 0;
		for (; prevAnimIndex < animationCount; ++prevAnimIndex)
		{
			auto& instance = animations[prevAnimIndex];
			if (instance.animation == anim && (remapFace == 255 || instance.remapFace == remapFace))
			{
				break;
			}
		}

		if (prevAnimIndex < animationCount)
		{
			removeAtIndex(prevAnimIndex);
		}
		// Else the animation isn't playing
	}

	/// <summary>
	/// Stop all currently running animations
	/// </summary>
	void stopAll()
	{
		for (int i = 0; i < animationCount; ++i)
		{
			animations[i].animation = nullptr;
			animations[i].startTime = 0;
		}
		animationCount = 0;
		APA102::clear();
		APA102::show();
	}

	/// <summary>
	/// Helper function to clear anim LED turned on by a current animation
	/// </summary>
	void stopAtIndex(int animIndex)
	{
		// Found the animation, start by killing the leds it controls
		auto board = BoardManager::getBoard();
		auto& faceToLEDs = board->faceToLedLookup;
		int canonIndices[MAX_LED_COUNT];
		int faceIndices[MAX_LED_COUNT];
		int ledIndices[MAX_LED_COUNT];
		uint32_t zeros[MAX_LED_COUNT];
		memset(zeros, 0, sizeof(uint32_t) * MAX_LED_COUNT);
		auto& anim = animations[animIndex];
		int ledCount = anim.animation->stop(canonIndices);
		for (int i = 0; i < ledCount; ++i) {
			faceIndices[i] = board->remapLed(anim.remapFace, canonIndices[i]);
			ledIndices[i] = faceToLEDs[faceIndices[i]];
		}
		APA102::setPixelColors(ledIndices, zeros, ledCount);
		APA102::show();
	}

	/// <summary>
	/// Helper method: Stop the animation at the given index. Used by Stop(IAnimation*)
	/// </summary>
	void removeAtIndex(int animIndex)
	{
		stopAtIndex(animIndex);

		// Shift the other animations
		for (; animIndex < animationCount - 1; ++animIndex)
		{
			animations[animIndex] = animations[animIndex + 1];
		}

		// Reduce the count
		animationCount--;
	}

	void onAccelFrame(void* param, const Accelerometer::AccelFrame& accelFrame) {
		auto sqrMag = accelFrame.jerk.sqrMagnitude();
		if (sqrMag > 0.0f) {
			currentRainbowIndex++;
			heat += sqrt(sqrMag) * SettingsManager::getSettings()->heatUpRate;
			if (heat > 1.0f) {
				heat = 1.0f;
			}
		}
	}

	uint32_t getColorForAnim(void* token, uint32_t colorIndex) {
		if (token != nullptr) {
			const AnimInstance* instance = (const AnimInstance*)token;
			switch (instance->animation->specialColorType) {
				case SpecialColor_Face:
				case SpecialColor_Heat_Start:
					// The payload is the color
					return instance->specialColorPayload;
				case SpecialColor_ColorWheel:
					{
						// Use the global rainbow
						int index = currentRainbowIndex / rainbowScale;
						if (index < 0) {
							index += 256;
						}
						return Rainbow::wheel((uint8_t)index);
					}
				case SpecialColor_Heat_Current:
					{
						auto& trk = AnimationSet::getHeatTrack();
						int heatMs = int(heat * trk.getDuration());
						return trk.evaluate(nullptr, heatMs);
					}
				default:
					return AnimationSet::getPaletteColor(colorIndex);
			}
		} else {
			return AnimationSet::getPaletteColor(colorIndex);
		}
	}
}
}
