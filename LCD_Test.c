#include <cr_section_macros.h>

#include <NXP/crp.h>

#include "image_upload.h"

// Variable to store CRP value in. Will be placed automatically

// by the linker when "Enable Code Read Protect" selected.

// See crp.h header for more information

__CRP const unsigned int CRP_WORD = CRP_NO_CRP;

#include "LPC17xx.h" /* LPC13xx definitions */

#include "ssp.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>

#include <math.h>

#include "font.h"

#include "extint.h"
#define PORT_NUM			1
#define LOCATION_NUM		0

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

uint8_t src_addr[SSP_BUFSIZE];

uint8_t dest_addr[SSP_BUFSIZE];

int colstart = 0;
int rowstart = 0;

#define ST7735_TFTWIDTH  127 //LCD width
#define ST7735_TFTHEIGHT 159 //LCD height
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define swap(x, y) { x = x + y; y = x - y; x = x - y; }

//color code
#define GREEN 0x00FF00
#define BLACK  0x000000
#define RED  0xFF0000
#define BLUE  0x0000FF
#define WHITE  0xFFFFFF
#define CYAN    0x00FFFF
#define PURPLE 0x8E388E
#define YELLOW  0xFFD700
#define ORANGE 0xFF8000
#define BROWN 800000

int _height = ST7735_TFTHEIGHT;
int _width = ST7735_TFTWIDTH;
int cursor_x = 0, cursor_y = 0;
//uint16_t textcolor = RED, textbgcolor= GREEN;
float textsize = 2;
int wrap = 1;
//char color=blue,red,;
void spiwrite(uint8_t c)
{
	int portnum = 1;

	src_addr[0] = c;
	SSP_SSELToggle( portnum, 0 );
	SSPSend( portnum, (uint8_t *)src_addr, 1 );
	SSP_SSELToggle( portnum, 1 );
}
void writecommand(uint8_t c) {
	LPC_GPIO0->FIOCLR |= (0x1<<21);
	spiwrite(c);
}
void writedata(uint8_t c) {

	LPC_GPIO0->FIOSET |= (0x1<<21);
	spiwrite(c);
}
void writeword(uint16_t c) {

	uint8_t d;

	d = c >> 8;
	writedata(d);
	d = c & 0xFF;
	writedata(d);
}
void write888(uint32_t color, uint32_t repeat) {
	uint8_t red, green, blue;
	int i;
	red = (color >> 16);
	green = (color >> 8) & 0xFF;
	blue = color & 0xFF;
	for (i = 0; i< repeat; i++) {
		writedata(red);
		writedata(green);
		writedata(blue);
	}
}
void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1,
					uint16_t y1) {

	  writecommand(ST7735_CASET);
	  writeword(x0);
	  writeword(x1);
	  writecommand(ST7735_RASET);
	  writeword(y0);
	  writeword(y1);

}
void drawPixel(int16_t x, int16_t y, uint16_t color) {
    if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;

    setAddrWindow(x,y,x+1,y+1);
    writecommand(ST7735_RAMWR);

    write888(color, 1);
}

void fillrect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color)
{
	//int16_t i;
	int16_t width, height;

	width = x1-x0+1;
	height = y1-y0+1;
	setAddrWindow(x0,y0,x1,y1);
	writecommand(ST7735_RAMWR);
	write888(color,width*height);
}
void lcddelay(int ms)
{
	int count = 24000;
	int i;

	for ( i = count*ms; i--; i > 0);
}
void lcd_init()
{
/*
 * portnum 	= 0 ;
 * cs 		= p0.16 / p0.6 ?
 * rs		= p0.21
 * rst		= p0.22
 */
	uint32_t portnum = 1;
	int i;
	printf(" inside lcd_init\n");
	/* Notice the hack, for portnum 0 p0.16 is used */
	if ( portnum == 0 )
	  {
		LPC_GPIO0->FIODIR |= (0x1<<16);		/* SSP1, P0.16 defined as Outputs */
	  }
	  else
	  {
		LPC_GPIO0->FIODIR |= (0x1<<6);		/* SSP0 P0.6 defined as Outputs */
	  }
	/* Set rs(dc) and rst as outputs */
	LPC_GPIO0->FIODIR |= (0x1<<21);		/* rs/dc P0.21 defined as Outputs */
	LPC_GPIO0->FIODIR |= (0x1<<22);		/* rst P0.22 defined as Outputs */


	/* Reset sequence */
	LPC_GPIO0->FIOSET |= (0x1<<22);

	lcddelay(500);						/*delay 500 ms */
	LPC_GPIO0->FIOCLR |= (0x1<<22);
	lcddelay(500);						/* delay 500 ms */
	LPC_GPIO0->FIOSET |= (0x1<<22);
	lcddelay(500);						/* delay 500 ms */
	 for ( i = 0; i < SSP_BUFSIZE; i++ )	/* Init RD and WR buffer */
	    {
	  	  src_addr[i] = 0;
	  	  dest_addr[i] = 0;
	    }

	 /* do we need Sw reset (cmd 0x01) ? */

	 /* Sleep out */
	 SSP_SSELToggle( portnum, 0 );
	 src_addr[0] = 0x11;	/* Sleep out */
	 SSPSend( portnum, (uint8_t *)src_addr, 1 );
	 SSP_SSELToggle( portnum, 1 );

	 lcddelay(200);
	/* delay 200 ms */
	/* Disp on */
	 SSP_SSELToggle( portnum, 0 );
	 src_addr[0] = 0x29;	/* Disp On */
	 SSPSend( portnum, (uint8_t *)src_addr, 1 );
	 SSP_SSELToggle( portnum, 1 );
	/* delay 200 ms */
	 lcddelay(200);
}

void draw_myline(int16_t x0, int16_t y0, int16_t x1, int16_t y1,uint16_t color) {
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	} else {
		ystep = -1;
	}

	for (; x0<=x1; x0++) {
		if (steep) {
			drawPixel(y0, x0, color);
		} else {
			drawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0) {
			y0 += ystep;
			err += dx;
		}
	}
}

void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color){
	draw_myline(x, y, x, y+h-1, color);
}
void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color){
	draw_myline(x, y, x+w-1, y, color);
}

void rotating_mysquare(int x1, int x2, int x3, int x4, int y1, int y2, int y3, int y4,int color1){
//	printf("inside mysquare\n");

	int q1x=0, q2x=0, q3x=0, q4x=0, q1y=0,q2y=0, q3y=0, q4y=0; //new coordinates
	int i;

	for(i=1;i<=11;i++){
		lcddelay(200);
		q1x = (x2+(0.8*(x1-x2))); //P = P0 + lamda*(P1 - P0)
//		printf("q1x : %d\n", q1x);

		q1y = (y2+(0.8*(y1-y2)));
//		printf("q1y : %d\n", q1y);

		q2x = (x3+(0.8*(x2-x3)));
//		printf("q2x : %d\n", q2x);

		q2y = (y3+(0.8*(y2-y3)));
//		printf("q2y : %d\n", q2y);

		q3x = (x4+(0.8*(x3-x4)));
//		printf("q3x : %d\n", q3x);

		q3y = (y4+(0.8*(y3-y4)));
//		printf("q3y : %d\n", q3y);

		q4x = (x1+(0.8*(x4-x1)));
//		printf("q4x : %d\n", q4x);

		q4y = (y1+(0.8*(y4-y1)));
//		printf("q4y : %d\n", q4y);



		draw_myline(q1x,q1y,q2x,q2y,color1);
		draw_myline(q2x,q2y,q3x,q3y,color1);
		draw_myline(q4x,q4y,q3x,q3y,color1);
		draw_myline(q1x,q1y,q4x,q4y,color1);

		x1 = q1x;
		x2 = q2x;
		x3 = q3x;
		x4 = q4x;

		y1 = q1y;
		y2 = q2y;
		y3 = q3y;
		y4 = q4y;
	}
//	printf("exiting mysquare\n");
}

void rotating_mytriangle(int x1, int x2, int x3, int y1, int y2, int y3){
//	printf("inside mytriangle\n");

	int q1x=0, q2x=0, q3x=0, q1y=0, q2y=0, q3y=0;
	int i;

	for(i=1;i<=11;i++){
		lcddelay(200);
		q1x = (x2+(0.8*(x1-x2)));
//		printf("q1x : %d\n", q1x);

		q1y = (y2+(0.8*(y1-y2)));
//		printf("q1y : %d\n", q1y);

		q2x = (x3+(0.8*(x2-x3)));
//		printf("q2x : %d\n", q2x);

		q2y = (y3+(0.8*(y2-y3)));
//		printf("q2y : %d\n", q2y);

		q3x = (x1+(0.8*(x3-x1)));
//		printf("q3x : %d\n", q3x);

		q3y = (y1+(0.8*(y3-y1)));
//		printf("q3y : %d\n", q3y);

		draw_myline(q1x,q1y,q2x,q2y,GREEN);
		draw_myline(q2x,q2y,q3x,q3y,GREEN);
		draw_myline(q1x,q1y,q3x,q3y,GREEN);

		x1 = q1x;
		x2 = q2x;
		x3 = q3x;

		y1 = q1y;
		y2 = q2y;
		y3 = q3y;
	}
//	printf("exiting mytriangle\n");
}

void grow_mytree(int x0, int y0, float angle, int length, int level, int color){
//	printf("inside mytree\n");

	int x1, y1, length1;
	float angle1;

	if(level>0){
		//x-y coordinates of branch
		x1 = x0+length*cos(angle);
//		printf("%f\n",x1);
	    y1 = y0+length*sin(angle);
//	    printf("%f\n",y1);

	    draw_myline(x0,y0,x1,y1,color); //tree branch

	    angle1 = angle + 0.52; //deviate right->0.52 rad/30 deg
	    length1 = 0.8 * length; //reduction of length by 20% of previous length
	    grow_mytree(x1,y1,angle1,length1,level-1,color);

	    angle1 = angle - 0.52; //deviate left->0.52 rad/30 deg
	    length1 = 0.8 * length;
	    grow_mytree(x1,y1,angle1,length1,level-1,color);

	    angle1 = angle; //center->0 deg
	    length1 = 0.8 * length;
	    grow_mytree(x1,y1,angle1,length1,level-1,color);
	}
	//	printf("exiting mytree\n");
}

void drawCircle(int16_t xPos, int16_t yPos, int16_t radius, uint16_t color) {

/*draws circle at x,y with given radius & color*/

int x, xEnd, y;

xEnd = (0.7071 * radius) + 1;

printf("%d\n", xEnd);

for (x = 0; x <= xEnd; x++) {

y = (sqrt(radius * radius - x * x));

printf("%d \n", y);

drawPixel(xPos + x, yPos + y, color);

draw_myline(xPos,yPos,xPos+x,yPos+y,color);

drawPixel(xPos + x, yPos - y, color);

draw_myline(xPos,yPos,xPos+x,yPos-y,color);

drawPixel(xPos - x, yPos + y, color);

draw_myline(xPos,yPos,xPos - x,yPos + y,color);

drawPixel(xPos - x, yPos - y, color);

draw_myline(xPos,yPos,xPos - x,yPos - y,color);

drawPixel(xPos + 	y, yPos + x, color);

draw_myline(xPos,yPos,xPos + y,yPos + x,color);

drawPixel(xPos + y, yPos - x, color);

draw_myline(xPos,yPos,xPos + y,yPos - x,color);

drawPixel(xPos - y, yPos + x, color);

draw_myline(xPos,yPos,xPos - y,yPos + x,color);

drawPixel(xPos - y, yPos - x, color);

draw_myline(xPos,yPos,xPos - y,yPos - x,color);

}

}


void drawRectangle(int16_t x0,int16_t y0,int16_t x1,int16_t y1,int16_t x2,int16_t y2,int16_t x3,int16_t y3,uint16_t color)

{

	draw_myline(x0,y0,x1,y1,color);

	draw_myline(x1,y1,x2,y2,color);

	draw_myline(x2,y2,x3,y3,color);

	draw_myline(x3,y3,x0,y0,color);

}



/******************************************************************************
**   Main Function  main()
******************************************************************************/
int main (void)
{
  SSP1Init(); //SPI initialize

  lcd_init(); //LCD initialize

   //========================ROTATING SQUARE========================
  fillrect(0, 0, ST7735_TFTWIDTH, ST7735_TFTHEIGHT, WHITE);


    //drawing base square

    draw_myline(0,0,36,0,BLACK);

    draw_myline(36,0,36,36,BLACK);

    draw_myline(0,36,36,36,BLACK);

    draw_myline(0,36,0,0,BLACK);

    rotating_mysquare(0,36,36,0,0,0,36,36,BLACK);

    draw_myline(120,120,86,120,GREEN);

    draw_myline(86,120,86,86,GREEN);

    draw_myline(120,86,86,86,GREEN);

    draw_myline(120,86,120,120,GREEN);

    rotating_mysquare(120,86,86,120,120,120,86,86,GREEN);

    draw_myline(60,60,86,60,CYAN);

    draw_myline(86,60,86,86,CYAN);

    draw_myline(60,86,86,86,CYAN);

    draw_myline(60,86,60,60,CYAN);

    rotating_mysquare(60,86,86,60,60,60,86,86,CYAN);



    //rotating squares
    int f;
  for (f = 0; f<3; f++) {
	  int ai=(rand()%130-115)+115;
	  int aj=(rand()%130-115)+115;
	  int color[4]={BLUE,GREEN,RED,CYAN,BLACK};

   //draw_myline(ai,ai,aj,ai,color[f]);
     //   draw_myline(aj,ai,aj,aj,color[f]);
        //draw_myline(ai,aj,aj,aj,color[f]);
        //draw_myline(ai,aj,ai,ai,color[f]);
        //rotating_mysquare(ai,aj,aj,ai,ai,ai,aj,aj,color[f]);

  }
  while(getchar() != '\n');



  //rotating_mysquare(0,75,75,0,0,0,75,75); //rotating squares


  //========================ROTATING TRIANGLE==========================
  //drawing base triangle
  fillrect(0, 0, ST7735_TFTWIDTH, ST7735_TFTHEIGHT, BLACK);
  draw_myline(63,0,126,158,GREEN);
  draw_myline(0,158,126,158,GREEN);
  draw_myline(63,0,0,158,GREEN);

  lcddelay(200);

  rotating_mytriangle(63,126,0,0,158,158); //rotating triangles

  lcddelay(500);
  while(getchar() != '\n');
  ///==========================TREES================================
  fillrect(0, 0, ST7735_TFTWIDTH, ST7735_TFTHEIGHT, BLACK);
  fillrect(0,75,ST7735_TFTWIDTH,ST7735_TFTHEIGHT,BLUE);

 /* draw_myline(64,100,64,160,BROWN); //tree trunk_1

//  draw_myline(32,100,32,160,GREEN); //tree trunk_2

  grow_mytree(64,100,5.23,20,7,GREEN); //right branch (angle = 5.23 rad/300 deg)
//  grow_mytree(32,150,5.23,30,4,GREEN);
  grow_mytree(64,100,4.18,20,7,GREEN); //left branch (angle = 4.18 rad/240 deg)
//  grow_mytree(32,150,4.18,30,4,GREEN);
  grow_mytree(64,100,4.71,20,7,GREEN); //center branch (angle = 4.71 rad/0 deg)
//  grow_mytree(32,150,4.71,30,4,GREEN);
  //void grow_mytree(int x0, int y0, float angle, int length, int level, int color){
   *
   *

  */
  drawCircle(10,5,12,WHITE);
  for (f = 0; f<15; f++) {
  /*int xi=(rand()%70-35)+35;
  int yi=(rand()%100-50)+50;
  int xj=(rand()%70-35)+35;
  int yj=(rand()%160-80)+80;*/
 int xi=rand()%100;
    int yi=rand()%160;
    int xj=rand()%64;
    int yj=rand()%160;/*
	  int xi=rand()%70;
	      int yi=rand()%80;
	      int xj=rand()%64;
	      int yj=rand()%100;*/
  float ani=rand()%6;
  int li=rand()%20;
  int levi=(rand()%(10-7))+7;

  //double_t x1= x0+length*cos(angle);

  //double_t y1=y0+length*sin(angle);

  draw_myline(xi,yi,xi,yi,RED);
  grow_mytree(xi,yi,ani,li,levi,GREEN);
  grow_mytree(xi,yi,ani,li,levi,GREEN);
  grow_mytree(xi,yi,ani,li,levi,GREEN);
  }
  lcddelay(500);
  while(getchar() != '\n');
  return 0;
}


/******************************************************************************
**                            End Of File
******************************************************************************/
