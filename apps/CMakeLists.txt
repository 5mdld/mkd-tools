if (NOT APPLE)
    return()
endif ()

if (CMAKE_GENERATOR MATCHES "Unix Makefiles")
    message(WARNING "Skipping receipt app: Swift is not supported by the '${CMAKE_GENERATOR}' generator. Use Ninja or Xcode to build the app.")
    return()
endif ()

enable_language(Swift)

add_subdirectory(monokakido-receipt-ui)
