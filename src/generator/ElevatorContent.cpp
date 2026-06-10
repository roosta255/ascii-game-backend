#include "CharacterAllocSpec.hpp"
#include "Chest.hpp"
#include "ElevatorContent.hpp"
#include "GeneratorElevator.hpp"
#include "ItemEnum.hpp"
#include "MatchController.hpp"
#include "Remodel.hpp"
#include "RoleEnum.hpp"

bool ElevatorContentBlank::mutateMatch(Remodel&, const Match& source, const PathfindingCounter&, std::function<bool(const Match&)> acceptance) const {
    acceptance(source);
    return true;
}

bool ElevatorContentHandcrafted::mutateMatch(Remodel& params, const Match& source, const PathfindingCounter&, std::function<bool(const Match&)> acceptance) const {
    Match copy = source;
    MatchController controller(copy, params.codeset);
    bool success = true;
    int containerId = -1;
    success &= controller.allocate(
        CharacterAllocSpec{
            .role = ROLE_CHEST,
            .chest = ChestAllocSpec{
                .inventory = {
                    CritterCharacterSpec{.character = CharacterAllocSpec{.role = ROLE_SPIDER}},
                    ItemSpec{.item = ITEM_KEY_ELEVATOR}
                }
            }
        },
        AttachmentContext::floor(GeneratorElevator::FLOOR_7_CHEST_ROOM_ID),
        containerId
    );
    if (success) acceptance(copy);
    return success;
}

bool ElevatorContentAtom::mutateMatch(Remodel& params, const Match& source, const PathfindingCounter&, std::function<bool(const Match&)> acceptance) const {
    std::vector<int> available;
    for (int i = 0; i < 63; ++i) {
        bool isColumn = false;
        for (const int col : GeneratorElevator::ELEVATOR_COLUMN_ROOM_IDS)
            if (i == col) { isColumn = true; break; }
        if (!isColumn)
            available.push_back(i);
    }

    const int roomId = available[static_cast<unsigned>(params.seed) % available.size()];
    Match copy = source;
    MatchController controller(copy, params.codeset);
    bool success = true;
    int containerId2 = -1;
    success &= controller.allocate(
        CharacterAllocSpec{
            .role = ROLE_CHEST,
            .chest = ChestAllocSpec{
                .inventory = {
                    CritterCharacterSpec{.character = CharacterAllocSpec{.role = ROLE_SPIDER}},
                    ItemSpec{.item = ITEM_KEY_ELEVATOR}
                }
            }
        },
        AttachmentContext::floor(roomId),
        containerId2
    );
    if (success) acceptance(copy);
    return success;
}

