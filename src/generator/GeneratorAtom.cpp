#include "ArchitectState.hpp"
#include "AtomLibrary.hpp"
#include "AtomPlacement.hpp"
#include "Cardinal.hpp"
#include "ChannelEnum.hpp"
#include "Chest.hpp"
#include "Codeset.hpp"
#include "DungeonAuthor.hpp"
#include "GeneratorAtom.hpp"
#include "GraphPathEmbedder.hpp"
#include "iLayout.hpp"
#include "int4.hpp"
#include "ItemEnum.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "LayoutGrid.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"
#include "to_int4.hpp"
#include <cstring>

// ─── Atom sequence ────────────────────────────────────────────────────────────

static std::vector<const Atom*> generateAtomSequence(int seed, int count) {
    int atomCount = 0;
    const Atom* allAtoms = AtomLibrary::getAll(atomCount);

    const Atom* doorwayAtom = nullptr;
    for (int i = 0; i < atomCount; ++i) {
        if (std::strcmp(allAtoms[i].name, "DOORWAY") == 0) {
            doorwayAtom = &allAtoms[i];
            break;
        }
    }

    std::vector<const Atom*> sequence;
    ArchitectState state;
    state.maxChests = Dungeon::MAX_CHESTS;

    sequence.push_back(doorwayAtom);
    if (doorwayAtom) state.apply(doorwayAtom->meta);

    for (int i = 1; i < count; ++i) {
        int entropy = seed ^ (i * 0x9e3779b9);
        auto candidates = AtomLibrary::getCompatible(state);
        unsigned int idx = static_cast<unsigned int>(entropy) % static_cast<unsigned int>(candidates.empty() ? 1 : candidates.size());
        const Atom* picked = candidates.empty() ? doorwayAtom : candidates[idx];
        sequence.push_back(picked);
        if (picked) state.apply(picked->meta);
    }
    return sequence;
}

// ─── Compilation ─────────────────────────────────────────────────────────────

static bool compileAtomIntoPlacement(
    const Atom&          atom,
    const AtomPlacement& p,
    const LayoutGrid&    grid,
    DungeonAuthor&       author,
    MatchController&     controller,
    Codeset&             codeset)
{
    bool success = true;

    for (int e = 0; e < 3; ++e) {
        int4 from = grid.getCoordinates(p.roomIds[e]);
        switch (atom.edges[e]) {
            case AtomEdgeType::DOORWAY:
                success &= author.setupDoorway(from, p.edgeDir);
                break;
            case AtomEdgeType::KEEPER_KEYED:
                success &= author.setupKeeper(from, p.edgeDir, true);
                break;
            case AtomEdgeType::TOGGLER_BLUE:
                success &= author.setupTogglerBlue(from, p.edgeDir);
                break;
            case AtomEdgeType::TOGGLER_ORANGE:
                success &= author.setupTogglerOrange(from, p.edgeDir);
                break;
        }
    }

    for (int n = 0; n < 4; ++n) {
        if (atom.nodeHasKeyChest[n]) {
            int roomId = p.roomIds[n];
            success &= controller.allocateChest(roomId, [&](Chest& chest, Character& container, Character& critter){
                container.role = ROLE_CHEST;
                critter.role   = ROLE_CAT;
                chest.inventory.giveItem(ITEM_KEY, codeset.error);
            });
        }
        if (atom.nodeHasToggler[n]) {
            int outCharId = -1, outFloorId = -1;
            success &= author.setupTogglerSwitch(grid.getCoordinates(p.roomIds[n]), outCharId, outFloorId);
        }
    }

    return success;
}

// ─── Generator ───────────────────────────────────────────────────────────────

bool GeneratorAtom::generate(int seed, Match& dst, Codeset& codeset) const {
    MatchController controller(dst, codeset);
    bool success = true;

    CodeEnum error = CODE_UNKNOWN_ERROR;
    if (!dst.setupSingleBuilder(error))
        return false;

    int builderId = -1, floorId = 0;
    dst.builders.access(0, [&](Builder& builder){
        builder.character.role = ROLE_BUILDER;
        if (dst.containsCharacter(builder.character, builderId)) {
            if (controller.findFreeFloor(START_ROOM, CHANNEL_CORPOREAL, floorId))
                controller.assignCharacterToFloor(builderId, START_ROOM, CHANNEL_CORPOREAL, floorId);
        }
        controller.updateTraits(builder.character);
    });

    LayoutFlyweight::getFlyweights().accessConst(dst.dungeon.layout, [&](const LayoutFlyweight& fw){
        fw.layout.accessConst([&](const iLayout& layoutIntf){
            layoutIntf.setupAdjacencyPointers(dst.dungeon.rooms);
            DungeonAuthor author(controller, layoutIntf);

            const auto* grid = dynamic_cast<const LayoutGrid*>(&layoutIntf);
            if (!grid) { success = false; return; }

            // Stage 2: atoms chosen first, count bounded by grid capacity
            constexpr int MAX_ATOMS = 16;
            int gridCapacity = (grid->width / 4) * grid->height;
            int atomCount = std::min(MAX_ATOMS, gridCapacity);
            auto atoms = generateAtomSequence(seed, atomCount);

            // Stage 1+4: embed atoms into room placements
            GraphPathEmbedder embedder;
            auto placements = embedder.build(atoms, layoutIntf, seed);

            // Compile: connection doorways + atom content
            for (int i = 0; i < static_cast<int>(placements.size()); ++i) {
                const AtomPlacement& p = placements[i];
                if (p.prevExitRoomId >= 0)
                    success &= author.setupDoorway(grid->getCoordinates(p.prevExitRoomId), p.connDir);
                if (atoms[i])
                    success &= compileAtomIntoPlacement(*atoms[i], p, *grid, author, controller, codeset);
            }

            // Exit door on the forward face of the last placement's final room
            if (!placements.empty()) {
                const AtomPlacement& last = placements.back();
                success &= author.setupExitDoor(grid->getCoordinates(last.roomIds[3]), Cardinal::north());
            }
        });
    });

    return success;
}
