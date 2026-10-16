cmake_minimum_required(VERSION 3.16)
project(match_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()


# Find dependencies
find_package(Drogon REQUIRED)

# ------------ Shared include directories -------------
set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/activator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/behavior
    ${CMAKE_CURRENT_SOURCE_DIR}/src/algorithm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animation
    ${CMAKE_CURRENT_SOURCE_DIR}/src/chest
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/damage
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data
    ${CMAKE_CURRENT_SOURCE_DIR}/src/door
    ${CMAKE_CURRENT_SOURCE_DIR}/src/enum
    ${CMAKE_CURRENT_SOURCE_DIR}/src/event
    ${CMAKE_CURRENT_SOURCE_DIR}/src/field
    ${CMAKE_CURRENT_SOURCE_DIR}/src/generator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/inventory
    ${CMAKE_CURRENT_SOURCE_DIR}/src/layout
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logging
    ${CMAKE_CURRENT_SOURCE_DIR}/src/macro
    ${CMAKE_CURRENT_SOURCE_DIR}/src/math
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/remodel
    ${CMAKE_CURRENT_SOURCE_DIR}/src/role
    ${CMAKE_CURRENT_SOURCE_DIR}/src/room
    ${CMAKE_CURRENT_SOURCE_DIR}/src/store
    ${CMAKE_CURRENT_SOURCE_DIR}/src/struct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/time
    ${CMAKE_CURRENT_SOURCE_DIR}/src/traits
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vector
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view/store
    ${CMAKE_CURRENT_SOURCE_DIR}/src/play
    ${CMAKE_CURRENT_SOURCE_DIR}/src/procgen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hider_seeker/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hider_seeker/ruleset
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hider_seeker/tick
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hider_seeker/turn
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hider_seeker/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hider_seeker/projection
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hider_seeker/ruleset/connection_rules
    /usr/include/jsoncpp
)

# Sync test line 40 - third

# ------------ Shared Library -------------
add_library(core_backend STATIC
    src/behavior/BehaviorEnum.cpp
    src/behavior/BehaviorFlyweight.cpp
    src/behavior/Conduct.cpp
    src/behavior/ConductFlyweight.cpp
    src/behavior/ConductMemoryVariableEnum.cpp
    src/behavior/TriggerWrapper.cpp
    src/activator/ActionEnum.cpp
    src/activator/ActionFlyweight.cpp
    src/activator/ActivationContext.cpp
    src/activator/ActivatorAlterTraitAffliction.cpp
    src/activator/ActivatorDeposit.cpp
    src/activator/ActivatorEndTurn.cpp
    src/activator/ActivatorFindInventoryItemByTraits.cpp
    src/activator/ActivatorLogResolvedItemEvent.cpp
    src/activator/ActivatorNpcAct.cpp
    src/activator/ActivatorMoveToDoor.cpp
    src/activator/ActivatorMoveToFloor.cpp
    src/activator/ActivatorPushBehaviorTrigger.cpp
    src/activator/ActivatorTransferItem.cpp
    src/activator/ActivatorUseCharacter.cpp
    src/activator/ActivatorUseDoor.cpp
    src/activator/ActivatorUseInventoryItem.cpp
    src/activator/ActivatorRuleExecution.cpp
    src/activator/ActivatorUseLock.cpp
    src/activator/ActivationAddTargetKeyframe.cpp
    src/activator/CharacterAction.cpp
    src/activator/RuleFlyweight.cpp
    src/animation/AnimationEnum.cpp
    src/animation/AnimationFlyweight.cpp
    src/animation/AnimationSemantic.cpp
    src/animation/AnimationTypes.cpp
    src/animation/AnimationType.cpp
    src/animation/Keyframe.cpp
    src/chest/ActivatorChestLockKey.cpp
    src/chest/ActivatorCritterBite.cpp
    src/chest/ActivatorLootChest.cpp
    src/chest/ActivatorUseChestLock.cpp
    src/chest/Chest.cpp
    src/chest/DungeonChestLockTransformer.cpp
    src/chest/LockEnum.cpp
    src/chest/LockFlyweight.cpp
    src/controllers/MatchController.cpp
    src/damage/ActivatorAttack.cpp
    src/damage/ActivatorDamage.cpp
    src/damage/ActivatorDamageBite.cpp
    src/damage/ActivatorDamageCold.cpp
    src/damage/ActivatorDamageCrush.cpp
    src/damage/ActivatorDamageElectric.cpp
    src/damage/ActivatorDamageFire.cpp
    src/damage/ActivatorDamagePierce.cpp
    src/door/ActivatorExitDungeon.cpp
    src/door/ActivatorElevator.cpp
    src/door/ActivatorInactiveDoor.cpp
    src/door/ActivatorLightningRod.cpp
    src/door/ActivatorTimeGate.cpp
    src/door/ActivatorTimeGateCube.cpp
    src/door/ActivatorSetAllDoorsByTrait.cpp
    src/door/ActivatorSetSharedDoorsByTrait.cpp
    src/door/DoorFlyweight.cpp
    src/generator/DungeonGenerator.cpp
    src/generator/DungeonMutator.cpp
    src/generator/ElevatorContent.cpp
    src/generator/GeneratorNoop.cpp
    src/generator/GeneratorRemodelGroupFlyweight.cpp
    src/generator/RemodelAuthor.cpp
    src/generator/RemodelGroup.cpp
    src/generator/GeneratorAtom.cpp
    src/generator/GeneratorFlyweight.cpp
    src/procgen/AtomLibrary.cpp
    src/procgen/GraphPathEmbedder.cpp
    src/procgen/SerpentineEmbedder.cpp
    src/generator/iGenerator.cpp
    src/damage/DamageAttrBits.cpp
    src/damage/DamageTypeBits.cpp
    src/damage/DamageFlyweight.cpp
    src/enum/Codeset.cpp
    src/enum/CodeEnum.cpp
    src/enum/TurnEnum.cpp
    src/field/FieldEnum.cpp
    src/field/FieldFlyweight.cpp
    src/field/FieldController.cpp
    src/field/OxygenSourceProvider.cpp
    src/field/HeatSourceProvider.cpp
    src/field/SmokeSourceProvider.cpp
    src/field/ROOM_ACCUMULATIONFieldResolver.cpp
    src/field/DISTANCE_FIELDFieldResolver.cpp
    src/field/RELAXATIONFieldResolver.cpp
    src/inventory/ActivatorInactiveItem.cpp
    src/inventory/Inventory.cpp
    src/inventory/Item.cpp
    src/inventory/ItemEnum.cpp
    src/inventory/ItemFlyweight.cpp
    src/layout/build_room_map.cpp
    src/layout/iLayout.cpp
    src/layout/LayoutFlyweight.cpp
    src/layout/LayoutGrid.cpp
    src/logging/StructuredLog.cpp
    src/model/Account.cpp
    src/model/Builder.cpp
    src/model/Character.cpp
    src/model/Dungeon.cpp
    src/model/Match.cpp
    src/model/Player.cpp
    src/model/Titan.cpp
    src/model/Turner.cpp
    src/remodel/iRemodel.cpp
    src/remodel/Pathfinder.cpp
    src/remodel/PathfindingCounter.cpp
    src/remodel/Remodel.cpp
    src/remodel/RemodelFlyweight.cpp
    src/remodel/RemodelAuthorChestSeeder.cpp
    src/remodel/RemodelAuthorRunGeneratorLoop.cpp
    src/remodel/RemodelAuthorSetBuilderStartingRoomId.cpp
    src/remodel/RemodelAuthorSetupBuilderRole.cpp
    src/remodel/RemodelJailer.cpp
    src/remodel/RemodelKeeper.cpp
    src/remodel/RemodelShifter.cpp
    src/remodel/RemodelTogglerBlue.cpp
    src/remodel/RemodelTogglerOrange.cpp
    src/remodel/RemodelTogglerSwitch.cpp
    src/play/ActivateNonPlayerCharacterNullBuilder.cpp
    src/play/ActivateNonPlayerCharacterNullTitan.cpp
    src/role/ActivatorCleanseAfflictions.cpp
    src/role/ActivatorToggler.cpp
    src/role/DungeonRoleTransformer.cpp
    src/role/RoleFlyweight.cpp
    src/room/ChannelEnum.cpp
    src/room/Location.cpp
    src/room/LocationEnum.cpp
    src/room/Room.cpp
    src/room/RoomFlyweight.cpp
    src/room/Wall.cpp
    src/store/FileStore.cpp
    src/time/Timestamp.cpp
    src/traits/TraitBits.cpp
    src/traits/TraitEnum.cpp
    src/traits/TraitFlyweight.cpp
    src/traits/TraitModifier.cpp
    src/vector/int4_to_json.cpp
    src/hider_seeker/domain/serialization.cpp
    src/hider_seeker/ruleset/regeneration.cpp
    src/hider_seeker/ruleset/loa_rules.cpp
    src/hider_seeker/ruleset/action_rules.cpp
    src/hider_seeker/ruleset/connection_rules.cpp
    src/hider_seeker/ruleset/server_metrics.cpp
    src/hider_seeker/ruleset/probe_rules.cpp
    src/hider_seeker/ruleset/intent_ordering.cpp
    src/hider_seeker/ruleset/connection_rules.cpp
    src/hider_seeker/ruleset/rng.cpp
    src/hider_seeker/ruleset/lifecycle_events.cpp
    src/hider_seeker/tick/tick.cpp
    src/hider_seeker/turn/turn_engine.cpp
    src/hider_seeker/api/match_record.cpp
    src/hider_seeker/api/request_validation.cpp
    src/hider_seeker/api/HiderSeekerService.cpp
    src/hider_seeker/projection/audience_views.cpp
    src/hider_seeker/api/view_snapshots.cpp
)

# 👇 CRITICAL: Apply include dirs to the library
target_include_directories(core_backend PUBLIC ${PROJECT_INCLUDE_DIRS})

target_link_libraries(core_backend PUBLIC jsoncpp Drogon::Drogon)

# ------------ Main Executable ------------
add_executable(match_backend
    main.cpp
    src/controllers/ApiController.cpp
    src/controllers/MatchRepository.cpp
    src/controllers/AccountRepository.cpp
    src/hider_seeker/api/HiderSeekerApiController.cpp
)

target_include_directories(match_backend PRIVATE ${PROJECT_INCLUDE_DIRS})
target_link_libraries(match_backend PRIVATE
    Drogon::Drogon
    jsoncpp
    core_backend
)

# ------------ Test Target ------------
add_subdirectory(extern/Catch2)

add_executable(test_target
    test/main.cpp
    test/CombatTests.cpp
    test/MatchTests.cpp
    test/TestController.cpp
    test/maps/GeneratorAtom.cpp
    test/maps/GeneratorChestKeyTutorial.cpp
    test/maps/GeneratorDoorwayDungeon1.cpp
    test/maps/GeneratorDoorwayDungeon2.cpp
    test/maps/GeneratorDoorwayDungeon3Traits.cpp
    test/maps/GeneratorMonkeyTutorial.cpp
    test/maps/GeneratorMonkeyPuzzle1.cpp
    test/maps/GeneratorTest.cpp
    test/maps/GeneratorTest4D.cpp
    test/maps/GeneratorTutorial.cpp
    test/ActivationAddTargetKeyframeTests.cpp
    test/CharacterRelationTests.cpp
    test/TraitModifierTests.cpp
    test/FieldTests.cpp
    test/CharacterSheetApiViewTests.cpp
    test/EventLogTests.cpp
    test/PietyAnimationTests.cpp
    test/StructuredLogTests.cpp
    test/MapTests.cpp
    test/hider_seeker/serialization_round_trip_test.cpp
    test/hider_seeker/ruleset_loa_test.cpp
    test/hider_seeker/ruleset_action_test.cpp
    test/hider_seeker/ruleset_probe_test.cpp
    test/hider_seeker/ruleset_ordering_test.cpp
    test/hider_seeker/ruleset_ring_buffer_test.cpp
    test/hider_seeker/ruleset_rng_test.cpp
    test/hider_seeker/ruleset_lifecycle_events_test.cpp
    test/hider_seeker/tick_test.cpp
    test/hider_seeker/serialization_pending_ring_test.cpp
    test/hider_seeker/regeneration_test.cpp
    test/hider_seeker/turn_immediate_apply_test.cpp
    test/hider_seeker/turn_advance_affordability_test.cpp
    test/hider_seeker/turn_queue_test.cpp
    test/hider_seeker/turn_advance_loop_test.cpp
    test/hider_seeker/turn_pass_ticks_test.cpp
    test/hider_seeker/ruleset/connection_rules_test.cpp
    test/hider_seeker/ruleset/hider_action_enum_test.cpp
    test/hider_seeker/tick/tick_intent_dispatch_test.cpp
    test/hider_seeker/turn/turn_engine_connection_test.cpp
    test/hider_seeker/turn/known_kind_bound_test.cpp
    test/hider_seeker/api/match_record_test.cpp
    test/hider_seeker/api/service_lifecycle_test.cpp
    test/hider_seeker/api/service_submit_move_test.cpp
    test/hider_seeker/api/service_queue_pass_cancel_test.cpp
    test/hider_seeker/api/service_concurrency_test.cpp
    test/hider_seeker/api/service_idempotency_test.cpp
    test/hider_seeker/api/service_validation_test.cpp
    test/hider_seeker/api/service_determinism_test.cpp
    test/hider_seeker/projection/audience_views_test.cpp
    test/hider_seeker/api/service_record_events_test.cpp
    test/hider_seeker/api/view_snapshots_test.cpp
    test/hider_seeker/api/service_snapshot_test.cpp
    test/hider_seeker/api/service_views_test.cpp
    test/hider_seeker/server_metrics_test.cpp
    test/hider_seeker/api/service_metrics_integration_test.cpp
    test/hider_seeker/api/service_match_start_test.cpp
)

target_include_directories(test_target PRIVATE ${PROJECT_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/test)
target_link_libraries(test_target PRIVATE
    Drogon::Drogon
    jsoncpp
    Catch2::Catch2WithMain
    core_backend
)

enable_testing()
add_test(NAME unit_tests COMMAND test_target)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
