#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>
#include "gamelib.h"
#include "milky_way.c"
#include "text.h"
#define ASTEROID_COUNT 10

static const char *device = "/dev/fb1";

typedef struct { // axis-aligned bounding box
    int x, y, w, h, xVel, yVel;
}rect;

typedef struct { // mine
    int x, y, t;
}mine;

void rectUpdate( rect* p ){
    p->x = p->x + p->xVel;
    p->y = p->y + p->yVel;
    if( p->x <= 0 ){// && p->xVel < 0 ){
        p->x = GRAPHICS_WIDTH;
    }else if( (p->x+p->w) > GRAPHICS_WIDTH){// && p->xVel>0){
        p->x = 0;
    }
    if( p->y <= 0){// && p->yVel < 0 ){
        p->y = GRAPHICS_HEIGHT;
    }else if( (p->y+p->h) > GRAPHICS_HEIGHT){// && p->yVel>0 ){
        p->y = 0;
    }
}

typedef struct{
    int xp[7];
    int yp[7];
    int x, y, w, h;
    int xVel, yVel;
    uint16_t c;
}asteroid;

// Initializes all the pixels of a frame to the same color
void initFrame(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], uint16_t color)
{
    int xPos, yPos;
    for( xPos = 0; xPos < GRAPHICS_WIDTH; xPos++){
        for( yPos = 0; yPos < GRAPHICS_HEIGHT; yPos++){
            frame[yPos][xPos] = color;
}   }   }

// Draws a rectangle on the screen, checking for edge cases where the rectangle could be partially off screen.
void drawRectangle(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], int x, int y, int height, int width, uint16_t color)
{
    if( x<0 )
        x=0;
    if( y<0 )
        y=0;
    int xPos,yPos;
    for( xPos = x; xPos<x+width && xPos<GRAPHICS_WIDTH; xPos++ ){
        for( yPos = y; yPos<y+height && yPos<GRAPHICS_HEIGHT; yPos++ ){
            frame[yPos][xPos] = color;
}   }   }

// Draws a rect on the screen
void drawRect( uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], rect r, uint16_t color ){
    drawRectangle( frame, r.x, r.y, r.h, r.w, color );
}

float absVal( float f ){
    if( f>0 ){
        return f;
    }
    return -f;
}

// Bresenham's Line algorithm
void drawLine( uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], int x0, int y0, int x1, int y1, uint16_t color ){
    float dx = x1-x0;
    float dy = y1-y0;
    // compare dx and dy to see if slope is steep ( > .5 )
    if( absVal(dy) > absVal(dx) ){ // slope steep
        float xChangePerStep = dx / dy;
        int y; // go ahead and initialize for both for loops
        if( y0 < y1 ){ // iterate from y0 to y1
            float x = x0;
            for( y = y0; y<=y1; y++ ){
                drawRectangle( frame, (int)x, y, 1, 1, color );
                x = x + xChangePerStep;
            }
        }else{ // iterate from y1 to y0
            float x = x1;
            for( y = y1; y<=y0; y++ ){
                drawRectangle( frame, (int)x, y, 1, 1, color );
                x = x + xChangePerStep;
            }
        }
    } else { // not steep
        float yChangePerStep = dy / dx;
        int x;
        if( x0 < x1 ){ // iterate from x0 to x1
            float y = y0;
            for( x=x0; x<=x1; x++ ){
                drawRectangle( frame, x, (int)y, 1, 1, color );
                y = y + yChangePerStep;
            }
        }else{ // iterate from x1 to x0
            float y = y1;
            for( x=x1; x<=x0; x++ ){
                drawRectangle( frame, x, (int)y, 1, 1, color );
                y = y + yChangePerStep;

}   }   }   }

void drawAsteroid( uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], asteroid a ){ //////// ADD COLOR
    int i;
    for( i=0; i<7; i++ ){
        // frame[ a.yp[i]+a.y ][ a.xp[i]+a.x ] = a.c;
        drawLine( frame, a.x+a.xp[i], a.y+a.yp[i], a.x+a.xp[(i+1)%7], a.y+a.yp[(i+1)%7], a.c );
}   }

void updateAsteroid( asteroid* a ){ // asteroids bounce (dont wrap)
    //if( a->x<-20 || a->x>(GRAPHICS_WIDTH+20) ){ for wrapping
    if( a->x<0 || (a->x+a->w) > GRAPHICS_WIDTH ){
        a->xVel = - a->xVel;
    }
    //if( a->y<-20 || a->y>(GRAPHICS_HEIGHT+20) ){ for wrapping
    if( a->y<0 || (a->y+a->h) > GRAPHICS_HEIGHT ){
        a->yVel = -a->yVel;
    }
    a->x = a->x + a->xVel;
    a->y = a->y + a->yVel;
}

// Draws an image to the screen. Note that the frame stores uint16_t, while the pixels are given in uint8_t.
void drawImage(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], int x, int y, int height, int width, const uint8_t *pixels)
{
    int imageWidth = gimp_image.width;
    int imageHeight = gimp_image.height;

    uint16_t* p = (uint16_t*) pixels;

    for( int xPos = x; xPos<x+width && xPos<GRAPHICS_WIDTH && xPos-x<imageWidth; xPos++){
        for( int yPos = y; yPos<y+height && yPos<GRAPHICS_HEIGHT && yPos-y<imageHeight; yPos++){
            frame[yPos][xPos] = p[xPos + yPos*GRAPHICS_WIDTH ];
}   }   }

// Draws a single character to the screen.
void drawChar(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], int x, int y, char c, uint16_t color, fontsize_t fontsize)
{
    int xi, yi;
    for(xi = 0; xi < 7; xi++ ){
        for(yi = 0; yi < 13; yi++ ){
            if ( charHasPixelSet( c, yi, xi ) ){
                int i;
                for( i = 0; i < fontsize; i++ ){
                    int j;
                    for( j = 0; j < fontsize; j++ ){
                        if( x+xi*fontsize+j && x+xi*fontsize+j < GRAPHICS_WIDTH &&  y+yi*fontsize+i && y+yi*fontsize+i < GRAPHICS_HEIGHT ){
                            frame[y+yi*fontsize+i][x+xi*fontsize+j] = color;
}   }   }   }   }   }   }

// Draws a string to the screen, using drawChar. Make sure you account for font size in your character spacing.
void drawString(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], int x, int y, char *string, uint16_t color, fontsize_t fontsize){
    int i = 0;
    while(*string){
        drawChar( frame, x + 7*fontsize*i++, y, *string, color, fontsize );
        string++;
}   }

// solves to see if the given point is inside of the polygon defined by the points of the asteroid
int pointInAsteroid( int xr, int yr, asteroid a ){
    if(!(xr>a.x && xr<a.x+a.w && yr>a.y && yr<a.y+a.h)){ // AABB collision between asteroid and point
        return 0;
    }

    int i;
    int count = 0;
    for( i=0; i<7; i++ ){ // for every line of the asteroid
        //drawLine( frame, a.x+a.xp[i], a.y+a.yp[i], a.x+a.xp[(i+1)%7], a.y+a.yp[(i+1)%7], a.c );
        int x0 = a.x+a.xp[i];
        int y0 = a.y+a.yp[i];
        int x1 = a.x+a.xp[(i+1)%7];
        int y1 = a.y+a.yp[(i+1)%7];
        if( y1<y0){ // switch so y0 is higher
            int tmp = y0;
            y0 = y1;
            y1 = tmp;
            tmp = x0;
            x0 = x1;
            x1 = tmp;
        }

        if( yr > y0 && yr < y1 ){ // potential hit (AABB collision between line and ray)
            float dx = x1-x0;
            float dy = y1-y0;
            int didCollide = 0;
            if( absVal(dy) > absVal(dx) ){ // slope steep
                float xChangePerStep = dx / dy;
                int y; // go ahead and initialize for both for loops
                if( y0 < y1 ){ // iterate from y0 to y1
                    float x = x0;
                    for( y = y0; y<=y1; y++ ){
                        x = x + xChangePerStep;
                        if(y == yr && ((int)x)<=xr ){
                            didCollide++;
                        }
                    }
                }else{ // iterate from y1 to y0
                    float x = x1;
                    for( y = y1; y<=y0; y++ ){
                        x = x + xChangePerStep;
                        if(y == yr && ((int)x)<=xr ){
                            didCollide++;
                        }
                    }
                }
            } else { // not steep
                float yChangePerStep = dy / dx;
                int x;
                if( x0 < x1 ){ // iterate from x0 to x1
                    float y = y0;
                    for( x=x0; x<=x1; x++ ){
                        y = y + yChangePerStep;
                        if(yr == (int)y && x<=xr ){
                            didCollide++;
                        }
                    }
                }else{ // iterate from x1 to x0
                    float y = y1;
                    //int didCollide = 0;
                    for( x=x1; x<=x0; x++ ){
                        y = y + yChangePerStep;
                        if(yr == (int)y && x<=xr){
                            didCollide++;
                        }
                    }
                }
            }
            if( didCollide ){ // if the ray and line did collide, count++
                count++;
            }
        }
    }
    return count%2; // if count is odd, return true, else, return false
}

// Initializes a septagon (asteroid);
void initAsteroidField( asteroid asteroids[ASTEROID_COUNT] ){
    int i;
    for( i=0; i<ASTEROID_COUNT; i++ ){
        asteroid a;
        int s = 1; // scaling factor for size of asteroids
        a.xp[0] = 0;
        a.yp[0] = 22*s;
        a.xp[1] = 18*s;
        a.yp[1] = 14*s;
        a.xp[2] = 22*s;
        a.yp[2] = -5*s;
        a.xp[3] = 10*s;
        a.yp[3] = -20*s;
        a.xp[4] = -10*s;
        a.yp[4] = -20*s;
        a.xp[5] = -22*s;
        a.yp[5] = -5*s;
        a.xp[6] = -18*s;
        a.yp[6] = 14*s;
        // TODO: random shifting of points;
        int ip; // point iterator/index, NOT inter-procedural scratch register
        int r = 9; // a scaling value for the random shifting of points
        for( ip=0; ip<7; ip++ ){
            a.xp[ip] = a.xp[ip] + (rand() % (2*r)) - r; // not a perfect distribution but that's okay
            a.yp[ip] = a.yp[ip] + (rand() % (2*r)) - r;
        }
        int xMin = 0; // find xMin, and then subtract it from every point, including xMax
        int xMax = 0;
        for( ip=0; ip<7; ip++ ){
            if( xMin > a.xp[ip] ){
                xMin = a.xp[ip];
            }
            if( xMax < a.xp[ip] ){
                xMax = a.xp[ip];
            }
        }
        for( ip=0; ip<7; ip++ ){
            a.xp[ip] = a.xp[ip] - xMin; // subtract xMin from every point
        }
        xMax = xMax - xMin;
        a.w = xMax;// xMax is now the width
        int yMin = 0; // find yMin and then subtract it from every point, including yMax
        int yMax = 0;
        for( ip=0; ip<7; ip++ ){
            if( yMin > a.yp[ip] ){
                yMin = a.yp[ip];
            }
            if( yMax < a.yp[ip] ){
                yMax = a.yp[ip];
            }
        }
        for( ip=0; ip<7; ip++ ){
            a.yp[ip] = a.yp[ip] - yMin; // subtract yMin from every point
        }
        yMax = yMax - yMin;
        a.h = yMax;// yMax is now the height;

        // initialize appropriate randomized values for a.x and a.y
        // asteroid will always be inside screen
        a.x = rand() %(320-a.w);
        a.y = rand() %(240-a.h);

        // initialize appropriate randomized values for a.xVel and a.yVel
        r = 5; // a scaling value for the speed of asteroids
        a.xVel = (rand()%(2*r))-r;
        a.yVel = (rand()%(2*r))-r;

        // initizlize appropriate randomized values for a.color: purplish
        a.c = 0xFFFF - rand()%32;
        a.c = 0xFFFF - ((rand()%64)<<5);

        asteroids[i] = a;
    }
}

int main(int argc, char **argv)
{
    // Initialize graphics
    int fd = open(device, O_RDWR);
    uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH];
    enableInput();
    initTimer(20.0);

    int gameState = 5;

    int score;
    mine m;
    rect playa;
    score = 0;
    playa.x = 1;
    playa.y = 1;
    playa.w = 5;
    playa.h = 5;
    playa.xVel = 0;
    playa.yVel = 0;
    asteroid asteroids[ASTEROID_COUNT];
    m.x = 1000;
    m.y = 1000;
    m.t = 1000;

    while(1){

        initFrame( frame, 0x0000);

        char typed = getKeyPress();
        fontsize_t fontsize;
        switch( gameState ){
            case 5: // init
                ;
                score = 0;
                playa.x = 1;
                playa.y = 1;
                playa.w = 5;
                playa.h = 5;
                playa.xVel = 0;
                playa.yVel = 0;
                m.x = 1000;
                m.y = 1000;
                m.t = 1000;
                initAsteroidField( asteroids );
                gameState = 0;
                // no break so it will just fall through
            case 0: // start menu
                ;
                fontsize = TINY;
                char string[] = "Press [Space] to Start!";
                drawString( frame, 76, 100, string, 0xFFFF, fontsize );

                fontsize = HUGE;
                char title[] = "Asteroid";
                drawString( frame, 22, 10, title, 0xF000, fontsize);

                if( typed == ' ' ){
                    gameState = 2;
                }
                break;

            case 1:
                ;

                if( typed=='w'){
                    playa.yVel = -5;
                    playa.xVel = 0;
                }
                if( typed=='a'){
                    playa.xVel = -5;
                    playa.yVel = 0;
                }
                if( typed=='s'){
                    playa.yVel = 5;
                    playa.xVel = 0;
                }
                if( typed=='d'){
                    playa.xVel = 5;
                    playa.yVel = 0;
                }
                if( typed=='p'){
                    gameState=2;
                }
                if( typed==' '){
                    m.x = playa.x+1;
                    m.y = playa.y+1;
                    m.t = 0;
                }

                if( m.t == 20 ){
                    m.x = 1000;
                    m.y = 1000;
                }

                m.t = m.t + 1;

                initFrame( frame, 0x0000);
                drawImage( frame, 0, 0, GRAPHICS_HEIGHT, GRAPHICS_WIDTH, gimp_image.pixel_data );

                int mineColor = 0x8000;
                if( m.t%2 == 0 ){ // every other frame color is bright
                    mineColor = 0xF000;
                }

                drawRectangle( frame, (m.x-2), (m.y-2), 5, 5, mineColor ); 

                fontsize = TINY;
                char scoreString[15];
                sprintf( scoreString, "Score: %d", score);
                drawString(frame, 5,5,scoreString, 0xFFFF, fontsize);

                int i;
                for( i=0; i<ASTEROID_COUNT; i++ ){
                    if( asteroids[i].x != 1000 && asteroids[i].y != 1000){
                        updateAsteroid( &asteroids[i] );
                        drawAsteroid( frame, asteroids[i] );
                    }
                    if( pointInAsteroid( playa.x, playa.y, asteroids[i] ) ){ // did collide with asteroid
                        gameState = 3;
                    }
                    if( pointInAsteroid( m.x, m.y, asteroids[i] ) ){
                        asteroids[i].x = 1000;
                        asteroids[i].y = 1000;
                        score = score+1;
                        m.x = 1000;
                        m.y = 1000;
                    }
                }

                if( score == 10 ){
                    gameState = 4;
                }

                rectUpdate( &playa );

                drawRect( frame, playa, 0x07E0 ); // 5 red 6 green 5 blue
                break;
            case 2: // paused
                ;
                fontsize = HUGE;
                char paused[] = "Paused.";
                drawString( frame, 5, 25, paused, 0xF000, fontsize );
                fontsize = TINY;
                char mines[] = "Press [Space] to drop mines!";
                drawString( frame, 10, 100, mines, 0xFFFF, fontsize );
                char move[] = "Press [w], [a], [s], [d] to move!";
                drawString( frame, 10, 115, move, 0xFFFF, fontsize );
                char pause[] = "Press [p] to pause/play.";
                drawString( frame, 10, 130, pause, 0xFFFF, fontsize );
                char warn[] = "Watch out! Mines don't last long.";
                drawString( frame, 10, 145, warn, 0xFFFF, fontsize );
                char warn2[] = "You can only drop one Mine at a time!";
                drawString( frame, 10, 160, warn2, 0xFFFF, fontsize );

                if( typed=='p'){
                    gameState=1;
                }
                break;
            case 3: // lost
                fontsize = HUGE;
                char lost[] = "You Lose :(";
                drawString( frame, 18, 25, lost, 0xF000, fontsize );

                fontsize = TINY;
                char string1[] = "Press [Space] to Restart!";
                drawString( frame, 76, 100, string1, 0xFFFF, fontsize );

                if( typed == ' ' ){
                    gameState = 5;
                }

                break;
            case 4:
                fontsize = HUGE;
                char won[] = "You Win!";
                drawString( frame, 18, 25, won, 0x07E0, fontsize );

                fontsize = TINY;
                char string2[] = "Press [Space] to Restart!";
                drawString( frame, 76, 100, string2, 0xFFFF, fontsize );

                if( typed == ' ' ){
                    gameState = 5;
                }

                break;
            default:
            ;
        }

        waitTimer();
        outputFrame(fd, frame);
    }
    stopTimer();
    disableInput();
    close(fd);
    return 0;
}
