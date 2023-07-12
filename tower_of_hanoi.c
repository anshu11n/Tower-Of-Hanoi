#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

// A linked list node
struct Node
{
	int diskNumber;
	int xpos;
	int size;
	short int colour;
	struct Node *next;
};

// this structure represents the arrow
struct Arrow
{
	int location;
	int towerTop;  // this is to see what disk is at the top of a node
	bool selected; // this tells us if the arrow is currently holding a disk or not
	int selectedDiskNumber;
};

/* Screen size  and dimensions*/
#define RESOLUTION_X 320
#define RESOLUTION_Y 240
#define POLE_LENGTH 80
#define X_OFFSET 20
#define POLE_BASE_Y 220
#define POLE_TOP_Y 120
#define POLE_THICK 5
#define DISK_THICK 14
#define ARROW_TOP_Y 10
#define ARROW_BOTTOM_Y 30
#define ARROW_RECT_THICK 5
#define ARROW_TRI_THICK 8
#define SUSPENSION_TOP_Y 70

/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00
#define BLACK 0x0000

/* Directions for the keys */
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define RKEY 5

// PROTOTYPES
void append(struct Node **head_ref, int new_disk);
void deleteTop(struct Node **head_ref);
int CheckTowerSize(struct Node **head_ref);
void updateArrowInfo();
void printArrowInfo();
void pickUpDisk();
// void wait_for_vsync();
void drawTowers(struct Node *level1, struct Node *level2, struct Node *level3);
void drawPoles();
void plot_pixel(int x, int y, short int line_color);
void clear_screen(int x1, int x2, int y1, int y2);
void drawLine(int x0, int y0, int x1, int y1, short int colour);
void drawArrow(struct Arrow theArrow);
void drawSuspendedDisk();
void dropDisk();
void clearDisk(struct Node *temp, int countUpLoc);
void getKey();
void clearArrows();
bool checkForWin();
void errorMessage();
void eraseErrorMessage();
void write_char(int x, int y, char c);
void drawIntro();
void winMessage();
void drawWinScreen();
void Delay();


// GLOBALS
int numOfDisks = 3;
int globalArrowLocation = 1; // the game starts with the arrow on Tower1
int fakeCount = 0;
int initialBlankCount = 0;
int minCount = 0;
int currCount = 0;
bool gameReset = false;
bool gameWon = false;
int reset = 1;
int c = 0;

// declaring BASES
volatile int pixel_buffer_start;
volatile int MPCORE_PRIV_TIMER = 0xFFFEC600;

/* Start with the empty list/Tower */
struct Node *Tower1 = NULL;
struct Node *Tower2 = NULL;
struct Node *Tower3 = NULL;
struct Arrow theArrow;

// information for drawing disks
int xposArray[5] = {25, 20, 15, 10, 5};
int sizeArray[5] = {30, 40, 50, 60, 70};
short int colourArray[5] = {YELLOW, ORANGE, GREEN, BLUE, RED};

// helper function for drawLine
#define ABS(x) (((x) > 0) ? (x) : -(x))

int main()
{

	volatile int *pixel_ctrl_ptr = (int *)0xFF203020;

	/* set front pixel buffer to start of FPGA On-chip memory */
	*(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the
										// back buffer
	/* now, swap the front/back buffers, to set the front buffer location */
	//wait_for_vsync();
	/* initialize a pointer to the pixel buffer, used by drawing functions */
	pixel_buffer_start = *pixel_ctrl_ptr;
	clear_screen(0, RESOLUTION_X, 0, RESOLUTION_Y); // pixel_buffer_start points to the pixel buffer
	/* set back pixel buffer to start of SDRAM memory */
	*(pixel_ctrl_ptr + 1) = 0xC0000000;

	pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer

	volatile int *KEY_ptr = (int *)0xFF200050;	// keys
	volatile int *LEDR_ptr = (int *)0xFF200000; // red LED address
	volatile int *SW_ptr = (int *)0xFF200040;	// SW slide switch address
	while ((*SW_ptr) != 32)
	{
		drawIntro();
	}

	eraseErrorMessage();

	//wait_for_vsync();
	pixel_buffer_start = *(pixel_ctrl_ptr + 1);

	int value;

	while (reset)
	{
		value = *(SW_ptr);	 // read the SW slider switch values
		*(LEDR_ptr) = value; // light up the red LEDs
		if (value == 1)
		{
			numOfDisks = 3;
			reset = 0;
		}
		else if (value == 2)
		{
			numOfDisks = 4;
			reset = 0;
		}
		else if (value == 4)
		{
			numOfDisks = 5;
			reset = 0;
		}
	}

	drawPoles();
	drawTowers(Tower1, Tower2, Tower3);
	drawArrow(theArrow);
	minCount = (pow(2, numOfDisks)) - 1;
	theArrow.location = 1;	   // intially, the arrow would point to the first tower
	theArrow.selected = false; // nothing would be selected
	theArrow.towerTop = 1;	   // disk 1 would always be at the top of a tower intially.

	// intialize the game for the number of disks and place all at the top of the third tower
	for (int i = numOfDisks; i != 0; i--)
	{
		append(&Tower1, i); // the game starts with all the disks at Tower1
	}

	while (1)
	{
		*(pixel_ctrl_ptr + 1) = 0xC8000000;
		// LATER: can erase what you draw here, call clearscreen before to test.
		// getKey();

		if (initialBlankCount == 0)
		{
			clear_screen(0, RESOLUTION_X, 0, RESOLUTION_Y);

			initialBlankCount++;
		}
		clear_screen(0, RESOLUTION_X, SUSPENSION_TOP_Y + 1, SUSPENSION_TOP_Y + DISK_THICK + 1); // for suspension zone
		clearArrows();
		// draw
		drawPoles(); // if we are not gonna clear_screen() above, can move this out of while
		drawTowers(Tower1, Tower2, Tower3);
		drawArrow(theArrow);
		drawSuspendedDisk();
		
		gameWon = checkForWin();
		if (gameWon)
		{
			drawWinScreen();
		}
		
		getKey();
		Delay();
		// wait_for_vsync();							// swap front and back buffers on VGA vertical sync
		pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
		
	}

	return 0;
}

/* Given a reference (pointer to pointer) to the bottom of the tower
of the disk number to be placed at the top */
void append(struct Node **head_ref, int new_disk)
{
	if (new_disk == 0)
	{
		return;
	}
	/* 1. allocate node */
	struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));

	struct Node *last = *head_ref; /* used in step 5*/

	/* 2. put in the disk */
	new_node->diskNumber = new_disk;
	new_node->xpos = xposArray[new_disk - 1];
	new_node->size = sizeArray[new_disk - 1];
	new_node->colour = colourArray[new_disk - 1];

	/* 3. This new node is going to be the last node, so make next of
		it as NULL*/
	new_node->next = NULL;

	/* 4. If the Linked List is empty, then make the new node as head */
	if (*head_ref == NULL)
	{
		*head_ref = new_node;
		return;
	}

	/* 5. Else traverse till the last node */
	while (last->next != NULL)
		last = last->next;

	/* 6. Change the next of last node */
	last->next = new_node;
	return;
}


void updateArrowInfo()
{
	if (globalArrowLocation == 1)
	{
		theArrow.location = 1;
		struct Node *temp = Tower1;

		if (temp == NULL)
		{
			theArrow.towerTop = 0;
			return;
		}

		while (temp != NULL)
		{
			theArrow.towerTop = temp->diskNumber;
			temp = temp->next;
		}
		return;
	}

	if (globalArrowLocation == 2)
	{
		theArrow.location = 2;
		struct Node *temp = Tower2;

		if (temp == NULL)
		{
			theArrow.towerTop = 0;
			return;
		}

		while (temp != NULL)
		{
			theArrow.towerTop = temp->diskNumber;
			temp = temp->next;
		}
		return;
	}

	if (globalArrowLocation == 3)
	{
		theArrow.location = 3;
		struct Node *temp = Tower3;

		if (temp == NULL)
		{
			theArrow.towerTop = 0;
			return;
		}

		while (temp != NULL)
		{
			theArrow.towerTop = temp->diskNumber;
			temp = temp->next;
		}
		return;
	}
}

/* Given a reference (pointer to pointer) to the head of a
   list , disk at the top of the tower */
void deleteTop(struct Node **head_ref)
{
	// Store head node
	struct Node *temp = *head_ref, *prev;
	int countUpLoc = 0;

	if (*head_ref == NULL)
	{
		printf("No disks on this tower\n");
		return;
	}

	// If tower has only one disk at the top
	if (temp->next == NULL)
	{
		theArrow.selectedDiskNumber = temp->diskNumber;
		clearDisk(temp, countUpLoc);
		free(temp);
		*head_ref = NULL;
		return;
	}

	// get to the end of linked list (a.k.a. the top of the tower)
	while (temp->next != NULL)
	{
		countUpLoc++;
		prev = temp;
		temp = temp->next;
	}
	// Unlink the node from linked list (remove the top disk in the tower)
	prev->next = temp->next;

	// remember the disk that you are moving
	theArrow.selectedDiskNumber = temp->diskNumber;

	clearDisk(temp, countUpLoc);

	free(temp); // Free memory
}

void clearDisk(struct Node *temp, int countUpLoc)
{
	int xoffBlack, yoffBlack;

	if (theArrow.location == 1)
	{
		for (int i = 1; i <= DISK_THICK; i++)
		{
			xoffBlack = temp->xpos;
			yoffBlack = countUpLoc * DISK_THICK;
			drawLine(X_OFFSET + xoffBlack, POLE_BASE_Y - yoffBlack - i, X_OFFSET + POLE_LENGTH / 2 - POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, BLACK);
			drawLine(X_OFFSET + POLE_LENGTH / 2 + POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, X_OFFSET + POLE_LENGTH - xoffBlack, POLE_BASE_Y - yoffBlack - i, BLACK);
			drawLine(X_OFFSET + POLE_LENGTH / 2 - POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, X_OFFSET + POLE_LENGTH / 2 + POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, WHITE);
		}
	}

	if (theArrow.location == 2)
	{
		for (int i = 1; i <= DISK_THICK; i++)
		{
			xoffBlack = temp->xpos;
			yoffBlack = countUpLoc * DISK_THICK;
			drawLine(POLE_LENGTH + 2 * X_OFFSET + xoffBlack, POLE_BASE_Y - yoffBlack - i, POLE_LENGTH + POLE_LENGTH / 2 + 2 * X_OFFSET - POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, BLACK);
			drawLine(POLE_LENGTH + POLE_LENGTH / 2 + 2 * X_OFFSET + POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, 2 * POLE_LENGTH + 2 * X_OFFSET - xoffBlack, POLE_BASE_Y - yoffBlack - i, BLACK);
			drawLine(POLE_LENGTH + POLE_LENGTH / 2 + 2 * X_OFFSET - POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, POLE_LENGTH + POLE_LENGTH / 2 + 2 * X_OFFSET + POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, WHITE);
		}
	}

	if (theArrow.location == 3)
	{
		for (int i = 1; i <= DISK_THICK; i++)
		{
			xoffBlack = temp->xpos;
			yoffBlack = countUpLoc * DISK_THICK;
			drawLine(2 * POLE_LENGTH + 3 * X_OFFSET + xoffBlack, POLE_BASE_Y - yoffBlack - i, 2 * POLE_LENGTH + POLE_LENGTH / 2 + 3 * X_OFFSET - POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, BLACK);
			drawLine(2 * POLE_LENGTH + POLE_LENGTH / 2 + 3 * X_OFFSET + POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, 3 * POLE_LENGTH + 3 * X_OFFSET - xoffBlack, POLE_BASE_Y - yoffBlack - i, BLACK);
			drawLine(2 * POLE_LENGTH + POLE_LENGTH / 2 + 3 * X_OFFSET - POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, 2 * POLE_LENGTH + POLE_LENGTH / 2 + 3 * X_OFFSET + POLE_THICK / 2, POLE_BASE_Y - yoffBlack - i, WHITE);
		}
	}
}

int checkTowerSize(struct Node **head_ref)
{
	// Store head node
	struct Node *temp = *head_ref, *prev;

	if (*head_ref == NULL)
	{
		return 0;
	}

	// If tower has only one disk at the top
	if (temp->next == NULL)
	{
		return 1;
	}

	int size = 1;
	// get to the end of linked list (a.k.a. the top of the tower)
	while (temp->next != NULL)
	{
		temp = temp->next;
		size++;
	}
	return size;
}

void drawPoles()
{
	// drawing a horizontal base for each pole
	for (int i = 0; i < POLE_THICK; i++)
	{
		drawLine(X_OFFSET, POLE_BASE_Y + i, POLE_LENGTH + X_OFFSET, POLE_BASE_Y + i, WHITE);
		drawLine(POLE_LENGTH + 2 * X_OFFSET, POLE_BASE_Y + i, 2 * POLE_LENGTH + 2 * X_OFFSET, POLE_BASE_Y + i, WHITE);
		drawLine(2 * POLE_LENGTH + 3 * X_OFFSET, POLE_BASE_Y + i, 3 * POLE_LENGTH + 3 * X_OFFSET, POLE_BASE_Y + i, WHITE);
	}

	// drawing three vertical poles
	for (int i = -POLE_THICK / 2; i < POLE_THICK / 2; i++)
	{
		drawLine(X_OFFSET + POLE_LENGTH / 2 + i, POLE_TOP_Y, X_OFFSET + POLE_LENGTH / 2 + i, POLE_BASE_Y, WHITE);
		drawLine(2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 + i, POLE_TOP_Y, 2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 + i, POLE_BASE_Y, WHITE);
		drawLine(3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 + i, POLE_TOP_Y, 3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 + i, POLE_BASE_Y, WHITE);
	}
}

void drawTowers(struct Node *level1, struct Node *level2, struct Node *level3)
{

	int xoff1, yoff1, xoff2, yoff2, xoff3, yoff3;
	int count1 = 0, count2 = 0, count3 = 0;

	// tower1
	if (level1 != NULL)
	{ // prevents crashes when empty list for Tower1
		while (level1->next != NULL)
		{
			xoff1 = level1->xpos;
			yoff1 = count1 * DISK_THICK;
			for (int i = 1; i <= DISK_THICK; i++)
			{
				drawLine(X_OFFSET + xoff1, POLE_BASE_Y - yoff1 - i, X_OFFSET + POLE_LENGTH - xoff1, POLE_BASE_Y - yoff1 - i, level1->colour);
			}
			count1++;
			level1 = level1->next;
		}

		if (level1->next == NULL)
		{
			xoff1 = level1->xpos;
			yoff1 = count1 * DISK_THICK;
			for (int i = 1; i <= DISK_THICK; i++)
			{
				drawLine(X_OFFSET + xoff1, POLE_BASE_Y - yoff1 - i, X_OFFSET + POLE_LENGTH - xoff1, POLE_BASE_Y - yoff1 - i, level1->colour);
			}
		}
	}

	// tower2
	if (level2 != NULL)
	{ // prevents crashes when empty list for Tower2
		while (level2->next != NULL)
		{
			xoff2 = level2->xpos;
			yoff2 = count2 * DISK_THICK;
			for (int i = 1; i <= DISK_THICK; i++)
			{
				drawLine(POLE_LENGTH + 2 * X_OFFSET + xoff2, POLE_BASE_Y - yoff2 - i, 2 * POLE_LENGTH + 2 * X_OFFSET - xoff2, POLE_BASE_Y - yoff2 - i, level2->colour);
			}
			count2++;
			level2 = level2->next;
		}

		if (level2->next == NULL)
		{
			xoff2 = level2->xpos;
			yoff2 = count2 * DISK_THICK;
			for (int i = 1; i <= DISK_THICK; i++)
			{
				drawLine(POLE_LENGTH + 2 * X_OFFSET + xoff2, POLE_BASE_Y - yoff2 - i, 2 * POLE_LENGTH + 2 * X_OFFSET - xoff2, POLE_BASE_Y - yoff2 - i, level2->colour);
			}
		}
	}

	// tower3
	if (level3 != NULL)
	{ // prevents crashes when empty list for Tower3
		while (level3->next != NULL)
		{
			xoff3 = level3->xpos;
			yoff3 = count3 * DISK_THICK;
			for (int i = 1; i <= DISK_THICK; i++)
			{
				drawLine(2 * POLE_LENGTH + 3 * X_OFFSET + xoff3, POLE_BASE_Y - yoff3 - i, 3 * POLE_LENGTH + 3 * X_OFFSET - xoff3, POLE_BASE_Y - yoff3 - i, level3->colour);
			}
			count3++;
			level3 = level3->next;
		}

		if (level3->next == NULL)
		{
			xoff3 = level3->xpos;
			yoff3 = count3 * DISK_THICK;
			for (int i = 1; i <= DISK_THICK; i++)
			{
				drawLine(2 * POLE_LENGTH + 3 * X_OFFSET + xoff3, POLE_BASE_Y - yoff3 - i, 3 * POLE_LENGTH + 3 * X_OFFSET - xoff3, POLE_BASE_Y - yoff3 - i, level3->colour);
			}
		}
	}
}

// dependent on input from the arrow keys on the keyboard
void drawArrow(struct Arrow theArrow)
{
	if (theArrow.location == 1)
	{
		for (int i = -ARROW_RECT_THICK / 2; i < ARROW_RECT_THICK / 2; i++)
		{
			drawLine(X_OFFSET + POLE_LENGTH / 2 + i, ARROW_TOP_Y, X_OFFSET + POLE_LENGTH / 2 + i, ARROW_BOTTOM_Y, PINK);
		}
		for (int i = 0; i < ARROW_TRI_THICK; i++)
		{
			drawLine(X_OFFSET + POLE_LENGTH / 2 - ARROW_TRI_THICK + i, ARROW_BOTTOM_Y + i, X_OFFSET + POLE_LENGTH / 2 + ARROW_TRI_THICK - i, ARROW_BOTTOM_Y + i, PINK);
		}
	}
	if (theArrow.location == 2)
	{
		for (int i = -ARROW_RECT_THICK / 2; i < ARROW_RECT_THICK / 2; i++)
		{
			drawLine(2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 + i, ARROW_TOP_Y, 2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 + i, ARROW_BOTTOM_Y, PINK);
		}
		for (int i = 0; i < ARROW_TRI_THICK; i++)
		{
			drawLine(2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 - ARROW_TRI_THICK + i, ARROW_BOTTOM_Y + i, 2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 + ARROW_TRI_THICK - i, ARROW_BOTTOM_Y + i, PINK);
		}
	}
	if (theArrow.location == 3)
	{
		for (int i = -ARROW_RECT_THICK / 2; i < ARROW_RECT_THICK / 2; i++)
		{
			drawLine(3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 + i, ARROW_TOP_Y, 3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 + i, ARROW_BOTTOM_Y, PINK);
		}
		for (int i = 0; i < ARROW_TRI_THICK; i++)
		{
			drawLine(3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 - ARROW_TRI_THICK + i, ARROW_BOTTOM_Y + i, 3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 + ARROW_TRI_THICK - i, ARROW_BOTTOM_Y + i, PINK);
		}
	}
}

// dependent on input from the spacebar on the keyboard
void drawSuspendedDisk()
{

	// checking if the arrow is pointing to Tower1 AND the disk is selected
	if (theArrow.selected == false)
	{
		return;
	}

	if (theArrow.location == 1)
	{
		for (int i = 1; i <= DISK_THICK; i++)
		{
			drawLine(X_OFFSET + POLE_LENGTH / 2 - sizeArray[theArrow.selectedDiskNumber - 1] / 2, SUSPENSION_TOP_Y + i, X_OFFSET + POLE_LENGTH / 2 + sizeArray[theArrow.selectedDiskNumber - 1] / 2, SUSPENSION_TOP_Y + i, colourArray[theArrow.selectedDiskNumber - 1]);
		}
	}
	if (theArrow.location == 2)
	{
		for (int i = 1; i <= DISK_THICK; i++)
		{
			drawLine(2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 - sizeArray[theArrow.selectedDiskNumber - 1] / 2, SUSPENSION_TOP_Y + i, 2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 + sizeArray[theArrow.selectedDiskNumber - 1] / 2, SUSPENSION_TOP_Y + i, colourArray[theArrow.selectedDiskNumber - 1]);
		}
	}
	if (theArrow.location == 3)
	{
		for (int i = 1; i <= DISK_THICK; i++)
		{
			drawLine(3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 - sizeArray[theArrow.selectedDiskNumber - 1] / 2, SUSPENSION_TOP_Y + i, 3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 + sizeArray[theArrow.selectedDiskNumber - 1] / 2, SUSPENSION_TOP_Y + i, colourArray[theArrow.selectedDiskNumber - 1]);
		}
	}
}

void drawLine(int x0, int y0, int x1, int y1, short int colour)
{
	int temp, y_step;
	bool isSteep = ABS(y1 - y0) > ABS(x1 - x0);
	if (isSteep)
	{
		// swap x0 and y0
		temp = x0;
		x0 = y0;
		y0 = temp;

		// swap x1 and y1
		temp = x1;
		x1 = y1;
		y1 = temp;
	}
	// flip if drawing backwards
	if (x0 > x1)
	{
		// swap x0 and x1
		temp = x0;
		x0 = x1;
		x1 = temp;

		// swap y0 and y1
		temp = y0;
		y0 = y1;
		y1 = temp;
	}

	int deltax = x1 - x0; // we already know going forward
	int deltay = ABS(y1 - y0);
	int error = -(deltax / 2);
	int y = y0;

	if (y0 < y1)
	{
		y_step = 1;
	}
	else
	{
		y_step = -1;
	}

	for (int x = x0; x < x1; x++)
	{
		if (isSteep)
		{
			plot_pixel(y, x, colour);
		}
		else
		{
			plot_pixel(x, y, colour);
		}

		error = error + deltay;
		if (error >= 0)
		{
			y = y + y_step;
			error = error - deltax;
		}
	}
}

void clear_screen(int x1, int x2, int y1, int y2)
{
	short int colour = BLACK;
	for (int x = x1; x < x2; x++)
	{
		for (int y = y1; y < y2; y++)
		{
			plot_pixel(x, y, colour);
		}
	}
}

void clearArrows()
{
	for (int i = -ARROW_RECT_THICK / 2; i < ARROW_RECT_THICK / 2; i++)
	{
		drawLine(X_OFFSET + POLE_LENGTH / 2 + i, ARROW_TOP_Y, X_OFFSET + POLE_LENGTH / 2 + i, ARROW_BOTTOM_Y, BLACK);
	}
	for (int i = 0; i < ARROW_TRI_THICK; i++)
	{
		drawLine(X_OFFSET + POLE_LENGTH / 2 - ARROW_TRI_THICK + i, ARROW_BOTTOM_Y + i, X_OFFSET + POLE_LENGTH / 2 + ARROW_TRI_THICK - i, ARROW_BOTTOM_Y + i, BLACK);
	}

	for (int i = -ARROW_RECT_THICK / 2; i < ARROW_RECT_THICK / 2; i++)
	{
		drawLine(2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 + i, ARROW_TOP_Y, 2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 + i, ARROW_BOTTOM_Y, BLACK);
	}
	for (int i = 0; i < ARROW_TRI_THICK; i++)
	{
		drawLine(2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 - ARROW_TRI_THICK + i, ARROW_BOTTOM_Y + i, 2 * X_OFFSET + POLE_LENGTH + POLE_LENGTH / 2 + ARROW_TRI_THICK - i, ARROW_BOTTOM_Y + i, BLACK);
	}

	for (int i = -ARROW_RECT_THICK / 2; i < ARROW_RECT_THICK / 2; i++)
	{
		drawLine(3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 + i, ARROW_TOP_Y, 3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 + i, ARROW_BOTTOM_Y, BLACK);
	}
	for (int i = 0; i < ARROW_TRI_THICK; i++)
	{
		drawLine(3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 - ARROW_TRI_THICK + i, ARROW_BOTTOM_Y + i, 3 * X_OFFSET + 2 * POLE_LENGTH + POLE_LENGTH / 2 + ARROW_TRI_THICK - i, ARROW_BOTTOM_Y + i, BLACK);
	}
}

void plot_pixel(int x, int y, short int line_color)
{
	*(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

// void wait_for_vsync()
// {
// 	volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
// 	int status;

// 	// launch the swap process(derefence and write in)
// 	*pixel_ctrl_ptr = 1;
// 	// poll the status bit
// 	status = *(pixel_ctrl_ptr + 3);
// 	while ((status & 0x01) != 0)
// 	{
// 		status = *(pixel_ctrl_ptr + 3);
// 	}
// }

void pickUpDisk()
{
	// updateArrowInfo();
	if (theArrow.selected == true)
	{
		return;
	}

	int currentTowerTop = theArrow.towerTop;
	if (currentTowerTop == 0)
	{
		return;
	}

	if (theArrow.location == 1)
	{
		deleteTop(&Tower1); // updated the selectedDiskNumber
		theArrow.selected = true;
		return;
	}
	if (theArrow.location == 2)
	{
		deleteTop(&Tower2); // updated the selectedDiskNumber
		theArrow.selected = true;
		return;
	}
	if (theArrow.location == 3)
	{
		deleteTop(&Tower3); // updated the selectedDiskNumber
		theArrow.selected = true;
		return;
	}
}

void dropDisk()
{
	if (theArrow.selected == false)
	{
		return;
	}
	currCount++;

	if ((theArrow.towerTop != 0) && (theArrow.towerTop < theArrow.selectedDiskNumber))
	{
		errorMessage();
		return;
	}

	if (theArrow.location == 1)
	{
		if ((theArrow.towerTop == 0) || (theArrow.towerTop > theArrow.selectedDiskNumber))
		{
			append(&Tower1, theArrow.selectedDiskNumber);
			theArrow.selected = false;
			return;
		}
	}
	if (theArrow.location == 2)
	{
		if ((theArrow.towerTop == 0) || (theArrow.towerTop > theArrow.selectedDiskNumber))
		{
			append(&Tower2, theArrow.selectedDiskNumber);
			theArrow.selected = false;
			return;
		}
	}
	if (theArrow.location == 3)
	{
		if ((theArrow.towerTop == 0) || (theArrow.towerTop > theArrow.selectedDiskNumber))
		{
			append(&Tower3, theArrow.selectedDiskNumber);
			theArrow.selected = false;
			return;
		}
	}
}

void getKey()
{
	volatile int *game_key = 0xff200050;

	while (1)
	{

		if ((fakeCount == 0) || (fakeCount == 1))
		{
			updateArrowInfo();
			fakeCount++;
			break;
		}

		
		if ((*game_key) == 4)
		{
			eraseErrorMessage();
			if (theArrow.selected == true)
			{
				errorMessage(); 
				break;
			}
			pickUpDisk();
			updateArrowInfo();
			break;
		}

		if ((*game_key) == 8)
		{
			eraseErrorMessage();
			if (theArrow.selected == false)
			{
				errorMessage();
				break;
			}
			dropDisk();
			updateArrowInfo();
			break;
		}
		if ((*game_key) == 2)
		{
			eraseErrorMessage();
			if (globalArrowLocation == 1)
			{
				globalArrowLocation = 3;
				updateArrowInfo();
				break;
			}
			globalArrowLocation--;
			updateArrowInfo();
			break;
		}
		if ((*game_key) == 1)
		{
			eraseErrorMessage();
			if (globalArrowLocation == 3)
			{
				globalArrowLocation = 1;
				updateArrowInfo();
				break;
			}
			globalArrowLocation++;
			updateArrowInfo();
			break;
		}
	}
}

bool checkForWin()
{
	int tower3Size = checkTowerSize(&Tower3);
	if (tower3Size == numOfDisks)
	{
		return true;
	}
	return false;
}

// function from http://www-ug.eecg.utoronto.ca/desl/nios_devices_SoC/dev_vga.html
void write_char(int x, int y, char c)
{
	// VGA character buffer
	volatile char *character_buffer = (char *)(0xc9000000 + (y << 7) + x);
	*character_buffer = c;
}

void errorMessage()
{
	int x = 34;
	char *errorMessage = "Invalid Move";
	while (*errorMessage)
	{
		write_char(x, 25, *errorMessage);
		x++;
		errorMessage++;
	}
}

void winMessage()
{
	int x = 34;
	char *errorMessage = "Game Over! :D";
	while (*errorMessage)
	{
		write_char(x, 25, *errorMessage);
		x++;
		errorMessage++;
	}
}

// erasing the error message
void eraseErrorMessage()
{
	int x = 34;
	char *blank = "                  ";
	while (*blank)
	{
		write_char(x, 25, *blank);
		blank++;
		x++;
	}
}

void drawIntro()
{
	int r = 34;
	char *welcome = "WELCOME!";
	while (*welcome)
	{
		write_char(r, 25, *welcome);
		welcome++;
		r++;
	}

	for (int y = 0; y < 240; y++)
	{
		for (int x = 0; x < 320; x++)
		{
			// short int pixel = (short int)introScreen[y][x];
			plot_pixel(x, y, BLACK);
		}
	}
}

void drawWinScreen()
{	
	int r = 34;
	char *welcome = "WON!";
	while (*welcome)
	{
		write_char(r, 25, *welcome);
		welcome++;
		r++;
	}

	for (int y = 0; y < 240; y++)
	{
		for (int x = 0; x < 320; x++)
		{
			// short int pixel = (short int)introScreen[y][x];
			plot_pixel(x, y, BLACK);
		}
	}
}

void Delay(){
	volatile int *MPCORE_PTR = (int *)MPCORE_PRIV_TIMER;
	*(MPCORE_PTR) = 39999999; // 1.5 second delay since cpulator runs @200MHz
	*(MPCORE_PTR + 2) = 0b011; // SET A and E bits

	*(MPCORE_PTR + 3) = 1; // set the F bit
	while (*(MPCORE_PTR + 3) == 0)
	{
	};					   // polling the F bit
	*(MPCORE_PTR + 3) = 1; // reset the F bit
}
