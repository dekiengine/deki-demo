#pragma once

#include <deki/Engine.h>

#include <string>

// Paces the owner left and right around its starting point. When the object
// also has an AnimationComponent, turning round plays the named animation for
// the new direction, so a two-direction sprite sheet faces the way it walks.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Paces the object left and right, switching animation to face each way.")
class Walker : public Deki::Behaviour
{
public:
    DEKI_EXPORT DEKI_SLIDER(0, 64)
    float speedPx = 16.0f;

    // How far from the starting point the object walks before turning back.
    DEKI_EXPORT DEKI_SLIDER(0, 160)
    float patrolPx = 24.0f;

    DEKI_EXPORT
    bool startLeftwards = false;

    // Animation names in the object's AnimationComponent; empty means the
    // sprite does not change when the direction does.
    DEKI_EXPORT
    std::string walkRightAnimation;

    DEKI_EXPORT
    std::string walkLeftAnimation;

    Walker() = default;

    void Awake() override;
    void Update() override;

private:
    void Face(bool leftwards);

    float m_BaseX = 0.0f;
    bool m_Leftwards = false;
};

