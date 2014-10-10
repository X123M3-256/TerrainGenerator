#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#define SIZE 256
#define CELL_WIDTH 128

typedef struct
{
float height;
    struct
    {
    float depth;
    float outflow_flux[4];
    float velocity_x;
    float velocity_y;
    }water;

double precipitation;
int sediment;
int wave_strength;
}terrain_cell_t;

terrain_cell_t terrain_buffer1[SIZE][SIZE];
terrain_cell_t terrain_buffer2[SIZE][SIZE];
terrain_cell_t (*terrain)[SIZE];
terrain_cell_t (*next_terrain)[SIZE];

void init_terrain()
{
FILE* file=fopen("mountains.data","r");
int x,y;
    for(x=0;x<SIZE;x++)
    for(y=0;y<SIZE;y++)
    {
    terrain_buffer1[x][y].height=fgetc(file);
    terrain_buffer1[x][y].water.depth=(x==0||x==SIZE-1||y==0||y==SIZE-1)?100:0;
    terrain_buffer1[x][y].water.outflow_flux[0]=0;
    terrain_buffer1[x][y].water.outflow_flux[1]=0;
    terrain_buffer1[x][y].water.outflow_flux[2]=0;
    terrain_buffer1[x][y].water.outflow_flux[3]=0;
    terrain_buffer1[x][y].precipitation=0.0001;
    terrain_buffer2[x][y]=terrain_buffer1[x][y];
    }
fclose(file);


//terrain_buffer1[150][150].precipitation=15;
//terrain_buffer2[150][150].precipitation=15;

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
        pixels[x*4]=terrain[x][y].water.depth*10;//>0?255:0;//(terrain[x][y].water_depth+terrain[x][y].height)/4:0;//>terrain[x][y].height?255:0;
        pixels[x*4+1]=terrain[x][y].water.depth>0.01?0:terrain[x][y].height;
        pixels[x*4+2]=terrain[x][y].sediment;
        }
    pixels+=screen->pitch;
    }
SDL_UnlockSurface(screen);
}



const char neighbour_offsets[8][2]={{1,0},{0,1},{0,-1},{-1,0},{-1,1},{1,1},{1,-1},{-1,-1}};
#define LEFT 3
#define RIGHT 0
#define TOP 2
#define BOTTOM 1


//Limits the slope of terrain by allowing material to run down to lower elevations
#define MAX_GRADIENT CELL_WIDTH

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
/*
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
            if(cur_cell->water_depth>0)
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

/*
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
*/


void calculate_precipitation(float delta_t)
{
int x,y,i;
    for(x=1;x<SIZE-1;x++)
    for(y=1;y<SIZE-1;y++)
    {
    terrain[x][y].water.depth+=delta_t*terrain[x][y].precipitation;
    }
}
#define PIPE_AREA 100
#define PIPE_LENGTH 1
void calculate_water(float delta_t)
{
int x,y,i;
    for(x=1;x<SIZE-1;x++)
    for(y=1;y<SIZE-1;y++)
    {
    terrain_cell_t* cur_cell=&terrain[x][y];
    //Compute the flow rate from the current cell to each of it's neighbours
    float total_flux=0;
        for(i=0;i<4;i++)
        {
        terrain_cell_t* neighbour=&terrain[x+neighbour_offsets[i][0]][y+neighbour_offsets[i][1]].height;
        cur_cell->water.outflow_flux[i]+=delta_t*PIPE_AREA*(9.81*(cur_cell->water.depth-neighbour->water.depth+cur_cell->height-neighbour->height))/PIPE_LENGTH;
            if(cur_cell->water.outflow_flux[i]<0)cur_cell->water.outflow_flux[i]=0;
        total_flux+=cur_cell->water.outflow_flux[i];
        }
    //Compute the scale factor required to prevent negative depth
    float scale_factor=(cur_cell->water.depth*CELL_WIDTH*CELL_WIDTH)/(total_flux*delta_t);
        if(scale_factor>1||total_flux==0)scale_factor=1;
    //Scale flow values for each neighbour by scale factor
        for(i=0;i<4;i++)cur_cell->water.outflow_flux[i]*=scale_factor;
    }

    for(x=1;x<SIZE-1;x++)
    for(y=1;y<SIZE-1;y++)
    {
    terrain_cell_t* cur_cell=&terrain[x][y];
    //Update the water height for each cell based on flow rates between it and it's neighbours
    float delta_v=0;//V stands for volume. Not velocity
    delta_v+=terrain[x][y+1].water.outflow_flux[TOP];
    delta_v+=terrain[x][y-1].water.outflow_flux[BOTTOM];
    delta_v+=terrain[x+1][y].water.outflow_flux[LEFT];
    delta_v+=terrain[x-1][y].water.outflow_flux[RIGHT];
    delta_v-=cur_cell->water.outflow_flux[TOP];
    delta_v-=cur_cell->water.outflow_flux[BOTTOM];
    delta_v-=cur_cell->water.outflow_flux[LEFT];
    delta_v-=cur_cell->water.outflow_flux[RIGHT];
    delta_v*=delta_t;
    float average_depth=terrain[x][y].water.depth;
    terrain[x][y].water.depth=cur_cell->water.depth+(delta_v/(CELL_WIDTH*CELL_WIDTH));
    average_depth=(average_depth+terrain[x][y].water.depth)/2;//Needed for next step

    //Now compute velocity field of water from flow rates
    float flow_x=(terrain[x-1][y].water.outflow_flux[RIGHT]-cur_cell->water.outflow_flux[LEFT]+cur_cell->water.outflow_flux[RIGHT]-terrain[x+1][y].water.outflow_flux[LEFT])/2.0;
    float flow_y=(terrain[x][y-1].water.outflow_flux[BOTTOM]-cur_cell->water.outflow_flux[TOP]+cur_cell->water.outflow_flux[BOTTOM]-terrain[x][y+1].water.outflow_flux[TOP])/2.0;
    terrain[x][y].water.velocity_x=flow_x/(average_depth*CELL_WIDTH);
    terrain[x][y].water.velocity_y=flow_y/(average_depth*CELL_WIDTH);
    }
}

void do_step()
{
//angle_of_repose();
calculate_precipitation(1);
calculate_water(1);
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
    SDL_Flip(screen);
    }

return 0;
}
