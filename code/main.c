#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#define Kibibytes(Value) ((Value) * 1024LL)
#define Mebibytes(Value) (Kibibytes(Value) * 1024LL)
#define Gibibytes(Value) (Mebibytes(Value) * 1024LL)
#define internal static
#define global_variable static
#define local_persist static
#define Assert(Expression)			\
  if(!(Expression)) {*(int*)0 = 0;}

typedef struct
{
  union
  {
    real32 E[3];
    struct{
      real32 X;
      real32 Y;
      real32 Z;
    };
  };
} Vector3f;

internal real32
MaxReal(real32 X, real32 Y)
{
  real32 Result;
  Result = X > Y ? X : Y;
  return Result;
}

internal real32
MinReal(real32 X, real32 Y)
{
  real32 Result;
  Result = X < Y ? X : Y;
  return Result;
}

internal void
render()
{
  int32 Width = 1024;
  int32 Height = 768;
  int32 Pitch = Width * 4;
  // TODO(l4v): Use memory allocation?
  //  Vector3f FrameBuffer[Width * Height];
  void* FrameBuffer = mmap(0,
			   Width * Height * 4,
			   PROT_READ | PROT_WRITE,
			   MAP_ANONYMOUS | MAP_PRIVATE,
			   -1,
			   0);

  // TODO(l4v): Check how to write to an image or use as OpenGL texture
  uint8* Row = (uint8*)FrameBuffer;
  for(int32 Y = 0;
      Y < Height;
      ++Y)
    {
      uint32* Pixel = (uint32*)Row;
      for(int32 X = 0;
	  X < Width;
	  ++X)
	{
	  uint8 Blue = X;
	  uint8 Green = Y;
	  uint8 Red = 1;
	  *Pixel++ = ((Green << 8) | (Blue << 16));
	  printf("%d\n", *Pixel);
	}
      Row += Pitch;
    }


  FILE* File = fopen("../data/out.ppm", "wb");
  if(!File)
    {
      printf("ERROR::Could not open file\n");
      return;
    }
  fprintf(File, "P6\n%d %d\n255\n", Width, Height);

  Row = (uint8*)FrameBuffer;
  for(int32 Y = 0;
      Y < Height;
      ++Y)
    {
      uint32* Pixel = (uint32*)Row;
      for(int32 X = 0;
	  X < Width;
	  ++X)
	{
	  fprintf(File, "%d", *Pixel++);
	}
      Row += Pitch;
    }
  
  fclose(File);
}

int main()
{
  render();
  return 0;
}
