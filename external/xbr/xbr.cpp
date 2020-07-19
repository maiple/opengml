// MIT License
//
// Hyllian's xBR-lv2 Shader, adapted from https://github.com/joseprio/xBRjs
//
// Copyright 2020 Josep del Rio
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "xbr.hpp"

#include <cmath>
#include <stdexcept>
#include <array>

// Options
//#define USE_3X_ORIGINAL_IMPLEMENTATION

const int REDMASK = 0x000000FF; // &MASK	>>0
const int GREENMASK = 0x0000FF00; // &MASK	>>8
const int BLUEMASK = 0x00FF0000; // &MASK	>>16
const int ALPHAMASK = 0xFF000000; // &MASK	>>24
const int THRESHHOLD_Y = 48;
const int THRESHHOLD_U = 7;
const int THRESHHOLD_V = 6;

typedef unsigned char byte;
typedef std::array<byte, 3> YUV;
typedef std::array<pixel, 21> ORI;
typedef int yuv_diff;

// Convert an ARGB byte to YUV
// See https://en.wikipedia.org/wiki/YUV
YUV getYuv(pixel p){

    byte r = (p & REDMASK);
    byte g = (p & GREENMASK) >> 8;
    byte b = (p & BLUEMASK) >> 16;
    byte y = r * .299000 + g * .587000 + b * .114000;
    byte u = r * -.168736 + g * -.331264 + b * .500000;
    byte v = r * .500000 + g * -.418688 + b * -.081312;
    return YUV{y, u, v};
}

yuv_diff yuvDifference(pixel A, pixel B, bool scaleAlpha){

    byte alphaA = ((A & ALPHAMASK) >> 24) & 0xff;
    byte alphaB = ((B & ALPHAMASK) >> 24) & 0xff;

    if(alphaA == 0 && alphaB == 0){
        return 0;
    }

    if(!scaleAlpha && (alphaA < 255 || alphaB < 255)){
        // Very large value not attainable by the thresholds
        return 1000000;
    }

    if(alphaA == 0 || alphaB == 0){
        // Very large value not attainable by the thresholds
        return 1000000;
    }

    YUV yuvA = getYuv(A);
    YUV yuvB = getYuv(B);

    /*Add HQx filters threshold & return*/
    return std::abs(yuvA[0] - yuvB[0]) * THRESHHOLD_Y
           + std::abs(yuvA[1] - yuvB[1]) * THRESHHOLD_U
           + std::abs(yuvA[2] - yuvB[2]) * THRESHHOLD_V;
}

bool isEqual(pixel A, pixel B, bool scaleAlpha){

    byte alphaA = ((A & ALPHAMASK) >> 24) & 0xff;
    byte alphaB = ((B & ALPHAMASK) >> 24) & 0xff;

    if(alphaA == 0 && alphaB == 0){
        return true;
    }

    if(!scaleAlpha && (alphaA < 255 || alphaB < 255)){
        return false;
    }

    if(alphaA == 0 || alphaB == 0){
        return false;
    }

    YUV yuvA = getYuv(A);
    YUV yuvB = getYuv(B);

    if(std::abs(yuvA[0] - yuvB[0]) > THRESHHOLD_Y){
        return false;
    }
    if(std::abs(yuvA[1] - yuvB[1]) > THRESHHOLD_U){
        return false;
    }
    if(std::abs(yuvA[2] - yuvB[2]) > THRESHHOLD_V){
        return false;
    }

    return true;
}

pixel pixelInterpolate(pixel A, pixel B, int q1, int q2){
    byte alphaA = ((A & ALPHAMASK) >> 24) & 0xff;
    byte alphaB = ((B & ALPHAMASK) >> 24) & 0xff;

    /*Extract each value from 32bit Uint & blend colors together*/
    pixel r, g, b, a;

    if(alphaA == 0){
        r = B & REDMASK;
        g = (B & GREENMASK) >> 8;
        b = (B & BLUEMASK) >> 16;
    }else if(alphaB == 0){
        r = A & REDMASK;
        g = (A & GREENMASK) >> 8;
        b = (A & BLUEMASK) >> 16;
    }else{
        r = (q2 * (B & REDMASK) + q1 * (A & REDMASK)) / (q1 + q2);
        g = (q2 * ((B & GREENMASK) >> 8) + q1 * ((A & GREENMASK) >> 8)) / (q1 + q2);
        b = (q2 * ((B & BLUEMASK) >> 16) + q1 * ((A & BLUEMASK) >> 16)) / (q1 + q2);
    }
    a = (q2 * alphaB + q1 * alphaA) / (q1 + q2);
    /*The bit hack '~~' is used to floor the values like std::floor, but faster*/
    return ((~~r) | ((~~g) << 8) | ((~~b) << 16) | ((~~a) << 24));
}

ORI getRelatedPoints(image oriPixelView, int oriX, int oriY, int oriW, int oriH){
    int xm1 = oriX - 1;
    if(xm1 < 0){
        xm1 = 0;
    }
    int xm2 = oriX - 2;
    if(xm2 < 0){
        xm2 = 0;
    }
    int xp1 = oriX + 1;
    if(xp1 >= oriW){
        xp1 = oriW - 1;
    }
    int xp2 = oriX + 2;
    if(xp2 >= oriW){
        xp2 = oriW - 1;
    }
    int ym1 = oriY - 1;
    if(ym1 < 0){
        ym1 = 0;
    }
    int ym2 = oriY - 2;
    if(ym2 < 0){
        ym2 = 0;
    }
    int yp1 = oriY + 1;
    if(yp1 >= oriH){
        yp1 = oriH - 1;
    }
    int yp2 = oriY + 2;
    if(yp2 >= oriH){
        yp2 = oriH - 1;
    }

    return {
            oriPixelView[xm1 + ym2 * oriW],  /* a1 */
            oriPixelView[oriX + ym2 * oriW], /* b1 */
            oriPixelView[xp1 + ym2 * oriW],  /* c1 */

            oriPixelView[xm2 + ym1 * oriW],  /* a0 */
            oriPixelView[xm1 + ym1 * oriW],  /* pa */
            oriPixelView[oriX + ym1 * oriW], /* pb */
            oriPixelView[xp1 + ym1 * oriW],  /* pc */
            oriPixelView[xp2 + ym1 * oriW],  /* c4 */

            oriPixelView[xm2 + oriY * oriW], /* d0 */
            oriPixelView[xm1 + oriY * oriW], /* pd */
            oriPixelView[oriX + oriY * oriW],/* pe */
            oriPixelView[xp1 + oriY * oriW], /* pf */
            oriPixelView[xp2 + oriY * oriW], /* f4 */

            oriPixelView[xm2 + yp1 * oriW],  /* g0 */
            oriPixelView[xm1 + yp1 * oriW],  /* pg */
            oriPixelView[oriX + yp1 * oriW], /* ph */
            oriPixelView[xp1 + yp1 * oriW],  /* pi */
            oriPixelView[xp2 + yp1 * oriW],  /* i4 */

            oriPixelView[xm1 + yp2 * oriW],  /* g5 */
            oriPixelView[oriX + yp2 * oriW], /* h5 */
            oriPixelView[xp1 + yp2 * oriW]   /* i5 */
    };
}

void kernel2Xv5(pixel pe, pixel pi, pixel ph, pixel pf, pixel pg, pixel pc,
                pixel pd, pixel pb, pixel f4, pixel i4, pixel h5, pixel i5, pixel& n1, pixel& n2, pixel& n3,
                bool blendColors, bool scaleAlpha);

void kernel3X(pixel pe, pixel pi, pixel ph, pixel pf, pixel pg, pixel pc, pixel pd, pixel pb,
              pixel f4, pixel i4, pixel h5, pixel i5, pixel& n2, pixel& n5,
              pixel& n6, pixel& n7, pixel& n8, bool blendColors, bool scaleAlpha);

void kernel4Xv2(pixel pe, pixel pi, pixel ph, pixel pf, pixel pg, pixel pc, pixel pd, pixel pb,
                pixel f4, pixel i4, pixel h5, pixel i5, pixel& n15, pixel& n14, pixel& n11,
                pixel& n3, pixel& n7, pixel& n10, pixel& n13, pixel& n12, bool blendColors, bool scaleAlpha);

// This is the XBR2x by Hyllian (see http://board.byuu.org/viewtopic.php?f=10&t=2248)
void computeXbr2x(image oriPixelView, int oriX, int oriY, int oriW, int oriH, image dstPixelView,
                  int dstX, int dstY, int dstW, bool blendColors, bool scaleAlpha){
    ORI relatedPoints = getRelatedPoints(oriPixelView, oriX, oriY, oriW, oriH);
    auto[a1, b1, c1, a0, pa, pb, pc, c4, d0, pd, pe, pf, f4, g0, pg, ph, pi, i4, g5, h5, i5] = relatedPoints;

    pixel e0, e1, e2, e3;
    e0 = e1 = e2 = e3 = pe;

    kernel2Xv5(pe, pi, ph, pf, pg, pc, pd, pb, f4, i4, h5, i5, e1, e2, e3, blendColors, scaleAlpha);
    kernel2Xv5(pe, pc, pf, pb, pi, pa, ph, pd, b1, c1, f4, c4, e0, e3, e1, blendColors, scaleAlpha);
    kernel2Xv5(pe, pa, pb, pd, pc, pg, pf, ph, d0, a0, b1, a1, e2, e1, e0, blendColors, scaleAlpha);
    kernel2Xv5(pe, pg, pd, ph, pa, pi, pb, pf, h5, g5, d0, g0, e3, e0, e2, blendColors, scaleAlpha);

    dstPixelView[dstX + dstY * dstW] = e0;
    dstPixelView[dstX + 1 + dstY * dstW] = e1;
    dstPixelView[dstX + (dstY + 1) * dstW] = e2;
    dstPixelView[dstX + 1 + (dstY + 1) * dstW] = e3;
}

void computeXbr3x(image oriPixelView, int oriX, int oriY, int oriW, int oriH, image dstPixelView,
                  int dstX, int dstY, int dstW, bool blendColors, bool scaleAlpha){
    ORI relatedPoints = getRelatedPoints(oriPixelView, oriX, oriY, oriW, oriH);
    auto[a1, b1, c1, a0, pa, pb, pc, c4, d0, pd, pe, pf, f4, g0, pg, ph, pi, i4, g5, h5, i5] = relatedPoints;

    pixel e0, e1, e2, e3, e4, e5, e6, e7, e8;
    e0 = e1 = e2 = e3 = e4 = e5 = e6 = e7 = e8 = pe;

    kernel3X(pe, pi, ph, pf, pg, pc, pd, pb, f4, i4, h5, i5, e2, e5, e6, e7, e8, blendColors, scaleAlpha);
    kernel3X(pe, pc, pf, pb, pi, pa, ph, pd, b1, c1, f4, c4, e0, e1, e8, e5, e2, blendColors, scaleAlpha);
    kernel3X(pe, pa, pb, pd, pc, pg, pf, ph, d0, a0, b1, a1, e6, e3, e2, e1, e0, blendColors, scaleAlpha);
    kernel3X(pe, pg, pd, ph, pa, pi, pb, pf, h5, g5, d0, g0, e8, e7, e0, e3, e6, blendColors, scaleAlpha);

    dstPixelView[dstX + dstY * dstW] = e0;
    dstPixelView[dstX + 1 + dstY * dstW] = e1;
    dstPixelView[dstX + 2 + dstY * dstW] = e2;
    dstPixelView[dstX + (dstY + 1) * dstW] = e3;
    dstPixelView[dstX + 1 + (dstY + 1) * dstW] = e4;
    dstPixelView[dstX + 2 + (dstY + 1) * dstW] = e5;
    dstPixelView[dstX + (dstY + 2) * dstW] = e6;
    dstPixelView[dstX + 1 + (dstY + 2) * dstW] = e7;
    dstPixelView[dstX + 2 + (dstY + 2) * dstW] = e8;
}


void computeXbr4x(image oriPixelView, int oriX, int oriY, int oriW, int oriH, image dstPixelView,
                  int dstX, int dstY, int dstW, bool blendColors, bool scaleAlpha){
    ORI relatedPoints = getRelatedPoints(oriPixelView, oriX, oriY, oriW, oriH);
    auto[a1, b1, c1, a0, pa, pb, pc, c4, d0, pd, pe, pf, f4, g0, pg, ph, pi, i4, g5, h5, i5] = relatedPoints;
    pixel e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, ea, eb, ec, ed, ee, ef;
    e0 = e1 = e2 = e3 = e4 = e5 = e6 = e7 = e8 = e9 = ea = eb = ec = ed = ee = ef = pe;

    kernel4Xv2(pe, pi, ph, pf, pg, pc, pd, pb, f4, i4, h5, i5, ef, ee, eb, e3, e7, ea, ed, ec, blendColors, scaleAlpha);
    kernel4Xv2(pe, pc, pf, pb, pi, pa, ph, pd, b1, c1, f4, c4, e3, e7, e2, e0, e1, e6, eb, ef, blendColors, scaleAlpha);
    kernel4Xv2(pe, pa, pb, pd, pc, pg, pf, ph, d0, a0, b1, a1, e0, e1, e4, ec, e8, e5, e2, e3, blendColors, scaleAlpha);
    kernel4Xv2(pe, pg, pd, ph, pa, pi, pb, pf, h5, g5, d0, g0, ec, e8, ed, ef, ee, e9, e4, e0, blendColors, scaleAlpha);

    dstPixelView[dstX + dstY * dstW] = e0;
    dstPixelView[dstX + 1 + dstY * dstW] = e1;
    dstPixelView[dstX + 2 + dstY * dstW] = e2;
    dstPixelView[dstX + 3 + dstY * dstW] = e3;
    dstPixelView[dstX + (dstY + 1) * dstW] = e4;
    dstPixelView[dstX + 1 + (dstY + 1) * dstW] = e5;
    dstPixelView[dstX + 2 + (dstY + 1) * dstW] = e6;
    dstPixelView[dstX + 3 + (dstY + 1) * dstW] = e7;
    dstPixelView[dstX + (dstY + 2) * dstW] = e8;
    dstPixelView[dstX + 1 + (dstY + 2) * dstW] = e9;
    dstPixelView[dstX + 2 + (dstY + 2) * dstW] = ea;
    dstPixelView[dstX + 3 + (dstY + 2) * dstW] = eb;
    dstPixelView[dstX + (dstY + 3) * dstW] = ec;
    dstPixelView[dstX + 1 + (dstY + 3) * dstW] = ed;
    dstPixelView[dstX + 2 + (dstY + 3) * dstW] = ee;
    dstPixelView[dstX + 3 + (dstY + 3) * dstW] = ef;
}

pixel alphaBlend32W(pixel dst, pixel src, bool blendColors){
    if(blendColors){
        return pixelInterpolate(dst, src, 7, 1);
    }

    return dst;
}

pixel alphaBlend64W(pixel dst, pixel src, bool blendColors){
    if(blendColors){
        return pixelInterpolate(dst, src, 3, 1);
    }
    return dst;
}

pixel alphaBlend128W(pixel dst, pixel src, bool blendColors){
    if(blendColors){
        return pixelInterpolate(dst, src, 1, 1);
    }
    return dst;
}

pixel alphaBlend192W(pixel dst, pixel src, bool blendColors){
    if(blendColors){
        return pixelInterpolate(dst, src, 1, 3);
    }
    return src;
}

pixel alphaBlend224W(pixel dst, pixel src, bool blendColors){
    if(blendColors){
        return pixelInterpolate(dst, src, 1, 7);
    }
    return src;
}

void left2_2X(pixel& n3, pixel& n2, pixel p, bool blendColors){
    n3 = alphaBlend192W(n3, p, blendColors);
    n2 = alphaBlend64W(n2, p, blendColors);
}

void up2_2X(pixel& n3, pixel& n1, pixel p, bool blendColors){
    n3 = alphaBlend192W(n3, p, blendColors);
    n1 = alphaBlend64W(n1, p, blendColors);
}

pixel dia_2X(pixel n3, pixel p, bool blendColors){
    return alphaBlend128W(n3, p, blendColors);
}

void kernel2Xv5(pixel pe, pixel pi, pixel ph, pixel pf, pixel pg, pixel pc,
                pixel pd, pixel pb, pixel f4, pixel i4, pixel h5, pixel i5, pixel& n1, pixel& n2, pixel& n3,
                bool blendColors, bool scaleAlpha){
    bool ex = (pe != ph && pe != pf);
    if(!ex){
        return;
    }
    yuv_diff e = (yuvDifference(pe, pc, scaleAlpha) +
               yuvDifference(pe, pg, scaleAlpha) +
               yuvDifference(pi, h5, scaleAlpha) +
               yuvDifference(pi, f4, scaleAlpha)) +
              (yuvDifference(ph, pf, 0) << 2);
    yuv_diff i = (yuvDifference(ph, pd, scaleAlpha) +
               yuvDifference(ph, i5, scaleAlpha) +
               yuvDifference(pf, i4, scaleAlpha) +
               yuvDifference(pf, pb, scaleAlpha)) +
              (yuvDifference(pe, pi, scaleAlpha) << 2);

    yuv_diff px = (yuvDifference(pe, pf, scaleAlpha) <= yuvDifference(pe, ph, scaleAlpha)) ? pf : ph;

    if((e < i) && (!isEqual(pf, pb, scaleAlpha) && !isEqual(ph, pd, scaleAlpha) ||
                   isEqual(pe, pi, scaleAlpha) && (!isEqual(pf, i4, scaleAlpha) && !isEqual(ph, i5, scaleAlpha)) ||
                   isEqual(pe, pg, scaleAlpha) || isEqual(pe, pc, scaleAlpha))){
        yuv_diff ke = yuvDifference(pf, pg, scaleAlpha);
        yuv_diff ki = yuvDifference(ph, pc, scaleAlpha);
        bool ex2 = (pe != pc && pb != pc);
        bool ex3 = (pe != pg && pd != pg);
        if(((ke << 1) <= ki) && ex3 || (ke >= (ki << 1)) && ex2){
            if(((ke << 1) <= ki) && ex3){
                left2_2X(n3, n2, px, blendColors);
            }
            if((ke >= (ki << 1)) && ex2){
                up2_2X(n3, n1, px, blendColors);
            }
        }else{
            n3 = dia_2X(n3, px, blendColors);
        }

    }else if(e <= i){
        n3 = alphaBlend64W(n3, px, blendColors);
    }
}

void leftUp2_3X(pixel& n7, pixel& n5, pixel& n6, pixel& n2, pixel& n8, pixel p, bool blendColors){
    pixel blendedN7 = alphaBlend192W(n7, p, blendColors);
    pixel blendedN6 = alphaBlend64W(n6, p, blendColors);
    n7 = blendedN7;
    n5 = blendedN7;
    n6 = blendedN6;
    n2 = blendedN6;
    n8 = p;
}

void left2_3X(pixel& n7, pixel& n5, pixel& n6, pixel& n8, pixel p, bool blendColors){
    n7 = alphaBlend192W(n7, p, blendColors);
    n5 = alphaBlend64W(n5, p, blendColors);
    n6 = alphaBlend64W(n6, p, blendColors);
    n8 = p;
}

void up2_3X(pixel& n5, pixel& n7, pixel& n2, pixel& n8, pixel p, bool blendColors){
    n5 = alphaBlend192W(n5, p, blendColors);
    n7 = alphaBlend64W(n7, p, blendColors);
    n2 = alphaBlend64W(n2, p, blendColors);
    n8 = p;
}

void dia_3X(pixel& n8, pixel& n5, pixel& n7, pixel p, bool blendColors){
    n8 = alphaBlend224W(n8, p, blendColors);
    n5 = alphaBlend32W(n5, p, blendColors);
    n7 = alphaBlend32W(n7, p, blendColors);
}

void kernel3X(pixel pe, pixel pi, pixel ph, pixel pf, pixel pg, pixel pc, pixel pd, pixel pb,
              pixel f4, pixel i4, pixel h5, pixel i5, pixel& n2, pixel& n5,
              pixel& n6, pixel& n7, pixel& n8, bool blendColors, bool scaleAlpha){
    bool ex = (pe != ph && pe != pf);
    if(!ex){
        return;
    }

    yuv_diff e = (yuvDifference(pe, pc, scaleAlpha) + yuvDifference(pe, pg, scaleAlpha) +
               yuvDifference(pi, h5, scaleAlpha) + yuvDifference(pi, f4, scaleAlpha)) +
              (yuvDifference(ph, pf, scaleAlpha) << 2);
    yuv_diff i = (yuvDifference(ph, pd, scaleAlpha) + yuvDifference(ph, i5, scaleAlpha) +
               yuvDifference(pf, i4, scaleAlpha) + yuvDifference(pf, pb, scaleAlpha)) +
              (yuvDifference(pe, pi, scaleAlpha) << 2);

    bool state;
#ifdef USE_3X_ORIGINAL_IMPLEMENTATION
    state = ((e < i) && (!isEqual(pf, pb, scaleAlpha) && !isEqual(ph, pd, scaleAlpha) || isEqual(pe, pi, scaleAlpha) && (!isEqual(pf, i4, scaleAlpha) && !isEqual(ph, i5, scaleAlpha)) || isEqual(pe, pg, scaleAlpha) || isEqual(pe, pc, scaleAlpha)));
#else
    state = ((e < i) &&
             (!isEqual(pf, pb, scaleAlpha) && !isEqual(pf, pc, scaleAlpha) ||
              !isEqual(ph, pd, scaleAlpha) && !isEqual(ph, pg, scaleAlpha) ||
              isEqual(pe, pi, scaleAlpha) &&
              (!isEqual(pf, f4, scaleAlpha) && !isEqual(pf, i4, scaleAlpha) ||
               !isEqual(ph, h5, scaleAlpha) && !isEqual(ph, i5, scaleAlpha)) ||
              isEqual(pe, pg, scaleAlpha) || isEqual(pe, pc, scaleAlpha)));
#endif

    if(state){
        yuv_diff ke = yuvDifference(pf, pg, scaleAlpha);
        yuv_diff ki = yuvDifference(ph, pc, scaleAlpha);
        bool ex2 = (pe != pc && pb != pc);
        bool ex3 = (pe != pg && pd != pg);
        pixel px = (yuvDifference(pe, pf, scaleAlpha) <= yuvDifference(pe, ph, scaleAlpha)) ? pf : ph;
        if(((ke << 1) <= ki) && ex3 && (ke >= (ki << 1)) && ex2){
            leftUp2_3X(n7, n5, n6, n2, n8, px, blendColors);
        }else if(((ke << 1) <= ki) && ex3){
            left2_3X(n7, n5, n6, n8, px, blendColors);
        }else if((ke >= (ki << 1)) && ex2){
            up2_3X(n5, n7, n2, n8, px, blendColors);
        }else{
            dia_3X(n8, n5, n7, px, blendColors);
        }
    }else if(e <= i){
        n8 = alphaBlend128W(n8, ((yuvDifference(pe, pf, scaleAlpha) <= yuvDifference(pe, ph, scaleAlpha)) ? pf : ph),
                            blendColors);
    }

}

// 4xBR
void leftUp2(pixel& n15, pixel& n14, pixel& n11, pixel& n13, pixel& n12, pixel& n10, pixel& n7, pixel& n3, pixel p,
             bool blendColors){

    pixel blendedN13 = alphaBlend192W(n13, p, blendColors);
    pixel blendedN12 = alphaBlend64W(n12, p, blendColors);
    n15 = p;
    n14 = p;
    n11 = p;
    n13 = blendedN12;
    n12 = blendedN12;
    n10 = blendedN12;
    n7 = blendedN13;
    //n3 = n3;

    //return {p, p, p, blendedN12, blendedN12, blendedN12, blendedN13, n3};
}

void left2(pixel& n15, pixel& n14, pixel& n11, pixel& n13, pixel& n12, pixel& n10, pixel p, bool blendColors){
    n15 = p;
    n14 = p;
    n11 = alphaBlend192W(n11, p, blendColors);
    n13 = alphaBlend192W(n13, p, blendColors);
    n12 = alphaBlend64W(n12, p, blendColors);
    n10 = alphaBlend64W(n10, p, blendColors);
}

void up2(pixel& n15, pixel& n14, pixel& n11, pixel& n3, pixel& n7, pixel& n10, pixel p, bool blendColors){
    n15 = p;
    n14 = alphaBlend192W(n14, p, blendColors);
    n11 = p;
    n3 = alphaBlend64W(n3, p, blendColors);
    n7 = alphaBlend192W(n7, p, blendColors);
    n10 = alphaBlend64W(n10, p, blendColors);
}

void dia(pixel& n15, pixel& n14, pixel& n11, pixel p, bool blendColors){
    n15 = p;
    n14 = alphaBlend128W(n14, p, blendColors);
    n11 = alphaBlend128W(n11, p, blendColors);
}

void kernel4Xv2(pixel pe, pixel pi, pixel ph, pixel pf, pixel pg, pixel pc, pixel pd, pixel pb,
                pixel f4, pixel i4, pixel h5, pixel i5, pixel& n15, pixel& n14, pixel& n11,
                pixel& n3, pixel& n7, pixel& n10, pixel& n13, pixel& n12, bool blendColors, bool scaleAlpha){
    bool ex = (pe != ph && pe != pf);
    if(!ex){
        return;
    }

    yuv_diff e = (yuvDifference(pe, pc, scaleAlpha) +
               yuvDifference(pe, pg, scaleAlpha) +
               yuvDifference(pi, h5, scaleAlpha) +
               yuvDifference(pi, f4, scaleAlpha)) +
              (yuvDifference(ph, pf, scaleAlpha) << 2);
    yuv_diff i = (yuvDifference(ph, pd, scaleAlpha) +
               yuvDifference(ph, i5, scaleAlpha) +
               yuvDifference(pf, i4, scaleAlpha) +
               yuvDifference(pf, pb, scaleAlpha)) +
              (yuvDifference(pe, pi, scaleAlpha) << 2);
    pixel px = (yuvDifference(pe, pf, scaleAlpha) <= yuvDifference(pe, ph, scaleAlpha)) ? pf : ph;
    if((e < i) && (!isEqual(pf, pb, scaleAlpha) && !isEqual(ph, pd, scaleAlpha) ||
                   isEqual(pe, pi, scaleAlpha) && (!isEqual(pf, i4, scaleAlpha) && !isEqual(ph, i5, scaleAlpha)) ||
                   isEqual(pe, pg, scaleAlpha) || isEqual(pe, pc, scaleAlpha))){

        yuv_diff ke = yuvDifference(pf, pg, scaleAlpha);
        yuv_diff ki = yuvDifference(ph, pc, scaleAlpha);
        bool ex2 = (pe != pc && pb != pc);
        bool ex3 = (pe != pg && pd != pg);
        if(((ke << 1) <= ki) && ex3 || (ke >= (ki << 1)) && ex2){
            if(((ke << 1) <= ki) && ex3){
                left2(n15, n14, n11, n13, n12, n10, px, blendColors);
            }
            if((ke >= (ki << 1)) && ex2){
                up2(n15, n14, n11, n3, n7, n10, px, blendColors);
            }
        }else{
            dia(n15, n14, n11, px, blendColors);
        }
    }else if(e <= i){
        n15 = alphaBlend128W(n15, px, blendColors);
    }
}

void xbr2x(image src, image dst, int width, int height, bool blendColors, bool scaleAlpha){
    for(size_t c = 0; c < width; c++){
        for(size_t d = 0; d < height; d++){
            computeXbr2x(src, c, d, width, height, dst, c * 2, d * 2, width * 2, blendColors, scaleAlpha);
        }
    }
}

void xbr3x(image src, image dst, int width, int height, bool blendColors, bool scaleAlpha){
    for(size_t c = 0; c < width; c++){
        for(size_t d = 0; d < height; d++){
            computeXbr3x(src, c, d, width, height, dst, c * 3, d * 3, width * 3, blendColors, scaleAlpha);
        }
    }
}

void xbr4x(image src, image dst, int width, int height, bool blendColors, bool scaleAlpha){
    for(size_t c = 0; c < width; c++){
        for(size_t d = 0; d < height; d++){
            computeXbr4x(src, c, d, width, height, dst, c * 4, d * 4, width * 4, blendColors, scaleAlpha);
        }
    }
}