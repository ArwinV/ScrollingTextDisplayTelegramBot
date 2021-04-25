#define _XOPEN_SOURCE
#define __USE_XOPEN
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include "std-display.h"
#include "std-bot.h"
#include <time.h>

static bool keepRunning = true;
pthread_t tid;

//Handle interrupts (like CTRL-C)
void intHandler(int dummy) {
	printf("Received stop signal\n");
	keepRunning = false;	
}

void *telebotThread(void *std_settings) {
	printf("Started telebot thread\n");
	//Read token from file
	char token[46];
	FILE *fptr;
	if ((fptr = fopen("token.txt", "r")) == NULL) {
		printf("Cannot read token from token.txt. Make sure token.txt is present in the same directory as the executable and contains your bot token");
		exit(1);
	}
	fscanf(fptr, "%[^\n]", token);
	fclose(fptr);
	//Create handle and bot
	telebot_handler_t telegramBot;
	if (telebot_create(&telegramBot, token) != TELEBOT_ERROR_NONE) {
		printf("Failed creating bot\n");
	}
	else {
		printf("Created Bot\n");
	}
	//Telegram update offset
	int offset = -1;
	//Create infinite loop that periodically checks of there are telegram messages
	while(keepRunning) {
		offset = getMessages(telegramBot, offset, (struct std_settings *) std_settings);
		//Wait a small time
		sleep(5);
	}
	//Destroy Telegram bot
	telebot_destroy(telegramBot);
	return NULL;
}

//Read quotes from file
void readQuotesFromFile(struct std_settings* s) {
	FILE *fp;
	struct std_string *head = NULL;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	fp = fopen("quotes.txt", "r");
	if (fp == NULL) {
		return;
	}
	while ((read = getline(&line, &len, fp)) != -1) {
		//Remove weird last character
		line[strlen(line)-1] = 0;
        	printf("Adding quote: %s", line);
		addStdString(s->stringsHead, line, s);
    	}

	fclose(fp);
	if (line) {
	    free(line);
	}
}

//Main function
int main(int argc, char **argv) {
	//Register interrupt handler
	struct sigaction act;
	act.sa_handler = intHandler;
	sigaction(SIGINT, &act, NULL);

	//Setup gpio pointer for direct register access
	setupIo();

	//Create struct that stores the scrolling text display settings	
	struct std_settings *std_settings = malloc(sizeof(struct std_settings));
	std_settings->scrollSpeed = 1;
	std_settings->colsBetweenLetters = 1;
	std_settings->rowOnUs = 2000;
	std_settings->stringsHead = NULL;
	
	//Variables that might later be changed
	unsigned long counter = 0;
	unsigned long smallCounter = 0;
	int err;

	//String that is to be displayed
	readQuotesFromFile(std_settings);;
	struct std_string *currentHead = std_settings->stringsHead;
	
	//Create the buffer that stores the bits (textBuffer[row][col])
	bool** textBuffer = createBuffer();
	//Create the telegram bot thread that checks for new messages
	err = pthread_create(&tid, NULL, telebotThread, (void *)std_settings);
	//Show the buffer
	while (keepRunning) {
		//Clear the buffer
		clearBuffer(textBuffer);
		if (currentHead != NULL) {
			//Test new function that uses the string struct
			stdStringToBuffer(*std_settings, textBuffer, currentHead, counter);	
			//Show the buffer
			showBuffer(*std_settings, textBuffer);
			
			smallCounter++;
			if (smallCounter == std_settings->scrollSpeed) {
				smallCounter = 0;
				counter++;
			}
			//Keep attention to the extra characters added by the time
			if (counter > getStringCols(currentHead->string, std_settings->colsBetweenLetters) + 14*(5+std_settings->colsBetweenLetters) - 1) {
				currentHead = currentHead->next;
				counter = 0;
			}
		}
		else {
			//Get time
			char* time = currentTime("%H:%M:%S");
			//Show time
			showText(*std_settings, textBuffer, time);
			free(time);
			//Set current head (not NULL when a message is received)
			currentHead = std_settings->stringsHead;
		}
	}
	//Clear contents in shift registers
	clearBuffer(textBuffer);
	showBuffer(*std_settings, textBuffer);
	//Destroy buffer
	destroyBuffer(textBuffer);
	//Free string
	free(std_settings);
	return 0;
}
