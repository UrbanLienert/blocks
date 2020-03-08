//
//  LightpadProgramm.cpp
//  Blocks
//
//  Created by Urban Lienert on 09.02.20.
//  Copyright © 2020 Urban Lienert. All rights reserved.
//

#include "LightpadProgram.hpp"

using namespace juce;

LightpadProgram::LightpadProgram (Block& b)  : Program (b) {}

String LightpadProgram::getLittleFootProgram()
{
    return R"littlefoot(
        
        #heapsize: 861
        
        //==============================================================================
        /*
           Heap layout:
        
           === Mode ===
        
           0     1 byte x 1   mode
                                    - 0 = Pd Logo
                                    - 1 = Drum Pads
                                    - 2 = Faders
                                    - 3 = X Y Z Touch
                                    - 4 = Drawing
           1     1 byte x 1   submode
                                    - 2 pads
                                    - 3 pads etc.
                                    
           === 25 x Pads / 5 x Faders ===
           
           2     4 byte x 25  colour
           102   4 byte x 5   values (for Faders)
           
           === 24 x Touch ===
           
           122   1 byte x 24  pad / fader index
           
           === 15 x 15 LEDs ===
           
           146   3 byte x 225 led colors
        
           === Mixer ===
           
           821   4 byte * 10  fader / buttons
           
        */
        //==============================================================================
        
        int numObjects;
        int objectWidth;
        int spaceWidth;
        int activeObjects;
        bool buttonTouch;
        
        void initialise() {
            activeObjects = 0;
            // fill colors
            int colIndex = 0;
            for (int i = 0; i < 25; i++) {
                int colour = 0xff87ceeb;
                if (colIndex==1) {
                    colour = 0xff98fb98;
                } else if (colIndex==2) {
                    colour = 0xffffe4e1;
                } else if (colIndex==3) {
                    colour = 0xff9370db;
                } else if (colIndex==4) {
                    colour = 0xfffffacd;
                } else if (colIndex==5) {
                    colour = 0xff008b8b;
                }
                colIndex ++;
                if (colIndex > 5)
                    colIndex = 0;
                setHeapInt(2 + i*4, 0xff000000 + colour);
            }
        }
        
        void repaint()
        {
            int mode = getHeapByte(0);
            if (mode==0) {
                drawLogo();
            } else if (mode==1) {
                drawDrumPads();
            } else if (mode==2) {
                drawFaders();
            } else if (mode==3) {
                drawXYZPad();
            } else if (mode==4) {
                drawPainting();
            } else if (mode==5) {
                drawMixer();
            }
        }
        
        int getNumObjects() {
            // number of pads, faders etc
            return getHeapByte(1);
        }
        
        void setColorForObject(int obj, int colour) {
            // color for pads, faders, etc
            setHeapInt(2 + obj*4, colour);
        }
        
        int getColorForObject(int obj) {
            return getHeapInt(2 + obj*4);
        }
        
        void setIndexForTouch(int objIndex, int touchIndex) {
            // value for object, faders, etc
            setHeapByte(122 + touchIndex - 1, objIndex);
        }
        
        int getIndexForTouch(int touchIndex) {
            return getHeapByte(122 + touchIndex - 1);
        }
        
        int getObjectIndex(float x, float y) {
            int row = int (y * (0.95 / 2.0) * float (numObjects)) + 1;
            int col = int (x * (0.95 / 2.0) * float (numObjects));
        
            return (numObjects * (numObjects-row)) + col;
        }
        
        void setObjectActive (int objIndex, bool active)
        {
            activeObjects = active ? (activeObjects | (1 << objIndex)) : (activeObjects & ~(1 << objIndex));
        }
        
        bool getObjectActive (int objIndex)
        {
            return activeObjects & (1 << objIndex);
        }
        
        bool anyObjectActive()
        {
            return activeObjects;
        }
        
        void setFaderValue(int objIndex, float value) {
            setHeapInt(102 + objIndex*4, int(value*1e6));
        }
        
        float getFaderValue(int objIndex) {
            return float(getHeapInt(102 + objIndex*4))/1e6;
        }
        
        void setMixerValue(int objIndex, float value) {
            setHeapInt(821 + objIndex*4, int(value*1e6));
        }
        
        float getMixerValue(int objIndex) {
            return float(getHeapInt(821 + objIndex*4))/1e6;
        }
        
        void recomputeObjectSizes() {
            numObjects = getNumObjects();
            objectWidth = 15 / numObjects;
            spaceWidth = 15 % numObjects;
            int numSpaces = numObjects - 1;
            if (numSpaces > 0)
                spaceWidth = (15 % numObjects) / numSpaces;
            else
                spaceWidth = 0;
        }
        
        
        
        void drawDrumPads() {
            if (getNumObjects()!=numObjects) {
                recomputeObjectSizes();
            }
            if (isConnectedToHost())
                drawPads();
        }
        
        void drawPads()
        {
            fillRect(0x000000, 0, 0, 15, 15);
            
            int padIndex = 0;
            int space = objectWidth + spaceWidth;
        
            for (int padY = numObjects -1 ; padY >= 0; --padY) {
                for (int padX = 0; padX < numObjects; ++padX) {
                    int colour = getColorForObject(padIndex);
                    if (getObjectActive(padIndex)) {
                        colour = 0xffffffff;
                    }
                    int dark = blendARGB (colour, 0xdd << 24);
                    int mid  = blendARGB (colour, (0xdd / 2) << 24);
                    blendGradientRect (colour, mid, dark, mid, padX * space, padY * space, objectWidth, objectWidth);
                    
                    ++padIndex;
                }
            }
        }
        
        void drawFaders() {
            if (getNumObjects()!=numObjects) {
                recomputeObjectSizes();
            }
            
            fillRect (0x000000, 0, 0, 15, 15);
            
            
            int faderIndex = 0;
            int space = objectWidth + spaceWidth;
            for (int faderX = 0; faderX < numObjects; ++faderX) {
                int colour = getColorForObject(faderIndex);
                int value = int(getFaderValue(faderIndex)*14) + 1;
                int dark = blendARGB (colour, 0xdd << 24);
                int mid  = blendARGB (colour, (0xdd / 2) << 24);
                blendGradientRect (colour, dark, dark, colour, faderX * space, 15-value, objectWidth, value);
                faderIndex++;
            }
        }
        
        void drawMixer() {
            if (getNumObjects()!=numObjects) {
                recomputeObjectSizes();
            }
            
            fillRect (0x000000, 0, 0, 15, 15);
            
            int space = objectWidth + spaceWidth;
            for (int faderX = 0; faderX < numObjects; ++faderX) {
                int colour = getColorForObject(faderX);
                int value = int(getMixerValue(faderX)*11) + 1;
                int dark = blendARGB (colour, 0xdd << 24);
                int mid  = blendARGB (colour, (0xdd / 2) << 24);
                blendGradientRect (colour, dark, dark, colour, faderX * space, 15-value, objectWidth, value);
            }
            
            for (int padX = 0; padX < numObjects; ++padX) {
                int colour = getColorForObject(padX+numObjects);
                int value = int(getMixerValue(padX+5));
                int dark = blendARGB (colour, 0xdd << 24);
                int mid  = blendARGB (colour, (0xdd / 2) << 24);
                blendGradientRect (colour, mid, dark, mid, padX * space, 0, objectWidth, 3);
                if (value==0) {
                    fillRect(0xff000000, padX * space + 1, 0, objectWidth, 3);
                }
            }
        }
        
        void drawXYZPad() {
            fillRect(0x000000, 0, 0, 15, 15);
            drawPressureMap();
            fadePressureMap();
            fillRect(0xffffff, 5, 7, 5, 1);
            fillRect(0xffffff, 7, 5, 1, 5);
            fillRect(0xffffff, 0, 0, 1, 15);
            fillRect(0xffffff, 14, 0, 1, 15);
            fillRect(0xffffff, 0, 0, 15, 1);
            fillRect(0xffffff, 0, 14, 15, 1);
        }
        
        void drawLED(int ledNr, int colour) {
            int red = (colour & 0x00ff0000) >> 16;
            int green = (colour & 0x0000ff00) >> 8;
            int blue = (colour & 0x000000ff);
            int byte = ledNr * 3 + 146;
            setHeapByte(byte, red);
            setHeapByte(byte + 1, green);
            setHeapByte(byte + 2, blue);
        }
        
        void blendLED(int ledNr, int colour) {
            int red = (colour & 0x00ff0000) >> 16;
            int green = (colour & 0x0000ff00) >> 8;
            int blue = (colour & 0x000000ff);
            int byte = ledNr * 3 + 146;
            red = red | getHeapByte(byte);
            green = green | getHeapByte(byte + 1);
            blue = blue | getHeapByte(byte + 2);
            setHeapByte(byte, red);
            setHeapByte(byte + 1, green);
            setHeapByte(byte + 2, blue);
        }
        
        void drawRect(int ledNr, int w, int h, int colour) {
            for (int i = ledNr; i<ledNr + w; ++i) {
                for (int j = 0; j<h; ++j) {
                    drawLED(i + j*15, colour);
                }
            }
        }
        
        void drawCircle(int cx, int cy, int r, int colour) {
            int red = (colour & 0x00ff0000) >> 16;
            int green = (colour & 0x0000ff00) >> 8;
            int blue = (colour & 0x000000ff);
            for (int y = 0; y < 15; ++y) {
                for (int x = 0; x < 15; ++x) {
                    int a = x - cx;
                    int b = y - cy;
                    int sqr = a*a + b*b;
                    if (sqr <= r*r) {
                        int c = makeARGB(0xff, red, green, blue);
                        drawLED(x + y*15, c);
                    } else {
                        int rm = r+1;
                        int max = rm*rm - r*r;
                        if (sqr < rm*rm) {
                            float diff = rm*rm - r*r;
                            float alpha =  (sqr - r*r) / diff;
                            alpha = alpha * 2.5;
                            int rDark = red - int(float(red)*alpha);
                            if (rDark<0) rDark = 0;
                            int bDark = blue - int(float(blue)*alpha);
                            if (bDark<0) bDark = 0;
                            int gDark = green - int(float(green)*alpha);
                            if (gDark<0) gDark = 0;
                            
                            int c = makeARGB(0xff, rDark, gDark, bDark);
                            blendLED(x + y*15, c);
                        }
                    }
                }
            }
        }
        
        void drawTriangle(int x, int y, int s, int deg, int c) {
            for (int a = 0; a < s; ++a) {
                for (int b = a; b < s; ++b) {
                    if (deg==0) drawLED(x+a/2 + (y+b-(a/2))*15, c); // 0
                    else if (deg==1) drawLED(x+(s-1)-a + (y+b)*15, c); // 45
                    else if (deg==2) drawLED(x+b-(a/2) + (y+a/2)*15, c); // 90
                    else if (deg==3) drawLED(x+a + (y+b)*15, c); // 135
                    else if (deg==4) drawLED(x+(s-1)/2-a/2 + (y+b-(a/2))*15, c); // 180
                    else if (deg==5) drawLED(x+(s-1)-b + (y+a)*15, c); // 225
                    else if (deg==6) drawLED(x+b-(a/2) + (y+(s-1)/2-a/2)*15, c); // 270
                    else if (deg==7) drawLED(x+b + (y+a)*15, c); // 315
                }
            }
        }
        
        void clearScreen() {
            for (int x = 0; x < 675; ++x) {
                int byte = x + 146;
                setHeapByte(byte, 0);
            }
        }
        
        void drawPainting() {
            for (int y = 0; y < 15; ++y) {
                for (int x = 0; x < 15; ++x) {
                    int byte = (x + y * 15) * 3 + 146;
                    int red = getHeapByte(byte);
                    int green = getHeapByte(byte + 1);
                    int blue = getHeapByte(byte + 2);
                    fillPixel (makeARGB(0xff, red, green, blue), x, y);
                }
            }
        }
        
        void touchStart(int index, float x, float y, float z, float vz) {
            int mode = getHeapByte(0);
            if (mode==1) {
                int padIndex = getObjectIndex(x, y);
                setIndexForTouch(padIndex, index);
                setObjectActive(padIndex, true);
            } else if (mode==2) {
                int faderIndex = getObjectIndex(x, 2.0);
                setIndexForTouch(faderIndex, index);
                float max = 1.931510272;
                float min = 0.071419848;
                float value = clamp(0.0, 1.0, 1 - (y-min)/(max-min));
                setFaderValue(faderIndex, value);
                int param1 = (10 << 26) + faderIndex;
                sendMessageToHost(param1, 1, int(value*1e6));
            } else if (mode==5) {
                float max = 1.931510272;
                float min = 0.531454406857143;
                if (y<min-0.2) {
                    int padIndex = getObjectIndex(x, 2.0);
                    bool value = bool(getMixerValue(padIndex+5));
                    setMixerValue(padIndex+5, float(!value));
                    buttonTouch = true;
                    int param1 = (11 << 26) + 0;
                    sendMessageToHost(param1, padIndex, int(!value));
                } else {
                    buttonTouch = false;
                    float value = clamp(0.0, 1.0, 1 - (y-min)/(max-min));
                    int faderIndex = getObjectIndex(x, 2.0);
                    setIndexForTouch(faderIndex, index);
                    setMixerValue(faderIndex, value);
                    int param1 = (11 << 26) + 1;
                    sendMessageToHost(param1, faderIndex, int(value*1e6));
                }
            }
        }
        
        void touchMove(int index, float x, float y, float z, float vz) {
            int mode = getHeapByte(0);
            if (mode==2) {
                int faderIndex = getIndexForTouch(index);
                float max = 1.931510272;
                float min = 0.071419848;
                float value = clamp(0.0, 1.0, 1 - (y-min)/(max-min));
                setFaderValue(faderIndex, value);
                int param1 = (10 << 26) + faderIndex;
                sendMessageToHost(param1, 2, int(value*1e6));
            } else if (mode==3) {
                addPressurePoint(0xff0000, x, y, z*10);
            } else if (mode==5) {
                float max = 1.931510272;
                float min = 0.531454406857143;
                if (y>=min-0.1 && !buttonTouch) {
                    float value = clamp(0.0, 1.0, 1 - (y-min)/(max-min));
                    int faderIndex = getIndexForTouch(index);
                    setMixerValue(faderIndex, value);
                    int param1 = (11 << 26) + 1;
                    sendMessageToHost(param1, faderIndex, int(value*1e6));
                }
            }
        }
        
        void touchEnd(int index, float x, float y, float z, float vz) {
            int mode = getHeapByte(0);
            if (mode==1) {
                int padIndex = getIndexForTouch(index);
                setObjectActive(padIndex, false);
            } else if (mode==2) {
                int faderIndex = getIndexForTouch(index);
                float max = 1.931510272;
                float min = 0.071419848;
                float value = clamp(0.0, 1.0, 1 - (y-min)/(max-min));
                int param1 = (10 << 26) + faderIndex;
                sendMessageToHost(param1, 0, int(value*1e6));
            }
        }
        
        void handleMessage (int param1, int param2, int param3) {
            int command = (param1 >> 26) & 0x3F;
            int subCommand = (param1 >> 18) & 0xFF;
            if (command==0) {
                if (subCommand==0) {
                    // set mode
                    setHeapByte(0, param3);
                } else if (subCommand==1) {
                    // set submode (grid size)
                    setHeapByte(1, param3);
                }
            } else if (command==1) {
                // set colors for 3 objects
                int color1 = ((param1 << 16) & 0x00ff0000) + ((param2 >> 16) & 0x0000ffff) + 0xff000000;
                int color2 = ((param2 << 8) & 0x00ffff00) + ((param3 >> 24) & 0x000000ff) + 0xff000000;
                int color3 = (param3 & 0x00ffffff) + 0xff000000;
                setColorForObject(subCommand, color1);
                setColorForObject(subCommand + 1, color2);
                setColorForObject(subCommand + 2, color3);
            } else if (command==2) {
                // set colors for 1 objects
                setColorForObject(subCommand, param3);
            } else if (command==3) {
                // set fader / button values
                float value = float(param3) / 1e6;
                if (param2==0) {
                    // fader
                    setFaderValue(subCommand, value);
                } else if (param2==1) {
                    // mixer buttons
                    setMixerValue(subCommand+5, value);
                } else if (param2==2) {
                    // mixer fader
                    setMixerValue(subCommand, value);
                }
            } else if (command==4) {
                // draw LED
                int colour = param3;
                drawLED(subCommand, colour);
            } else if (command==5) {
                // clear Screen
                clearScreen();
            } else if (command==6) {
                // draw rect
                drawRect(subCommand, param1 & 0xff, param2, param3);
            } else if (command==7) {
                // draw circle
                drawCircle(subCommand, param1 & 0xff, param2, param3);
            } else if (command==8) {
                // draw triangle
                drawTriangle(subCommand, param1 & 0xff, (param2 >> 16) & 0xff, param2 & 0xff, param3);
            }
            // send back message for confirmation
            sendMessageToHost(param1, 0 , 0);
        }
        
        void drawLogo() {
            int foreground = 0xffffff;
            int background = 0x000000;
            
            fillRect (background, 0, 0, 15, 15);
            fillPixel (foreground, 2, 3);
            fillPixel (foreground, 3, 3);
            fillPixel (foreground, 4, 3);
            fillPixel (foreground, 5, 3);
            fillPixel (foreground, 6, 3);
            fillPixel (foreground, 3, 4);
            fillPixel (foreground, 7, 4);
            fillPixel (foreground, 11, 4);
            fillPixel (foreground, 3, 5);
            fillPixel (foreground, 7, 5);
            fillPixel (foreground, 11, 5);
            fillPixel (foreground, 3, 6);
            fillPixel (foreground, 7, 6);
            fillPixel (foreground, 11, 6);
            fillPixel (foreground, 3, 7);
            fillPixel (foreground, 5, 7);
            fillPixel (foreground, 6, 7);
            fillPixel (foreground, 9, 7);
            fillPixel (foreground, 10, 7);
            fillPixel (foreground, 11, 7);
            fillPixel (foreground, 3, 8);
            fillPixel (foreground, 8, 8);
            fillPixel (foreground, 11, 8);
            fillPixel (foreground, 3, 9);
            fillPixel (foreground, 8, 9);
            fillPixel (foreground, 11, 9);
            fillPixel (foreground, 3, 10);
            fillPixel (foreground, 8, 10);
            fillPixel (foreground, 11, 10);
            fillPixel (foreground, 3, 11);
            fillPixel (foreground, 4, 11);
            fillPixel (foreground, 9, 11);
            fillPixel (foreground, 10, 11);
            fillPixel (foreground, 11, 11);
        }
    
    )littlefoot";
}

