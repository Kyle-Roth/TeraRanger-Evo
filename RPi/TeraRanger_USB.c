/******************************************************/
/* Uses POSIX serial port functions provided by macOs */
/* to communicate with the TeraRanger Evo             */
/******************************************************/

//use gcc -std=c++11 -o <filename> <filename.cpp> -lwiringPi

#include <iostream>
#include <unistd.h>
#include <wiringSerial.h>
#include <wiringPi.h>

//! CRC 8 lookup table
static const uint8_t CRC_8_TABLE[256] = {
	  0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
	157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
	 35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
	190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
	 70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
	219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
	101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
	 17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
	 50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
	202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
	 87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
	116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
};

uint8_t calcCRC8(const uint8_t *DataArray, const uint16_t Length)
{
	uint16_t i;
	uint8_t CRC;

	CRC = 0;
	for (i=0; i<Length; i++)
		CRC = CRC_8_TABLE[CRC ^ DataArray[i]];

	return CRC;
}

// Initializes communication with TeraRanger Evo 60m
// Returns a non-negative file descriptor on success, or -1 on failure.
// mode = 0 for binary mode, mode = 1 for text mode
int initializeTeraRanger(const char* device, uint8_t mode)
{

  // set TeraRanger Write Mode, 4 bytes
  // in hex, 00 11 01 45 is text mode
  // in hex, 00 11 02 4C is binary mode
  uint8_t* buffer = malloc(sizeof(uint8_t) * 4);
  switch(mode)
  {
    default: printf("Mode %d Not Supported. Using Binary Mode\n",mode);
    case 0: *(buffer+0) = 0x00;
            *(buffer+1) = 0x11;
            *(buffer+2) = 0x02;
            *(buffer+3) = 0x4C;
            break;
    case 1: *(buffer+0) = 0x00;
            *(buffer+1) = 0x11;
            *(buffer+2) = 0x02;
            *(buffer+3) = 0x4C;
            break;
  }

  int fd  = serialOpen(device, 115200);
  if(fd < 0) return -1;

  int ch = writePort(fd, buffer, 4);
  if(ch == -1)
  {
    perror("Could not initialize Output Mode");
    return -1;
  }

  return fd;
}

int main()
{
  // Find proper device address
  // using 'ls -l /dev/tty.*' in terminal should help
  const char* device = "/dev/";
  uint8_t mode = 0;
  int fd = initializeTeraRanger(device, mode);
  if(fd == -1) return -1;

  uint8_t* buffer = NULL;
  if(mode == 1) // text mode
  {
    buffer = realloc(buffer, sizeof(uint8_t) * 7);  // clean and resize buffer

      /* collect 20,000 measurements*/
    for(uint32_t i= 0; i < 20000 ; i++)
    {
      readPort(fd, buffer, 7);
      printf("%s",(char*)buffer);
    }
  }
  else // binary mode
  {
    buffer = realloc(buffer, sizeof(uint8_t) * 4); // clean and resize buffer
    uint8_t CRC = 0;
    uint16_t temp = 0;

      /* collect 20,000 measurements*/
    for(uint32_t i= 0; i < 20000 ; i++)
    {
      readPort(fd, buffer, 4); // read 4 bytes

       /* check header */
      if(*(buffer) != 0x54)
      {
        perror("Header Mismatch");
        continue;
      }

       /* CRC Checksum */
      CRC = calcCRC8(buffer,3);
      if(CRC != *(buffer+3))
      {
        perror("checksum failed");
        continue;
      }

      temp = (*(buffer+2) << 4) + *(buffer+1);  // build final number

        /* Print Measurement */
      switch(temp)
      {
        case 0: printf("-inf\n");
                break;
        case 1: printf("-1\n");
                break;
        case 0xFFFF:  printf("+inf\n");
                      break;
        default: printf("%d\n",temp);
      }
    }
  }

  free(buffer);
	serialClose(fd);
  return 0;
}
