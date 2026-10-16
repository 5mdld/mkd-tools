//
// kiwakiwaaにより 2026/02/25 に作成されました。
//

#include "platform/parallel.hpp"

#include <dispatch/dispatch.h>

namespace MKD::detail
{
    void parallel_dispatch(const size_t count, void* ctx, void (*fn)(size_t, void*))
    {
        struct Context
        {
            void* data;
            void (*function)(size_t, void*);
        } context{ctx, fn};

        dispatch_apply_f(count, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), &context,
                         [](void* rawContext, const size_t index)
                         {
                             const auto& context = *static_cast<Context*>(rawContext);
                             context.function(index, context.data);
                         });
    }
}
