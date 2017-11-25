#include "matrixAnimation.h"

MatrixAnimation::MatrixAnimation()
{
  init(0, NULL, RGB24, 0, 0);
}

MatrixAnimation::MatrixAnimation(uint16_t frameCount_,
                     PGM_VOID_P frameData_,
                     Encoding encoding_,
                     uint16_t ledCount_,
                     uint16_t frameDelay_)
{
  init(frameCount_, frameData_, encoding_, ledCount_, frameDelay_);
  reset();
}

void MatrixAnimation::init(uint16_t frameCount_,
                     PGM_VOID_P frameData_,
                     Encoding encoding_,
                     uint16_t ledCount_,
                     uint16_t frameDelay_)
{
  frameCount = frameCount_;
  frameData = frameData_;
  encoding = encoding_;
  ledCount = ledCount_;
  frameDelay = frameDelay_;

  switch(encoding) {
    case RGB24:
      drawFunction = &MatrixAnimation::drawRgb24;
      break;

    case RGB565_RLE:
      drawFunction = &MatrixAnimation::drawRgb565_RLE;
      break;

#ifdef SUPPORTS_PALLETE_ENCODING
    case INDEXED:
      drawFunction = &MatrixAnimation::drawIndexed;
      loadColorTable();
      break;

    case INDEXED_RLE:
      drawFunction = &MatrixAnimation::drawIndexed_RLE;
      loadColorTable();
      break;
#endif
  }

  reset();
}
 
#ifdef SUPPORTS_PALLETE_ENCODING
void MatrixAnimation::loadColorTable() {
  colorTableEntries = pgm_read_byte(frameData) + 1;

  for(int i = 0; i < colorTableEntries; i++) {
    colorTable[i] = CRGB(pgm_read_byte(frameData + 1 + i*3    ),
                         pgm_read_byte(frameData + 1 + i*3 + 1),
                         pgm_read_byte(frameData + 1 + i*3 + 2));
  }
}
#endif

void MatrixAnimation::reset() {
  frameIndex = 0;
}

void MatrixAnimation::draw(RGBMatrix &matrix)
{
  (this->*drawFunction)(matrix);

  matrix.show();
  
  frameIndex = (frameIndex + 1)%frameCount;
};

uint16_t MatrixAnimation::getLedCount() const {
  return ledCount;
}

uint16_t MatrixAnimation::getFrameCount() const {
  return frameCount;
}

uint16_t MatrixAnimation::getFrameIndex() const {
  return frameIndex;
}

uint16_t MatrixAnimation::getFrameDelay() const {
  return frameDelay;
}

void MatrixAnimation::drawRgb24(RGBMatrix &matrix) {
  currentFrameData = frameData
    + frameIndex*ledCount*3;  // Offset for current frame
  
  for(uint16_t i = 0; i < ledCount; i++) {
    PGM_VOID_P pixelPointer = currentFrameData + i*3;
    int row = i / LED_COLS;
    int col = i % LED_COLS;
    uint8_t r = pgm_read_byte(pixelPointer);
    uint8_t g = pgm_read_byte(pixelPointer + 1);
    uint8_t b = pgm_read_byte(pixelPointer + 2);
    matrix.set(row, col, r, g, b);
  }
}

void MatrixAnimation::drawRgb565_RLE(RGBMatrix &matrix) {
  if(frameIndex == 0) {
    currentFrameData = frameData;
  }

  // Read runs of RLE data until we get enough data.
  uint16_t count = 0;
  while(count < ledCount) {
    uint8_t run_length = pgm_read_byte(currentFrameData);
    uint8_t upperByte = pgm_read_byte(currentFrameData + 1);
    uint8_t lowerByte = pgm_read_byte(currentFrameData + 2);
    
    uint8_t r = ((upperByte & 0xF8)     );
    uint8_t g = ((upperByte & 0x07) << 5)
              | ((lowerByte & 0xE0) >> 3);
    uint8_t b = ((lowerByte & 0x1F) << 3);
    
    for(uint8_t i = 0; i < run_length; i++) {
      int ledindex = count + i;
      int row = ledindex / LED_COLS;
      int col = ledindex % LED_COLS;
      matrix.set(row, col, r, g, b);
    }
    
    count += run_length;
    currentFrameData += 3;
  }
};

#ifdef SUPPORTS_PALLETE_ENCODING
void MatrixAnimation::drawIndexed(RGBMatrix &matrix) {
  currentFrameData = frameData
    + 1 + 3*colorTableEntries   // Offset for color table
    + frameIndex*ledCount;      // Offset for current frame
  
  for(uint16_t i = 0; i < ledCount; i++) {
    int row = i / LED_COLS;
    int col = i % LED_COLS;
    matrix.set(row, col, colorTable[pgm_read_byte(currentFrameData + i)]);
  }
}

void MatrixAnimation::drawIndexed_RLE(RGBMatrix &matrix) {
  if(frameIndex == 0) {
    currentFrameData = frameData
      + 1 + 3*colorTableEntries;   // Offset for color table
  }

  // Read runs of RLE data until we get enough data.
  uint16_t count = 0;
  while(count < ledCount) {
    uint8_t run_length = pgm_read_byte(currentFrameData++);
    uint8_t colorIndex = pgm_read_byte(currentFrameData++);
    
    for(uint8_t i = 0; i < run_length; i++) {
      int row = count / LED_COLS;
      int col = count % LED_COLS;
      matrix.set(row, col, colorTable[colorIndex]);
      count++;
    }
  }
};
#endif
