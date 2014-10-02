/*
 * BioGenesis Screensaver for XBox Media Center
 * Copyright (c) 2004 Team XBMC
 *
 * Ver 1.0 2007-02-12 by Asteron  http://asteron.projects.googlepages.com/home
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2  of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include <xbmc/xbmc_scr_dll.h>
#include <GL/gl.h>
#include "Life.h"
#include "types.h"
#include <memory.h>

struct Cell
{
  CRGBA color; // The cell color.
  short lifetime;
  char nextstate, state; 
};


#define DEAD 0
#define ALIVE 1
#define COLOR_TIME 0
#define COLOR_COLONY 1
#define COLOR_NEIGHBORS 2

struct Grid
{
  int minSize;
  int maxSize;
  int width;
  int height;
  int spacing;
  int resetTime;
  int cellSizeX;
  int cellSizeY;
  int colorType;
  int ruleset;
  int frameCounter;
  int maxColor;
  int presetChance;
  int allowedColoring;
  int cellLineLimit;
  CRGBA palette[800];
  Cell * cells;
  Cell * fullGrid;
};

Grid grid;
int PALETTE_SIZE = sizeof(grid.palette)/sizeof(CRGBA);
CRGBA COLOR_TIMES[] = {  
  CRGBA(30,30,200,255),
  CRGBA(120,10,255,255),
  CRGBA(50,100,250,255),
  CRGBA(0,250,200,255),
  CRGBA(60,250,40,255),
  CRGBA(244,200,40,255),
  CRGBA(250,100,30,255),
  CRGBA(255,10,20,255)
  };
int MAX_COLOR = sizeof(COLOR_TIMES)/sizeof(CRGBA);


float frand(){return ((float) rand() / (float) RAND_MAX);}

CRGBA randColor()
{
  float h=(float)(rand()%360), s = 0.3f + 0.7f*frand(), v=0.67f+0.25f*frand();
  if (grid.colorType == COLOR_NEIGHBORS || grid.colorType == COLOR_TIME)
  {
    s = 0.9f + 0.1f*frand();
    //v = 0.5f + 0.3f*frand();
  }
  return HSVtoRGB(h,s,v);
}

void SeedGrid()
{
  memset(grid.cells,0, grid.width*grid.height*sizeof(Cell));
  for ( int i = 0; i<grid.width*grid.height; i++ )
  {
    grid.cells[i].lifetime = 0;
    if (rand() % 4 == 0)
    {
      grid.cells[i].state = ALIVE;
      grid.cells[i].nextstate = grid.cells[i].state;
      if (grid.colorType == COLOR_TIME)
        grid.cells[i].color = grid.palette[grid.cells[i].lifetime];
      else
      {
        grid.cells[i].color = randColor();
      }
    }
  }
}


void presetPalette()
{
  grid.palette[11] = CRGBA(0x22, 0x22, 0x22, 0xFF); 0xFF2222FF; //block

  grid.palette[2]  = CRGBA(0x66, 0x00, 0xFF, 0xFF);
  grid.palette[24] = CRGBA(0xFF, 0x33, 0xFF, 0xFF);

  grid.palette[12] = CRGBA(0xFF, 0x00, 0xAA, 0xFF);
  
  grid.palette[36] = CRGBA(0x00, 0x88, 0x00, 0xFF);
  grid.palette[5]  = CRGBA(0x00, 0xDD, 0xDD, 0xFF);

  grid.palette[10]  = CRGBA(0x00, 0x00, 0xAA, 0xFF);
  grid.palette[13]  = CRGBA(0xCC, 0x99, 0x00, 0xFF);

}

void CreateGrid()
{
  int i, cellmin, cellmax;
  cellmin = (int)sqrt((float)(g_iWidth*g_iHeight/(int)(grid.maxSize*grid.maxSize*g_fRatio)));
  cellmax = (int)sqrt((float)(g_iWidth*g_iHeight/(int)(grid.minSize*grid.minSize*g_fRatio)));
  grid.cellSizeX = rand()%(cellmax - cellmin + 1) + cellmin;
  grid.cellSizeY = grid.cellSizeX > 5 ? (int)(g_fRatio * grid.cellSizeX) : grid.cellSizeX;
  grid.width = g_iWidth/grid.cellSizeX;
  grid.height = g_iHeight/grid.cellSizeY;

  if (grid.cellSizeX <= grid.cellLineLimit )
    grid.spacing = 0;
  else grid.spacing = 1;


  if (grid.fullGrid)
    delete grid.fullGrid;
  grid.fullGrid = new Cell[grid.width*(grid.height+2)+2]; 
  memset(grid.fullGrid,0, (grid.width*(grid.height+2)+2) * sizeof(Cell));
  grid.cells = &grid.fullGrid[grid.width + 1];
  grid.frameCounter = 0;
  do
  {
    grid.colorType = rand()%3;
  } while (!(grid.allowedColoring & (1 << grid.colorType)) && grid.allowedColoring != 0);
  grid.ruleset = 0;

  for (i=0; i< PALETTE_SIZE; i++)
    grid.palette[i] = randColor();

  grid.maxColor = MAX_COLOR;
  if (grid.colorType == COLOR_TIME && (rand()%100 < grid.presetChance))
    for (i=0; i< MAX_COLOR; i++)
      grid.palette[i] = COLOR_TIMES[i];
  else
    grid.maxColor += (rand()%2)*(rand()%60);  //make it shimmer sometimes
  if (grid.colorType == COLOR_TIME && rand()%3)
  {
    for (i=grid.maxColor-1; i<PALETTE_SIZE; i++)
      grid.palette[i] = CRGBA::Lerp(grid.palette[grid.maxColor-1],grid.palette[PALETTE_SIZE-1],(float)(i-grid.maxColor+1)/(float)(PALETTE_SIZE-grid.maxColor));
    grid.maxColor = PALETTE_SIZE;
  }
  if (grid.colorType == COLOR_NEIGHBORS)
  {
    if (rand()%100 < grid.presetChance)
      presetPalette();
    reducePalette();
  }  
  SeedGrid();
}



int * rotateBits(int * bits)
{
  int temp;
  temp = bits[0];
  bits[0] = bits[2];
  bits[2] = bits[7];
  bits[7] = bits[5];
  bits[5] = temp;  
  temp = bits[1];
  bits[1] = bits[4];
  bits[4] = bits[6];
  bits[6] = bits[3];
  bits[3] = temp;
  return bits;
}
int * flipBits(int * bits)
{
  int temp;
  temp = bits[0];
  bits[0] = bits[2];
  bits[2] = temp;
  temp = bits[3];
  bits[3] = bits[4];
  bits[4] = temp;
  temp = bits[5];
  bits[5] = bits[7];
  bits[7] = temp;
  return bits;
}
int packBits(int * bits)
{
  int packed = 0;
  for(int j = 0; j<8; j++)
    packed |= bits[j] << j;
  return packed;
}
void unpackBits(int num, int * bits)
{  
  for(int i=0; i<8; i++)
    bits[i] = (num & (1<<i))>>i ;
}
// This simplifies the neighbor palette based off of symmetry
void reducePalette()
{
  int i = 0, bits[8], inf, temp;
  for(i = 0; i < 256; i++)
  {
    inf = i;
    unpackBits(i, bits);
    for(int k = 0; k < 2; k++)
    {
      for(int j = 0; j<4; j++)
        if ((temp = packBits(rotateBits(bits))) < inf)
          inf = temp;
      flipBits(bits);
    }
    grid.palette[i] = grid.palette[inf];
  }
}



void DrawGrid()
{
  for(int i = 0; i<grid.width*grid.height; i++ )
    if (grid.cells[i].state != DEAD)
      DrawRectangle((i%grid.width)*grid.cellSizeX,(i/grid.width)*grid.cellSizeY, 
        grid.cellSizeX - grid.spacing, grid.cellSizeY - grid.spacing, grid.cells[i].color);
}

void UpdateStates()
{
  for(int i = 0; i<grid.width*grid.height; i++ )
    grid.cells[i].state = grid.cells[i].nextstate;
}

void StepLifetime()
{
  int i;
  for(i = 0; i<grid.width*grid.height; i++ )
  {
    int count = 0;
    if(grid.cells[i-grid.width-1].state) count++;
    if(grid.cells[i-grid.width  ].state) count++;
    if(grid.cells[i-grid.width+1].state) count++;
    if(grid.cells[i           -1].state) count++;
    if(grid.cells[i           +1].state) count++;
    if(grid.cells[i+grid.width-1].state) count++;
    if(grid.cells[i+grid.width  ].state) count++;
    if(grid.cells[i+grid.width+1].state) count++;

    if(grid.cells[i].state == DEAD)
    {
      grid.cells[i].lifetime = 0;
      if (count == 3 || (grid.ruleset && count == 6))
      {
        grid.cells[i].nextstate = ALIVE;
        grid.cells[i].color = grid.palette[0];
      }
    }
    else 
    {
      if (count == 2 || count == 3)
      {
        grid.cells[i].lifetime++;
        if (grid.cells[i].lifetime >= grid.maxColor)
          grid.cells[i].lifetime = grid.maxColor - 1;
        grid.cells[i].color = grid.palette[grid.cells[i].lifetime];
      }
      else
        grid.cells[i].nextstate = DEAD;
    }
  }
  UpdateStates();
}

void StepNeighbors()
{
  UpdateStates();
  int i;
  for(i = 0; i<grid.width*grid.height; i++ )
  {
    int count = 0;
    int neighbors = 0;
    if(grid.cells[i-grid.width-1].state) {count++; neighbors |=  1;}
    if(grid.cells[i-grid.width  ].state) {count++; neighbors |=  2;}
    if(grid.cells[i-grid.width+1].state) {count++; neighbors |=  4;}
    if(grid.cells[i           -1].state) {count++; neighbors |=  8;}
    if(grid.cells[i           +1].state) {count++; neighbors |= 16;}
    if(grid.cells[i+grid.width-1].state) {count++; neighbors |= 32;}
    if(grid.cells[i+grid.width  ].state) {count++; neighbors |= 64;}
    if(grid.cells[i+grid.width+1].state) {count++; neighbors |=128;}

    if(grid.cells[i].state == DEAD)
    {
      if (count == 3 || (grid.ruleset && (neighbors == 0x7E || neighbors == 0xDB)))
      {
        grid.cells[i].nextstate = ALIVE;
        grid.cells[i].color = grid.palette[neighbors];
      }
    }
    else 
    {
      if (count != 2 && count != 3)
        grid.cells[i].nextstate = DEAD;
      grid.cells[i].color = grid.palette[neighbors];
    }
  }
}


void StepColony()
{
  CRGBA foundColors[8];
  int i;
  for(i = 0; i<grid.width*grid.height; i++ )
  {
    int count = 0;
    if(grid.cells[i-grid.width-1].state) foundColors[count++] = grid.cells[i-grid.width-1].color;
    if(grid.cells[i-grid.width  ].state) foundColors[count++] = grid.cells[i-grid.width  ].color;
    if(grid.cells[i-grid.width+1].state) foundColors[count++] = grid.cells[i-grid.width+1].color;
    if(grid.cells[i           -1].state) foundColors[count++] = grid.cells[i           -1].color;
    if(grid.cells[i           +1].state) foundColors[count++] = grid.cells[i           +1].color;
    if(grid.cells[i+grid.width-1].state) foundColors[count++] = grid.cells[i+grid.width-1].color;
    if(grid.cells[i+grid.width  ].state) foundColors[count++] = grid.cells[i+grid.width  ].color;
    if(grid.cells[i+grid.width+1].state) foundColors[count++] = grid.cells[i+grid.width+1].color;

    if(grid.cells[i].state == DEAD)
    {
      if (count == 3 || (grid.ruleset && count == 6))
      {
        if (foundColors[0] == foundColors[2])
          grid.cells[i].color = foundColors[0];
        else 
          grid.cells[i].color = foundColors[1];
        grid.cells[i].nextstate = ALIVE;
      }
    }
    else if (count != 2 && count != 3)
      grid.cells[i].nextstate = DEAD;
    
  }
  UpdateStates();
}

void Step()
{
  switch(grid.colorType)
  {
    case COLOR_COLONY:    StepColony(); break;
    case COLOR_TIME:    StepLifetime(); break;
    case COLOR_NEIGHBORS:  StepNeighbors(); break;
  }
}

CRGBA HSVtoRGB( float h, float s, float v )
{
  int i;
  float f;
  int r, g, b, p, q, t, m;

  if( s == 0 ) { // achromatic (grey)
    r = g = b = (int)(255*v);
    return CRGBA(r,g,b,255);
  }

  h /= 60;      // sector 0 to 5
  i = (int)( h );
  f = h - i;      // frational part of h
  m = (int)(255*v);
  p = (int)(m * ( 1 - s ));
  q = (int)(m * ( 1 - s * f ));
  t = (int)(m * ( 1 - s * ( 1 - f ) ));
  

  switch( i ) {
    case 0: return CRGBA(m,t,p,255);
    case 1: return CRGBA(q,m,p,255);
    case 2: return CRGBA(p,m,t,255);
    case 3: return CRGBA(p,q,m,255);
    case 4: return CRGBA(t,p,m,255);
    default: break;    // case 5:
  }
  return CRGBA(m,p,q,255);
}

void DrawRectangle(int x, int y, int w, int h, const CRGBA& dwColour)
{
    //Store each point of the triangle together with it's colour
    CUSTOMVERTEX cvVertices[] =
    {
        {(float) x, (float) y+h, 0.0f, 0.5, dwColour,},
        {(float) x, (float) y, 0.0f, 0.5, dwColour,},
    {(float) x+w, (float) y+h, 0.0f, 0.5, dwColour,},
        {(float) x+w, (float) y, 0.0f, 0.5, dwColour,},
    };

    glBegin(GL_TRIANGLE_STRIP);
    for (size_t j=0;j<4;++j)
    {
      glColor3f(cvVertices[j].color.r/255.0, cvVertices[j].color.g/255.0, cvVertices[j].color.b/255.0);
      glVertex2f(cvVertices[j].x, cvVertices[j].y);
    }
    glEnd();
}

void SetDefaults()
{
  grid.minSize = 50;
  grid.maxSize = 250;
  grid.spacing = 1;
  grid.resetTime = 2000;
  grid.presetChance = 30;
  grid.allowedColoring = 7;
  grid.cellLineLimit = 3;
}

////////////////////////////////////////////////////////////////////////////
// XBMC has loaded us into memory, we should set our core values
// here and load any settings we may have from our config file
//
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  SCR_PROPS* scrprops = (SCR_PROPS*)props;

  g_iWidth = scrprops->width;
  g_iHeight  = scrprops->height;
  g_fRatio = (float)g_iWidth/(float)g_iHeight;
  SetDefaults();
  CreateGrid();

  return ADDON_STATUS_NEED_SETTINGS;
}

// XBMC tells us we should get ready
// to start rendering. This function
// is called once when the screensaver
// is activated by XBMC.
extern "C" void Start()
{
  SeedGrid();
}

// XBMC tells us to render a frame of
// our screensaver. This is called on
// each frame render in XBMC, you should
// render a single frame only - the DX
// device will already have been cleared.
extern "C" void Render()
{
  if (grid.frameCounter++ == grid.resetTime)
    CreateGrid();
  Step();
  DrawGrid();
}

// XBMC tells us to stop the screensaver
// we should free any memory and release
// any resources we have created.
extern "C" void ADDON_Stop()
{
  delete grid.fullGrid;
}

extern "C" void ADDON_Destroy()
{
}

extern "C" ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

extern "C" bool ADDON_HasSettings()
{
  return true;
}

extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

extern "C" ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void *value)
{
  if (strcmp(strSetting, "minsize") == 0)
    grid.minSize = *(int*)value;
  if (strcmp(strSetting, "maxsize") == 0)
    grid.maxSize = *(int*)value;
  if (strcmp(strSetting, "resettime") == 0)
    grid.resetTime = *(int*)value;
  if (strcmp(strSetting, "presetchance") == 0)
    grid.presetChance = 100*(*(float*)value);
  if (strcmp(strSetting, "lineminsize") == 0)
    grid.cellLineLimit = *(int*)value;
  if (strcmp(strSetting, "colony") == 0 && !*(bool*)value)
    grid.allowedColoring ^= (1 << COLOR_COLONY);
  if (strcmp(strSetting, "lifetime") == 0 && !*(bool*)value)
    grid.allowedColoring ^= (1 << COLOR_TIME);
  if (strcmp(strSetting, "neighbour") == 0 && !*(bool*)value)
    grid.allowedColoring ^= (1 << COLOR_NEIGHBORS);

  return ADDON_STATUS_OK;
}

extern "C" void ADDON_FreeSettings()
{
}

extern "C" void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

extern "C" void GetInfo(SCR_INFO *info)
{
}
