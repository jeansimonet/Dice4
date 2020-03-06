#pragma once

#include <stdint.h>
#include "config/sdk_config.h"
#include "modules/accelerometer.h"

#define MAX_DATA_SIZE 100

#pragma pack(push, 1)

namespace Bluetooth
{
/// <summary>
///  Base class for messages from the die to the app
/// </summary>
struct Message
{
	enum MessageType : uint8_t
	{
		MessageType_None = 0,
		MessageType_WhoAreYou,
		MessageType_IAmADie,
		MessageType_State,
		MessageType_Telemetry,
		MessageType_BulkSetup,
		MessageType_BulkSetupAck,
		MessageType_BulkData,
		MessageType_BulkDataAck,
		MessageType_TransferAnimSet,
		MessageType_TransferAnimSetAck,
		MessageType_TransferSettings,
		MessageType_TransferSettingsAck,
		MessageType_DebugLog,

		MessageType_PlayAnim,
		MessageType_PlayAnimEvent,
		MessageType_StopAnim,
		MessageType_RequestState,
		MessageType_RequestAnimSet,
		MessageType_RequestSettings,
		MessageType_RequestTelemetry,
		MessateType_ProgramDefaultAnimSet,
		MessateType_ProgramDefaultAnimSetFinished,
		MessageType_Flash,
		MessageType_FlashFinished,
		MessageType_RequestDefaultAnimSetColor,
		MessageType_DefaultAnimSetColor,
		MessageType_RequestBatteryLevel,
		MessageType_BatteryLevel,
		MessageType_Calibrate,
		MessageType_CalibrateFace,
		MessageType_NotifyUser,
		MessageType_NotifyUserAck,
		MessageType_TestHardware,
		MessageType_SetStandardState,
		MessageType_SetLEDAnimState,
		MessageType_SetBattleState,
		MessateType_ProgramDefaultParameters,
		MessateType_ProgramDefaultParametersFinished,

		// TESTING 
		MessageType_TestBulkSend,
		MessageType_TestBulkReceive,
		MessageType_SetAllLEDsToColor,
		MessageType_AttractMode,
		MessageType_PrintNormals,

		MessageType_Count
	};

	MessageType type;

	inline Message(MessageType msgType) : type(msgType) {}
	static const char* GetMessageTypeString(MessageType msgType);

protected:
	inline Message() : type(MessageType_None) {}
};

/// <summary>
/// Describes a state change detection message
/// </summary>
struct MessageDieState
	: public Message
{
	uint8_t state;
	uint8_t face;

	inline MessageDieState() : Message(Message::MessageType_State) {}
};

/// <summary>
/// Describes an acceleration readings message (for telemetry)
/// </summary>
struct MessageAcc
	: public Message
{
	Modules::Accelerometer::AccelFrame data;

	inline MessageAcc() : Message(Message::MessageType_Telemetry) {}
};

struct MessageBulkSetup
	: Message
{
	uint16_t size;

	inline MessageBulkSetup() : Message(Message::MessageType_BulkSetup) {}
};

struct MessageBulkData
	: Message
{
	uint8_t size;
	uint16_t offset;
	uint8_t data[MAX_DATA_SIZE];

	inline MessageBulkData() : Message(Message::MessageType_BulkData) {}
};

struct MessageBulkDataAck
	: Message
{
	uint16_t offset;
	inline MessageBulkDataAck() : Message(Message::MessageType_BulkDataAck) {}
};

struct MessageTransferAnimSet
	: Message
{
	uint16_t paletteSize;
	uint16_t keyFrameCount;
	uint16_t rgbTrackCount;
	uint16_t trackCount;
	uint16_t animationCount;
	uint16_t heatTrackIndex;

	inline MessageTransferAnimSet() : Message(Message::MessageType_TransferAnimSet) {}
};

struct MessageDebugLog
	: public Message
{
	char text[MAX_DATA_SIZE];

	inline MessageDebugLog() : Message(Message::MessageType_DebugLog) {}
};

struct MessagePlayAnim
	: public Message
{
	uint8_t animation;
	uint8_t remapFace;  // Assumes that an animation was made for face 0
	uint8_t loop; 		// 1 == loop, 0 == once

	inline MessagePlayAnim() : Message(Message::MessageType_PlayAnim) {}
};

struct MessagePlayAnimEvent
	: public Message
{
	uint8_t evt;
	uint8_t remapFace;
	uint8_t loop;

	inline MessagePlayAnimEvent() : Message(Message::MessageType_PlayAnimEvent) {}
};

struct MessageStopAnim
	: public Message
{
	uint8_t animation;
	uint8_t remapFace;  // Assumes that an animation was made for face 0

	inline MessageStopAnim() : Message(Message::MessageType_StopAnim) {}
};

struct MessageRequestTelemetry
	: public Message
{
	uint8_t telemetry;

	inline MessageRequestTelemetry() : Message(Message::MessageType_RequestTelemetry) {}
};

struct MessageProgramDefaultAnimSet
	: public Message
{
	uint32_t color;

	inline MessageProgramDefaultAnimSet() : Message(Message::MessateType_ProgramDefaultAnimSet) {}
};

struct MessageFlash
	: public Message
{
	uint8_t animIndex;

	inline MessageFlash() : Message(Message::MessageType_Flash) {}
};

struct MessageDefaultAnimSetColor
	: public Message
{
	uint32_t color;
	inline MessageDefaultAnimSetColor() : Message(Message::MessageType_DefaultAnimSetColor) {}
};

struct MessageSetAllLEDsToColor
: public Message
{
	uint32_t color;
	inline MessageSetAllLEDsToColor() : Message(Message::MessageType_SetAllLEDsToColor) {}
};

struct MessageBatteryLevel
: public Message
{
	float level;
	inline MessageBatteryLevel() : Message(Message::MessageType_BatteryLevel) {}
};

struct MessageIAmADie
: public Message
{
	uint8_t id;
	inline MessageIAmADie() : Message(Message::MessageType_IAmADie) {}
};

struct MessageNotifyUser
: public Message
{
	uint8_t timeout_s;
	uint8_t ok; // Boolean
	uint8_t cancel; // Boolean
	char text[MAX_DATA_SIZE - 4];
	inline MessageNotifyUser() : Message(Message::MessageType_NotifyUser) {
		timeout_s = 30;
		ok = 1;
		cancel = 0;
		text[0] = '\0';
	}
};

struct MessageNotifyUserAck
: public Message
{
	uint8_t okCancel; // Boolean
	inline MessageNotifyUserAck() : Message(Message::MessageType_NotifyUserAck) {}
};

struct MessageCalibrateFace
: public Message
{
	uint8_t face;
	inline MessageCalibrateFace() : Message(MessageType_CalibrateFace) {}
};

struct MessagePrintNormals
: public Message
{
	uint8_t face;
	inline MessagePrintNormals() : Message(MessageType_PrintNormals) {}
};
}

#pragma pack(pop)
