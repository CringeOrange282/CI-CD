set(SOURCES
    src/main.cpp
    src/AsyncComponentBase.cpp
    src/CalculateRequestMaker.cpp
    src/CalculateServiceSender.cpp
    src/DummyClasses.cpp
    src/ExternalResponseCache.cpp
    src/ExternalResponseHandler.cpp
    src/HTTPServer.cpp
    src/JsonDatabase.cpp
    src/Logger.cpp
    src/Notifier.cpp
    src/Utils.cpp
    src/RecipientList.cpp
    src/request.cpp
)

add_executable(orchestra ${SOURCES})
target_include_directories(
    orchestra
    PUBLIC
    include
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(
    orchestra
    PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
    httplib::httplib
)
