#pragma once

#include <memory>

#include "Interfaces.h"

namespace orch {

std::shared_ptr<IResponseCache> MakeDummyCache();
std::shared_ptr<IRecipientRegistry> MakeDummyRegistry();
std::shared_ptr<IDataReader> MakeDummyReader();

} // namespace orch
