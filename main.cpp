#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include "Planner.hpp"

using namespace geode::prelude;

namespace {
    constexpr int kJumpButton = 1;

    autojump::Rect toRect(cocos2d::CCRect const& rect) {
        return {rect.getMinX(), rect.getMinY(), rect.getMaxX(), rect.getMaxY()};
    }

    // Skip modes requiring continuous holding, or repeated/special inputs.
    bool supportedMode(PlayerObject const* player) {
        return player && !player->m_isDead && !player->m_isPlatformer &&
            !player->m_isShip && !player->m_isBird && !player->m_isBall &&
            !player->m_isDart && !player->m_isRobot && !player->m_isSpider &&
            !player->m_isSwing && !player->m_isDashing &&
            !player->m_isGoingLeft;
    }
}

class $modify(AutoJumpPlayLayer, PlayLayer) {
    struct Fields {
        autojump::Planner planner;
        bool jumpDown = false;
    };

    void releaseAutoplayJump() {
        if (!m_fields->jumpDown) return;
        m_fields->jumpDown = false;
        // Routes through the game's own input path (not OS touch injection).
        this->handleButton(false, kJumpButton, true);
    }

    // Capture level collision geometry as it is loaded, not every video frame.
    void addObject(GameObject* object) {
        PlayLayer::addObject(object);
        if (!object) return;
        auto type = object->m_objectType;
        const bool hazard = type == GameObjectType::Hazard ||
                            type == GameObjectType::AnimatedHazard;
        const bool solid = type == GameObjectType::Solid;
        if (!hazard && !solid) return;

        auto b = toRect(object->getObjectRect());
        // Sanity check in case a custom object provides unusual collision bounds.
        if (b.right - b.left <= 0 || b.top - b.bottom <= 0) return;
        m_fields->planner.add({b, hazard});
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        // The last injected click is released after one game update.
        releaseAutoplayJump();

        auto* mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return;
        if (m_isPaused || m_hasCompletedLevel || !supportedMode(m_player1)) return;
        if (!m_player1 || m_player1->m_controlsDisabled || m_player1->m_inputsLocked)
            return;

        const bool jumpOverSolids = mod->getSettingValue<bool>("jump-solids");
        const float lookahead = static_cast<float>(
            mod->getSettingValue<double>("lookahead")
        );
        auto playerRect = toRect(m_player1->getObjectRect());
        if (m_fields->planner.shouldJump(
                playerRect,
                m_player1->m_isOnGround,
                m_player1->m_isUpsideDown,
                lookahead,
                jumpOverSolids)) {
            this->handleButton(true, kJumpButton, true);
            m_fields->jumpDown = true;
        }
    }

    void resetLevel() {
        releaseAutoplayJump();
        m_fields->planner.newAttempt();
        PlayLayer::resetLevel();
    }

    void onExit() {
        releaseAutoplayJump();
        m_fields->planner.clear();
        PlayLayer::onExit();
    }
};
