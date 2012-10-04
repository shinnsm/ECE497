/* Stephen Shinn
 * 9/25/12
 * MiniProject 2: Force Sensitive Resistor example

Input: Force-sensitive resistor on Pin 36 (analog input)
Output: LED on Pin 12 (GPIO)
Result: LED lights-up when a strong force is placed on the resistor.

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SYSFS_GPIO_DIR "/sys/class/gpio"

// Read analog value and print to console
int readAnalog() {
	FILE* file = fopen("/sys/devices/platform/omap/tsc/ain6", "r");
	int num = 0;
	fscanf (file, "%d", &num);
	fclose (file);
	printf("\rAnalog num: %d    ", num);
	fflush(stdout);
	return num;
}

// Light the LED if the analog value is low (ie, strong force)
void lightLedIfStrong(analogValue) {
	FILE *file;
	file = fopen("/sys/class/gpio/gpio60/value","w+");
	
	if (analogValue < 100) {
		fprintf(file, "1");
	} else {
		fprintf(file, "0");
	}

	fclose(file);
}

// Main
void main(int argc, char **argv, char **envp) {
	// Export gpio60
	FILE *fp;
	char set_value[5];
	fp = fopen(SYSFS_GPIO_DIR "/export", "ab");
	rewind(fp);
	strcpy(set_value, "60");
	fwrite(&set_value, sizeof(char), 2, fp);
	fclose(fp);

	// Set gpio60 direction
	FILE *file;
	file = fopen("/sys/class/gpio/gpio60/direction","w+");
	fprintf(file,"%s","out");
	fclose(file);
	
	// Keep going; CTRL+C to exit
	while (1) {
		int analogValue = readAnalog();

		// Do anything based on analog value
		lightLedIfStrong(analogValue);

		sleep(0.5);
	}
}

