import sys
import numpy as np
import soundfile as sf

# Creates a 2-dimensional C array: 2 large arrays of dobles, each for one stereo channel
# Usage example: python3 /tmp/my_audio_file.wav /tmp/my_audio_file.h

numExtraZeros = 8192 # number of zeros that are appended to the impulse response. This is needed during processing, to align the length of the IR with the block sizes used for convolution

if (len(sys.argv) != 3):
	sys.exit("Usage: python3 <source_file.wav> <dest_file.h>")

wav_file = sys.argv[1]
header_file = sys.argv[2]

print("Source wav file: " + wav_file)
print("Destination header file: " + header_file)

data, fs = sf.read(wav_file)
audio_len = len(data)

print("Fs = " + str(fs/1000) + " kHz. Audio length: " + str(round(audio_len/fs, 2)) + " s (" + str(audio_len) + " samples)")

#trim final zeros...
trim_count = 0
for i in range(0, audio_len):
	index = (audio_len - 1) - i
	if ((data[index][0] == 0) and (data[index][1] == 0)):
		trim_count += 1
	else:
		break

audio_len -= trim_count

print("Audio length after trimming final zeros: " + str(round(audio_len/fs, 2)) + " s (" + str(audio_len) + " samples)")

if (fs != 48000):
	sys.exit("ERROR: Only a samplerate of Fs = 48 kHz is supported")
if (len(data[0]) != 2):
	sys.exit("ERROR: Source audio file should be stereo (2 channels)")

print("Writing output file...")

with open(header_file, 'w') as file:
	audioPlusZerosLen = audio_len + numExtraZeros
	header_file_split_slashes = header_file.split('/')
	arrayName = header_file_split_slashes[len(header_file_split_slashes)-1].split('.')[0]
	file.write('/*\n************************ AUTOMATICALLY GENERATED FILE, DO NOT MODIFY! ************************\n*/\n\n')
	file.write('#pragma once\n\n')
	file.write('#define __' + arrayName + '_EXTRA_ZEROS__        (' + str(numExtraZeros) + ')\n\n')
	file.write('float __' + arrayName + '__[2][' + str(audioPlusZerosLen) + '] = {\n')
	for ch in range(0, 2):
		file.write('    {\n        ')

		for i in range(0, audioPlusZerosLen):
			if (i < audio_len):
				file.write(str(data[i][ch]) + ', ')
			else:
				file.write('0.0, ')

		file.write('\n    },\n')

	file.write('};\n')

print("Done!")
