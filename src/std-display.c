#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "std-display.h"

//Setup io
void setupIo() {
	//Open /dev/mem
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0) {
		printf("Can't open /dev/mem \n");
		exit(-1);
	}

	//Mmap gpio
	gpio_map = mmap(
			NULL,
			BLOCK_SIZE,
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			mem_fd,
			GPIO_BASE
		       );
	//Mem_fd is not needed after mmap
	close(mem_fd);

	if (gpio_map == MAP_FAILED) {
		printf("mmap error %d\n", (int)gpio_map);
		exit(-1);
	}

	//Use volatile pointer for gpio
	gpio = (volatile unsigned *)gpio_map;

	//Set gpio pins as outputs
	for (int pin = 0; pin < 7; pin++) {
		INP_GPIO(row_pins[pin]);
		OUT_GPIO(row_pins[pin]);
	}
	OUT_GPIO(clk_pin);
	INP_GPIO(data_pin);
	OUT_GPIO(data_pin);
}

//Returns a string with the current time
char* currentTime(char* format) {
	struct tm * timeinfo;
	struct timeval tv;
	char* timeString = malloc(32*sizeof(char));
	//Get time
	gettimeofday(&tv, NULL);
	timeinfo = localtime(&tv.tv_sec);
	//Format string
	strftime(timeString, 32, format, timeinfo);
	return timeString;
}

/*
//Change special words starting with $ in string to their value
char* parseMessage(char* message) {
	//Create return string (and allocate memory)
	char* tempString = malloc(4096*sizeof(char));
	//char tempString[4096];
	char* word;
	if (tempString == NULL) {
		printf("Malloc while parsing string failed\n");
		return "";
	}
	//Make empty string
	tempString[0] = '\0';
	//Copy received message into local variable
	char tempMessage[4096];
	strcpy(tempMessage, message);
	//Cut string into words
	word = strtok(tempMessage, " ");
	while (word != NULL) {
		//printf("%s\n", word);
		if (word[0] != '$') {
			strcat(tempString, word);	
		}
		//Special word in string
		else {
			//Time variable
			if (strcmp(&word[1], "time") == 0) {
				//printf("Adding time to string\n");
				char* time = currentTime("%H:%M:%S");
				//printf("Current time: %s\n", time);
				strcat(tempString, time);
				//And free the returned time string
				free(time);
			}
			//Date variable
			else if (strcmp(&word[1], "date") == 0) {
				//printf("Adding date to string\n");
				char* date = currentTime("%Y-%m-%d");
				//printf("Current date: %s\n", date);
				strcat(tempString, date);
				//And free the returned date string
				free(date);
			}
			//Unknown variable
			else {
				//printf("Unknown command, doing nothing.\n");
			}
		}
		//Add space after each word
		strcat(tempString, " ");
		//Get next word
		word = strtok(NULL, " ");
	}
	return tempString;
}
*/

//Show contents of buffer
void showBuffer(struct std_settings s, bool** buffer) {
	for (int row = 0; row < ROWS; row++) {
		for (int col = COLS-1; col >= 0; col--) {
			//Push all bits for column (set bit, clock high, wait, clock low)
			if (buffer[ROWS-row-1][col]) {
				GPIO_SET = 1 << data_pin;
			}
			else {
				GPIO_CLR = 1 << data_pin;
			}
			GPIO_SET = 1 << clk_pin;
			GPIO_SET = 1 << clk_pin;
			//usleep(1); //Not needed, the shift register can han handle the maximum speed of the raspberry pi
			//GPIO_CLR = 1 << clk_pin;
			GPIO_CLR = 1 << clk_pin;
		}
		//Turn row on for a brief moment
		GPIO_SET = 1 << row_pins[row];
		usleep(s.rowOnUs);
		GPIO_CLR = 1 << row_pins[row];
	}
}

//Create the buffer
bool** createBuffer() {
	//Get (the start of) a free memory block
	bool* pixels = calloc(COLS*ROWS, sizeof(bool));
	//Create block for pointers to the start of each row
	bool** buffer = malloc(ROWS*sizeof(bool*));
	//Initialize the buffer array with memory addresses of each row
	for (int i=0; i < ROWS; ++i) {
		buffer[i] = pixels + i*COLS;
	}
	return buffer;
}

//Clear the buffer (set all values to 0)
void clearBuffer(bool** buffer) {
	//Set buffer to zero blocks
	for (int i=0; i < ROWS; ++i) {
		memset(buffer[i], 0, COLS);
	}
}

//Destroy the buffer
void destroyBuffer(bool** buffer) {
	free(*buffer);
	free(buffer);
}

//Get total amount of columns of a string
int getStringCols(char* string, int emptyCols) {
	//This is easy as long as I keep the font monospaced
	//Every character is 5 columns wide
	//The time added after every string is (5+emptycols)*14
	const int timeLength = 14;
	//return (strlen(string)+timeLength)*(5+emptyCols);
	return (strlen(string))*(5+emptyCols);
}

//String to buffer
bool** textToBuffer(struct std_settings s, bool** buffer, char* string, int startCol) {
	//Keep track of total column place
	int col = 0;
	//Loop over each character
	for (int c = 0; c < strlen(string); c++) {
		if (string[c] >= 32 && string[c] <=127) {
			//Loop over all columns in each character
			for (int charCol = 0; charCol < sizeof(letters[c-32][0]); charCol++) {
				//Only print after startcolumn has been reached
				if (col >= startCol) {
					//Then loop over all rows in a character
					for (int row = 0; row < ROWS; row++) {
						//printf("Doing character %d, column %d and row %d, value is %d\n", string[c], charCol, row, letters[c-32][row][charCol]);
						if (col >= COLS + startCol) break;
						buffer[row][col-startCol] = letters[string[c]-32][row][charCol];
					}
				}
				col++;
			}
			col += s.colsBetweenLetters;
		}
	}
	return buffer;
}

//Show text on the middle of the display
void showText(struct std_settings s, bool** buffer, char* string) {
	//Determine where to begin the text
	int stringLeft = -(COLS+1)/2 + getStringCols(string, s.colsBetweenLetters)/2;
	//Clear the buffer
	clearBuffer(buffer);
	//Copy text into the buffer
	textToBuffer(s, buffer, string, stringLeft);
	//Show the buffer
	showBuffer(s, buffer);
}

/*
//Scroll a text over the display and return
void scrollText(struct std_settings s, bool** buffer) {
	//Variables needed for shift
	int shift = -COLS; //Let the text start at the right of the screen
	unsigned long counter = 0;
	char* parsedMessage = parseMessage(s.string);
	while (shift < getStringCols(parsedMessage, s.colsBetweenLetters)) {
		//And free the parsed message again
		free(parsedMessage);
		//First clear the buffer
		clearBuffer(buffer);
		//Parse message
		parsedMessage = parseMessage(s.string);
		//Copy text into the buffer
		textToBuffer(s, buffer, parsedMessage, shift);
		//Show the buffer
		showBuffer(s, buffer);
		//Set shift
		counter++;
		shift = -COLS + 2 * counter / s.scrollSpeed;
	}
	free(parsedMessage);
  }
*/

//Generate one string from multiple std_strings
bool **stdStringToBuffer(struct std_settings s, bool** buffer, struct std_string *string, int shift) {
	int offset = 0;
	int currentChar;
	int charCol;
	char stringWithTime[1024];
	stringWithTime[0] = '\0';
	//Add string if head is not NULL
	if (string != NULL) {
		strcat(stringWithTime, string->string);	
	}
	//Add time to string
	char* time = currentTime(" | %H:%M:%S | ");
	strcat(stringWithTime, time);
	free(time);
	//Loop over all columns in the buffer
	for (int col = shift; col < COLS+shift; col++) {
		//Determine current character and column to be copied into the buffer
		currentChar = col / (s.colsBetweenLetters + 5);
		charCol = col % (s.colsBetweenLetters + 5);
		//Copy all rows of the column if the column is not bigger than 5
		if (charCol < 5) {
			for (int row = 0; row < ROWS; row++) {
				buffer[row][col-shift] = letters[stringWithTime[currentChar-offset]-32][row][charCol];
			}
		}
		//If all columns of the string are copied, go to the next string
		if (stringWithTime[currentChar-offset+1] == 0 && charCol >= 4) {
			if (string != NULL) {
				//Next string
				string = string->next;
				//Add time
				stringWithTime[0] = '\0';
				strcat(stringWithTime, string->string);	
				char* time = currentTime(" | %H:%M:%S | ");
				strcat(stringWithTime, time);
				free(time);
			}
			//Set offset
			offset += (currentChar-offset)+1;
			col = col + s.colsBetweenLetters;
		}
	}
	return buffer;
}
