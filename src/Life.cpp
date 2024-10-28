/*
 *  Copyright (C) 2016-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2004 Team XBMC
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

/*
 * Ver 1.0 2007-02-12 by Asteron  http://asteron.projects.googlepages.com/home
 */

#include <kodi/addon-instance/Screensaver.h>

#include "types.h"
#include <memory.h>
#include <stddef.h>
#ifdef WIN32
#include <d3d11.h>
#else
#include <kodi/gui/gl/GL.h>
#include <kodi/gui/gl/Shader.h>
#endif

struct CUSTOMVERTEX
{
  float x, y, z; // The transformed position for the vertex.
  CRGBA color; // The vertex colour.
};

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

#ifdef WIN32
ID3D11DeviceContext* g_pContext = nullptr;
ID3D11Buffer*        g_pVBuffer = nullptr;
ID3D11PixelShader*   g_pPShader = nullptr;
#endif

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

class ATTR_DLL_LOCAL CScreensaverBiogenesis
  : public kodi::addon::CAddonBase
  , public kodi::addon::CInstanceScreensaver
#ifndef WIN32
  , public kodi::gui::gl::CShaderProgram
#endif
{
public:
  CScreensaverBiogenesis();

  // kodi::addon::CInstanceScreensaver
  bool Start() override;
  void Stop() override;
  void Render() override;

#ifndef WIN32
  // kodi::gui::gl::CShaderProgram
  void OnCompiledAndLinked() override;
  bool OnEnabled() override { return true; };
#endif

private:
  static const int PALETTE_SIZE;
  static const int MAX_COLOR;

  Grid m_grid;
  int m_width;
  int m_height;
  float m_ratio;

  CRGBA randColor();
  void SetDefaults();
  void SeedGrid();
  void presetPalette();
  void CreateGrid();
  void reducePalette();
  void DrawGrid();
  void UpdateStates();
  void StepLifetime();
  void StepNeighbors();
  void StepColony();
  void Step();
  CRGBA HSVtoRGB( float h, float s, float v );
  void DrawRectangle(int x, int y, int w, int h, const CRGBA& dwColour);
#ifdef WIN32
  void InitDXStuff(void);
#else
  GLint m_aPosition = -1;
  GLint m_aColor = -1;
  GLuint m_vertexVBO;
  GLuint m_indexVBO;
#endif
};

const int CScreensaverBiogenesis::PALETTE_SIZE = sizeof(Grid::palette)/sizeof(CRGBA);
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

const int CScreensaverBiogenesis::MAX_COLOR = sizeof(COLOR_TIMES)/sizeof(CRGBA);


float frand() { return ((float) rand() / (float) RAND_MAX); }

////////////////////////////////////////////////////////////////////////////
// Kodi has loaded us into memory, we should set our core values
// here and load any settings we may have from our config file
//
CScreensaverBiogenesis::CScreensaverBiogenesis()
{
  m_grid.cells = nullptr;
  m_grid.fullGrid = nullptr;
  m_width = Width();
  m_height = Height();
  m_ratio = (float)m_width/(float)m_height;
  SetDefaults();
  CreateGrid();
#ifdef WIN32
  g_pContext = reinterpret_cast<ID3D11DeviceContext*>(Device());
  InitDXStuff();
#endif
}

// Kodi tells us we should get ready
// to start rendering. This function
// is called once when the screensaver
// is activated by Kodi.
bool CScreensaverBiogenesis::Start()
{
#ifndef WIN32
  std::string fraqShader = kodi::addon::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/frag.glsl");
  std::string vertShader = kodi::addon::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/vert.glsl");
  if (!LoadShaderFiles(vertShader, fraqShader) || !CompileAndLink())
  {
    kodi::Log(ADDON_LOG_ERROR, "Failed to create and compile shader");
    return false;
  }

  glGenBuffers(1, &m_vertexVBO);
  glGenBuffers(1, &m_indexVBO);
#endif

  SeedGrid();

  return true;
}

// Kodi tells us to render a frame of
// our screensaver. This is called on
// each frame render in Kodi, you should
// render a single frame only - the DX
// device will already have been cleared.
void CScreensaverBiogenesis::Render()
{
#ifndef WIN32
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
#else
  ID3D11RenderTargetView* renderTargetView;
  g_pContext->OMGetRenderTargets(1, &renderTargetView, nullptr);
  float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
  g_pContext->ClearRenderTargetView(renderTargetView, clearColor);
  SAFE_RELEASE(renderTargetView);
#endif


  if (m_grid.frameCounter++ == m_grid.resetTime)
    CreateGrid();
  Step();
  DrawGrid();
}

// Kodi tells us to stop the screensaver
// we should free any memory and release
// any resources we have created.
void CScreensaverBiogenesis::Stop()
{
  delete m_grid.fullGrid;
  m_grid.fullGrid = nullptr;
#ifdef WIN32
  SAFE_RELEASE(g_pPShader);
  SAFE_RELEASE(g_pVBuffer);
#else
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &m_vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &m_indexVBO);
#endif
}

CRGBA CScreensaverBiogenesis::randColor()
{
  float h=(float)(rand()%360), s = 0.3f + 0.7f*frand(), v=0.67f+0.25f*frand();
  if (m_grid.colorType == COLOR_NEIGHBORS || m_grid.colorType == COLOR_TIME)
  {
    s = 0.9f + 0.1f*frand();
    //v = 0.5f + 0.3f*frand();
  }
  return HSVtoRGB(h,s,v);
}

void CScreensaverBiogenesis::SetDefaults()
{
  m_grid.minSize = 50;
  m_grid.maxSize = 250;
  m_grid.spacing = 1;
  m_grid.resetTime = 2000;
  m_grid.presetChance = 30;
  m_grid.allowedColoring = 7;
  m_grid.cellLineLimit = 3;
}

void CScreensaverBiogenesis::SeedGrid()
{
  memset(m_grid.cells,0, m_grid.width*m_grid.height*sizeof(Cell));
  for ( int i = 0; i<m_grid.width*m_grid.height; i++ )
  {
    m_grid.cells[i].lifetime = 0;
    if (rand() % 4 == 0)
    {
      m_grid.cells[i].state = ALIVE;
      m_grid.cells[i].nextstate = m_grid.cells[i].state;
      if (m_grid.colorType == COLOR_TIME)
        m_grid.cells[i].color = m_grid.palette[m_grid.cells[i].lifetime];
      else
      {
        m_grid.cells[i].color = randColor();
      }
    }
  }
}

void CScreensaverBiogenesis::presetPalette()
{
  m_grid.palette[11] = CRGBA(0x22, 0x22, 0x22, 0xFF); 0xFF2222FF; //block

  m_grid.palette[2]  = CRGBA(0x66, 0x00, 0xFF, 0xFF);
  m_grid.palette[24] = CRGBA(0xFF, 0x33, 0xFF, 0xFF);

  m_grid.palette[12] = CRGBA(0xFF, 0x00, 0xAA, 0xFF);

  m_grid.palette[36] = CRGBA(0x00, 0x88, 0x00, 0xFF);
  m_grid.palette[5]  = CRGBA(0x00, 0xDD, 0xDD, 0xFF);

  m_grid.palette[10]  = CRGBA(0x00, 0x00, 0xAA, 0xFF);
  m_grid.palette[13]  = CRGBA(0xCC, 0x99, 0x00, 0xFF);

}

void CScreensaverBiogenesis::CreateGrid()
{
  int i, cellmin, cellmax;

  m_grid.minSize = kodi::addon::GetSettingInt("minsize");
  m_grid.maxSize = kodi::addon::GetSettingInt("maxsize");
  m_grid.resetTime = kodi::addon::GetSettingInt("resettime");
  m_grid.presetChance = kodi::addon::GetSettingInt("presetchance");
  m_grid.cellLineLimit = kodi::addon::GetSettingInt("lineminsize");

  if (!kodi::addon::GetSettingBoolean("colony"))
    m_grid.allowedColoring ^= (1 << COLOR_COLONY);
  if (!kodi::addon::GetSettingBoolean("lifetime"))
    m_grid.allowedColoring ^= (1 << COLOR_TIME);
  if (!kodi::addon::GetSettingBoolean("neighbour"))
    m_grid.allowedColoring ^= (1 << COLOR_NEIGHBORS);

  cellmin = (int)sqrt((float)(m_width*m_height/(int)(m_grid.maxSize*m_grid.maxSize*m_ratio)));
  cellmax = (int)sqrt((float)(m_width*m_height/(int)(m_grid.minSize*m_grid.minSize*m_ratio)));
  m_grid.cellSizeX = rand()%(cellmax - cellmin + 1) + cellmin;
  m_grid.cellSizeY = m_grid.cellSizeX > 5 ? (int)(m_ratio * m_grid.cellSizeX) : m_grid.cellSizeX;
  m_grid.width = m_width/m_grid.cellSizeX;
  m_grid.height = m_height/m_grid.cellSizeY;

  if (m_grid.cellSizeX <= m_grid.cellLineLimit )
    m_grid.spacing = 0;
  else m_grid.spacing = 1;


  if (m_grid.fullGrid)
    delete m_grid.fullGrid;
  m_grid.fullGrid = new Cell[m_grid.width*(m_grid.height+2)+2];
  memset(m_grid.fullGrid,0, (m_grid.width*(m_grid.height+2)+2) * sizeof(Cell));
  m_grid.cells = &m_grid.fullGrid[m_grid.width + 1];
  m_grid.frameCounter = 0;
  do
  {
    m_grid.colorType = rand()%3;
  } while (!(m_grid.allowedColoring & (1 << m_grid.colorType)) && m_grid.allowedColoring != 0);
  m_grid.ruleset = 0;

  for (i=0; i< PALETTE_SIZE; i++)
    m_grid.palette[i] = randColor();

  m_grid.maxColor = MAX_COLOR;
  if (m_grid.colorType == COLOR_TIME && (rand()%100 < m_grid.presetChance))
    for (i=0; i< MAX_COLOR; i++)
      m_grid.palette[i] = COLOR_TIMES[i];
  else
    m_grid.maxColor += (rand()%2)*(rand()%60);  //make it shimmer sometimes
  if (m_grid.colorType == COLOR_TIME && rand()%3)
  {
    for (i=m_grid.maxColor-1; i<PALETTE_SIZE; i++)
      m_grid.palette[i] = CRGBA::Lerp(m_grid.palette[m_grid.maxColor-1],m_grid.palette[PALETTE_SIZE-1],(float)(i-m_grid.maxColor+1)/(float)(PALETTE_SIZE-m_grid.maxColor));
    m_grid.maxColor = PALETTE_SIZE;
  }
  if (m_grid.colorType == COLOR_NEIGHBORS)
  {
    if (rand()%100 < m_grid.presetChance)
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
void CScreensaverBiogenesis::reducePalette()
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
    m_grid.palette[i] = m_grid.palette[inf];
  }
}

void CScreensaverBiogenesis::DrawGrid()
{
#ifdef WIN32
  g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  UINT strides = sizeof(CUSTOMVERTEX), offsets = 0;
  g_pContext->IASetVertexBuffers(0, 1, &g_pVBuffer, &strides, &offsets);
  g_pContext->PSSetShader(g_pPShader, NULL, 0);
#endif
  for(int i = 0; i<m_grid.width*m_grid.height; i++ )
    if (m_grid.cells[i].state != DEAD)
      DrawRectangle((i%m_grid.width)*m_grid.cellSizeX,(i/m_grid.width)*m_grid.cellSizeY,
        m_grid.cellSizeX - m_grid.spacing, m_grid.cellSizeY - m_grid.spacing, m_grid.cells[i].color);
}

void CScreensaverBiogenesis::UpdateStates()
{
  for(int i = 0; i<m_grid.width*m_grid.height; i++ )
    m_grid.cells[i].state = m_grid.cells[i].nextstate;
}

void CScreensaverBiogenesis::StepLifetime()
{
  int i;
  for(i = 0; i<m_grid.width*m_grid.height; i++ )
  {
    int count = 0;
    if(m_grid.cells[i-m_grid.width-1].state) count++;
    if(m_grid.cells[i-m_grid.width  ].state) count++;
    if(m_grid.cells[i-m_grid.width+1].state) count++;
    if(m_grid.cells[i           -1].state) count++;
    if(m_grid.cells[i           +1].state) count++;
    if(m_grid.cells[i+m_grid.width-1].state) count++;
    if(m_grid.cells[i+m_grid.width  ].state) count++;
    if(m_grid.cells[i+m_grid.width+1].state) count++;

    if(m_grid.cells[i].state == DEAD)
    {
      m_grid.cells[i].lifetime = 0;
      if (count == 3 || (m_grid.ruleset && count == 6))
      {
        m_grid.cells[i].nextstate = ALIVE;
        m_grid.cells[i].color = m_grid.palette[0];
      }
    }
    else
    {
      if (count == 2 || count == 3)
      {
        m_grid.cells[i].lifetime++;
        if (m_grid.cells[i].lifetime >= m_grid.maxColor)
          m_grid.cells[i].lifetime = m_grid.maxColor - 1;
        m_grid.cells[i].color = m_grid.palette[m_grid.cells[i].lifetime];
      }
      else
        m_grid.cells[i].nextstate = DEAD;
    }
  }
  UpdateStates();
}

void CScreensaverBiogenesis::StepNeighbors()
{
  UpdateStates();
  int i;
  for(i = 0; i<m_grid.width*m_grid.height; i++ )
  {
    int count = 0;
    int neighbors = 0;
    if(m_grid.cells[i-m_grid.width-1].state) {count++; neighbors |=  1;}
    if(m_grid.cells[i-m_grid.width  ].state) {count++; neighbors |=  2;}
    if(m_grid.cells[i-m_grid.width+1].state) {count++; neighbors |=  4;}
    if(m_grid.cells[i           -1].state) {count++; neighbors |=  8;}
    if(m_grid.cells[i           +1].state) {count++; neighbors |= 16;}
    if(m_grid.cells[i+m_grid.width-1].state) {count++; neighbors |= 32;}
    if(m_grid.cells[i+m_grid.width  ].state) {count++; neighbors |= 64;}
    if(m_grid.cells[i+m_grid.width+1].state) {count++; neighbors |=128;}

    if(m_grid.cells[i].state == DEAD)
    {
      if (count == 3 || (m_grid.ruleset && (neighbors == 0x7E || neighbors == 0xDB)))
      {
        m_grid.cells[i].nextstate = ALIVE;
        m_grid.cells[i].color = m_grid.palette[neighbors];
      }
    }
    else
    {
      if (count != 2 && count != 3)
        m_grid.cells[i].nextstate = DEAD;
      m_grid.cells[i].color = m_grid.palette[neighbors];
    }
  }
}

void CScreensaverBiogenesis::StepColony()
{
  CRGBA foundColors[8];
  int i;
  for(i = 0; i<m_grid.width*m_grid.height; i++ )
  {
    int count = 0;
    if(m_grid.cells[i-m_grid.width-1].state) foundColors[count++] = m_grid.cells[i-m_grid.width-1].color;
    if(m_grid.cells[i-m_grid.width  ].state) foundColors[count++] = m_grid.cells[i-m_grid.width  ].color;
    if(m_grid.cells[i-m_grid.width+1].state) foundColors[count++] = m_grid.cells[i-m_grid.width+1].color;
    if(m_grid.cells[i           -1].state) foundColors[count++] = m_grid.cells[i           -1].color;
    if(m_grid.cells[i           +1].state) foundColors[count++] = m_grid.cells[i           +1].color;
    if(m_grid.cells[i+m_grid.width-1].state) foundColors[count++] = m_grid.cells[i+m_grid.width-1].color;
    if(m_grid.cells[i+m_grid.width  ].state) foundColors[count++] = m_grid.cells[i+m_grid.width  ].color;
    if(m_grid.cells[i+m_grid.width+1].state) foundColors[count++] = m_grid.cells[i+m_grid.width+1].color;

    if(m_grid.cells[i].state == DEAD)
    {
      if (count == 3 || (m_grid.ruleset && count == 6))
      {
        if (foundColors[0] == foundColors[2])
          m_grid.cells[i].color = foundColors[0];
        else
          m_grid.cells[i].color = foundColors[1];
        m_grid.cells[i].nextstate = ALIVE;
      }
    }
    else if (count != 2 && count != 3)
      m_grid.cells[i].nextstate = DEAD;

  }
  UpdateStates();
}

void CScreensaverBiogenesis::Step()
{
  switch(m_grid.colorType)
  {
    case COLOR_COLONY:    StepColony(); break;
    case COLOR_TIME:    StepLifetime(); break;
    case COLOR_NEIGHBORS:  StepNeighbors(); break;
  }
}

CRGBA CScreensaverBiogenesis::HSVtoRGB( float h, float s, float v )
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

void CScreensaverBiogenesis::DrawRectangle(int x, int y, int w, int h, const CRGBA& dwColour)
{
  //Store each point of the triangle together with it's colour
  CUSTOMVERTEX cvVertices[] =
  {
    {(float)     x, (float) y + h, 0.0f, dwColour,},
    {(float)     x, (float)     y, 0.0f, dwColour,},
    {(float) x + w, (float) y + h, 0.0f, dwColour,},
    {(float) x + w, (float)     y, 0.0f, dwColour,},
  };
#ifndef WIN32
  EnableShader();

  GLfloat x1 = -1.0 + 2.0*x/m_width;
  GLfloat y1 = -1.0 + 2.0*y/m_height;
  GLfloat x2 = -1.0 + 2.0*(x+w)/m_width;
  GLfloat y2 = -1.0 + 2.0*(y+h)/m_height;

  struct PackedVertex
  {
    GLfloat x, y, z;
    GLfloat r, g, b;
  } vertex[4] = {{x1, y1, 0.0, dwColour.r, dwColour.g, dwColour.b},
                 {x2, y1, 0.0, dwColour.r, dwColour.g, dwColour.b},
                 {x2, y2, 0.0, dwColour.r, dwColour.g, dwColour.b},
                 {x1, y2, 0.0, dwColour.r, dwColour.g, dwColour.b}};

  GLubyte idx[] = {0, 1, 2, 2, 3, 0};

  glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex)*4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(m_aPosition, 3, GL_FLOAT, 0, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, x)));
  glVertexAttribPointer(m_aColor, 3, GL_FLOAT, 0, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, r)));

  glEnableVertexAttribArray(m_aPosition);
  glEnableVertexAttribArray(m_aColor);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte)*6, idx, GL_STATIC_DRAW);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(m_aPosition);
  glDisableVertexAttribArray(m_aColor);

  DisableShader();
#else
  D3D11_MAPPED_SUBRESOURCE res = {};
  if (SUCCEEDED(g_pContext->Map(g_pVBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
  {
    memcpy(res.pData, cvVertices, sizeof(cvVertices));
    g_pContext->Unmap(g_pVBuffer, 0);
  }
  g_pContext->Draw(4, 0);
#endif
}

#ifdef WIN32
const BYTE PixelShader[] =
{
     68,  88,  66,  67,  18, 124,
    182,  35,  30, 142, 196, 211,
     95, 130,  91, 204,  99,  13,
    249,   8,   1,   0,   0,   0,
    124,   1,   0,   0,   4,   0,
      0,   0,  48,   0,   0,   0,
    124,   0,   0,   0, 188,   0,
      0,   0,  72,   1,   0,   0,
     65, 111, 110,  57,  68,   0,
      0,   0,  68,   0,   0,   0,
      0,   2, 255, 255,  32,   0,
      0,   0,  36,   0,   0,   0,
      0,   0,  36,   0,   0,   0,
     36,   0,   0,   0,  36,   0,
      0,   0,  36,   0,   0,   0,
     36,   0,   0,   2, 255, 255,
     31,   0,   0,   2,   0,   0,
      0, 128,   0,   0,  15, 176,
      1,   0,   0,   2,   0,   8,
     15, 128,   0,   0, 228, 176,
    255, 255,   0,   0,  83,  72,
     68,  82,  56,   0,   0,   0,
     64,   0,   0,   0,  14,   0,
      0,   0,  98,  16,   0,   3,
    242,  16,  16,   0,   1,   0,
      0,   0, 101,   0,   0,   3,
    242,  32,  16,   0,   0,   0,
      0,   0,  54,   0,   0,   5,
    242,  32,  16,   0,   0,   0,
      0,   0,  70,  30,  16,   0,
      1,   0,   0,   0,  62,   0,
      0,   1,  73,  83,  71,  78,
    132,   0,   0,   0,   4,   0,
      0,   0,   8,   0,   0,   0,
    104,   0,   0,   0,   0,   0,
      0,   0,   1,   0,   0,   0,
      3,   0,   0,   0,   0,   0,
      0,   0,  15,   0,   0,   0,
    116,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      3,   0,   0,   0,   1,   0,
      0,   0,  15,  15,   0,   0,
    122,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      3,   0,   0,   0,   2,   0,
      0,   0,   3,   0,   0,   0,
    122,   0,   0,   0,   1,   0,
      0,   0,   0,   0,   0,   0,
      3,   0,   0,   0,   2,   0,
      0,   0,  12,   0,   0,   0,
     83,  86,  95,  80,  79,  83,
     73,  84,  73,  79,  78,   0,
     67,  79,  76,  79,  82,   0,
     84,  69,  88,  67,  79,  79,
     82,  68,   0, 171,  79,  83,
     71,  78,  44,   0,   0,   0,
      1,   0,   0,   0,   8,   0,
      0,   0,  32,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   3,   0,   0,   0,
      0,   0,   0,   0,  15,   0,
      0,   0,  83,  86,  95,  84,
     65,  82,  71,  69,  84,   0,
    171, 171
};

void CScreensaverBiogenesis::InitDXStuff(void)
{
  ID3D11Device* pDevice = nullptr;
  g_pContext->GetDevice(&pDevice);

  CD3D11_BUFFER_DESC vbDesc(sizeof(CUSTOMVERTEX) * 5, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  pDevice->CreateBuffer(&vbDesc, nullptr, &g_pVBuffer);

  pDevice->CreatePixelShader(PixelShader, sizeof(PixelShader), nullptr, &g_pPShader);

  SAFE_RELEASE(pDevice);
}
#else
void CScreensaverBiogenesis::OnCompiledAndLinked()
{
  // Variables passed directly to the Vertex shader
  m_aPosition = glGetAttribLocation(ProgramHandle(), "a_position");
  m_aColor = glGetAttribLocation(ProgramHandle(), "a_color");
}
#endif // WIN32

ADDONCREATOR(CScreensaverBiogenesis);
