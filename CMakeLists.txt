cmake_minimum_required(VERSION 3.1)

project(akonadi-maniphest)

# ECM setup
find_package(ECM 5.14.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)

# Dependencies

set(KF5_VERSION "5.12.0")
set(AKONADI_VERSION "5.0")
set(QT_REQUIRED_VERSION "5.3.0")
set(KCALCORE_MIN_VERSION "4.82.0")

find_package(Qt5 ${QT_REQUIRED_VERSION} REQUIRED Core Network Gui Test)

find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5I18n ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5WidgetsAddons ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Akonadi ${AKONADI_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KCALCORE_MIN_VERSION} CONFIG REQUIRED)

find_package(KAsync CONFIG REQUIRED)

find_package(Xsltproc REQUIRED)
set_package_properties(Xsltproc PROPERTIES DESCRIPTION "XSLT processor from libxslt" TYPE REQUIRED PURPOSE "Required to generate D-Bus interfaces for all Akonadi resources.")


# Subdirectories

add_subdirectory(src)
add_subdirectory(autotests)

# Summary

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# Install stuff
install(FILES akonadi_phabricator_resource.desktop
        DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents"
)
