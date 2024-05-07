#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <chrono>

using namespace std::chrono;
using std::vector;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;

#pragma pack(1)
#pragma once

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef struct tagBITMAPFILEHEADER
{
  WORD bfType;
  DWORD bfSize;
  WORD bfReserved1;
  WORD bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

int rows;
int cols;

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize)
{
  std::ifstream file(fileName);

  if (file)
  {
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);
    cout << "File size: " << length << endl;
    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&buffer[0]);
    info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
    cout << "TOTAL HEADER SIZE: " << sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)<< endl;
    rows = info_header->biHeight;
    cols = info_header->biWidth;
    bufferSize = file_header->bfSize;
    return 1;
  }
  else
  {
    cout << "File" << fileName << " doesn't exist!" << endl;
    return 0;
  }
}

struct RGB
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer, vector<vector<RGB>> &pixles)
{
  int count = 1;
  int extra = cols % 4;
  unsigned char r, g, b;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--) {
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
          case 0:
            r = fileReadBuffer[end - count];
            break;
          case 1:
            g = fileReadBuffer[end - count];
            break;
          case 2:
            b = fileReadBuffer[end - count];
            break;
        // go to the next position in the buffer
        }
        count++;
      }
      pixles[i][j] = RGB{r, g, b};
    }
  }
}

void mirrorBmp24(vector<vector<RGB>> &resultPic, vector<vector<RGB>> &pixles)
{
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
      resultPic[i][j] = pixles[i][cols - j - 1];
}

RGB convolv(vector<vector<RGB>> &pixles, int row, int col){
  int sumR = 0, sumG = 0, sumB = 0;
  int kernel[3][3] = {{-2, -1, 0}, {-1, 1, 1}, {0, 1, 2}};
  for (int i = -1; i < 2; i++){
    for (int j = -1; j < 2; j++)
    {
      sumR += pixles[row + i][col + j].r * kernel[i + 1][j + 1];
      sumG += pixles[row + i][col + j].g * kernel[i + 1][j + 1];
      sumB += pixles[row + i][col + j].b * kernel[i + 1][j + 1];
    }
  }
  sumR = sumR > 255 ? 255 : sumR;
  sumR = sumR < 0 ? 0 : sumR;
  sumG = sumG > 255 ? 255 : sumG;
  sumG = sumG < 0 ? 0 : sumG;
  sumB = sumB > 255 ? 255 : sumB;
  sumB = sumB < 0 ? 0 : sumB;
  return RGB{(unsigned char)sumR, (unsigned char)sumG, (unsigned char)sumB};
}

void deepBmp24(vector<vector<RGB>> &resultPic, vector<vector<RGB>> &pixles)
{
  for (int i = 1; i < rows-1; i++)
    for(int j = 1; j < cols-1; j++)
      resultPic[i][j] = convolv(pixles, i, j);

}

void RhombusBmp24(vector<vector<RGB>> &pic){
  RGB white = RGB{255, 255, 255};
  RGB black = RGB{0, 0, 0};
  int col = cols / 2;
  int distence = 0;
  for (int i = 0; i < rows; i++){
      if(col - distence < 0 || col + distence > cols)
        continue;
      pic[i][col - distence] = white;
      pic[i][col + distence] = white;
      if (i < rows / 2)
        distence++;
      else
        distence--;
  }
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize, vector<vector<RGB>> &pixles)
{
  std::ofstream write(nameOfFileToCreate);
  if (!write)
  {
    cout << "Failed to write " << nameOfFileToCreate << endl;
    return;
  }
  int count = 1;
  int extra = cols % 4;

  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--) 
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          // if(i == 1 && j == 0)
          //   cout << "143: " << pixles[i][cols+2].r << endl; it doesnt triger error!!!!
          fileBuffer[bufferSize - count] = pixles[i][j].r;
          break;
        case 1:
          fileBuffer[bufferSize - count] = pixles[i][j].g;
          break;
        case 2:
          fileBuffer[bufferSize - count] = pixles[i][j].b;
          break;
        // go to the next position in the buffer
        }
        count++;
      }
  }
  write.write(fileBuffer, bufferSize);
}

int main(int argc, char *argv[])
{
  const int TOTAL_HEADER_SIZE = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
  char *fileBuffer;
  int bufferSize;
  char *fileName = argv[1];

  auto start = high_resolution_clock::now();

  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize))
  {
    cout << "File read error" << endl;
    return 1;
  }
  cout << "sizeof:\t" << sizeof(char*) << endl;
  cout << "paddings: \t" << cols % 4<< endl;
  cout << "rows: " << rows << "\tcols: " <<  cols << endl;
  if(cols % 4) {
  cout << cols * rows * 3 + ((cols % 4) * rows) << '\t' << bufferSize << endl;
  } else {
  cout << cols * rows * 3 << '\t' << bufferSize << '\t' << bufferSize - cols * rows * 3 << endl;
  }
  vector<vector<RGB>> pic(rows, vector<RGB>(cols)); 
  vector<vector<RGB>> resultPic(rows, vector<RGB>(cols)); 
  // read input file
  getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer, pic);
  // apply filters
  mirrorBmp24(resultPic, pic);
  deepBmp24(pic,resultPic);
  RhombusBmp24(pic);
  // write output file
  writeOutBmp24(fileBuffer, "output.bmp", bufferSize, pic);

  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  cout << "Time taken by function: " << duration.count() << " microseconds" << endl;
  return 0;
}