// include file for screensaver template
#include "types.h"

int	g_iWidth;
int g_iHeight;
float g_fRatio;

CRGBA HSVtoRGB( float h, float s, float v );
void DrawRectangle(int x, int y, int w, int h, const CRGBA& dwColour);
void CreateGrid();
void reducePalette();

struct CUSTOMVERTEX
{
    float x, y, z; // The transformed position for the vertex.
    CRGBA color; // The vertex colour.
};
