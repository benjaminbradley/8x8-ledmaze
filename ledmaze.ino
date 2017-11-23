/** \file
 */

#include "Badge.h"

Badge badge;

uint32_t last_draw_millis, last_move_millis;



// simple square map
const int MAP_MIN_X = -20;
const int MAP_MAX_X = 20;
const int MAP_MIN_Y = -20;
const int MAP_MAX_Y = 20;

// player characteristics
const int PLAYER_SIZE = 2;

// calculate derived player vars
const int PLAYER_OFFSET_X = LED_ROWS/2-PLAYER_SIZE/2;
const int PLAYER_OFFSET_Y = LED_COLS/2-PLAYER_SIZE/2;

// movement-related constants
const float MOVE_LIMIT_0 = 0.7;   // tilt less than this threshold is ignored
const float MOVE_LIMIT_1 = 3.0;   // tilt less than this threshold = move 1 @ 5KHz
                                  // tilt more than this threshold = move 1 @ 10KHz

void setup()
{
    badge.begin();
    badge.matrix.setBrightness(100);
}

float ax, ay, az;
float oldax, olday, oldaz;
int oldx,oldy;
int posx = 0, posy = 0;
int move_speed = 10;


void loop()
{
  const uint32_t now = millis();
  static int x=0;
  static int y=0;
  static uint32_t color = 0xffffff;

  // Draw the LEDs at 60Hz
  if (now - last_draw_millis < (1000/60))
      return;

  last_draw_millis = now;

  if (badge.button_edge()) {
    Serial.println("button");
    color += 0xf0;
  }

  // Calculate moves at variable speed
  if (now - last_move_millis > (1000/move_speed)) {
    last_move_millis = now;
    badge.poll();
    // smooth the inputs
    //const float smooth = 0.10;
    //ax = (ax * smooth + badge.ax) / (smooth + 1);
    //ay = (ay * smooth + badge.ay) / (smooth + 1);
    ax = badge.ax;
    ay = badge.ay;
    // rotate 45 degrees
    const float rx = ax * 0.851 - ay * 0.525;
    const float ry = ay * 0.851 + ax * 0.525;
    // debug
    if(oldax != int(rx) || olday != int(ry)) {
      oldax = int(rx);
      olday = int(ry);
      Serial.print("rx: ");
      Serial.print(rx);
      Serial.print(", ry: ");
      Serial.print(ry);
      Serial.println("");
    }
    // chunk to movement
    int dx;
    int dy;
    if(-MOVE_LIMIT_0 < rx && rx < MOVE_LIMIT_0) {
      dx = 0;
    } else if(-MOVE_LIMIT_1 < rx && rx < MOVE_LIMIT_1) {
      dx = (rx < 0 ? -1 : 1);
      move_speed = 5;
    } else {
      dx = (rx < 0 ? -1 : 1);
      move_speed = 10;
    }
    if(-MOVE_LIMIT_0 < ry && ry < MOVE_LIMIT_0) {
      dy = 0;
    } else if(-MOVE_LIMIT_1 < ry && ry < MOVE_LIMIT_1) {
      dy = (ry < 0 ? 1 : -1);
      move_speed = 5;
    } else {
      dy = (ry < 0 ? 1 : -1);
      move_speed = 10;
    }
    // apply movement to position
    posx += dx;
    posy += dy;
    // check for wall collisions
    if(posx+PLAYER_SIZE >= MAP_MAX_X) posx = MAP_MAX_X-PLAYER_SIZE;
    if(posx <= MAP_MIN_X) posx = MAP_MIN_X+1;
    if(posy+PLAYER_SIZE >= MAP_MAX_Y) posy = MAP_MAX_Y-PLAYER_SIZE;
    if(posy <= MAP_MIN_Y) posy = MAP_MIN_Y+1;
    // log position update
    if(oldx != posx || oldy != posy) {
      oldx = posx;
      oldy = posy;
      Serial.print("posx: ");
      Serial.print(posx);
      Serial.print(", posy: ");
      Serial.print(posy);
      Serial.println("");
    }
  }

  for(int drawx = 0; drawx < LED_COLS; drawx++) {
    for(int drawy = 0; drawy < LED_ROWS; drawy++) {
      // draw the "player" marker
      if(drawx >= PLAYER_OFFSET_X && drawx < PLAYER_OFFSET_X+PLAYER_SIZE && 
         drawy >= PLAYER_OFFSET_Y && drawy < PLAYER_OFFSET_Y+PLAYER_SIZE) {
        badge.matrix.set(drawx, drawy, 0xaaaaaa);
      } else {
        int mapx = posx - PLAYER_OFFSET_X + drawx;
        int mapy = posy - PLAYER_OFFSET_Y + drawy;

        // don't draw anything outside the map
        if(mapx < MAP_MIN_X || mapx > MAP_MAX_X || mapy < MAP_MIN_Y || mapy > MAP_MAX_Y) {
          color = 0;
        } else {
          // bg default color is black
          color = 0;
          // draw map edge
          if(mapx == MAP_MAX_X || mapx == MAP_MIN_X || mapy == MAP_MAX_Y || mapy == MAP_MIN_Y)
            color = 0x444444;
          // if nothing else there, draw background grid
          if(color == 0) {
            if(mapx % 4 == 0) {
              color |= 0x30;
            }
            if(mapy % 4 == 0) {
              color |= 0x3000;
            }
          }
        }
        badge.matrix.set(drawx, drawy, color);
      }
    }
  }
  badge.matrix.show();
}

