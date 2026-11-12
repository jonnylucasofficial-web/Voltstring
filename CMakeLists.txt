cmake_minimum_required(VERSION 3.22)
project(Voltstring VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Use ./JUCE if present, otherwise download it automatically
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt)
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

option(COPY_PLUGINS "Copy plugins to system folders after build" ON)

set(VS_FORMATS VST3)
if(APPLE)
    list(APPEND VS_FORMATS AU)
endif()

juce_add_plugin(Voltstring
    COMPANY_NAME "Jonny Lucas"
    PLUGIN_MANUFACTURER_CODE Jluc
    PLUGIN_CODE Vlts
    FORMATS ${VS_FORMATS}
    PRODUCT_NAME "Voltstring"
    BUNDLE_ID com.jonnylucas.voltstring
    IS_SYNTH FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Generator
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_PLUGINS})

target_sources(Voltstring PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Voltstring PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Voltstring
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

