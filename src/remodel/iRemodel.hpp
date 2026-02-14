#pragma once

#include "Bitstick.hpp"
#include <functional>
#include "Match.hpp"
#include "Maybe.hpp"
#include "RemodelEnum.hpp"

struct PathfindingCounter;
struct Remodel;

// input: seed, Match&, Codeset&, Gene&
class iRemodel {
public:
    // this could've been invoked to fix or complicate.
    // if broken, then fix
    // if fixed, then complicate

    // or, give this specific fix vs complicate functions
    // all the code is the exact same though?

    // break vs fix, who orchestrates this?
    // break <- there are no breaks, all breaks must be followed with a valid fix.
    //          the "breaker" has to orchestrate its fix.
    // RemodelJailer:
    //
    // 1) run A*->solution, (maybe skip it? maybe cache it? maybe disk save it?)
    // 2) for each breakpoint:
    //      3) remodel with jailer at breakpoint
    //      3) remodel to breaker
    //      4) run A*->solution,frontier
    //      5) if solved:
    //          6) (solved) then if acceptance(): then return final else continue
    //          7) (broken)
    //              for each fixer remodel, <- ok so here we invoke a remodel on a broken match to fix it. we should 100% be caching Pathfinders
    // 
    // 1) first A*->broken,
    // 2) fatal error because RemodelJailer can't know how to fix it
    // ^^ a lot of duplicated code can be removed by only providing the mutation and the banlist

    Maybe<Match> buildMatch(Remodel& remodel, const Match& source) const;
    // static bool findRemodel(const Bitstick<REMODEL_COUNT>& list);
    virtual bool mutateMatch(Remodel& params, const Match& source, const PathfindingCounter&, std::function<bool(const Match&)> acceptance) const = 0;
    virtual const Bitstick<REMODEL_COUNT>& getFixlist() const = 0;

    static const Bitstick<REMODEL_COUNT>& getEmpty();
    static const Bitstick<REMODEL_COUNT>& getKeyProviders();
};
