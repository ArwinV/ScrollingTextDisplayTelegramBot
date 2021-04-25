#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <telebot.h>
#include <std-display.h>

//Add an std string
struct std_string *addStdString(struct std_string *head, char *string, struct std_settings* s) {
	//Allocate memory for new string	
	struct std_string *newStdString = malloc(sizeof(struct std_string));
	if (newStdString == NULL) {
		printf("Memory allocation while adding string failed!\n");
		return NULL;
	}
	//Copy new string into new string struct
	strcpy(newStdString->string, string);
	//Find last item in list
	if (head != NULL) {
		printf("Finding last string\n");
		struct std_string *currStr = head;
		while(currStr->next != head) {
			currStr = currStr->next;
		}
		currStr->next = newStdString;
		//Set next string to head because this new string is added to the end
		newStdString->next = head;
		return head;
	}
	else {
		printf("Adding first string to list\n");
		newStdString->next = newStdString;
		s->stringsHead = newStdString;
		return newStdString;
	}
}

void writeQuotesToFile(struct std_string *head) {
	FILE *fp;
	fp = fopen("quotes.txt", "w+");	
	if (fp == NULL) {
		return;
	}

	struct std_string *currStr = head;
	while(currStr->next != head) {
		fprintf(fp, "%s\n", currStr->string);
		currStr = currStr->next;
	}
	fprintf(fp, "%s\n", currStr->string);
	fclose(fp);
}

int nrOfQuotes(struct std_string *head) {
	int count = 2;
	struct std_string *currStr = head;
	if (head == NULL) {
		return 0;
	}
	while(currStr->next != head) {
		count++;
		currStr = currStr->next;
	}
	printf("There are %d quotes\n", count);
	return count;
}

//Get telegram bot messages
int getMessages(telebot_handler_t handle, int offset, struct std_settings* s) {
	int count  = -1;
	telebot_update_t *updates;
	telebot_message_t message;
	telebot_document_t *document;
	telebot_update_type_e update_types[] = {TELEBOT_UPDATE_TYPE_MESSAGE};
	telebot_error_e ret;
	ret = telebot_get_updates(handle, offset, 20, 0, update_types, 0, &updates, &count);
	
	//Return if no message is received
	if (ret != TELEBOT_ERROR_NONE) {
	 	printf("No new messages\n");
		return -1;
	}
	//Create a return string
	int nrQuotes = nrOfQuotes(s->stringsHead);
	char *returnString = malloc(256*nrQuotes+64);
	//Print received messages
	printf("New messages\n");
	for (int index = 0; index < count; index++) {
		if (updates[index].update_type == TELEBOT_UPDATE_TYPE_MESSAGE) {
			message = updates[index].message;
			//Received a file
			if (message.document != NULL) {
				printf("Received document\n");
				document = message.document;
				printf("Document file id: %s\n", document->file_id);
				//Download file
				//const char *file_id = document->file_id;
				//const char *download_path = "/home/pi/";
				//ret = telebot_download_file(handle, file_id, download_path);
				//if (ret != TELEBOT_ERROR_NONE) {
				//	printf("Failed downloading file\n");
				//}
			}
			//Received a text message
			else if (message.text != NULL) {
				printf("Received text message\n");
				printf("%s: %s\n", message.from->first_name, message.text);
				
				//if (strstr(message.text, "/newstring")) {
				//	char *newText = strchr(message.text, ' ');
				//	if (newText != NULL) {
				//		//Set new string
				//		printf("New quote(s): %s\n", newText+1);
				//		//strcpy(s->string, newText);
				//		s->stringsHead = addStdString(s->stringsHead, newText+1, s);
				//		//Return string
				//		strcpy(returnString, "New quote(s) received\n"); 
				//	}
				//	else {
				//		//User dit not type string after the command, ask for it now
				//		strcpy(returnString, "No string specified. Please use the following syntax /newstring <string>.\n"); 
				//	}
				//}
				if (strstr(message.text, "/shutdown")) {
					//Get messages an extra time to let telegram know we got the message
					offset = updates[index].update_id + 1;
					ret = telebot_get_updates(handle, offset, 20, 0, NULL, 0, &updates, &count);
					//Sync and shutdown system
					sync();
					system("nohup  bash -c 'sleep 10; shutdown now' > /tmp/shutdown.log&");
					strcpy(returnString, "Shutting down the display.\n");
					return -2;
				}
				else if (strstr(message.text, "/scrollspeed")) {
					if (strlen(message.text) == 14 && message.text[13] >= '1' && message.text[13] <= '9') {
						char newSpeedChar = message.text[13];
						int scrollSpeed = newSpeedChar - 48;
						s->scrollSpeed = scrollSpeed;
						strcpy(returnString, "Scrolling speed is changed\n");
					}
					else {
						strcpy(returnString, "Wrong scrollspeed setting, expected \"/scrollspeed <1-9>\" where 1 is fast and 9 is slow\n");
					}
				}
				else if (strstr(message.text, "/rowontime")) {
					char *newRowOnTimeString = strchr(message.text, ' ');
					long newRowOnTime = strtol(newRowOnTimeString, NULL, 10);
					s->rowOnUs = newRowOnTime;
					strcpy(returnString, "Time every row is on is changed.\n");
				}
				else if (strcmp(message.text, "/getquotes") == 0) {
					printf("Getquotes received\n");

					struct std_string* head = s->stringsHead;
					struct std_string* curr = head;
					int quotenr = 0;
					char tempstring[256];
					if (head == NULL) {
						strcpy(returnString, "No quotes added yet");
					}
					else {
						//Add all quotes to reply
						strcpy(returnString, "ID: Quote\n---------------\n");
						while (curr->next != head) {
							sprintf(tempstring, "%d: %s\n", quotenr, curr->string);
							strcat(returnString, tempstring);
							curr = curr->next;	
							quotenr++;
						}
						sprintf(tempstring, "%d: %s\n", quotenr, curr->string);
						strcat(returnString, tempstring);
					}
				}
				else if (strstr(message.text, "/deletequote")) {
					printf("Deleting quote\n");
					char *idString = strchr(message.text, ' ');
					if (idString != NULL) {
						long id = strtol(idString, NULL, 10);
						struct std_string* head = s->stringsHead;
						struct std_string* curr = head;
						if (head == NULL) {
							strcpy(returnString, "No quotes added yet");
						}
						else {
							if (id == 0) {
								while (curr->next != head) {
									curr = curr->next;
								}
							}
							else {
								for (int i = 0; i < id-1; i++) {
									curr = curr->next;
								}
							}
							free(curr->next);
							curr->next = curr->next->next;
							strcpy(returnString, "Quote removed");
						}
						//Write quotes to file
						writeQuotesToFile(s->stringsHead);
					}
					else {
						strcpy(returnString, "Use the command as /removequote [ID]");
					}

				}
				else {
					//New quote
					const char delim[2] = "\n";
					char *quote;
					quote = strtok(message.text, delim);
					while (quote != NULL) {
						printf("Adding quote: %s\n", quote);
						addStdString(s->stringsHead, quote, s);
						quote = strtok(NULL, delim);
					}
					//Write quotes to file
					writeQuotesToFile(s->stringsHead);
					//Return string
					strcpy(returnString, "New quote(s) received\n"); 
				}
				
				//Send return message to user
				printf(returnString);
				ret = telebot_send_message(handle, message.chat->id, returnString, "", false, false, 0, "");
				if (ret != TELEBOT_ERROR_NONE) {
					printf("Failed to send message\n");
				}
				
			}
		}
		offset = updates[index].update_id + 1;
	}
	telebot_put_updates(updates, count);
	return offset;
}
