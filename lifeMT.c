#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

typedef struct BoardTag {
   int row;     // number of rows
   int col;     // number of columns
   char** src;  // 2D matrix holding the booleans (0 = DEAD , 1 = ALIVE). 
} Board;

typedef struct boards2 {
    Board* board1;
    Board* board2;
    int start;
    //int end;
    pthread_barrier_t*  bar;
    int numT;
    int numInstr;
} boards2;


/**
 * Allocates memory to hold a board of rxc cells. 
 * The board is made thoroidal (donut shaped) by simply wrapping around
 * with modulo arithmetic when going beyond the end of the board.
 */
Board* makeBoard(int r,int c)
{
   Board* p = (Board*)malloc(sizeof(Board));
   p->row = r;
   p->col = c;
   p->src = (char**)malloc(sizeof(char*)*r);
   int i;
   for(i=0;i<r;i++)
      p->src[i] = (char*)malloc(sizeof(char)*c);
   return p;
}
/**
 * Deallocate the memory used to represent a board
 */
void freeBoard(Board* b)
{
   int i;
   for(i=0;i<b->row;i++)
      free(b->src[i]);
   free(b->src);
   free(b);
}

/**
 * Reads a board from a file named `fname`. The routine allocates
 * a board and fills it with the data from the file. 
 */
Board* readBoard(char* fName)
{
   int row,col,i,j;
   FILE* src = fopen(fName,"r");
   fscanf(src,"%d %d\n",&row,&col);
   Board* rv = makeBoard(row,col);
   for(i=0;i<row;i++) {
      for(j=0;j<col;j++) {
         char ch = fgetc(src);
         rv->src[i][j] = ch == '*';
      }
      char skip = fgetc(src);
      while (skip != '\n') skip = fgetc(src);
   }
   fclose(src);   
   return rv;
}

/**
 * Save a board `b` into a FILE pointed to by `fd`
 */
void saveBoard(Board* b,FILE* fd)
{
   int i,j;
   for(i=0;i<b->row;i++) {
      fprintf(fd,"|");
      for(j=0;j < b->col;j++) 
         fprintf(fd,"%c",b->src[i][j] ? '*' : ' ');
      fprintf(fd,"|\n");
   }
}
/**
 * Low-level convenience API to clear a terminal screen
 */
void clearScreen()
{
   static const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
   static int l = 10;
   write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, l);
}

/*
 * For debugging purposes. print the board on the standard
 * output (after clearing the screen)
 */
void printBoard(Board* b)
{
   clearScreen();	
   saveBoard(b,stdout);
}

/*
 * Simple routine that counts the number of neighbors that
 * are alive around cell (i,j) on board `b`.
 */
int liveNeighbors(int i,int j,Board* b)
{
   const int pc = (j-1) < 0 ? b->col-1 : j - 1;
   const int nc = (j + 1) % b->col;
   const int pr = (i-1) < 0 ? b->row-1 : i - 1;
   const int nr = (i + 1) % b->row;
   int xd[8] = {pc , j , nc,pc, nc, pc , j , nc };
   int yd[8] = {pr , pr, pr,i , i , nr , nr ,nr };
   int ttl = 0;
   int k;
   for(k=0;k < 8;k++)
      ttl += b->src[yd[k]][xd[k]];
   return ttl;
}

/*
 * Sequential routine that writes into the `out` board the 
 * result of evolving the `src` board for one generation.
 */
void evolveBoard(Board* src,Board* out)
{
   static int rule[2][9] = {
      {0,0,0,1,0,0,0,0,0},
      {0,0,1,1,0,0,0,0,0}
   };
   int i,j;
   for(i=0;i<src->row;i++) {
      for(j=0;j<src->col;j++) {
         int ln = liveNeighbors(i,j,src);
         int c  = src->src[i][j];
         out->src[i][j] = rule[c][ln];
      }
   }
}



void* secondEvolve(void* b) {
    boards2* a = (boards2*)b;
    static int rule[2][9] = {
      {0,0,0,1,0,0,0,0,0},
      {0,0,1,1,0,0,0,0,0}
   };
   int i, j;
   int index = 0;
   Board* tempBoard;
   //while indexing  < num instructions given 
   while(index < a->numInstr){
       //go through rows of board1 
       for(i = a->start; i<a->board1->row; i+= a->numT){
           //columsn board 1 
            for(j = 0; j<a->board1->col; j++){
                int ln = liveNeighbors(i ,j, a->board1);
                int c = a->board1->src[i][j];
                //assign values for board 2 2d matrix 
                a->board2->src[i][j] = rule[c][ln];
        }
           
      }
      
      //need to switch board 1 and 2 
      //need to set temp to board1, board 1 to board 2, and board 2 to board 1  
      tempBoard = a->board1;
      //set board 1 in a as current board 2, the  update board 2
      a->board1 = a->board2;
      a->board2 = tempBoard;
      //increase index after while loop 
      index++;
      //set up a wait for the barrier defined in 2boards
      pthread_barrier_wait(a->bar);
      
   }
   //free a and return null 
   free(a);
   return NULL;
}



/*
void initBoards(boards2* b, int nbw, Board *b0, Board *b1) {
    int blockSize;
    blockSize = b0->col / nbw;
    int rem; 
    rem = b0->col % nbw;
    for(int i = 0; i< nbw; i++){
        b[i]->board1 = b1;
        b[i]->board2 = b2;
        if(i == 0){
            b[i]->start = 0;
        }
        if(i != 0){
            b[i]->start = b[i-1]->end;
        }
        if(i < rem){
            b[i]->end = b[i]->start + blockSize + 1;
        }
        else{
            b[i]->end = b[i]->start + blockSize;
        }
    }
}
*/

int main(int argc,char* argv[])
{
   if (argc < 3) {
      printf("Usage: lifeMT <dataFile> #iter #workers\n");
      exit(1);
   }
   
   //declare 2 boards , 1 to read and one to make 
   Board* life1 = readBoard(argv[1]);
   Board* life2 = makeBoard(life1->row,life1->col);
   
   int nbi = atoi(argv[2]);
   int nbw = atoi(argv[3]);
   
   //set up barrier and initiaalize it for multithreading use 
   pthread_barrier_t bar2;
   pthread_barrier_init(&bar2, NULL, nbw);
   
   //array of threads --> did this in Q2 !! --> init it to size of num of threads 
   pthread_t threadArray[nbw];
   
   for(int i = 0; i < nbw; i++){
       //init size of boards2 struct for new boards2, a 
       //then set values in a 
       boards2* a = malloc(sizeof(boards2));
       //b1 and b2 should be defines life1/2 --> read and make 
       a->board1 = life1;
       a->board2 = life2;
       //barrier 
       a->bar = &bar2;
       
       //set nbw and nbi as values from command line 
       a->numT = nbw;
       a->numInstr = nbi;
       
       //set start index to i so each thread starts somewhere else
       a->start = i;
       //pthread create w max so + i
       pthread_create(threadArray + i, NULL, secondEvolve, (void*)a);
   }
   
   
   //join after creating 
   for(int j = 0; j < nbw; j++){
       pthread_join(threadArray[j] , NULL);
   }
   
   //need to handle for even vs odd # of instructions
    //if even , life 1
    // if odd, life 2 
   
   Board* B_ptr;
   int numberInstr = nbi;
   if(numberInstr % 2 == 1){
       B_ptr = life2;
   }
   else{
       B_ptr = life1;
   }
   
   
   
   //open text file 
   FILE* final = fopen("final.txt","w");
   //save 
   saveBoard(B_ptr, final);
   
   //print and dont forget to free!
   printBoard(B_ptr);
   freeBoard(life1);
   freeBoard(life2);
   
   
   /*
   boards2* b;
   pthread_t* boards2_array;
   boards2_array = malloc(sizeof(boards2) * nbw);
   
   
   pthread_t *threads_array;
   
   threads_array = malloc(sizeof(pthread_t)* nbw);
   
   Board* life1 = readBoard(argv[1]);
   Board* life2 = makeBoard(life1->row,life1->col);
   Board *b0, *b1;
   b0 = life1;
   b1 = life2;
   initBoards(boards2_array, nbw, b0, b1);
   int g;
   for(g =0; g<nbi; g++){
       pthread_create(&threads_array[i], NULL, secondEvolve, &boards2_array );
   }
   
  
   for (i = 0; i < nbw; i++) {
        pthread_join(threads_array[i], NULL);
    }
    printBoard(b1);
    FILE* final = fopen("final.txt","w");
    saveBoard(b1,final);
    fclose(final);
    freeBoard(b0);
    freeBoard(b1);

   return 0;
} 
