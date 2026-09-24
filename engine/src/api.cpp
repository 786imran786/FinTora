#include "fintora/engine/api.hpp"

namespace fintora::engine {

static_assert(sizeof(OrderId) == sizeof(std::uint64_t));
static_assert(sizeof(Price) == sizeof(std::uint64_t));
static_assert(sizeof(Quantity) == sizeof(std::uint64_t));

}  // namespace fintora::engine