#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#define SIZE 256
#define POINT_SPACING 128
typedef struct
{
int height;
int water_level;
int precipitation;
int sediment;
int wave_strength;
}terrain_cell_t;

terrain_cell_t terrain_buffer1[SIZE][SIZE];
terrain_cell_t terrain_buffer2[SIZE][SIZE];
terrain_cell_t (*terrain)[SIZE];
terrain_cell_t (*next_terrain)[SIZE];

void init_terrain()
{
int x,y;
    for(x=0;x<SIZE;x++)
    for(y=0;y<SIZE;y++)
    {
    terrain_buffer1[x][y].height=x>150?2000:0;
    terrain_buffer1[x][y].water_level=10;
    terrain_buffer2[x][y].height=x>150?2000:0;
    terrain_buffer2[x][y].water_level=10;
    }
terrain_buffer1[200][128].precipitation=10;
terrain=terrain_buffer1;
next_terrain=terrain_buffer2;
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
        pixels[x*4]=terrain[x][y].water_level>terrain[x][y].height?255:0;
        pixels[x*4+1]=terrain[x][y].height/4;
        pixels[x*4+2]=terrain[x][y].sediment;
        }
    pixels+=screen->pitch;
    }
SDL_UnlockSurface(screen);
}



const char neighbour_offsets[8][2]={{1,0},{0,1},{0,-1},{-1,0},{-1,1},{1,1},{1,-1},{-1,-1}};

//Limits the slope of terrain by allowing material to run down to lower elevations
#define MAX_GRADIENT POINT_SPACING

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
            if(i>3)
            {
            slope*=70;
            slope/=99;
            }
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
            int slope=terrain[x][y].height-terrain[x+neighbour_offsets[i][0]][y+neighbour_offsets[i][1]].height;
                if(i>3)
                {
                slope*=70;
                slope/=99;
                }
                if(slope==max_slope)
                {
                int diff=(slope-MAX_GRADIENT+1)/2;
                    if(transfer_square==0)
                    {
                    next_terrain[x+neighbour_offsets[i][0]][y+neighbour_offsets[i][1]].height+=diff;
                    next_terrain[x][y].height-=diff;
                    break;
                    }
                transfer_square--;
                }
            }
        }

    }
}

//Causes sediment to diffuse to neighbouring squares of a lower concentration
void diffuse_sediment()
{
int x,y,i;
    for(x=1;x<SIZE-1;x++)
    for(y=1;y<SIZE-1;y++)
    {
    int num_water_squares=0;
    //Randomize the order in which neighbours are checked to minimize bias
    int offset=rand()%8;
        for(i=0;i<8;i++)
        {
        int index=(i+offset)%8;
        terrain_cell_t* cur_cell=&terrain[x+neighbour_offsets[index][0]][y+neighbour_offsets[index][1]];
            if(cur_cell->water_level>cur_cell->height)
            {
            int delta=terrain[x][y].sediment-cur_cell->sediment;
                if(index>3)
                {
                delta*=70;
                delta/=99;
                }
                if(delta>0)
                {
                delta/=10;
                terrain[x][y].sediment-=delta;
                cur_cell->sediment+=delta;
                }
            }
        }
    }
}

//Calculates sediment deposition
void deposit_sediment()
{
int x,y;
    for(y=0;y<SIZE;y++)
    for(x=0;x<SIZE;x++)
    {
    int depth=terrain[x][y].water_level-terrain[x][y].height;
        if(depth<=0)continue;
        //if(terrain[x][y].wave_strength<10)
    int density=terrain[x][y].sediment/depth;
    int capacity=terrain[x][y].wave_strength;
        if(density>capacity)
        {
        int excess=terrain[x][y].sediment-(depth*capacity);
        terrain[x][y].sediment-=excess;
        terrain[x][y].height+=excess;
        }
    }
}

void wave_erosion()
{
int x,y;
    for(y=0;y<SIZE;y++)
    {
    int wave_strength=0;
        for(x=0;x<SIZE;x++)
        {
        terrain[x][y].wave_strength=wave_strength;
            if(terrain[x][y].water_level>terrain[x][y].height)wave_strength++;
            else if(wave_strength>0)
            {
            wave_strength/=20;
            terrain[x][y].height-=wave_strength;
            terrain[x-1][y].sediment+=wave_strength;
            wave_strength=0;
            }
        }
    }
}

void calculate_water()
{
int x,y;
    for(x=1;x<SIZE-1;x++)
    for(y=1;y<SIZE-1;y++)
    {
    next_terrain[x][y].water_level+=terrain[x][y].precipitation;
    }
}

void do_step()
{
angle_of_repose();
}

void flip_buffers()
{
int x,y;
terrain_cell_t (*temp)[SIZE];

    for(y=0;y<SIZE;y++)
    for(x=0;x<SIZE;x++)
    {
    terrain[x][y]=next_terrain[x][y];
    }

temp=terrain;
terrain=next_terrain;
next_terrain=temp;
}

int main(int argc,char* argv[])
{
SDL_Surface* screen=SDL_SetVideoMode(SIZE,SIZE,32,SDL_DOUBLEBUF);
    if(screen==NULL)return 1;
init_terrain();

    while(!SDL_GetKeyState(NULL)[SDLK_SPACE])
    {
    SDL_PumpEvents();
    draw_terrain(screen);
    do_step();
    flip_buffers();
    SDL_Flip(screen);
    }

return 0;
}
