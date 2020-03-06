#include "led_color_tester.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "drivers_hw/apa102.h"
#include "utils/utils.h"
#include "nrf_log.h"
#include "modules/anim_controller.h"
#include "animations/animation.h"
#include "animations/animation_set.h"

using namespace Modules;
using namespace DriversHW;
using namespace Bluetooth;
using namespace Utils;
using namespace Animations;

namespace Modules
{
namespace LEDColorTester
{
    void SetAllLEDsToColorHandler(void* context, const Message* msg);
    void PlayLEDAnim(void* context, const Message* msg);

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_SetAllLEDsToColor, nullptr, SetAllLEDsToColorHandler);
        NRF_LOG_INFO("LED Color tester initialized");
    }

    void SetAllLEDsToColorHandler(void* context, const Message* msg) {
        auto colorMsg = (const MessageSetAllLEDsToColor*)msg;
        uint8_t r = Utils::getRed(colorMsg->color);
        uint8_t g = Utils::getGreen(colorMsg->color);
        uint8_t b = Utils::getBlue(colorMsg->color);
        uint32_t color = Utils::toColor(r, g, b);
        NRF_LOG_INFO("Setting All LEDs to %06x -> %06x", colorMsg->color, color);
        APA102::setAll(color);
        APA102::show();
    }
}
}