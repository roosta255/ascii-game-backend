One helper i'll need is a function to query a keyframe by an AnimationEnum. so Maybe<Keyframe> queryKeyframes(Rack<Keyframe>, AnimationEnum)

TestController could also use a helper function to get the current room using latestPosition. Something like: Room*&* getLatestRoom(), which uses match.dungeon.rooms.access(latestPosition, [&](Room& room){output= &room;})

TestController could also track the latestActivateDirection via the TestController::activateLock() and TestController::activateDoor(). Caveat: the move needs to be a success before overwriting the latestActivateDirection.

The first step i want to take is to add in a REQUIRE(queryKeyframes(keyframes, animation) == Maybe<Keyframe>::empty()) statement after the test that outputs the whole character keyframes array. Just so that i can see the current output. Once all the tests are outputing the keyframes, then i'll be able to handcode what the code is correctly outputting. (or have you fix the code to create that keyframe).

The following is a list of code lines that generate Keyframe objects. For each of the lines, they'll have data points in the bullet points below it. There will be:
* animation: (this tells you what the exact animationEnum of the Keyframe.animation to query)
* keyframes: (this tells you where to find the list of keyframes to query)
* test/Generator*:* (this tells you the line of the test that will create the keyframe)

So just to reiterate, take each of the below lines's test-line-number, and add in a REQUIRE(queryKeyframes(keyframes, animation) == Maybe<Keyframe>::empty()). The keyframes and animation fields will be in bulletpoints alongside the test-number

// ------------------------- Toggler activations in GeneratorTutorial

ActivatorBounceFloor.cpp:38
* animation: ANIMATION_FLOOR_BOUNCE_FROM_DOOR
* keyframes: "testController.builderCharacterPtr->keyframes"
* test/GeneratorTutorial:71

ActivatorToggler.cpp:33
* animation: ANIMATION_TOGGLER_SWITCH_ORANGE
* keyframes: "match.characters[toggler1Offset].keyframes"
* test/GeneratorTutorial:71

ActivatorBounceFloor.cpp:36
* animation: ANIMATION_FLOOR_BOUNCE_FROM_FLOOR
* BOUNCE: FLOOR -> FLOOR
* keyframes: "testController.builderCharacterPtr->keyframes"
* test/GeneratorTutorial:118
* find activate toggler after a move to floor

// ------------------------- Keeper activations in GeneratorTutorial

ActivatorBounceLock.cpp:21
* animation: ANIMATION_BOUNCE_FROM_FLOOR_TO_LOCK
* keyframes: "testController.builderCharacterPtr->keyframes"
* test/GeneratorTutorial:154

ActivatorBounceLock.cpp:23
* animation: ANIMATION_BOUNCE_FROM_DOOR_TO_LOCK
* keyframes: "testController.builderCharacterPtr->keyframes"
* test/GeneratorTutorial:192

MatchController.cpp:469
* animation: ANIMATION_FALL
* keyframes: "testController.inventoryPtr->accessItem(ITEM_KEY, [&](const Item& item){ item.keyframes })"
* test/GeneratorTutorial:154

MatchController.cpp:787
* animation: ANIMATION_RISE
* keyframes: "testController.inventoryPtr->accessItem(ITEM_KEY, [&](const Item& item){ item.keyframes })"
* test/GeneratorTutorial:192

Wall.cpp:39
* animation: ANIMATION_CRUSH
* keyframes: "testController.getLatestRoom()->walls[testController.latestDirection].keyframes"
* test/GeneratorTutorial:154

Wall.cpp:43
* animation: ANIMATION_SLIDE
* keyframes: "testController.getLatestRoom()->walls[testController.latestDirection].lockKeyframes"
* test/GeneratorTutorial:154

// ------------------------- loot chests in GeneratorDoorwayDungeon3Traits

ActivatorCritterBite.cpp:48
* test/GeneratorTest:
* JUMP

ActivatorCritterBite.cpp:52
* test/GeneratorTest:
* BITE

ActivatorChestLockKey.cpp:60
* test/GeneratorTest:
* CHEST_LOCK CATALYST

ActivatorChestLockKey.cpp:89
* test/GeneratorTest:
* CHECK_LOCK CONSUMER

