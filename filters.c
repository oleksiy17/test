#include "filters.h"

	FILE wav_track;					/* File handler and pionter to file handler */
	FILE *ptrWav_track;
	
	FILE *filteredWav_track;
	
	waveFileHeader wav_header;
	waveFileHeader *ptrWav_header = &wav_header;
/*
In this main function may be opened wav file if we run Windows or 
*/
int main(int argc, char **argv)
{
	if(argc < 2)					/* Check for command promp parameter existance(name of wav file)*/
	{
		printf("Path to wav file is not exist!");
		exit(1);
	}
	

	/* wav file opening */
	ptrWav_track = fopen(argv[1], "r");		// open wav file only for read
	if(ptrWav_track == NULL)				// if file doesn't open exit program
	{
		printf("Failed during opening wav file/n");
		exit(1);
	}
	
	int read = 0;
	read = fread(ptrWav_header, sizeof(wav_header), 1, ptrWav_track);			/* Read data header from wav file into a structure */ 
	
	unsigned int dataInBytes =   wav_header.data_size[0] | 
								(wav_header.data_size[1]<<8) | 
								(wav_header.data_size[2]<<16) | 
								(wav_header.data_size[3]<<24); 		// read number of bytes in data segment (with changed endian)
	
	unsigned char data[dataInBytes];							// array for audio data
	read = fread(data, dataInBytes, 1, ptrWav_track);			// write data into array from wav file
	
	/* get number of channels, sampling rate and a number of bits per sample and change endian*/
	
	unsigned int ch =   	 wav_header.channels[0] | 
							(wav_header.channels[1]<<8);			// number of channels
						
	unsigned int sampRate =  wav_header.sample_rate[0] | 
							(wav_header.sample_rate[1]<<8) | 
							(wav_header.sample_rate[2]<<16) | 
							(wav_header.sample_rate[3]<<24);		// sampling rate
							
	unsigned int bitsPerSam =    wav_header.bits_per_sample[0] | 
								(wav_header.bits_per_sample[1]<<8);	// bits per sample
	
	/* Assume that we have 1 channel and 16 bits per sample for simplifying algorithm */
	
	
	
	
	long double coeffNum = (5.5 * sampRate)/TRANSIENT_BAND;		//Length of filter (double value)
	int N = (int)coeffNum;											//Converting length into integer value
	int Half_N = N/2;
	
	/* Let's fill the buffer with the values from wav-file (with conversion from two chars into a signed short) */
	
	signed short x[dataInBytes/2];		// array of input values
	
	for(int i = 0, k = 0; i < dataInBytes/2; i++, k++)
	{
		x[i] = data[k] | (data[k++]<<8);		/* x[i] - buffer filled in big-endian order	*/
	}
	
	signed short *x_begin = &x[0];					/* beginning and the end of data*/
	signed short *x_end = &x[(dataInBytes/2)-1];
	signed short *x_right_edge = &x[N-1];				/* range between x_begin and x_right_edge can be filtered in one function call */
	
	
	long double Fc_tr_lower = (LOWER_CUTOFF - (TRANSIENT_BAND/2))/sampRate;			//lower cutoff frequency
	long double Fc_tr_higher = (HIGNER_CUTOFF + (TRANSIENT_BAND/2))/sampRate;		//higher cutoff frequency
	
	/*Calculating parameters for Band-Pass filter and Band-Stop filter*/
	
	long double Hd_BandPass[Half_N] = {0};			//Ideal frequency characteristic of Band-Pass filter
	long double h_BandPass[N] = {0};					//Band-Pass filter coefficients
	
	long double Hd_BandStop[Half_N] = {0};			//Ideal frequency characteristic of Band-Stop filter
	long double h_BandStop[N] = {0};					//Band-Stop filter coefficients
	
	long double W[Half_N] = {0};						//Weight function of Blackman
	
	for(int i = 0; i<Half_N; i++)				
	{
		/* Find the impulse characteristics of standard frequency-selectable filter*/
		if(i == 0)
		{
			Hd_BandPass[i] = 2*(Fc_tr_higher - Fc_tr_lower);
		}
		else
		{
			Hd_BandPass[i] = 2 * Fc_tr_higher * (sin(i*2*M_PI*Fc_tr_higher) / (i*2*M_PI*Fc_tr_higher)) - 2 * Fc_tr_lower *(sin(i*2*M_PI*Fc_tr_lower) / (i*2*M_PI*Fc_tr_lower));
		}
		
		/* Find the impulse characteristics of standard frequency-selectable filter*/
		if(i == 0)
		{
			Hd_BandStop[i] = 1 - (2*(Fc_tr_higher - Fc_tr_lower));
		}
		else
		{
			Hd_BandStop[i] = 2 * Fc_tr_lower *(sin(i*2*M_PI*Fc_tr_lower) / (i*2*M_PI*Fc_tr_lower)) - 2 * Fc_tr_higher * (sin(i*2*M_PI*Fc_tr_higher) / (i*2*M_PI*Fc_tr_higher));
		}
		
		/* Blackman weight function */
		
		W[i] = 0.42 + 0.5*cos((2*M_PI*i)/(N-1)) + 0.08*cos((4*M_PI*i)/(N-1));
		
		/* Find h(i) for Band-Pass*/
		
		h_BandPass[i] = W[i] * Hd_BandPass[i];
		h_BandPass[i] = h_BandPass[N-i];		//Filer values are symetrical
		
		/* Find h(i) for Band-Stop*/
		
		h_BandStop[i] = W[i] * Hd_BandStop[i];
		h_BandStop[i] = h_BandStop[N-i];		//Filer values are symetrical
		
	}
	
	int i;
	signed short y[dataInBytes/2];
	
	for(i = 0; x_right_edge != x_end; i++, x_begin++, x_right_edge++)
	{		
		y[i] = deEsser(&x, &h_BandPass, &h_BandStop, CircBuf &ptrBuf, x_begin);
	}
	/* Data in range of (dataInBytes/2 - N) to (dataInBytes/2) don't been filtered, therefor store this data into output sequence */
	
	for(int k = 0; x_right_edge<=x_end; x_right_edge++, k++)
	{
		y[i] = x[(dataInBytes/2)-N+k];
	}
	
	/* After that we have filtered values, now lets write it into file*/
	unsigned char *new_y_most = &y[0];
	unsigned char *new_y_less = &y[1];
	unsigned char buffer;
	
	for(int i = 0; i < dataInBytes; i++, new_y_most + 2, new_y_less + 2)	//converting big-endian to little-endian for writing data in wav-file
	{
		buffer = *new_y_most;
		*new_y_most = *new_y_less;
		*new_y_less = buffer;
		
	}
	
	filteredWav_track = fopen("filtered.wav", "w");											/* creating wav file*/
	if(filteredWav_track == NULL)
	{
		printf("Error in creating a new filtered wav file/n");
		exit(1);
	}
	
	size_t write = fwrite(ptrWav_header, sizeof(wav_header), 1, filteredWav_track);			/* 	write header to wav file */
	if(write = 0)
	{
		printf("Error in writing header of wav file/n");
		exit(1);
		
	}
	
	write = fwrite(y, dataInBytes, 1, filteredWav_track);									/*	write data to wav file	*/
	if(write = 0)
	{
		printf("Error in writing audio data to wav file/n");
		exit(1);
	}
	
	return 0;
}


/*
*This function represents a FIR Band-Pass Filter
*Band-Pass Filter is used for highlighting the friquency band, which consist sibilant noices
*
*@param x - circular buffer
*@param h_BandPass - coefficients of Band-Pass filter
*@param *ptrBuf - pointer to the control elements of circular buffer
*@return yBandPass - output of the Band-Pass filter
*/
long double bandPassFilter(const long double &x, const long double &h_BandPass, CircBuf &ptrBuf, signed short *x_begin)
{
	long double yBandPass =0.0;											// output of filter
	signed short *x_start = x_begin;
	for(int i = 0; i < N; i++, x_start++)												// go throught the all input values 
	{	
			yBandPass = yBandPass + (*x_start * h_BandPass[i]);				//convolution
	}
	
	return yBandPass;
}

/*
*This function represents a FIR Band-Stop Filter
*Band-Stop Filter is used for cut the friquency band, which consist sibilant noises
*
*@param x - circular buffer
*@param h_BandStop - coefficients of Band-Stop filter
*@param *ptrBuf - pointer to the control elements of circular buffer
*@return yBandStop - output of the Band-Stop filter
*/
long double bandStopFilter(const long double &x, const long double &h_BandStop, CircBuf &ptrBuf, signed short *x_begin)
{
	long double yBandStop = 0.0;												// output of filter
	signed short *x_start = x_begin;
	for(int i = 0; i < N; i++)													// go throught the all input values
	{
		yBandStop = yBandStop + (*x_start * h_BandStop[i]);						//convolution
	}
	return yBandStop;
}

/*
*This function must decrease a dynamic range of the sibilant noise signal
*/
long double Compressor(const long double yBandPass, ...)
{
	long double yCompresed;
	/*compressor function, not completed*/
	return yCompresed;
}

/*
*This is function, which implement the de-esser effect
*
*@param input - current input value
*@return - expression, which consist output filtred signal
*/
signed short deEsser(const signed short &x, const signed short &h_BandPass, const signed short &h_BandStop, CircBuf &ptrBuf, signed short x_begin)
{
	long double deEssBandPass = bandPassFilter(x, h_BandPass, ptrBuf, x_begin);		/* seril cooected Band-Pass filter and Compressor*/
	long double deEssCompress = Compressor(deEssBandPass);
	
	long double deEssBandStop = bandStopFilter(x, h_BandStop, ptrBuf, x_begin);		//second branch
	
	return (signed short)(deEssCompress + deEssBandStop);								//output signal
}
