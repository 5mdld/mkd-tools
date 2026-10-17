//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include "MKD/dictionary/dictionary_product.hpp"
#include "MKD/dictionary/dictionary.hpp"
#include "MKD/dictionary/dictionary_search.hpp"
#include "MKD/dictionary/metadata.hpp"
#include "MKD/dictionary/paths.hpp"
#include "MKD/dictionary/product_writer.hpp"

#include "MKD/output/export.hpp"

#include "MKD/platform/directory_dictionary_source.hpp"

#ifdef __APPLE__
#include "MKD/platform/macos/paths.hpp"
#include "MKD/receipt/purchase_receipt.hpp"
#endif

#include "MKD/resource/resource_loader.hpp"
#include "MKD/resource/appendix_entry_list.hpp"
#include "MKD/resource/keystore.hpp"
#include "MKD/resource/keystore_search.hpp"
#include "MKD/resource/search_mode.hpp"
#include "MKD/resource/headline_store.hpp"
#include "MKD/resource/named_resource_store.hpp"
#include "MKD/resource/resource_store.hpp"
#include "MKD/resource/font.hpp"
#include "MKD/resource/stylesheet.hpp"
#include "MKD/resource/retained_span.hpp"
#include "MKD/resource/resource_store_writer.hpp"
#include "MKD/resource/named_resource_store_writer.hpp"
#include "MKD/resource/keystore_writer.hpp"
#include "MKD/resource/headline_store_writer.hpp"
#include "MKD/resource/appendix_entry_list_writer.hpp"
