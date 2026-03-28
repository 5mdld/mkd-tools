//
// kiwakiwaaにより 2026/02/25 に作成されました。
//

#include "platform/parallel.hpp"

#include <Dispatch/Dispatch.hpp>
#include <utility>

namespace MKD::detail
{
    namespace
    {
        void applyTrampoline(void* pContext, const size_t index)
        {
            const auto* payload = static_cast<std::pair<void (*)(size_t, void*), void*>*>(pContext);
            payload->first(index, payload->second);
        }
    }

    void parallel_dispatch(const size_t count, void* ctx, void (*fn)(size_t, void*))
    {
        auto* queue = Dispatch::globalQueue(QOS_CLASS_USER_INITIATED, 0);
        std::pair payload{fn, ctx};
        Dispatch::apply(count, queue, &payload, applyTrampoline);
    }
}
