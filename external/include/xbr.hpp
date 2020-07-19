#pragma once

typedef unsigned int pixel;
typedef pixel* image;

// width and height are dimensions of src image, which is assumed to be in RGBA format
// dst should be large enough to hold scaled up image (so should have dimensions width*2, height*2)
void xbr2x(image src, image dst, int width, int height, bool blendColors, bool scaleAlpha) ;

// width and height are dimensions of src image, which is assumed to be in RGBA format
// dst should be large enough to hold scaled up image (so should have dimensions width*3, height*3)
void xbr3x(image src, image dst, int width, int height, bool blendColors, bool scaleAlpha);

// width and height are dimensions of src image, which is assumed to be in RGBA format
// dst should be large enough to hold scaled up image (so should have dimensions width*4, height*4)
void xbr4x(image src, image dst, int width, int height, bool blendColors, bool scaleAlpha);