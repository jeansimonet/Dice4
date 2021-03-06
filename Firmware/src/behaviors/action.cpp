#include "action.h"
#include "data_set/data_set.h"
#include "modules/anim_controller.h"
#include "bluetooth/bluetooth_stack.h"
#include "bluetooth/bluetooth_message_service.h"
#include "nrf_log.h"

using namespace Modules;
using namespace Bluetooth;

namespace Behaviors
{
    void triggerActions(int actionOffset, int actionCount) {
        for (int index = actionOffset; index < actionOffset + actionCount; ++index) {
            // Fetch the action from the dataset
            auto action = DataSet::getAction(index);
            switch (action->type) {
                case Action_PlayAnimation:
                    {
                        auto playAnimAction = static_cast<const ActionPlayAnimation*>(action);
                        if (playAnimAction->animIndex < DataSet::getAnimationCount()) {
                            NRF_LOG_INFO("Playing anim %d on face %d", playAnimAction->animIndex, playAnimAction->faceIndex);
                            if (playAnimAction->faceIndex == FACE_INDEX_CURRENT_FACE) {
                                AnimController::play(playAnimAction->animIndex, Accelerometer::currentFace(), false); // FIXME, handle remapFace and loopCount properly
                            } else {
                                AnimController::play(playAnimAction->animIndex, playAnimAction->faceIndex, false); // FIXME, handle remapFace and loopCount properly
                            }
                        } else {
                            NRF_LOG_ERROR("Invalid animation index %d", playAnimAction->animIndex);
                        }
                    }
                    break;
                case Action_PlaySound:
                    {
                        auto playSoundAction = static_cast<const ActionPlaySound*>(action);
                        if (Stack::isConnected())
                        {
                            NRF_LOG_INFO("Playing sound %08x", playSoundAction->clipId);
                            MessagePlaySound playSound;
                            playSound.clipId = playSoundAction->clipId;
                            MessageService::SendMessage(&playSound);
                        }
                        else
                        {
                            NRF_LOG_INFO("(Ignored) Playing sound %08x", playSoundAction->clipId);
                        }
                    }
                    break;
                default:
                    NRF_LOG_ERROR("Unknown action type %d for action index %d", action->type, index);
                    break;
            }
        }
    }
}
