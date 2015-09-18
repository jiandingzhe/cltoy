#include "htio2/RefCounted.h"

namespace htio2
{

RefCounted::RefCounted(): counter() { }

RefCounted::RefCounted(const RefCounted &other): counter() { }

RefCounted::~RefCounted() { }

} // namespace htio2
