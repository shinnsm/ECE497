/* Stephen Shinn
 * Matt Moravec
 * 9/13/12
 * MiniProject 1 */


/* Code based on:
 * Copyright (c) 2011, RidgeRun
 * All rights reserved.
 * 
From https://www.ridgerun.com/developer/wiki/index.php/Gpio-int-test.c

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the RidgeRun.
 * 4. Neither the name of the RidgeRun nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY RIDGERUN ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL RIDGERUN BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>	// Defines signal-handling functions (i.e. trap Ctrl-C)
#include "i2c-dev.h"

 /****************************************************************
 * Constants
 ****************************************************************/
 
#undef DEBUG
#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64

/****************************************************************
 * Global variables
 ****************************************************************/
int keepgoing = 1;	// Set to 0 when ctrl-c is pressed

// Used to write the direction of a GPIO pin
// ledNumber = 0 => gpio48
// ledNumber = 1 => gpio49
// numToWrite = 0 => in
// numToWrite = 1 => out
void writeLedDirection(int ledNumber, int numToWrite) {
	FILE *file;

	if (ledNumber == 0)
		file = fopen("/sys/class/gpio/gpio48/direction","w+");
	else
		file = fopen("/sys/class/gpio/gpio49/direction","w+");

	if (numToWrite == 1)
		fprintf(file,"%s","out");
	else
		fprintf(file,"%s","in");

	fclose(file);
}

// Used to write an LED on or off
// ledNumber = 0 => gpio48
// ledNumber = 1 => gpio49
// numToWrite = 0 => off
// numToWrite = 1 => on
void writeToLed(int ledNumber, int numToWrite) {
	FILE *file;

	if (ledNumber == 0)
		file = fopen("/sys/class/gpio/gpio48/value","w+");
	else
		file = fopen("/sys/class/gpio/gpio49/value","w+");

	if (numToWrite == 0)
		fprintf(file,"%s","0");
	else
		fprintf(file,"%s","1");

	fclose(file);
}

// Takes a temperature in F and analog value (0 to 4096)
// and uses those values to change the output LED's.
// If the temp > 75, the multi-colored LED lights red.
// If the potentiometer is between 2000 and 3000, the LED
// turns blue. Otherwise, the multi-colored LED is off.
void processNums(int temp, int analog) {
	writeToLed(1,1);
	writeToLed(0,1);

	if (temp > 75) {
		writeToLed(0,1); // red
		writeLedDirection(0,1);
		writeToLed(1,0);
		writeLedDirection(1,0);
	}
	else if (analog > 2000 && analog < 3000) {
		writeToLed(1,1); // blue
		writeLedDirection(1,1);
		writeToLed(0,0);
		writeLedDirection(0,0);
	}
	else {
		writeToLed(0,0); // nothing
		writeLedDirection(0,0);
		writeToLed(1,0);
		writeLedDirection(1,0);
	}
}

/*** Read Potentiometer ***/
// Read potentiometer value from ain6 (range from 0 to 4096).
// Return value as int.
int readAnalog()
{
  FILE* file = fopen("/sys/devices/platform/tsc/ain6", "r");
  
  int num = 0;
  fscanf (file, "%d", &num);
  fclose (file);

  printf("Potentiometer num: %d", num);
  return num;
}

/*** Read Temp ***/
// Read temperature value from pins 19 (SCL) and 20 (SCA).
// Return the temp as an int in Fahrenheit.
int readTemp() {
	char *end;
	int res, i2cbus, address, size, file;
	int daddress;
	char filename[20];

	i2cbus   = 3;
	address  = 72;
	daddress = 0;
	size = I2C_SMBUS_BYTE;

	sprintf(filename, "/dev/i2c-%d", i2cbus);
	file = open(filename, O_RDWR);
	if (file<0) {
		if (errno == ENOENT) {
			fprintf(stderr, "Error: Could not open file "
				"/dev/i2c-%d: %s\n", i2cbus, strerror(ENOENT));
		} else {
			fprintf(stderr, "Error: Could not open file "
				"`%s': %s\n", filename, strerror(errno));
			if (errno == EACCES)
				fprintf(stderr, "Run as root?\n");
		}
		//exit(1);
	}

	if (ioctl(file, I2C_SLAVE, address) < 0) {
		fprintf(stderr,
			"Error: Could not set address to 0x%02x: %s\n",
			address, strerror(errno));
		//return -errno;
	}

	res = i2c_smbus_write_byte(file, daddress);
	if (res < 0) {
		fprintf(stderr, "Warning - write failed, filename=%s, daddress=%d\n",
			filename, daddress);
	}
	res = i2c_smbus_read_byte_data(file, daddress);
	close(file);

	if (res < 0) {
		fprintf(stderr, "Error: Read failed, res=%d\n", res);
		exit(2);
	}

	//printf("0x%02x (%d)\n", res, res);

	float fahr = (9.0/5.0)*res + 32;
	printf("\nTemp in fahrenheit: %.0f \n", fahr);

	return fahr;
}


/****************************************************************
 * signal_handler
 ****************************************************************/
// Callback called when SIGINT is sent to the process (Ctrl-C)
void signal_handler(int sig)
{
	printf( "\nCtrl-C pressed, cleaning up and exiting..\n" );
	keepgoing = 0;
}

/****************************************************************
 * gpio_export
 ****************************************************************/
int gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
 
	return 0;
}

/****************************************************************
 * gpio_unexport
 ****************************************************************/
int gpio_unexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_dir
 ****************************************************************/
int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
	int fd, len;
	char buf[MAX_BUF];
 
	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction");
		return fd;
	}
 
	if (out_flag)
		write(fd, "out", 4);
	else
		write(fd, "in", 3);
 
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_value
 ****************************************************************/
int gpio_set_value(unsigned int gpio, unsigned int value)
{
	int fd, len;
	char buf[MAX_BUF];
 
	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-value");
		return fd;
	}
 
	if (value)
		write(fd, "1", 2);
	else
		write(fd, "0", 2);
 
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_get_value
 ****************************************************************/
int gpio_get_value(unsigned int gpio, unsigned int *value)
{
	int fd, len;
	char buf[MAX_BUF];
	char ch;

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/get-value");
		return fd;
	}
 
	printf("%d chars read\n",read(fd, &ch, 1));

	if (ch != '0') {
		*value = 1;
	} else {
		*value = 0;
	}
 
	close(fd);
	return 0;
}


/****************************************************************
 * gpio_set_edge
 ****************************************************************/

int gpio_set_edge(unsigned int gpio, char *edge)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-edge");
		return fd;
	}
 
	write(fd, edge, strlen(edge) + 1); 
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_fd_open
 ****************************************************************/

int gpio_fd_open(unsigned int gpio, unsigned int dir)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, dir | O_NONBLOCK );

	if (fd < 0) {
		perror("gpio/fd_open");
	}
	return fd;
}

/****************************************************************
 * gpio_fd_close
 ****************************************************************/

int gpio_fd_close(int fd)
{
	return close(fd);
}

/****************************************************************
 * Main
 ****************************************************************/
int main(int argc, char **argv, char **envp)
{
	struct pollfd fdset[2];
	int nfds = 2;
	int gpio_fdIn, gpio_fdOut, timeout, rc;
	char *buf[MAX_BUF];
	unsigned int gpioIn, gpioOut;
	int len;
	int count = 0;		// Counts the number of interupts

	if (argc < 3) {
		printf("Usage: %s <gpio-input-pin> <gpio-output-pin>\n\n", argv[0]);
		printf("Waits for a change in the GPIO pin voltage level or input on stdin\n");
		exit(-1);
	}

	// Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);

	// Set up input
	gpioIn = atoi(argv[1]);
	gpio_export(gpioIn);
	gpio_set_dir(gpioIn, 0);
	gpio_set_edge(gpioIn, "both");
	gpio_fdIn = gpio_fd_open(gpioIn, O_RDONLY);

	// Set up output
	gpioOut=atoi(argv[2]);
	gpio_export(gpioOut);
	gpio_set_dir(gpioOut, 1);
	gpio_fdOut = gpio_fd_open(gpioOut, O_WRONLY);

	timeout = POLL_TIMEOUT;

	// initialize
	int temp = 0;
	int analog = 0;
 
	while (keepgoing) {
		memset((void*)fdset, 0, sizeof(fdset));

		fdset[0].fd = STDIN_FILENO;
		fdset[0].events = POLLIN;
      
		fdset[1].fd = gpio_fdIn;
		fdset[1].events = POLLPRI;

		rc = poll(fdset, nfds, timeout);      

		if (rc < 0) {
			printf("\npoll() failed!\n");
			return -1;
		}
      
		if (rc == 0) {
			printf(".");
		}
            
		if (fdset[1].revents & POLLPRI) {
			lseek(fdset[1].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[1].fd, buf, MAX_BUF);
			++count;
#ifdef DEBUG
			printf("\npoll() GPIO %d interrupt occurred %d times, value=%c, len=%d\n",
				 gpioIn, count, buf[0], len);
#endif
			len = write(gpio_fdOut, buf, len);
#ifdef DEBUG
			printf("Writing %d chars to GPIO %d\n", len, gpioOut);
#endif
			// Get the temperature and potentiometer values
			temp = readTemp();
			analog = readAnalog();

			// Call the function to manage the values
			processNums(temp, analog);
		}

		if (fdset[0].revents & POLLIN) {
			(void)read(fdset[0].fd, buf, 1);
			printf("\npoll() stdin read 0x%2.2X\n", (unsigned int) buf[0]);
		}

		fflush(stdout);
	}

	printf("%d interupts processed\n", count);
	gpio_fd_close(gpio_fdIn);
	gpio_fd_close(gpio_fdOut);
	return 0;
}

