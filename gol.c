/*
 * Swarthmore College, CS 31
 * Copyright (c) 2023 Swarthmore College Computer Science Department,
 * Swarthmore PA
 */

// Game of life game. This game consists of a 2d array square board with each element
// storing a "."(dead) or a "@"(alive). After each iteration, the board will change
// based on the rules of the game.

/*
 * To run:
 * ./gol file1.txt  0  # run with config file file1.txt, do not print board
 * ./gol file1.txt  1  # run with config file file1.txt, ascii animation
 * ./gol file1.txt  2  # run with config file file1.txt, ParaVis animation
 *
 */
#include <pthreadGridVisi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "colors.h"

/****************** Definitions **********************/
/* Three possible modes in which the GOL simulation can run */
#define OUTPUT_NONE   (0)   // with no animation
#define OUTPUT_ASCII  (1)   // with ascii animation
#define OUTPUT_VISI   (2)   // with ParaVis animation

/* Used to slow down animation run modes: usleep(SLEEP_USECS);
 * Change this value to make the animation run faster or slower
 */
//#define SLEEP_USECS  (1000000)
#define SLEEP_USECS    (750000)

/* A global variable to keep track of the number of live cells in the
 * world (this is the ONLY global variable you may use in your program)
 */
static int total_live = 0;

/* This struct represents all the data you need to keep track of your GOL
 * simulation.  Rather than passing individual arguments into each function,
 * we'll pass in everything in just one of these structs.
 * this is passed to play_gol, the main gol playing loop
 *
 * NOTE: You will need to use the provided fields here, but you'll also
 *       need to add additional fields. (note the nice field comments!)
 * NOTE: DO NOT CHANGE THE NAME OF THIS STRUCT!!!!
 */
struct gol_data 
{

    // NOTE: DO NOT CHANGE the names of these 4 fields (but USE them)
    int rows;  // the row dimension
    int cols;  // the column dimension
    int iters; // number of iterations to run the gol simulation
    int output_mode; // set to:  OUTPUT_NONE, OUTPUT_ASCII, or OUTPUT_VISI
    char** board;
    char** next_board;

    /* fields used by ParaVis library (when run in OUTPUT_VISI mode). */
    // NOTE: DO NOT CHANGE their definitions BUT USE these fields
    visi_handle handle;
    color3 *image_buff;
};


/****************** Function Prototypes **********************/
/* the main gol game playing loop (prototype must match this) */
void play_gol(struct gol_data *data);
//function to get # of neighbors
int neighbors(struct gol_data *data, int row, int col);

/* init gol data from the input file and run mode cmdline args */
int init_game_data_from_args(struct gol_data *data, char **argv);

// A mostly implemented function, but a bit more for you to add.
/* print board to the terminal (for OUTPUT_ASCII mode) */
void print_board(struct gol_data *data, int round);
void update_colors(struct gol_data *data) ;
/************ Definitions for using ParVisi library ***********/
/* initialization for the ParaVisi library (DO NOT MODIFY) */
int setup_animation(struct gol_data* data);
/* register animation with ParaVisi library (DO NOT MODIFY) */
int connect_animation(void (*applfunc)(struct gol_data *data),
        struct gol_data* data);
/* name for visi (you may change the string value if you'd like) */
static char visi_name[] = "GOL!";


int main(int argc, char **argv) 
{
    int ret;
    struct gol_data data;
    struct timeval start_time, stop_time;
    double secs;

    /* check number of command line arguments */
    if (argc < 3) 
    {
        printf("usage: %s <infile.txt> <output_mode>[0|1|2]\n", argv[0]);
        printf("(0: no visualization, 1: ASCII, 2: ParaVisi)\n");
        exit(1);
    }

    /* Initialize game state (all fields in data) from information
     * read from input file */
    ret = init_game_data_from_args(&data, argv);
    gettimeofday(&start_time, NULL);
    if (ret != 0) 
    {
        printf("Initialization error: file %s, mode %s\n", argv[1], argv[2]);
        exit(1);
    }
    /* initialize ParaVisi animation (if applicable) */
    if (data.output_mode == OUTPUT_VISI) 
    {
        setup_animation(&data);
    }
    /* ASCII output: clear screen & print the initial board */
    if (data.output_mode == OUTPUT_ASCII) 
    {
        if (system("clear")) { perror("clear"); exit(1); }
        print_board(&data, 0);
    }
    /* Invoke play_gol in different ways based on the run mode */
    if (data.output_mode == OUTPUT_NONE) 
    {  // run with no animation
        //printf("1\n");
        play_gol(&data);
    }
    else if (data.output_mode == OUTPUT_ASCII) 
    { // run with ascii animation
        usleep(200000);
        play_gol(&data);
        // (NOTE: you can comment out this line while debugging)
        if (system("clear")) { perror("clear"); exit(1); }
        // NOTE: DO NOT modify this call to print_board at the end
        //       (it's to help us with grading your output)
        print_board(&data, data.iters);
    }
    else //###############
    {  // OUTPUT_VISI: run with ParaVisi animation
            // tell ParaVisi that it should run play_gol
            usleep(500000);
        connect_animation(play_gol, &data);
        // start ParaVisi animation
        run_animation(data.handle, data.iters);
    } //#################

    gettimeofday(&stop_time, NULL);
    if (data.output_mode != OUTPUT_VISI) 
    {
        secs = (stop_time.tv_sec - start_time.tv_sec) +
                    (stop_time.tv_usec - start_time.tv_usec) / 1000000.0;

        /* Print the total runtime, in seconds. */
        // NOTE: do not modify these calls to fprintf
        fprintf(stdout, "Total time: %0.3f seconds\n", secs);
        fprintf(stdout, "Number of live cells after %d rounds: %d\n\n",
                data.iters, total_live);
    }

    //free allocated memory
    for (int i = 0; i < data.rows; i++)
    {
        free(data.board[i]);
        free(data.next_board[i]);
    }
    free(data.board);
    free(data.next_board);

    return 0;
}

/* initialize the gol game state from command line arguments
 *       argv[1]: name of file to read game config state from
 *       argv[2]: run mode value
 * data: pointer to gol_data struct to initialize
 * argv: command line args
 *       argv[1]: name of file to read game config state from
 *       argv[2]: run mode
 * returns: 0 on success, 1 on error
 */
int init_game_data_from_args(struct gol_data *data, char **argv) 
{
    FILE *file = fopen(argv[1], "r");
    if(file == NULL)
    {
        perror("ERROR");
        return 1;
    }

    int width, height, next, iter, num_pairs, i, j;
    next = fscanf(file, "%d", &height);
    next = fscanf(file, "%d", &width);

    next = fscanf(file, "%d", &iter);
    next = fscanf(file, "%d", &num_pairs);

    int mode;
    if(*argv[2] == '0')
    {
        mode = OUTPUT_NONE;
    }
    if(*argv[2] == '1')
    {
        mode = OUTPUT_ASCII;
    }
    if(*argv[2] == '2')
    {
        mode = OUTPUT_VISI;
    }
    
    
    char** board = (char**)malloc(height*sizeof(char*));
    char** next_board = (char**)malloc(height*sizeof(char*));
    for(int i = 0;i<height;i++)
    {
        board[i] = (char*)malloc(width*sizeof(char));
        next_board[i] = (char*)malloc(width*sizeof(char));
    }
    
    for(int i = 0; i < height; i ++)
    {
        for (int j = 0; j < width; j ++)
        {
            board[i][j] = '.';
            next_board[i][j] = '.';
        }
    }
    for(int x = 0; x<num_pairs; x++)
    {
        next = fscanf(file, "%d%d", &i, &j);
        board[i][j] = '@';
        next_board[i][j] = '@';
        total_live += 1;
    }
    if(next<0) // to avoid error
    {
        perror("ERROR");
    }

    data->board = board;
    data->next_board = next_board;
    data->rows = height;
    data->cols = width;
    data->iters = iter;
    data->output_mode = mode;

    //print_board(data, 0);
    return 0;
}

/* the gol application main loop function:
 *  runs rounds of GOL,
 *    * updates program state for next round (world and total_live)
 *    * performs any animation step based on the output/run mode
 *
 *   data: pointer to a struct gol_data  initialized with
 *         all GOL game playing state
 */
void play_gol(struct gol_data *data) 
{
    //data->image_buff = get_animation_buffer(data->handle);
    for(int x=1; x<=data->iters; x++)
    {
        for(int i = 0; i<data->rows; i++)
        {
            for(int j = 0; j < data->cols; j++)
            {
                if(data->board[i][j]== '@')
                {
                    if((neighbors(data, i, j) < 2) | (neighbors(data, i, j) > 3))
                    {
                        data->next_board[i][j] = '.';
                        total_live -= 1;
                    }
                }
                else
                {
                    if(neighbors(data, i, j) == 3)
                    {
                        data->next_board[i][j] = '@';
                        total_live += 1;
                    }
                }
            }
        }
        for(int i = 0; i < data->rows;i++)
        {
            for(int j = 0; j < data->cols; j++)
            {
                data->board[i][j] = data->next_board[i][j];
            }
        }
        if(data->output_mode == OUTPUT_ASCII)
        {
            system("clear");
            print_board(data, x);
            usleep(SLEEP_USECS);
        }
        if(data->output_mode == OUTPUT_VISI)
        {
            update_colors(data);
            draw_ready(data->handle);
            usleep(SLEEP_USECS);
        }
    }
}

/*
 * this function returns an int representing the number of alive neighbors
 * a coordinate has.
 * Parameters: data: game data; index: the coordinate whose neighbors we
 * are checking.
 * Returns: int: # of alive neighbors
 */
int neighbors(struct gol_data *data, int row, int col)
{
    int count = 0;
    int check_row, check_col;
    for(int i=-1; i<=1; i++)
    {
        for(int j=-1; j<=1; j++)
        {//print_board(data, x);
            if((i != 0) | (j != 0))
            {
                check_row = row+i;
                check_col = col+j;
                if (check_row < 0)
                {
                    check_row = data->rows - 1;
                }
                if(check_row >= data->rows)
                {
                    check_row = 0;
                }
                if(check_col < 0)
                {
                    check_col = data->cols - 1;
                }
                if(check_col >= data->cols)
                {
                    check_col = 0;
                }
                if(data->board[check_row][check_col] == '@')
                {
                    count++;
                }
            }
        }
    }
    return count;
}

/* Print the board to the terminal.
 *   data: gol game specific data
 *   round: the current round number
 */
void print_board(struct gol_data *data, int round) 
{
    int i, j;

    /* Print the round number. */
    fprintf(stderr, "Round: %d\n", round);

    for (i = 0; i < data->rows; ++i) 
    {
        for (j = 0; j < data->cols; ++j) 
        {
            if(data->board[i][j] == '@')
            {
                fprintf(stderr, " @");
            }
            else
            {
                fprintf(stderr, " .");
            }
        }
        fprintf(stderr, "\n");
    }
    /* Print the total number of live cells. */
    fprintf(stderr, "Live cells: %d\n\n", total_live);
}

/* Describes how the pixels in the image buffer should 
 * be colored based on data in the board
*/
void update_colors(struct gol_data *data) 
{
    int i, j, r, c, buff_i;
    color3 *buff;

    buff = data->image_buff;  // just for readability
    r = data->rows;
    c = data->cols;

    for (i = 0; i < r; i++) 
    {
        for (j = 0; j < c; j++) 
        {
            // translate row index to y-coordinate value because in
            // the image buffer, (r,c)=(0,0) is the _lower_ left but
            // in the grid, (r,c)=(0,0) is _upper_ left.
            buff_i = (r - (i+1))*c + j;

            // update animation buffer
            if (data->board[i][j] < 32) 
            {
                buff[buff_i] = c3_red;
            } 
            else if (data->board[i][j] < 64) 
            {
                buff[buff_i] = c3_green;
            } 
            else if (data->board[i][j] < 128) 
            {
                buff[buff_i] = c3_pink;
            } 
            else 
            {
                buff[buff_i] = c3_teal;
            }
        }
    }
}


/**********************************************************/
/***** START: DO NOT MODIFY THIS CODE *****/
/* initialize ParaVisi animation */
int setup_animation(struct gol_data* data) 
{
    /* connect handle to the animation */
    int num_threads = 1;
    data->handle = init_pthread_animation(num_threads, data->rows,
            data->cols, visi_name);
    if (data->handle == NULL) {
        printf("ERROR init_pthread_animation\n");
        exit(1);
    }
    // get the animation buffer
    data->image_buff = get_animation_buffer(data->handle);
    if(data->image_buff == NULL) {
        printf("ERROR get_animation_buffer returned NULL\n");
        exit(1);
    }
    return 0;
}

/* sequential wrapper functions around ParaVis library functions */
void (*mainloop)(struct gol_data *data);

void* seq_do_something(void * args)
{
    mainloop((struct gol_data *)args);
    return 0;
}

int connect_animation(void (*applfunc)(struct gol_data *data),
        struct gol_data* data)
{
    pthread_t pid;

    mainloop = applfunc;
    if( pthread_create(&pid, NULL, seq_do_something, (void *)data) ) {
        printf("pthread_created failed\n");
        return 1;
    }
    return 0;
}
/***** END: DO NOT MODIFY THIS CODE *****/
/******************************************************/
