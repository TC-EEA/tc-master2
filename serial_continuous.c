// File name: serial_continous.c
// Author: Dmitri Roukin
// 	The original serial port file was taken from stackoverflow or similar
// Purpose: poll serial port every N milliseconds,
// 	at the end of time period wait until new line,
// 	write combined output to a ram file with time stamp (msec)
//
//
// Status: Beta 0 
// 	Should not be used in production yet - no boundary checks, ASCII input only
//
// Build command: w|!gcc -o serial_cont.a -lm && ./serial_cont.a
//
// Changelog:
// 	01-11-2017 Changed malloc to realloc (victim of previous cut-and-paste) 
// 	at the end of the cycle to prevent memory leak
// TODO: Run as a deamon



#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h> 			//needed for serial comms
#include <unistd.h>
#include <time.h>    			//to check time window

#define TIME_LIMIT 1050			//Approx time limit (ms) to read enough data
#define BUFFER_INC 1024			//Buffer increment size


// Taken from stackoverflow
int set_interface_attribs(int fd, int speed)
{
	struct termios tty;

	if (tcgetattr(fd, &tty) < 0) {
		printf("Error from tcgetattr: %s\n", strerror(errno));
		return -1;
	}

	cfsetospeed(&tty, (speed_t)speed);
	cfsetispeed(&tty, (speed_t)speed);

	tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;         /* 8-bit characters */
	tty.c_cflag &= ~PARENB;     /* no parity bit */
	tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
	tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

	/* setup for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	/* fetch bytes as they become available */
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		printf("Error from tcsetattr: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

//taken from stack overflow
void set_mincount(int fd, int mcount)
{
	struct termios tty;

	if (tcgetattr(fd, &tty) < 0) {
		printf("Error tcgetattr: %s\n", strerror(errno));
		return;
	}

	tty.c_cc[VMIN] = mcount ? 1 : 0;
	tty.c_cc[VTIME] = 5;        /* half second timer */

	if (tcsetattr(fd, TCSANOW, &tty) < 0)
		printf("Error tcsetattr: %s\n", strerror(errno));
}


int main()
{
	char *portname = "/dev/ttyUSB0";	//serial port
	int fd;					//file descriptor
	int rdlen = 0;				//serial read length
	char *mem_file = "/mnt/memory/memfile.txt";	//RAM file
	FILE *mem_file_fd;			//RAM file descriptor
	int i=0;				//iterator variable
	int n=0;
	unsigned char *buffer;			//serial buffer
	unsigned int buf_size = BUFFER_INC;	//initial buffer size
	unsigned int counter = 0;		//chacracter counter
	unsigned char c;			//read character
	struct timespec spec;			//time structure
	long prev_time, new_time;		//timestamps

	//open serial port
	fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		printf("Error opening %s: %s\n", portname, strerror(errno));
		return -1;
	}
	//set baud rate to 115200 bps
	set_interface_attribs(fd, B115200);
	//allocate initial buffer
	buffer = (unsigned char *)malloc(buf_size);
	//set the timestamps
	clock_gettime(CLOCK_REALTIME, &spec);
	// Convert nanoseconds to milliseconds
	prev_time = spec.tv_sec*1000 + round(spec.tv_nsec / 1.0e6); 
	new_time = prev_time;
	while(1){
		rdlen = read(fd, &c,1 );	//read char
		if(counter == buf_size){	//if buffer full - expand buffer
			buf_size += BUFFER_INC;
			buffer = (unsigned char *)realloc(buffer, buf_size);
		}
		buffer[counter] = c;		//update buffer
		counter++;			//increment counter
		clock_gettime(CLOCK_REALTIME, &spec);//get current time
		new_time = spec.tv_sec*1000 + round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
		if(c == '\n' && (new_time - prev_time) > TIME_LIMIT){
			//check if time is up and the character is newline
			if(mem_file_fd = fopen(mem_file,"w+")){//open RAM file for writing with truncation
				char timestamp[32];//reserve room for timestamp
				int size = sprintf(timestamp,"time %ld\n",new_time);
				//write timestamp to file
				fwrite(timestamp, 1, size, mem_file_fd);
				//frite buffer to file
				fwrite(buffer, 1 , counter, mem_file_fd);
				fclose(mem_file_fd);//close file
			}
			counter = 0;	//reset counter
			buf_size = BUFFER_INC;//rest buffer size
			//reallocate memory for buffer
			buffer = (unsigned char *)realloc(buffer,buf_size);
			prev_time = new_time;//reset previous time stamp
		}

	}
	close(fd);			//close serial port
}
