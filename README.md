# Barcelona Reverbera

Non-Uniform Partitioned Convolution Reverb VST3 plugin, developed on C++. Used to create the Barcelona Reverbera VST3 plugin.

This convolution reverb was developed as part of the Barcelona Reverbera project, which aims to create a VST plugin where some iconic spaces of Barcelona can be used as convolution reverb impulse responses. The original impulse responses are not uploaded on this repo due to proprietary issues.

https://sbrkdevices.com/barcelona-reverbera

This plugin uses Steinberg's VST3 SDK.

The source code is in vst3/src. The Non-Uniform Partitioned Convolution implementation is located at vst3/src/convolution_reverb. It uses modern features of C++ in order to create the different FFT convolution stages using template metaprogramming. The fft/fft.h file includes a helper class for the forward an inverse FFTs. The pffft (https://bitbucket.org/jpommier/pffft/src) library is used to compute the FFTs.

In convert_wav_to_c_array, a python script creates C++ header files from impulse responses stored in WAV files. The generated header files can be placed in vst/src/impulse_responses/ to be used for convolution.

An impulse response (a delta function) is given as an example at vst3/src/impulse_responses/IR_example.h.

## Build process

In the build_scripts directory, execute either build_debug.sh or build_release.sh. VST3 plugin will be generated at build/VST3/Debug/BarcelonsReverbera.vst3 or build/VST3/Release/BarcelonsReverbera.vst3

Tested on Linux and Mac (Intel & Apple Silicon).