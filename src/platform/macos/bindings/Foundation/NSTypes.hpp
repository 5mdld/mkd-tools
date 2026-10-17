//
// kiwakiwaaにより 2026/03/27 に作成されました。
//

// Adapted from Apple's metal-cpp. Original portions:
// Copyright 2020-2024 Apple Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Modifications copyright (c) 2026 kiwakiwaa.
// Modifications are licensed under the GNU General Public License v3.0 only.
// SPDX-License-Identifier: Apache-2.0 AND GPL-3.0-only
// See LICENSE-APACHE-2.0.txt in the bindings directory.

#pragma once

#include "NSDefines.hpp"

#include <cstdint>

namespace NS
{
    using UInteger = std::uintptr_t;

    struct Range
    {
        UInteger location;
        UInteger length;
    };
}
