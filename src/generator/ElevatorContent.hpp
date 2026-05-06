#pragma once

#include "RemodelAuthor.hpp"

class ElevatorContentBlank : public RemodelAuthorBase {
public:
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)> acceptance) const override;
};

class ElevatorContentHandcrafted : public RemodelAuthorBase {
public:
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)> acceptance) const override;
};

class ElevatorContentAtom : public RemodelAuthorBase {
public:
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)> acceptance) const override;
};
