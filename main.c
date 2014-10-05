#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#define SIZE 256

typedef struct
{
int height;
}terrain_cell_t;

terrain_cell_t terrain[SIZE][SIZE];


void init_terrain()
{
int x,y;
    for(x=0;x<SIZE;x++)
    for(y=0;y<SIZE;y++)
    {
    terrain[x][y].height=100;
    }
}


void draw_terrain(SDL_Surface* screen)
{
SDL_LockSurface(screen);
Uint8* pixels=screen->pixels;
int y,x;
    for(y=0;y<SIZE;y++)
    {
        for(x=0;x<SIZE;x++)
        {
        pixels[x*4+1]=terrain[x][y].height;
        }
    pixels+=screen->pitch;
    }
SDL_UnlockSurface(screen);
}



const char neighbour_offsets[8][2]={{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0}};

//Limits the slope of terrain by allowing material to run down to lower elevations
#define MAX_GRADIENT 2

void angle_of_repose()
{
int x,y,i;
    for(x=1;x<SIZE-1;x++)
    for(y=1;y<SIZE-1;y++)
    {
    int max_slope=0;
    int number_equal=0;
        for(i=0;i<8;i++)
        {
        int slope=terrain[x][y].height-terrain[x+neighbour_offsets[i][0]][y+neighbour_offsets[i][1]].height;
            if(slope>max_slope)
            {
            max_slope=slope;
            number_equal=1;
            }
            else if(slope==max_slope)number_equal++;
        }

        if(max_slope>MAX_GRADIENT)
        {
        //Randomly select a square to transfer to
        int transfer_square=rand()%number_equal;//TODO-fix modulo bias
            for(i=0;i<8;i++)
            {
                if(terrain[x][y].height-terrain[x+neighbour_offsets[i][0]][y+neighbour_offsets[i][1]].height==max_slope)
                {
                    if(transfer_square==0)
                    {
                    terrain[x+neighbour_offsets[i][0]][y+neighbour_offsets[i][1]].height++;
                    terrain[x][y].height--;
                    break;
                    }
                transfer_square--;
                }
            }
        }

    }
}

void do_step()
{
terrain[100][101].height++;
terrain[100][100].height++;
terrain[101][101].height++;
terrain[101][100].height++;
angle_of_repose();
}

int main(int argc,char* argv[])
{
SDL_Surface* screen=SDL_SetVideoMode(SIZE,SIZE,32,SDL_DOUBLEBUF);
    if(screen==NULL)return 1;

    while(1)
    {
    SDL_PumpEvents();
    draw_terrain(screen);
    do_step();
    SDL_Flip(screen);
    }

getchar();
return 0;
}
