#include "asciiLetters.h"

int mem_fd;
void *gpio_map;
volatile unsigned *gpio;
// Pins connected to rows of the STD
static int row_pins[] = {12, 13, 19, 16, 26, 20, 21};
static int data_pin = 5;
static int clk_pin = 6;

// GPIO address
#define BCM2708_PERI_BASE	0x20000000
#define GPIO_BASE		(BCM2708_PERI_BASE + 0x200000)
#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

//Setup GPIO macros
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |= (1<<(((g)%10)*3))
#define GPIO_SET *(gpio+7)
#define GPIO_CLR *(gpio+10)

//Display dimensions
#define ROWS 7
#define COLS 95

struct std_string {
	char string[256]; 
	struct std_string *next;
};

//Scrolling text display settings
struct std_settings{
	int scrollSpeed;
	int colsBetweenLetters;
	int rowOnUs;
	struct std_string *stringsHead;
};

//Function declarations
void setupIo();
char* currentTime(char*);
char* parseMessage(char*);
void showBuffer(struct std_settings, bool**);
bool** createBuffer();
void clearBuffer(bool**);
void destroyBuffer(bool**);
int getStringCols(char*, int);
bool** textToBuffer(struct std_settings, bool**, char*, int);
void showText(struct std_settings, bool**, char*);
void scrollText(struct std_settings, bool**);
bool** stdStringToBuffer(struct std_settings, bool**, struct std_string*, int);
