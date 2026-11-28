# SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.21)

project(JockerMX VERSION 0.15.0 DESCRIPTION "Software DJ libre" LANGUAGES C CXX)
set(JOCKER_VERSION_STRING "0.15.0-dev")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Concurrent Sql Network)
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
if(ANDROID)
  target_link_libraries(miniaudio PUBLIC ${CMAKE_DL_LIBS} m log)
elseif(UNIX AND NOT APPLE)
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

# Codificadores para transmisión en vivo: Opus (BSD-3, sólo C genérico) y Shine MP3 (LGPL-2.0+)
file(STRINGS third_party/opus/SOURCES.txt JX_OPUS_SOURCES)
list(TRANSFORM JX_OPUS_SOURCES PREPEND third_party/opus/)
add_library(jxopus STATIC ${JX_OPUS_SOURCES})
target_include_directories(jxopus PUBLIC third_party/opus/include
  PRIVATE third_party/opus/celt third_party/opus/silk third_party/opus/silk/float third_party/opus/src third_party/opus)
target_compile_definitions(jxopus PRIVATE OPUS_BUILD USE_ALLOCA)
if(NOT MSVC)
  target_compile_definitions(jxopus PRIVATE HAVE_LRINTF HAVE_LRINT)
  target_compile_options(jxopus PRIVATE -w)
endif()
set_target_properties(jxopus PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(UNIX AND NOT APPLE)
  target_link_libraries(jxopus PRIVATE m)
endif()
# Códigos QR (MIT)
add_library(jxqr STATIC third_party/qrcodegen/qrcodegen.cpp)
target_include_directories(jxqr PUBLIC third_party/qrcodegen)
set_target_properties(jxqr PROPERTIES POSITION_INDEPENDENT_CODE ON)
file(GLOB JX_SHINE_SOURCES third_party/shine/*.c)
add_library(jxshine STATIC ${JX_SHINE_SOURCES})
target_include_directories(jxshine PUBLIC third_party/shine)
if(NOT MSVC)
  target_compile_options(jxshine PRIVATE -w)
endif()
set_target_properties(jxshine PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Decodificador del sistema para AAC/M4A (AudioToolbox / Media Foundation / FFmpeg)
add_library(jxplatformdec INTERFACE)
if(APPLE)
  target_link_libraries(jxplatformdec INTERFACE "-framework AudioToolbox" "-framework CoreFoundation")
elseif(WIN32)
  target_link_libraries(jxplatformdec INTERFACE mfplat mfreadwrite mfuuid ole32)
elseif(NOT ANDROID)
  find_package(PkgConfig)
  if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG IMPORTED_TARGET libavformat libavcodec libavutil libswresample)
  endif()
  if(FFMPEG_FOUND)
    target_compile_definitions(jxplatformdec INTERFACE JX_HAVE_FFMPEG)
    target_link_libraries(jxplatformdec INTERFACE PkgConfig::FFMPEG)
    message(STATUS "AAC/M4A mediante FFmpeg ${FFMPEG_libavcodec_VERSION}")
  else()
    message(WARNING "FFmpeg no encontrado: sin soporte AAC/M4A en Linux")
  endif()
endif()

# Video en los decks y decodificador de respaldo (Qt Multimedia; opcional)
option(JOCKER_VIDEO "Video en los decks (Qt Multimedia)" ON)
if(JOCKER_VIDEO)
  find_package(Qt6 6.4 QUIET COMPONENTS Multimedia)
endif()
if(JOCKER_VIDEO AND TARGET Qt6::Multimedia)
  target_compile_definitions(jxplatformdec INTERFACE JX_HAVE_VIDEO)
  target_link_libraries(jxplatformdec INTERFACE Qt6::Multimedia)
  message(STATUS "Video mediante Qt Multimedia ${Qt6Multimedia_VERSION}")
else()
  message(WARNING "Qt Multimedia no encontrado: Jocker-MX se compila sin video")
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
  src/analysis/KeyDetector.h src/analysis/KeyDetector.cpp src/analysis/Structure.cpp
  src/analysis/PlatformDecoder.h src/analysis/PlatformDecoder.cpp
  src/library/TagReader.h src/library/TagReader.cpp
  src/library/LibraryDb.h src/library/LibraryDb.cpp
  src/stems/StemModel.h src/stems/StemModel.cpp
  src/stems/StemSeparator.h src/stems/StemSeparator.cpp
  src/stems/StemsService.h src/stems/StemsService.cpp
)

set_source_files_properties(qml/Theme.qml qml/Stage.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(jocker-mx
  URI JockerMX
  VERSION 1.0
  RESOURCE_PREFIX /
  SOURCES
    src/app/DeckController.h src/app/DeckController.cpp
    src/app/Mixer.h src/app/Mixer.cpp
    src/app/LibraryModel.h src/app/LibraryModel.cpp
    src/ui/WaveformItem.h src/ui/WaveformItem.cpp
    src/ui/CdgItem.h src/ui/CdgItem.cpp
    src/karaoke/Cdg.h src/karaoke/Cdg.cpp
    src/karaoke/Lrc.h src/karaoke/Lrc.cpp
    src/video/DeckVideo.h src/video/DeckVideo.cpp
    src/ui/VideoItem.h src/ui/VideoItem.cpp
    src/broadcast/Encoders.h src/broadcast/Encoders.cpp
    src/broadcast/Broadcaster.h src/broadcast/Broadcaster.cpp
    src/app/BroadcastController.h src/app/BroadcastController.cpp
    src/app/UiPrefs.h
    src/broadcast/LocalRadio.h src/broadcast/LocalRadio.cpp
    src/app/RadioController.h src/app/RadioController.cpp
    src/ui/QrItem.h src/ui/QrItem.cpp
    src/control/MidiManager.h src/control/MidiManager.cpp
    src/app/SamplerController.h src/app/SamplerController.cpp
    src/app/RecordingController.h src/app/RecordingController.cpp
    src/app/AutomixPlan.h src/app/AutomixPlan.cpp
    src/app/AutomixController.h src/app/AutomixController.cpp
    src/app/EventScheduler.h src/app/EventScheduler.cpp
    src/app/KaraokeController.h src/app/KaraokeController.cpp
  QML_FILES
    qml/Main.qml
    qml/Theme.qml
    qml/Stage.qml
    qml/VideoPanel.qml
    qml/BroadcastPanel.qml
    qml/DesktopUi.qml
    qml/TouchUi.qml
    qml/TouchDeck.qml
    qml/TouchMixer.qml
    qml/TouchLibrary.qml
    qml/TouchMore.qml
    qml/TouchPads.qml
    qml/TouchAutomix.qml
    qml/TouchKaraoke.qml
    qml/RadioPanel.qml
    qml/TouchButton.qml
    qml/TouchFader.qml
    qml/nav.js
    qml/JxField.qml
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
    qml/AutomixPanel.qml
    qml/JxMenu.qml
    qml/JxMenuItem.qml
    qml/JxPrompt.qml
    qml/KaraokeView.qml
    qml/KaraokeScreen.qml
    qml/KaraokePanel.qml
  RESOURCES
    resources/icon-256.png
    resources/logo-3d-256.png
    resources/fonts/Montserrat.ttf
)
# Rutas de recursos resultantes: qrc:/JockerMX/qml/... y qrc:/JockerMX/resources/...

target_include_directories(jocker-mx PRIVATE src src/app src/ui src/control)
target_include_directories(jocker-mx SYSTEM PRIVATE third_party/eigen)
target_compile_definitions(jocker-mx PRIVATE EIGEN_MPL2_ONLY)

# Modelo de stems (Spleeter 4 stems de Deezer, MIT) en el formato de Jocker-MX.
# Se genera con tools/export_stems_model.py; si no existe, la app funciona sin stems.
set(JOCKER_STEMS_MODEL "${CMAKE_SOURCE_DIR}/models/jx-stems-4.bin" CACHE FILEPATH "Modelo de stems (jx-stems-4.bin)")
if(EXISTS "${JOCKER_STEMS_MODEL}")
  message(STATUS "Modelo de stems: ${JOCKER_STEMS_MODEL}")
  set(JOCKER_HAVE_STEMS_MODEL ON)
  if(APPLE)
    set_source_files_properties("${JOCKER_STEMS_MODEL}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/models")
    target_sources(jocker-mx PRIVATE "${JOCKER_STEMS_MODEL}")
  elseif(ANDROID)
    file(COPY "${JOCKER_STEMS_MODEL}" DESTINATION "${CMAKE_SOURCE_DIR}/android/assets/models")
  else()
    # Para ejecutar desde la carpeta de compilación
    file(COPY "${JOCKER_STEMS_MODEL}" DESTINATION "${CMAKE_BINARY_DIR}/models")
  endif()
else()
  message(WARNING "Sin modelo de stems: la separación de voz/instrumentos estará desactivada")
endif()
target_compile_definitions(jocker-mx PRIVATE JOCKER_VERSION="${JOCKER_VERSION_STRING}" JOCKER_BROADCAST_VERSION="${JOCKER_VERSION_STRING}")
if(MSVC)
  target_compile_definitions(jocker-mx PRIVATE _USE_MATH_DEFINES NOMINMAX)
endif()
target_link_libraries(jocker-mx PRIVATE
  Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent Qt6::Sql Qt6::Network rubberband miniaudio rtmidi jxplatformdec jxopus jxshine jxqr)

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

# --- Android ---------------------------------------------------------------
if(ANDROID)
  set_target_properties(jocker-mx PROPERTIES
    QT_ANDROID_PACKAGE_NAME "org.jockermx.app"
    QT_ANDROID_APP_NAME "Jocker-MX"
    QT_ANDROID_VERSION_NAME "${JOCKER_VERSION_STRING}"
    QT_ANDROID_VERSION_CODE 5
    QT_ANDROID_TARGET_SDK_VERSION 34
    QT_ANDROID_MIN_SDK_VERSION 28
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android")
  target_link_libraries(jocker-mx PRIVATE Qt6::CorePrivate)
endif()

# --- Instalación y paquetes -------------------------------------------------
include(GNUInstallDirs)
install(TARGETS jocker-mx
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

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

if(JOCKER_HAVE_STEMS_MODEL AND NOT APPLE AND NOT ANDROID)
  if(WIN32)
    install(FILES "${JOCKER_STEMS_MODEL}" DESTINATION ${CMAKE_INSTALL_BINDIR}/models)
  else()
    install(FILES "${JOCKER_STEMS_MODEL}" DESTINATION ${CMAKE_INSTALL_DATADIR}/jocker-mx/models)
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
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qml6-module-qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-layouts, qml6-module-qtquick-dialogs, qml6-module-qtquick-window, qml6-module-qtqml-workerscript, qml6-module-qtquick-templates, qml6-module-qt-labs-folderlistmodel, libqt6sql6-sqlite, libqt6multimedia6")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "gstreamer1.0-plugins-good, gstreamer1.0-libav")
# Fedora / openSUSE
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_PACKAGE_AUTOREQ OFF)
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase >= 6.4, qt6-qtdeclarative >= 6.4, qt6-qtmultimedia >= 6.4, (libavcodec-free or ffmpeg-libs), (libavformat-free or ffmpeg-libs), (libswresample-free or ffmpeg-libs)")
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
    src/analysis/Analyzer.cpp src/analysis/KeyDetector.cpp src/analysis/Structure.cpp src/analysis/PlatformDecoder.cpp
    src/karaoke/Cdg.cpp src/karaoke/Lrc.cpp
    src/video/DeckVideo.h src/video/DeckVideo.cpp
    src/broadcast/Encoders.h src/broadcast/Encoders.cpp
    src/broadcast/Broadcaster.h src/broadcast/Broadcaster.cpp
    src/broadcast/LocalRadio.h src/broadcast/LocalRadio.cpp
    src/ui/QrItem.h src/ui/QrItem.cpp
    src/library/TagReader.cpp src/library/LibraryDb.cpp
    src/stems/StemModel.cpp src/stems/StemSeparator.cpp src/stems/StemsService.h src/stems/StemsService.cpp
    src/app/DeckController.h src/app/DeckController.cpp
    src/app/Mixer.h src/app/Mixer.cpp
    src/control/MidiManager.h src/control/MidiManager.cpp
    src/app/SamplerController.h src/app/SamplerController.cpp
    src/app/RecordingController.h src/app/RecordingController.cpp
    src/app/AutomixPlan.h src/app/AutomixPlan.cpp
    src/app/AutomixController.h src/app/AutomixController.cpp
    src/app/EventScheduler.h src/app/EventScheduler.cpp
    src/app/KaraokeController.h src/app/KaraokeController.cpp)
  target_include_directories(jx-tests PRIVATE src src/app src/control)
  target_compile_definitions(jx-tests PRIVATE JX_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data" EIGEN_MPL2_ONLY JOCKER_BROADCAST_VERSION="${JOCKER_VERSION_STRING}")
  target_include_directories(jx-tests SYSTEM PRIVATE third_party/eigen)
  if(MSVC)
    target_compile_definitions(jx-tests PRIVATE _USE_MATH_DEFINES NOMINMAX)
  endif()
  target_link_libraries(jx-tests PRIVATE jxopus jxshine jxqr Qt6::Quick Qt6::Network Qt6::Core Qt6::Gui Qt6::Qml Qt6::Concurrent Qt6::Sql rubberband miniaudio rtmidi jxplatformdec)
  add_test(NAME motor COMMAND jx-tests)
endif()
