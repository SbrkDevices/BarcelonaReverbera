cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

project(BarcelonaReverbera
    VERSION 0.0.0.1
    DESCRIPTION "Barcelona Reverbera VST 3 Plug-in"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external)

smtg_enable_vst3_sdk()

smtg_add_vst3plugin(BarcelonaReverbera
    external/pffft/pffft.h
    external/pffft/pffft.c
    src/fft/fft.h
    src/impulse_responses/impulse_responses.h
    src/impulse_responses/impulse_responses.cpp
    src/samplerate_converter/samplerate_converter.h
    src/samplerate_converter/samplerate_converter.cpp
    src/convolution_reverb/convolution_reverb_direct_stage.h
    src/convolution_reverb/convolution_reverb_fft_stage.h
    src/convolution_reverb/convolution_reverb.h
    src/barcelona_reverbera_version.h
    src/barcelona_reverbera_cids.h
    src/barcelona_reverbera_processor.h
    src/barcelona_reverbera_processor.cpp
    src/barcelona_reverbera_controller.h
    src/barcelona_reverbera_controller.cpp
    src/barcelona_reverbera_entry.cpp
    resource/barcelona_reverbera_editor.uidesc
)

target_include_directories(BarcelonaReverbera
    PRIVATE
        external/pffft
        src
)

target_link_libraries(BarcelonaReverbera
    PRIVATE
        sdk
        vstgui_support
)

smtg_target_add_plugin_resources(BarcelonaReverbera
    RESOURCES
        resource/barcelona_reverbera_editor.uidesc
        resource/background.png
        resource/animation_knob.png
)

# XXX TODO: add snapshots
#smtg_target_add_plugin_snapshots (BarcelonaReverbera
#    RESOURCES
#        resource/32C50013FF5F5CB4871C312DB4F42368_snapshot.png
#        resource/32C50013FF5F5CB4871C312DB4F42368_snapshot_2.0x.png
#)

smtg_target_configure_version_file(BarcelonaReverbera)

if(SMTG_MAC)
    smtg_target_set_bundle(BarcelonaReverbera
        BUNDLE_IDENTIFIER net.steinberg.barcelona-reverbera
        COMPANY_NAME "sbrk devices"
    )
    smtg_target_set_debug_executable(BarcelonaReverbera
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(BarcelonaReverbera PRIVATE 
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BarcelonaReverbera)

        smtg_target_set_debug_executable(BarcelonaReverbera
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)
