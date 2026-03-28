// Copyright (c) 2026 kiwakiwaa
// https://github.com/kiwakiwaa
// SPDX-License-Identifier: GPL-3.0-only
// Licensed under the GNU General Public License v3.0.

#pragma once

#include <dispatch/dispatch.h>

#include <cstddef>

namespace Dispatch
{
    using Queue = dispatch_queue_t;
    using ApplyFunction = void (*)(void* pContext, size_t index);

    inline Queue globalQueue(const dispatch_qos_class_t qosClass = QOS_CLASS_DEFAULT, const unsigned long flags = 0)
    {
        return dispatch_get_global_queue(qosClass, flags);
    }

    inline void apply(const size_t count, const Queue queue, void* pContext, const ApplyFunction pFunction)
    {
        dispatch_apply_f(count, queue, pContext, pFunction);
    }
}
