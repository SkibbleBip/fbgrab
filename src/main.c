/***************************************************************************
Copyright (C) 2024  Skibblebip

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <inttypes.h>
#include <errno.h>
#include "main.h"

//............................................................................
int main(int argc, char *argv[]) {
	char* framebufferDevice = "/dev/fb";
	char* frame = "frame.bmp";
	int resolution = 1;
	uint8_t * framePtr;
	int fb, ffd;
	struct fb_var_screeninfo vinfo;


	int opt;
	struct option long_options[] = {
			{"input",    required_argument, 0, 'i'},
			{"output",   required_argument, 0, 'o'},
			{"resolution",required_argument, 0, 'r'},
			{0, 0, 0, 0} // Required for getopt_long to identify the end of options
	};

	while ((opt = getopt_long(argc, argv, "i:o:r:", long_options, NULL)) != -1) {
		//parse the options
		switch (opt) {
		case 'i':
			framebufferDevice = optarg;
			break;
		case 'o':
			frame = optarg;
			break;
		case 'r':
			resolution = atoi(optarg);
			if(resolution >= 1){
				break;
			}
			fprintf(stderr, "Invalid resolution division: %d\n", resolution);
			// fall through
		default:
			fprintf(stderr,
			"Grabs the current frame in the framebuffer and saves it as a .bmp file, with optional lossy compression for thumbnailing\n"
			"Usage: %s --input/-i <input_file> --output/-o <output_file> --resolution/-r <resolution>\n",
			argv[0]);
			return -1;
		}
	}





	if ((fb = open(framebufferDevice, O_RDWR)) < 0 ) {
		//open the framebuffer
		fprintf(stderr, "Error opening framebuffer device: %s\n", strerror(errno));
		return 1;
	}

	if (fcntl(fb, F_SETFL, fcntl(fb, F_GETFL) & ~O_NONBLOCK) == -1) {
		//set framebuffer to blocking mode, to garantee that it gets read in it's entirety
		fprintf(stderr, "Error setting framebuffer file descriptor to blocking mode: %s\n", strerror(errno));
		close(fb);
		return 1;
	}


	if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo)) {
		//get framebuffer properties
		fprintf(stderr, "Error reading framebuffer information: %s\n", strerror(errno));
		close(fb);
		return 1;
	}

	if(vinfo.bits_per_pixel % 8 != 0){
		//check if pixel format is correct
		fprintf(stderr, "Bits per pixel is non-byte divisible (%" PRIu32 ")\n", vinfo.bits_per_pixel);
		close(fb);
		return 1;
	}

	size_t frameSize = (vinfo.xres_virtual * vinfo.yres_virtual * (vinfo.bits_per_pixel / 8));
	framePtr = malloc(sizeof(uint8_t) * frameSize);
	//generate buffer to hold the frame buffer output


	if(0 > read(fb, framePtr, frameSize)){
		//read entire frame buffer
		fprintf(stderr, "Error reading framebuffer data: %s\n", strerror(errno));
		free(framePtr);
		close(fb);
		return 1;
	}

	close(fb);
	//close frame buffer, we are done with it
	int alphaOffset = (vinfo.red.length + vinfo.blue.length + vinfo.green.length);
	//for whatever the reason, some framebuffers dont declare an alpha channel but have the red green and blue channels
	//smaller than the actual bits per pixel, leading to the remaining bits to be used as an alpha channel _I GUESS_
	if(alphaOffset - vinfo.bits_per_pixel != 0){
		//if there is a transparency/alpha channel, then cycle through the entire thing and garantee that it's
		//100% visable
		for(size_t i = 0; i < frameSize; i += vinfo.bits_per_pixel / 8){
			if(framePtr[i + alphaOffset / 8] != 0xff){
				//force set the alpha channel (transparency) to max visibility
				framePtr[i + alphaOffset / 8] = 0xff;
			}
		}

	}

	if( (ffd = open(frame, O_WRONLY | O_CREAT, 0700)) < 0){
		//create output bmp file
		fprintf(stderr, "Error creating output file: %s\n", strerror(errno));
		free(framePtr);
		return 1;
	}

	uint8_t header[bmp_header_length];
	memcpy(header, bmp_header, bmp_header_length);
	//copy template header information
	uint32_t u4byte;
	int32_t s4byte;
	uint16_t bbp;

	size_t x = vinfo.xres_virtual;
	size_t y = vinfo.yres_virtual;

	u4byte = htole32(((x - x % resolution) / resolution) * ((y - y % resolution) / resolution)*(vinfo.bits_per_pixel / 8)  + bmp_header_length);
	memcpy(&header[0x02], &u4byte, sizeof(uint32_t));
	//copy to the header the total size of the file
	u4byte = htole32(vinfo.xres_virtual / resolution);
	memcpy(&header[0x12], &u4byte, sizeof(uint32_t));
	//copy to the header the expected x resolution
	s4byte = htole32(- (vinfo.yres_virtual/resolution));
	memcpy(&header[0x16], &s4byte, sizeof(uint32_t));
	//copy to the header the expected y resolution
	//(negative, as frame buffer writes top to bottom instead of the other way around)
	bbp = htole16(vinfo.bits_per_pixel);
	memcpy(&header[0x1C], &bbp, sizeof(uint16_t));
	//copy to the header the bits per pixel
	u4byte = htole32(((x - x % resolution) / resolution) * ((y - y % resolution) / resolution)*(vinfo.bits_per_pixel / 8) );
	memcpy(&header[0x22], &u4byte, sizeof(uint32_t));
	//copy to the header the size of the pixel array

	size_t ret = 0;
	do{
		int q = 0;
		if(0 > (q = write(ffd, &header[ret], bmp_header_length-ret))){
			//write the header to the output bmp file
			fprintf(stderr, "Error writing to bitmap file: %s\n", strerror(errno));
			close(ffd);
			free(framePtr);
			return 1;
		}
		ret += q;
	}while(ret < bmp_header_length);
	//in case we are interrupted, cycle back and continue writing



	if(resolution > 1){
		size_t buffSize = ((x - x % resolution) / resolution) * ((y - y % resolution) / resolution)*(vinfo.bits_per_pixel / 8);
		uint8_t *buff = malloc(sizeof(uint8_t) * buffSize);
		size_t buffIdx = 0;
		//calculate a new buffer size to hold the compressed lossy bitmap pixels
		//based on the resolution we gave it

		for(uint j = 0; j < y - y % resolution; j = j + resolution){
			for(uint i = 0; i < x - x % resolution; i = i + resolution){
				//cycle through each sampled pixel, apply it to the compressed buffer
				uint pixIdx = (j * x + i)*(vinfo.bits_per_pixel / 8);
				memcpy(&buff[buffIdx], &framePtr[pixIdx], (vinfo.bits_per_pixel / 8));
				buffIdx += vinfo.bits_per_pixel / 8;
			}

		}

		ret = 0;
		do{
			int q = 0;
			if( 0 > (q = write(ffd, &buff[ret], buffSize - ret))){
				//write the calculated lossy buffer in one go
				fprintf(stderr, "Error writing to bitmap file: %s\n", strerror(errno));
				close(ffd);
				free(framePtr);
				free(buff);
				return 1;
			}
			ret += q;
		}while(ret < buffSize);
		//if we were interrupted, cycle back and continue
		free(buff);

	}
	else{
		//if the resolution is just 1:1, then dump the entire frame into the .bmp file
		ret = 0;
		do{
			int q = 0;
			if(0 > (q = write(ffd, &framePtr[ret], frameSize-ret))){
				//write frame to output bmp file
				fprintf(stderr, "Error writing to bitmap file: %s\n", strerror(errno));
				close(ffd);
				free(framePtr);
				return 1;
			}
			ret += q;
		}while(ret < frameSize);
		//continue off where we left off, if we were interrupted.
	}

	fsync(ffd);
	//sync file

	free(framePtr);
	close(ffd);
	return 0;
	//clean up and exit
}

