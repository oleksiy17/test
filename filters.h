#ifndef FILE_RW_H
#define FILE_RW_H

#include "math.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <windows.h>

#define SAMPLE_RATE		44100
#define LOWER_CUTOFF	3000
#define HIGNER_CUTOFF	5000
#define TRANSIENT_BAND	1000
#define M_PI			3.1415

long double bandPassFilter(const long double &x, const long double &h_BandPass, CircBuf &ptrBuf, signed short *x_begin);
long double bandStopFilter(const long double &x, const long double &h_BandStop, CircBuf &ptrBuf, signed short *x_begin);
long double Compressor(const long double yBandPass, ...);
signed short deEsser(const signed short &x, const signed short &h_BandPass, const signed short &h_BandStop, CircBuf &ptrBuf, signed short *x_begin);

typedef struct{
	
	unsigned char riff[4];                      // RIFF string
	unsigned char overall_size[4];  	            // overall size of file in bytes
    unsigned char wave[4];                      // WAVE string
    unsigned char fmt_chunk_marker[4];          // fmt string with trailing null char
    unsigned char length_of_fmt[4];                 // length of the format data
    unsigned char format_type[2];                   // format type
    unsigned char channels[2];                      // # of channels
    unsigned char sample_rate[4];                   // sampling rate 
    unsigned char byterate[4];                      // SampleRate * NumChannels * BitsPerSample/8
    unsigned char block_align[2];                   // NumChannels * BitsPerSample/8
    unsigned char bits_per_sample[2];               // bits per sample, 8- 8bits, 16- 16 bits
    unsigned char data_chunk_header [4];        // DATA string or FLLR string
    unsigned char data_size[4];                     // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
} waveFileHeader;

#endif