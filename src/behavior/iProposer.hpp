#pragma once

#include <functional>
#include <utility>

#include "ConductEnum.hpp"
#include "Maybe.hpp"

struct ActivationContext;

class iProposer {
public:
    virtual ~iProposer() = default;
    virtual ConductEnum conduct() const = 0;
    virtual Maybe<std::pair<int, std::function<void(ActivationContext&)>>> buildProposal(ActivationContext& ctx) const = 0;
};
