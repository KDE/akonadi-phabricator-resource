add_subdirectory(liphrary)


set(akonadi_phabricator_resource_SRCS
    resource.cpp
    settings.cpp
    configdialog.cpp
)

qt5_wrap_ui(akonadi_phabricator_resource_SRCS
    configdialog.ui
)

ecm_qt_declare_logging_category(akonadi_phabricator_resource_SRCS
    HEADER debug.h
    IDENTIFIER LOG
    CATEGORY_NAME log_maniphestresource
)

kconfig_add_kcfg_files(akonadi_phabricator_resource_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfgc
)

kcfg_generate_dbus_interface(
    ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
    org.kde.Akonadi.ManiphestResource.Settings
)

qt5_add_dbus_adaptor(akonadi_phabricator_resource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.ManiphestResource.Settings.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.h Settings
)


add_executable(akonadi_phabricator_resource ${akonadi_phabricator_resource_SRCS})
target_link_libraries(akonadi_phabricator_resource
    Qt5::Network
    Qt5::DBus
    KF5::AkonadiAgentBase
    KF5::ConfigCore
    KF5::I18n
    KF5::WidgetsAddons
    KF5::CalendarCore
    KAsync
    liphrary
)

install(TARGETS akonadi_phabricator_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
