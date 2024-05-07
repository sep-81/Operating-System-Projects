#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <chrono>
#include <pthread.h>


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

struct RGB
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
};


struct Arguments
{
  Arguments(vector<vector<RGB>> &pic, vector<vector<RGB>> &resPic)
      : pixles(pic), resultPic(resPic) {}
  char *fileName;
  int bufferSize;
  char *fileBuffer;
  vector<vector<RGB>> &pixles;
  vector<vector<RGB>> &resultPic;
  int row1=0,row2=0;
};

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


void* getPixlesFromBMP24(void *argp)
{
  Arguments args = *(Arguments *)argp;
  // int count = 1;
  const int extra = cols % 4;
  unsigned char r, g, b;
  int i = args.row1;
  int count = (i) * extra + 1 + i * cols * 3; 
  char* fileReadBuffer = args.fileBuffer;
  const int end = args.bufferSize;
  for (; i < args.row2; i++)
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
      args.resultPic[i][j] = RGB{r, g, b};
    }
  }
  return NULL;
}

void* mirrorBmp24(void *argp)
{
  Arguments args = *(Arguments *)argp;

  int i = args.row1;
  for (; i < args.row2; i++)
    for (int j = 0; j < cols; j++)
      args.resultPic[i][j] = args.pixles[i][cols - j - 1];
  return NULL;
}

RGB convolv(const vector<vector<RGB>> &pixles, int row, int col){
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

void *deepBmp24(void *argp)
{
  Arguments args = *(Arguments *)argp;
  cout << "deep" << endl;
  int i = args.row1;
  if(i == 0)
    i=1;
  if(args.row2 >= rows)
    args.row2 = rows-1;
  for (; i < args.row2; i++)
    for(int j = 1; j < cols-1; j++)
      args.resultPic[i][j] = convolv(args.pixles, i, j);
  cout << "Thread " << i << " finished" << endl;
  pthread_exit(NULL);
  return (void *)0;
}

void* RhombusBmp24(void *argp){
  Arguments args = *(Arguments *)argp;
  RGB white = RGB{255, 255, 255};
  RGB black = RGB{0, 0, 0};
  int col = cols / 2;
  int distence = 0;
  int i = args.row1;
  for (; i < args.row2; i++){
      if (i < rows / 2)
        distence = i;
      else
        distence = rows - i - 1;
      if(col - distence < 0 || col + distence > cols)
        continue;
      args.resultPic[i][col - distence] = white;
      args.resultPic[i][col + distence] = white;
  }
  return NULL;
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
const int pt_num = 4;

void invokeParallel(void*(*func)(void*), Arguments &arg, bool swap_f) {
  pthread_t threads[pt_num];
  // arg.pixles = arg.resultPic;
  for (int i = 0; i < pt_num; i++)
  {
    Arguments *argp = !swap_f ? new Arguments(arg.pixles, arg.resultPic) : new Arguments(arg.resultPic, arg.pixles);
    argp->bufferSize = arg.bufferSize;
    argp->fileBuffer = arg.fileBuffer;
    argp->fileName = arg.fileName;
    argp->row1 = i * (rows / pt_num);
    argp->row2 = (i + 1) * (rows / pt_num) <= rows ?  (i + 1) * (rows / pt_num) : rows;
    if(pthread_create(&threads[i], NULL, func, (void *)argp) != 0)
      cout << "Error creating thread" << endl;
    else 
      cout << "Thread " << i << " created" << endl;
  }
  for (int i = 0; i < pt_num; i++)
    pthread_join(threads[i], NULL);
}

int main(int argc, char *argv[])
{
  int bufferSize;
  const int TOTAL_HEADER_SIZE = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
  char *fileBuffer;
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
  Arguments arg(pic, resultPic);
  arg.bufferSize = bufferSize;
  arg.fileBuffer = fileBuffer;

  // read input file
  invokeParallel(&getPixlesFromBMP24, arg, false);
  // apply filters
  invokeParallel(&mirrorBmp24, arg,true);
  // writeOutBmp24(fileBuffer, "output.bmp", bufferSize, arg.pixles);
  invokeParallel(&deepBmp24, arg, false);
  invokeParallel(&RhombusBmp24, arg,false);



  // mirrorBmp24(resultPic, pic);
  // deepBmp24(pic,resultPic);
  // RhombusBmp24(pic);

  // write output file
  writeOutBmp24(fileBuffer, "output.bmp", bufferSize, arg.resultPic);

  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  cout << "Time taken by function: " << duration.count() << " microseconds" << endl;
  return 0;
}