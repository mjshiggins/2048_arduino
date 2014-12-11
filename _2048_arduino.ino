/*
 ============================================================================
 Name        : 2048.c
 Author      : Maurits van der Schee
 Description : Console version of the game "2048" for GNU/Linux
 ============================================================================
 
 ============================================================================
 Name        : _2048_arduino
 Author      : MacCallister Higgins
 Description : Arduino Microcontroller version of the game "2048" for Gameduino 2 Platform
 				Modified from Console Version by Maurits van der Schee
 ============================================================================
 */

#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

// Need to figure out which ones are compatible, which ones are used, and which ones are needed
#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>

#define SIZE 4
uint32_t score=0;
uint8_t scheme=0;

// Once color scheme is simplified this can be greatly cut down
// OK
void getColor(uint16_t value, char *color, size_t length) {
  uint8_t original[] = {
    8,255,1,255,2,255,3,255,4,255,5,255,6,255,7,255,9,0,10,0,11,0,12,0,13,0,14,0,255,0,255,0    };
  uint8_t blackwhite[] = {
    232,255,234,255,236,255,238,255,240,255,242,255,244,255,246,0,248,0,249,0,250,0,251,0,252,0,253,0,254,0,255,0    };
  uint8_t bluered[] = {
    235,255,63,255,57,255,93,255,129,255,165,255,201,255,200,255,199,255,198,255,197,255,196,255,196,255,196,255,196,255,196,255    };
  uint8_t *schemes[] = {
    original,blackwhite,bluered    };
  uint8_t *background = schemes[scheme]+0;
  uint8_t *foreground = schemes[scheme]+1;
  if (value > 0) while (value >>= 1) {
    if (background+2<schemes[scheme]+sizeof(original)) {
      background+=2;
      foreground+=2;
    }
  }
  snprintf(color,length,"\033[38;5;%d;48;5;%dm",*foreground,*background);
}

void drawBoard(uint16_t board[SIZE][SIZE]) {
  int8_t x,y;
  int arr[x][y];

  // Convert working array to display array
  for (y=0;y<SIZE;y++) 
  {
    for (x=0;x<SIZE;x++) 
    {
      arr[x][y]=board[x][y];
    }
  }


GD.Clear();
GD.Begin(BITMAPS);
GD.Vertex2ii(0,0,0);

  for (y=0;y<SIZE;y++) 
      {
       for (x=0;x<SIZE;x++) 
           {
           char test[8];
           char score2[8];
           snprintf(test,8,"%u",board[x][y]);
           if(board[x][y] != 0 )
             {
             GD.cmd_text((x*100)+50, (y*50)+25, 31, OPT_CENTER, test);
             }
           snprintf(score2,8,"%u",score);
           GD.cmd_text(150, 240, 31, OPT_CENTER, "Score: "); 
           GD.cmd_text(270, 240, 31, OPT_CENTER, score2);           
           }
     }
     

GD.swap();

}

// I believe this finds the maximum slide distance for each square. It's only one dimension as it is called on the rotated array
// OK
int8_t findTarget(uint16_t array[SIZE],int8_t x,int8_t stop) {
  int8_t t;
  // if the position is already on the first, don't evaluate
  if (x==0) {
    return x;
  }
  for(t=x-1;t>=0;t--) {
    if (array[t]!=0) {
      if (array[t]!=array[x]) {
        // merge is not possible, take next position
        return t+1;
      }
      return t;
    } 
    else {
      // we should not slide further, return this one
      if (t==stop) {
        return t;
      }
    }
  }
  // we did not find a
  return x;
}

// The electric slide
// OK
bool slideArray(uint16_t array[SIZE]) {
  bool success = false;
  int8_t x,t,stop=0;

  for (x=0;x<SIZE;x++) {
    if (array[x]!=0) {
      t = findTarget(array,x,stop);
      // if target is not original position, then move or merge
      if (t!=x) {
        // if target is not zero, set stop to avoid double merge
        if (array[t]!=0) {
          score+=array[t]+array[x];
          stop = t+1;
        }
        array[t]+=array[x];
        array[x]=0;
        success = true;
      }
    }
  }
  return success;
}

// Interesting logic here, the code seems to rotate the board within the array so that all movements are "upward," checking pairs from below, and then moves them back
// If that's what works... I guess it cuts down on redundant, slightly modified code. Helps with target algorithm
// OK(?)
void rotateBoard(uint16_t board[SIZE][SIZE]) {
  int8_t i,j,n=SIZE;
  uint16_t tmp;
  for (i=0; i<n/2; i++){
    for (j=i; j<n-i-1; j++){
      tmp = board[i][j];
      board[i][j] = board[j][n-i-1];
      board[j][n-i-1] = board[n-i-1][n-j-1];
      board[n-i-1][n-j-1] = board[n-j-1][i];
      board[n-j-1][i] = tmp;
    }
  }
}

bool moveUp(uint16_t board[SIZE][SIZE]) {
  bool success = false;
  int8_t x;
  for (x=0;x<SIZE;x++) {
    success |= slideArray(board[x]);
  }
  return success;
}

bool moveLeft(uint16_t board[SIZE][SIZE]) {
  bool success;
  rotateBoard(board);
  success = moveUp(board);
  rotateBoard(board);
  rotateBoard(board);
  rotateBoard(board);
  return success;
}

bool moveDown(uint16_t board[SIZE][SIZE]) {
  bool success;
  rotateBoard(board);
  rotateBoard(board);
  success = moveUp(board);
  rotateBoard(board);
  rotateBoard(board);
  return success;
}

bool moveRight(uint16_t board[SIZE][SIZE]) {
  bool success;
  rotateBoard(board);
  rotateBoard(board);
  rotateBoard(board);
  success = moveUp(board);
  rotateBoard(board);
  return success;
}

// Checks for pair in the downward direction
// Where are the rest? (Rotateboard?)
// OK
bool findPairDown(uint16_t board[SIZE][SIZE]) {
  bool success = false;
  int8_t x,y;
  for (x=0;x<SIZE;x++) {
    for (y=0;y<SIZE-1;y++) {
      if (board[x][y]==board[x][y+1]) return true;
    }
  }
  return success;
}

// Only used for game end condition testing
// OK
int16_t countEmpty(uint16_t board[SIZE][SIZE]) {
  int8_t x,y;
  int16_t count=0;
  for (x=0;x<SIZE;x++) {
    for (y=0;y<SIZE;y++) {
      if (board[x][y]==0) {
        count++;
      }
    }
  }
  return count;
}

// Game end check, should be fine
// OK
bool gameEnded(uint16_t board[SIZE][SIZE]) {
  bool ended = true;
  if (countEmpty(board)>0) return false;
  if (findPairDown(board)) return false;
  rotateBoard(board);
  if (findPairDown(board)) ended = false;
  rotateBoard(board);
  rotateBoard(board);
  rotateBoard(board);
  return ended;
}


// Adds the random 2 or 4 block? Check arduino random libraries
// If required, only returning a value of 2 would be acceptable
// NOT OK
void addRandom(uint16_t board[SIZE][SIZE]) {
  static bool initialized = false;
  int8_t x,y;
  int16_t r,len=0;
  uint16_t n,list[SIZE*SIZE][2];

  if (!initialized) {
    //srand(time(NULL));
    randomSeed(analogRead(A0));
    initialized = true;
  }

  for (x=0;x<SIZE;x++) {
    for (y=0;y<SIZE;y++) {
      if (board[x][y]==0) {
        list[len][0]=x;
        list[len][1]=y;
        len++;
      }
    }
  }

  if (len>0) {
    //r = rand()%len;
    r = random(1000)%len;
    x = list[r][0];
    y = list[r][1];
    //n = ((rand()%10)/9+1)*2;
    n = ((random(1000)%10)/9+1)*2;
    board[x][y]=n;
  }
}




void setup()
{
  GD.begin();
  GD.BitmapHandle(0);
  GD.cmd_loadimage(0,0);
  GD.load("g5.jpg");  

}


void loop()
{
  // Intro Screen
  GD.ClearColorRGB(0x103000);
  GD.Clear();
  GD.cmd_text(240, 136, 31, OPT_CENTER, "2048: Embedded");
  GD.swap();
  


  delay(3000);



  uint16_t board[SIZE][SIZE];
  char c;
  bool success;

  memset(board,0,sizeof(board));
  addRandom(board);
  addRandom(board);
  drawBoard(board);


  // Must be modified with on screen directionals
  // Region of screen touch? That's easy. Maybe have swiping from zone to zone?
  while (true) {
    // Retrieve Touch Inputs
    GD.get_inputs();

    // Test for screen location
    if(GD.inputs.x > 0) // Test for any touch
    {
      if(GD.inputs.x<160)
      {
        c='l';
        //GD.Clear();
        //GD.cmd_text(240, 136, 31, OPT_CENTER, "Left Register");
        //GD.swap();

      }
      else if(GD.inputs.x>320)
      {
        c='r';
        //GD.Clear();
        //GD.cmd_text(240, 136, 31, OPT_CENTER, "Right Register");
        //GD.swap();
      }
      else if(GD.inputs.x>160 && GD.inputs.x<320)
      {
        if(GD.inputs.y<136)
        {
          c='u';
          //GD.Clear();
          //GD.cmd_text(240, 136, 31, OPT_CENTER, "Up Register");
          //GD.swap();
        } 
        else
        {
          c='d'; 
          //GD.Clear();
          //GD.cmd_text(240, 136, 31, OPT_CENTER, "Down Register");
          //GD.swap();
        }
      }

    }

    // Directional Inputs
    switch(c) {
    case 'l':	// left
      success = moveLeft(board);  
      break;
    case 'r':	// right
      success = moveRight(board); 
      break;
    case 'u':	// up
      success = moveUp(board);    
      break;
    case 'd':	// down
      success = moveDown(board);  
      break;
    default: 
      success = false;
    }
    if (success) {
      //drawBoard(board);
      // usleep delays for X microseconds, converted to milliseconds
      //usleep(150000);
      delay(150);
      addRandom(board);
      drawBoard(board);
      if (gameEnded(board)) {
        // printf("         GAME OVER          \n");
        delay(1000);
        GD.Clear();
        GD.cmd_text(240, 136, 31, OPT_CENTER, "GAME OVER");
        GD.swap();
        delay(3000);
        break;
      }
      // Stop continually repeating 
      c=NULL;
    }

    // No way to input these characters
    /*
		if (c=='q') {
     			printf("        QUIT? (y/n)         \n");
     			while (true) {
     			c=getchar();
     				if (c=='y'){
     					setBufferedInput(true);
     					printf("\033[?25h");
     					exit(0);
     				}
     				else {
     					drawBoard(board);
     					break;
     				}
     			} 
     		}
     		if (c=='r') {
     			printf("       RESTART? (y/n)       \n");
     			while (true) {
     			c=getchar();
     				if (c=='y'){
     					memset(board,0,sizeof(board));
     					addRandom(board);
     					addRandom(board);
     					drawBoard(board);
     					break;
     				}
     				else {
     					drawBoard(board);
     					break;
     				}
     			}
     		}
     */
  }
  //setBufferedInput(true);

  // More ansi escape
  //printf("\033[?25h");

  //return EXIT_SUCCESS;

  delay(1000);
}



