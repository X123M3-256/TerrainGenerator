#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#define SIZE 256
#define CELL_WIDTH 128

#define SEDIMENT_CAPACITY 0.1
#define SEDIMENT_EROSION 0.01
#define SQR(x) ((x)*(x))

typedef struct
{
float height;
    struct
    {
    float depth;
    float outflow_flux[8];
    float velocity_x;
    float velocity_y;
    }water;

float precipitation;
float sediment;
int wave_strength;
}terrain_cell_t;

terrain_cell_t terrain[SIZE][SIZE];
terrain_cell_t next_terrain[SIZE][SIZE];


void init_terrain()
{
FILE* file=fopen("../TerrainGenerator/testing.data","r");
    if(file==NULL)exit(1337);
int x,y;
    for(x=0;x<SIZE;x++)
    for(y=0;y<SIZE;y++)
    {
    terrain[x][y].height=fgetc(file)*20;
    terrain[x][y].water.depth=0;//terrain[x][y].height<450?450-terrain[x][y].height:0;
    terrain[x][y].water.outflow_flux[0]=0;
    terrain[x][y].water.outflow_flux[1]=0;
    terrain[x][y].water.outflow_flux[2]=0;
    terrain[x][y].water.outflow_flux[3]=0;
    terrain[x][y].water.outflow_flux[4]=0;
    terrain[x][y].water.outflow_flux[5]=0;
    terrain[x][y].water.outflow_flux[6]=0;
    terrain[x][y].water.outflow_flux[7]=0;
    terrain[x][y].water.velocity_x=0;
    terrain[x][y].water.velocity_y=0;
    terrain[x][y].sediment=0;
    terrain[x][y].precipitation=0.1;
    }
fclose(file);

}


void draw_terrain(SDL_Surface* screen)
{
SDL_LockSurface(screen);
Uint8* pixels=screen->pixels;
int y,x;
    for(y=1;y<SIZE-1;y++)
    {
        for(x=1;x<SIZE-1;x++)
        {


        pixels[x*4]=terrain[x][y].water.depth;//terrain[x][y].water.velocity_x+terrain[x][y].water.velocity_y;//terrain[x][y].water.depth*5;
        pixels[x*4+1]=terrain[x][y].height/10.0;
        pixels[x*4+2]=terrain[x][y].sediment*1000;
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
#define TOP_LEFT 7
#define TOP_RIGHT 6
#define BOTTOM_LEFT 4
#define BOTTOM_RIGHT 5


//Limits the slope of terrain by allowing material to run down to lower elevations
#define MAX_GRADIENT CELL_WIDTH

void angle_of_repose()
{
int x,y,i;
    for(x=0;x<SIZE;x++)
    for(y=0;y<SIZE;y++)
    {
    next_terrain[x][y].height=terrain[x][y].height;
    }


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

    for(x=0;x<SIZE;x++)
    for(y=0;y<SIZE;y++)
    {
    terrain[x][y].height=next_terrain[x][y].height;
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
#define PIPE_AREA 10
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
        for(i=0;i<8;i++)
        {
        terrain_cell_t* neighbour=&terrain[x+neighbour_offsets[i][0]][y+neighbour_offsets[i][1]].height;
            if(i>3)cur_cell->water.outflow_flux[i]+=delta_t*PIPE_AREA*(9.81*(cur_cell->water.depth-neighbour->water.depth+cur_cell->height-neighbour->height))/PIPE_LENGTH;
            else cur_cell->water.outflow_flux[i]+=delta_t*PIPE_AREA*(9.81*(cur_cell->water.depth-neighbour->water.depth+cur_cell->height-neighbour->height))/(PIPE_LENGTH*1.414);
            if(cur_cell->water.outflow_flux[i]<0)cur_cell->water.outflow_flux[i]=0;
        total_flux+=cur_cell->water.outflow_flux[i];
        }
    //Compute the scale factor required to prevent negative depth
    float scale_factor=(cur_cell->water.depth*CELL_WIDTH*CELL_WIDTH)/(total_flux*delta_t);
        if(scale_factor>1||total_flux==0)scale_factor=1;
    //Scale flow values for each neighbour by scale factor
        for(i=0;i<8;i++)cur_cell->water.outflow_flux[i]*=scale_factor;
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
    delta_v+=terrain[x+1][y+1].water.outflow_flux[TOP_LEFT];
    delta_v+=terrain[x+1][y-1].water.outflow_flux[BOTTOM_LEFT];
    delta_v+=terrain[x-1][y+1].water.outflow_flux[TOP_RIGHT];
    delta_v+=terrain[x-1][y-1].water.outflow_flux[BOTTOM_RIGHT];
    delta_v-=cur_cell->water.outflow_flux[TOP];
    delta_v-=cur_cell->water.outflow_flux[BOTTOM];
    delta_v-=cur_cell->water.outflow_flux[LEFT];
    delta_v-=cur_cell->water.outflow_flux[RIGHT];
    delta_v-=cur_cell->water.outflow_flux[TOP_LEFT];
    delta_v-=cur_cell->water.outflow_flux[BOTTOM_LEFT];
    delta_v-=cur_cell->water.outflow_flux[TOP_RIGHT];
    delta_v-=cur_cell->water.outflow_flux[BOTTOM_RIGHT];
    delta_v*=delta_t;
    float average_depth=terrain[x][y].water.depth;
    cur_cell->water.depth+=delta_v/(CELL_WIDTH*CELL_WIDTH);
    average_depth=(average_depth+terrain[x][y].water.depth)/2;//Needed for next step

    //Now compute velocity field of water from flow rates
    float flow_x=(terrain[x-1][y].water.outflow_flux[RIGHT]-cur_cell->water.outflow_flux[LEFT]+cur_cell->water.outflow_flux[RIGHT]-terrain[x+1][y].water.outflow_flux[LEFT])/2.0;
    float flow_y=(terrain[x][y-1].water.outflow_flux[BOTTOM]-cur_cell->water.outflow_flux[TOP]+cur_cell->water.outflow_flux[BOTTOM]-terrain[x][y+1].water.outflow_flux[TOP])/2.0;
        if(average_depth==0)
        {
        terrain[x][y].water.velocity_x=0;
        terrain[x][y].water.velocity_y=0;
        }
        else
        {
        terrain[x][y].water.velocity_x=flow_x/(average_depth*CELL_WIDTH);
        terrain[x][y].water.velocity_y=flow_y/(average_depth*CELL_WIDTH);
        }

    }
}
void calculate_erosion(float delta_t)
{
int x,y;
    for(x=1;x<SIZE-1;x++)
    for(y=1;y<SIZE-1;y++)
    {
    float dx=(terrain[x-1][y].height+terrain[x+1][y].height)/CELL_WIDTH;
    float dy=(terrain[x][y-1].height+terrain[x][y+1].height)/CELL_WIDTH;
    float angle=atan(sqrt(SQR(dx)+SQR(dy)));
    float sediment_capacity=SEDIMENT_CAPACITY*sin(angle)*sqrt(SQR(terrain[x][y].water.velocity_x)+SQR(terrain[x][y].water.velocity_y))*terrain[x][y].water.depth;

    //printf("%d %d\n",x,y);
    float delta_s=SEDIMENT_EROSION*angle*(sediment_capacity-terrain[x][y].sediment);

    terrain[x][y].height-=delta_s;
    terrain[x][y].sediment=delta_s;
    }
}

float lerp(float a,float b,float u)
{
return a+(b-a)*u;
}

void transport_sediment(float delta_t)
{
    int x,y;
    for(x=10;x<SIZE-10;x++)
    for(y=10;y<SIZE-10;y++)
    {
    float prev_x_f=x-delta_t*terrain[x][y].water.velocity_x/CELL_WIDTH;
    float prev_y_f=y-delta_t*terrain[x][y].water.velocity_y/CELL_WIDTH;

    int prev_x=(int)prev_x_f;
    int prev_y=(int)prev_y_f;

    float dummy;
    float ux=modf(prev_x_f,&dummy);
    float uy=modf(prev_y_f,&dummy);

    float s1=0,s2=0,s3=0,s4=0;
    if(prev_x>0&&prev_x<SIZE&&prev_y>0&&prev_y<SIZE)s1=terrain[prev_x][prev_y].sediment;
    if(prev_x>-1&&prev_x<SIZE-1&&prev_y>0&&prev_y<SIZE)s2=terrain[prev_x+1][prev_y].sediment;
    if(prev_x>0&&prev_x<SIZE&&prev_y>-1&&prev_y<SIZE-1)s3=terrain[prev_x][prev_y+1].sediment;
    if(prev_x>-1&&prev_x<SIZE-1&&prev_y>-1&&prev_y<SIZE-1)s4=terrain[prev_x+1][prev_y+1].sediment;

    next_terrain[x][y].sediment=lerp(lerp(s1,s2,ux),lerp(s3,s4,ux),uy);
    }

    for(x=0;x<SIZE;x++)
    for(y=0;y<SIZE;y++)
    {
    terrain[x][y].sediment=next_terrain[x][y].sediment;
    }
}

void do_step()
{
angle_of_repose();
calculate_precipitation(1);
calculate_water(1);
calculate_erosion(1);
//transport_sediment(1);
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

    int x,y;
    for(y=0;y<SIZE;y++)
    for(x=0;x<SIZE;x++)
    {
    float val=terrain[x][y].height;
    write(STDOUT_FILENO,&val,sizeof(float));
    val=terrain[x][y].water.depth;
    write(STDOUT_FILENO,&val,sizeof(float));
    }

    }
return 0;
}
