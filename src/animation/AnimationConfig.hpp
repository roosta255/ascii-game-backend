#pragma once

#include "AnimationEnum.hpp"

enum AnimationAct {
    ANIMATION_ACT_SUBJECT,
    ANIMATION_ACT_TARGET,
    ANIMATION_ACT_SUBJECT_TO_TARGET,
    ANIMATION_ACT_TARGET_TO_SUBJECT
};

struct AnimationConfig {
    AnimationEnum animation;
    AnimationAct act;
    bool isDebug = false;
};