#include "ChannelEnum.hpp"
#include "CodeEnum.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Remodel.hpp"
#include "RemodelAuthorSetupBuilderRole.hpp"
#include "RoleEnum.hpp"

bool RemodelAuthorSetupBuilderRole::mutateMatch(
    Remodel& dst,
    const Match& source,
    const PathfindingCounter&,
    std::function<bool(const Match&)> acceptance) const
{
    Match copy = source;
    MatchController controller(copy, dst.codeset);

    CodeEnum error = CODE_UNKNOWN_ERROR;
    if (!copy.setupSingleBuilder(error))
        return false;

    int floorId = 0;
    int builderId = -1;

    copy.builders.access(this->builderIndex, [&](Builder& builder) {
        builder.character.role = ROLE_BUILDER;

        if (copy.containsCharacter(builder.character, builderId)) {
            bool ok = controller.findFreeFloor(builder.startingRoomId, CHANNEL_CORPOREAL, floorId);
            if (ok)
                controller.assignCharacterToFloor(builderId, builder.startingRoomId, CHANNEL_CORPOREAL, floorId);
        }

        controller.updateTraits(builder.character);
    });

    return acceptance(copy);
}
