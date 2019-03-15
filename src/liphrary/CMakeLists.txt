set(liphrary_SRCS
    project.cpp
    server.cpp
    maniphest.cpp
    markup.cpp
    user.cpp
)

add_library(liphrary STATIC ${liphrary_SRCS})


target_link_libraries(liphrary
PUBLIC
    Qt5::Core
    KAsync
PRIVATE
    KF5::KIOCore
)
