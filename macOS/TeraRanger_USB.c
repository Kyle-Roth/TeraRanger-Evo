/******************************************************/
/* Uses POSIX serial port functions provided by macOs */
/* to communicate with the TeraRanger Evo             */
/******************************************************/


#include <fcntl.h> // open, close
#include <stdio.h> // perror
#include <stdlib.h> // malloc, realloc
#include <unistd.h> // fwrite, write, fread, read, close
#include <stdint.h> // uint#_t
#include <termios.h> // lots of the Serial Port Stuff
#include <sys/time.h>

//! CRC 8 lookup table
static const uint8_t CRC_8_TABLE[] = {
  0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31,
  0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
  0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9,
  0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
  0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1,
  0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
  0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe,
  0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
  0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16,
  0x03, 0x04, 0x0d, 0x0a, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
  0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80,
  0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
  0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8,
  0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
  0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10,
  0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
  0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f,
  0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
  0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7,
  0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
  0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef,
  0xfa, 0xfd, 0xf4, 0xf3
};



// calculate the CRC8 Checksum
uint8_t calcCRC8(const uint8_t *DataArray, const uint16_t Length)
{
	uint16_t i;
	uint8_t CRC;

	CRC = 0;
	for (i=0; i<Length; i++)
		CRC = CRC_8_TABLE[CRC ^ DataArray[i]];

	return CRC;
}

// Calculate time difference between two tiem structures
uint8_t* time_diff(struct timeval x , struct timeval y)
{
    double x_ms , y_ms;
    uint64_t* diff = malloc(sizeof(double));

    x_ms = (double)x.tv_sec*1000000 + (double)x.tv_usec;
    y_ms = (double)y.tv_sec*1000000 + (double)y.tv_usec;

    *diff = (double)y_ms - (double)x_ms;

    // printf("%llu,",*diff);
    return (uint8_t*) diff;
}

// Opens the specified serial port, sets it up for binary communication,
// configures its read timeouts, and sets its baud rate.
// Returns a non-negative file descriptor on success, or -1 on failure.
int openSerialPort(const char * device, uint32_t baud_rate)
{
	// Open Specified Port; R&W, No Controling Terminal, Syncronized I/O
  int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0)
  {
    perror(device);
    return -1;
  }

  // Flush away any bytes previously read or written.
  int result = tcflush(fd, TCIOFLUSH);
  if(result)
  {
    perror("tcflush failed");  // just a warning, not a fatal error
  }

  // Get the current configuration of the serial port.
  struct termios options;
  result = tcgetattr(fd, &options);
  if(result)
  {
    perror("tcgetattr failed");
    close(fd);
    return -1;
  }

  // Turn off any options that might interfere with our ability to send and
  // receive raw binary bytes.
  options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF | IGNBRK);
  options.c_oflag &= 0; // no remapping, no delays ~(ONLCR | OCRNL);
  options.c_lflag &= 0; // no signaling chars, no echo,
                        // no canonical processing ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

  // Set up timeouts: Calls to read() will return as soon as there is
  // at least one byte available or when 100 ms has passed.
  options.c_cc[VTIME] = 1;
  options.c_cc[VMIN] = 0;

  // This code only supports certain standard baud rates. Supporting
  // non-standard baud rates should be possible but takes more work.
  switch(baud_rate)
  {
  case 4800:   cfsetospeed(&options, B4800);
							 cfsetispeed(&options, B4800);		break;
  case 9600:   cfsetospeed(&options, B9600);
							 cfsetispeed(&options, B9600);		break;
  case 19200:  cfsetospeed(&options, B19200);
							 cfsetispeed(&options, B19200);		break;
  case 38400:  cfsetospeed(&options, B38400);
							 cfsetispeed(&options, B38400);		break;
  case 115200: cfsetospeed(&options, B115200);
							 cfsetispeed(&options, B115200);	break;
  default:
    fprintf(stderr, "warning: baud rate %u is not supported, using 9600.\n",
      baud_rate);
    cfsetospeed(&options, B9600);
    break;
  }
  cfsetispeed(&options, cfgetospeed(&options));

  result = tcsetattr(fd, TCSANOW, &options);
  if (result)
  {
    perror("tcsetattr failed");
    close(fd);
    return -1;
  }

  return fd;
}

// Writes bytes to the serial port, returning 0 on success and -1 on failure.
int writePort(int fd, uint8_t* buffer, size_t size)
{
  ssize_t result = write(fd, buffer, size);
  if (result != (ssize_t)size)
  {
    perror("failed to write to port");
    return -1;
  }
  return 0;
}

// Reads bytes from the serial port.
// Returns after all the desired bytes have been read, or if there is a
// timeout or other error.
// Returns the number of bytes successfully read into the buffer, or -1 if
// there was an error reading.
ssize_t readPort(int fd, uint8_t* buffer, size_t size)
{
  size_t received = 0;
  while (received < size)
  {
    ssize_t r = read(fd, buffer + received, size - received);
    if (r < 0)
    {
      perror("failed to read from port");
      return -1;
    }
    if (r == 0)
    {
      // Timeout
      break;
    }
    received += r;
  }
  return received;
}

// Initializes communication with TeraRanger Evo 60m
// Returns a non-negative file descriptor on success, or -1 on failure.
// mode = 0 for binary mode, mode = 1 for text mode
int initializeTeraRanger(const char* device, uint8_t mode)
{
  uint32_t baud = 115200;

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
	printf("Mode Chosen\n");

  int fd = openSerialPort(device, baud);
  if(fd < 0) return -1;
	printf("Port Opened\n");

  int ch = writePort(fd, buffer, 4);
  if(ch == -1)
  {
    perror("Could not initialize Output Mode");
    return -1;
  }
	printf("Mode Set\n");
  return fd;
}

int main()
{
  // Find proper device address
  // using 'ls -l /dev/tty.*' in terminal should help
  const char* device = "/dev/cu.usbmodem00000000001A1";

  uint8_t mode = 0;
  int fd = initializeTeraRanger(device, mode);
  if(fd == -1) return -1;

   /* initialize time calculations */
  struct timeval before,after;
  int Tfile = open("./Toutput.bin", O_WRONLY | O_CREAT | O_TRUNC);
  int Dfile = open("./Doutput.bin", O_WRONLY | O_CREAT | O_TRUNC);
  uint32_t* Toutput = malloc(sizeof(uint32_t));
  uint16_t* Doutput = malloc(sizeof(uint16_t));
  uint8_t* buffer = NULL;

  gettimeofday(&before , NULL);

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
    uint8_t* temp = NULL;
    uint32_t temp3 = 0;
    uint16_t temp2 = 0;
    uint8_t CRC = 0;

      /* collect 100,000 measurements*/
    for(uint32_t i= 0; i < 100000 ; i++)
    {
      readPort(fd, buffer, 4); // read 4 bytes
      gettimeofday(&after , NULL); // get current time

       /* check header */
      if(*(buffer) != 0x54)
      {
        printf("Header Mismatch\n");
        continue;
      }

       /* CRC Checksum */
      CRC = calcCRC8(buffer,3);
      if(CRC != *(buffer+3))
      {
        printf("Checksum Failed\n");
        continue;
      }

      temp = time_diff(before,after);
      // temp2 = (*(buffer+1) << 8) | *(buffer+2);  // build final number
      *Toutput = (*(temp+7) << 56) | (*(temp+6) << 48) | (*(temp+5) << 40) | (*(temp+4) << 32) | (*(temp+3) << 24) | (*(temp+2) << 16) | (*(temp+1) << 8) | (*(temp+0) << 0);  // build final number
      *Doutput = (*(buffer+1)<< 8) | *(buffer+2);


      // printf("%d\n",temp3);
      // printf("%02x%02x%02x%02x%02x%02x%02x%02x\n",*(temp+7),*(temp+6),*(temp+5),*(temp+4),*(temp+3),*(temp+2),*(temp+1),*(temp+0));
      // continue;

      // *(output+0) = *(temp+0);
      // *(output+1) = *(temp+1);
      // *(output+2) = *(temp+2);
      // *(output+3) = *(temp+3);
      // *(output+4) = *(buffer+1);
      // *(output+5) = *(buffer+2);
      write(Tfile, Toutput, 4);
      write(Dfile, Doutput, 2);
      // printf("%d,%d\n",*Toutput,*Doutput);

      if(i%1000 == 0)
        printf("%d\n",i);
        /* Print Measurement */
      // printf("%d,",(*(output+3) << 24) | (*(output+2) << 16) | (*(output+1) << 8) | (*(output+0)));
      // switch((*(buffer+1) << 8) | *(buffer+2))
      // {
      //   case 0: printf("-inf\n");
      //           break;
      //   case 1: printf("-1\n");
      //           break;
      //   case 0xFFFF:  printf("+inf\n");
      //                 break;
      //   default: printf("%d\n",(*(output+4) << 8) | (*(output+4)));
      // }
    }

    printf("%d,%d\n",*Toutput,*Doutput);
  }

  free(buffer);
  close(fd);
  close(Tfile);
  close(Dfile);
  return 0;
}
