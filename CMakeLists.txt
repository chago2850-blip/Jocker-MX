# SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.21)

project(JockerMX VERSION 0.4.0 DESCRIPTION "Software DJ libre" LANGUAGES C CXX)
set(JOCKER_VERSION_STRING "0.4.0-dev")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Concurrent Sql)
qt_standard_project_setup()

# --- Terceros ---------------------------------------------------------------
# Rubber Band (GPL-2.0-or-later): estiramiento de tiempo para "key lock"
add_library(rubberband STATIC third_party/rubberband/single/RubberBandSingle.cpp)
target_include_directories(rubberband PUBLIC third_party/rubberband)
if(MSVC)
  target_compile_definitions(rubberband PRIVATE _USE_MATH_DEFINES NOMINMAX)
endif()
if(APPLE)
  target_link_libraries(rubberband PUBLIC "-framework Accelerate")
endif()

# miniaudio (dominio público / MIT-0): E/S de audio multiplataforma y decodificación
add_library(miniaudio STATIC src/engine/ma_impl.c)
target_include_directories(miniaudio PUBLIC third_party/miniaudio)
if(UNIX AND NOT APPLE)
  target_link_libraries(miniaudio PUBLIC ${CMAKE_DL_LIBS} pthread m)
elseif(APPLE)
  target_link_libraries(miniaudio PUBLIC "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox")
endif()

# RtMidi (MIT): entrada MIDI multiplataforma
add_library(rtmidi STATIC third_party/rtmidi/RtMidi.cpp)
target_include_directories(rtmidi PUBLIC third_party/rtmidi)
if(APPLE)
  target_compile_definitions(rtmidi PUBLIC __MACOSX_CORE__)
  target_link_libraries(rtmidi PUBLIC "-framework CoreMIDI" "-framework CoreAudio" "-framework CoreFoundation")
elseif(WIN32)
  target_compile_definitions(rtmidi PUBLIC __WINDOWS_MM__)
  target_link_libraries(rtmidi PUBLIC winmm)
elseif(ANDROID)
  target_compile_definitions(rtmidi PUBLIC __RTMIDI_DUMMY__)
else()
  find_package(ALSA)
  if(ALSA_FOUND)
    target_compile_definitions(rtmidi PUBLIC __LINUX_ALSA__)
    target_link_libraries(rtmidi PUBLIC ALSA::ALSA pthread)
  else()
    target_compile_definitions(rtmidi PUBLIC __RTMIDI_DUMMY__)
  endif()
endif()

# --- Aplicación -------------------------------------------------------------
qt_add_executable(jocker-mx WIN32 MACOSX_BUNDLE
  src/main.cpp
  src/engine/Track.h
  src/engine/Biquad.h
  src/engine/Deck.h src/engine/Deck.cpp
  src/engine/AudioEngine.h src/engine/AudioEngine.cpp
  src/engine/Effects.h src/engine/Effects.cpp
  src/engine/Sampler.h src/engine/Sampler.cpp
  src/engine/Recorder.h src/engine/Recorder.cpp
  src/analysis/Analyzer.h src/analysis/Analyzer.cpp
  src/analysis/KeyDetector.h src/analysis/KeyDetector.cpp
  src/library/TagReader.h src/library/TagReader.cpp
  src/library/LibraryDb.h src/library/LibraryDb.cpp
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(jocker-mx
  URI JockerMX
  VERSION 1.0
  RESOURCE_PREFIX /
  SOURCES
    src/app/DeckController.h src/app/DeckController.cpp
    src/app/Mixer.h src/app/Mixer.cpp
    src/app/LibraryModel.h src/app/LibraryModel.cpp
    src/ui/WaveformItem.h src/ui/WaveformItem.cpp
    src/control/MidiManager.h src/control/MidiManager.cpp
    src/app/SamplerController.h src/app/SamplerController.cpp
    src/app/RecordingController.h src/app/RecordingController.cpp
  QML_FILES
    qml/Main.qml
    qml/Theme.qml
    qml/Knob.qml
    qml/JxButton.qml
    qml/VuMeter.qml
    qml/DeckPanel.qml
    qml/JogWheel.qml
    qml/MixerPanel.qml
    qml/LibraryPanel.qml
    qml/TopBar.qml
    qml/PitchFader.qml
    qml/MidiDialog.qml
    qml/SamplerPanel.qml
  RESOURCES
    resources/icon-256.png
    resources/logo-3d-256.png
    resources/fonts/Montserrat.ttf
)
# Rutas de recursos resultantes: qrc:/JockerMX/qml/... y qrc:/JockerMX/resources/...

target_include_directories(jocker-mx PRIVATE src src/app src/ui src/control)
target_compile_definitions(jocker-mx PRIVATE JOCKER_VERSION="${JOCKER_VERSION_STRING}")
if(MSVC)
  target_compile_definitions(jocker-mx PRIVATE _USE_MATH_DEFINES NOMINMAX)
endif()
target_link_libraries(jocker-mx PRIVATE
  Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent Qt6::Sql rubberband miniaudio rtmidi)

set_target_properties(jocker-mx PROPERTIES
  OUTPUT_NAME "jocker-mx"
  MACOSX_BUNDLE_BUNDLE_NAME "Jocker-MX"
  MACOSX_BUNDLE_GUI_IDENTIFIER "org.jockermx.app"
  MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
  MACOSX_BUNDLE_ICON_FILE "jocker-mx.icns")

if(APPLE)
  set_target_properties(jocker-mx PROPERTIES OUTPUT_NAME "Jocker-MX")
  set(APP_ICNS ${CMAKE_SOURCE_DIR}/assets/icons/jocker-mx.icns)
  set_source_files_properties(${APP_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(jocker-mx PRIVATE ${APP_ICNS})
endif()
if(WIN32)
  target_sources(jocker-mx PRIVATE packaging/windows/jocker-mx.rc)
endif()

# --- Instalación y paquetes -------------------------------------------------
include(GNUInstallDirs)
install(TARGETS jocker-mx
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Windows y macOS: incluir Qt y módulos QML dentro del instalador (Qt ≥ 6.5)
if((WIN32 OR APPLE) AND Qt6_VERSION VERSION_GREATER_EQUAL 6.5)
  qt_generate_deploy_qml_app_script(
    TARGET jocker-mx
    OUTPUT_SCRIPT JOCKER_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR)
  install(SCRIPT ${JOCKER_DEPLOY_SCRIPT})
  if(APPLE)
    # Firma ad-hoc tras el despliegue (obligatoria en Apple Silicon; macdeployqt modifica las bibliotecas).
    install(CODE [[
      set(_app "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Jocker-MX.app")
      message(STATUS "Firmando (ad-hoc) ${_app}")
      execute_process(COMMAND codesign --force --deep --sign - "${_app}" RESULT_VARIABLE _rc)
      if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "codesign falló")
      endif()
    ]])
  endif()
endif()

if(UNIX AND NOT APPLE)
  install(FILES packaging/linux/jocker-mx.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  foreach(size 16 24 32 48 64 128 256 512)
    install(FILES assets/icons/jocker-mx-${size}.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps
            RENAME jocker-mx.png)
  endforeach()
  install(FILES LICENSE README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()

set(CPACK_PACKAGE_NAME "jocker-mx")
set(CPACK_PACKAGE_VENDOR "Jocker-MX")
set(CPACK_PACKAGE_CONTACT "Santiago Miguel <chago2850@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Software DJ libre: decks, mezclador, sync, key lock, loops y cues")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/chago2850-blip/Jocker-MX")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Jocker-MX")
set(CPACK_PACKAGE_FILE_NAME "jocker-mx-${JOCKER_VERSION_STRING}-${CMAKE_SYSTEM_NAME}")
set(CPACK_DMG_VOLUME_NAME "Jocker-MX")
set(CPACK_PACKAGE_EXECUTABLES "jocker-mx" "Jocker-MX")
# Debian / Ubuntu
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qml6-module-qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-layouts, qml6-module-qtquick-dialogs, qml6-module-qtquick-window, qml6-module-qtqml-workerscript, qml6-module-qtquick-templates, qml6-module-qt-labs-folderlistmodel, libqt6sql6-sqlite")
# Fedora / openSUSE
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_PACKAGE_AUTOREQ OFF)
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase >= 6.4, qt6-qtdeclarative >= 6.4")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr/share/applications /usr/share/icons /usr/share/icons/hicolor)
# Windows (NSIS)
set(CPACK_NSIS_DISPLAY_NAME "Jocker-MX")
set(CPACK_NSIS_PACKAGE_NAME "Jocker-MX")
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/icons/jocker-mx.ico")
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\jocker-mx.exe")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$DESKTOP\\\\Jocker-MX.lnk' '$INSTDIR\\\\bin\\\\jocker-mx.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$DESKTOP\\\\Jocker-MX.lnk'")
include(CPack)

# --- Pruebas ----------------------------------------------------------------
option(JOCKER_BUILD_TESTS "Compilar pruebas" ON)
if(JOCKER_BUILD_TESTS)
  enable_testing()
  add_executable(jx-tests
    tests/engine_tests.cpp
    src/engine/Deck.cpp src/engine/AudioEngine.cpp src/engine/Effects.cpp src/engine/Sampler.cpp src/engine/Recorder.cpp
    src/analysis/Analyzer.cpp src/analysis/KeyDetector.cpp
    src/library/TagReader.cpp src/library/LibraryDb.cpp
    src/app/DeckController.h src/app/DeckController.cpp
    src/app/Mixer.h src/app/Mixer.cpp
    src/control/MidiManager.h src/control/MidiManager.cpp
    src/app/SamplerController.h src/app/SamplerController.cpp
    src/app/RecordingController.h src/app/RecordingController.cpp)
  target_include_directories(jx-tests PRIVATE src src/app src/control)
  if(MSVC)
    target_compile_definitions(jx-tests PRIVATE _USE_MATH_DEFINES NOMINMAX)
  endif()
  target_link_libraries(jx-tests PRIVATE Qt6::Core Qt6::Qml Qt6::Concurrent Qt6::Sql rubberband miniaudio rtmidi)
  add_test(NAME motor COMMAND jx-tests)
endif()
