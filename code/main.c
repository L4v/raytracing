#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <math.h>
#include <float.h>

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
#define Pi32 3.14159265359f
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

typedef struct
{
  union
  {
    real32 E[2];
    struct{
      real32 X;
      real32 Y;
    };
  };
} Vector2f;

internal Vector3f
Difference3D(Vector3f* A, Vector3f* B)
{
  // NOTE(l4v): A - B
  Vector3f Result = {};
  Result.X = A->X - B->X;
  Result.Y = A->Y - B->Y;
  Result.Z = A->Z - B->Z;

  return Result;
}

internal Vector2f
Difference2D(Vector2f* A, Vector2f* B)
{
  // NOTE(l4v): A - B
  Vector2f Result = {};
  Result.X = A->X - B->X;
  Result.Y = A->Y - B->Y;

  return Result;
}

internal Vector3f
Scale3D(Vector3f* A, real32 Scale)
{
  Vector3f Result = {};
  Result.X = Scale * A->X;
  Result.Y = Scale * A->Y;
  Result.Z = Scale * A->Z;

  return Result;
}

internal real32
Dot3D(Vector3f* A, Vector3f* B)
{
  real32 Result = 0;
  Result = A->X * B->X + A->Y * B->Y + A->Z * B->Z;
  return Result;
}

internal Vector3f
Normalize3D(Vector3f* A)
{
  Vector3f Result = {};
  real32 Length = sqrt(Dot3D(A, A));
  Result.X = A->X / Length;
  Result.Y = A->Y / Length;
  Result.Z = A->Z / Length;

  return Result;
}

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

internal bool32
RayIntersect(Vector3f* Origin, Vector3f* Dir,
	     Vector3f* Center, real32 Radius, real32* SphereDist)
{
  // NOTE(l4v): Vector from the Origin of the array to the Center of the sphere
  Vector3f L = Difference3D(Center, Origin);
  // NOTE(l4v): Dot product of L and Direction
  real32 CenterToRayLen = Dot3D(&L, Dir);
  // NOTE(l4v): This is the (distance between Origin and Center projection) ^ 2
  real32 D2 = Dot3D(&L, &L) - CenterToRayLen * CenterToRayLen;

  // NOTE(l4v): If the D2 is greater than the Radius ^ 2 => there is no intersection
  if(D2 > (Radius * Radius))
    {
      return 0;
    }
  // NOTE(l4v): Distance from the projected dot onto the Ray to the
  // first point of intersection
  real32 PCI1Distance = sqrt(Radius * Radius - D2);

  // NOTE(l4v): Distance from Origin to first intersection
  *SphereDist = CenterToRayLen - PCI1Distance;
  // NOTE(l4v): Distance from Origin to second intersection
  real32 T1 = CenterToRayLen + PCI1Distance;
  // NOTE(l4v): There is only one interseciton maybe
  if(*SphereDist < 0)
    {
      *SphereDist = T1;
    }
  // NOTE(l4v): There are no intersections
  if(*SphereDist < 0)
    {
      return 0;
    }
  return 1;
}

internal uint32
CastRay(Vector3f* Origin, Vector3f* Dir, Vector3f* Center, real32 Radius)
{
  real32 SphereDistance = FLT_MAX;
  uint32 Result = 0;
  // NOTE(l4v): Sphere color
  uint8 Red = (uint8)(0.4f * 255);
  uint8 Green = (uint8)(0.4f * 255);
  uint8 Blue = (uint8)(0.3f * 255);
  
  if(!RayIntersect(Origin, Dir, Center, Radius, &SphereDistance))
    {
      // NOTE(l4v): Background color
      Red = (uint8)(0.2f * 255);
      Green = (uint8)(0.7f * 255);
      Blue = (uint8)(0.8f * 255);
    }
  Result = ((Red << 24) | (Green << 16) | (Blue << 8));
  return Result;
}

internal void
render(Vector3f* SphereCenter, real32 SphereRadius)
{
  int32 Width = 1024;
  int32 Height = 768;
  real32 FOV = Pi32 / 2.0f;
  int32 BytesPerPixel = 4;
  int32 FrameBufferSize = Width * Height * BytesPerPixel;
  
  // TODO(l4v): Use memory allocation?
  void* FrameBuffer = mmap(0,
			   FrameBufferSize,
			   PROT_READ | PROT_WRITE,
			   MAP_ANONYMOUS | MAP_PRIVATE,
			   -1,
			   0);

  // NOTE(l4v): Generating gradient image
  uint32* Pixel = (uint32*)FrameBuffer;
  for(int32 Y = 0;
      Y < Height;
      ++Y)
    {
      for(int32 X = 0;
	  X < Width;
	  ++X)
	{
	  uint8 Blue = 0;
	  uint8 Green = 255 * (X / (real32)Width);
	  uint8 Red = 255 * (Y / (real32)Height);
	  // NOTE(l4v): Stored as RR GG BB XX
	  *Pixel++ = ((Red << 24) | (Green << 16) | (Blue << 8));
	}
    }

  Pixel = (uint32*)FrameBuffer;
  for(size_t Row = 0;
      Row < Height;
      ++Row)
    {
      for(size_t Column = 0;
	  Column < Width;
	  ++Column)
	{
	  real32 X = (2 * (Column + 0.5f) / (real32)Width - 1) * tan(FOV / 2.0f) * Width / (real32)Height;
	  real32 Y = -(2 * (Row + 0.5f) / (real32)Height - 1) * tan(FOV / 2.0f);
	  Vector3f Temp = {.X = X, .Y = Y, .Z = -1};
	  Vector3f Dir = Normalize3D(&Temp);
	  Vector3f Zero3D = {};
	  *Pixel++ = CastRay(&Zero3D, &Dir, SphereCenter, SphereRadius);
	}
    }
  

  // NOTE(l4v): Writing image data to file
  FILE* File = fopen("../data/out.ppm", "wb");
  if(!File)
    {
      printf("ERROR::Could not open file\n");
      return;
    }
  fprintf(File, "P6\n%d %d\n255\n", Width, Height);
  Pixel = (uint32*)FrameBuffer;
  for(int32 PixelIndex = 0;
      PixelIndex < Width * Height;
      ++PixelIndex)
    {
      uint8 Red = (uint8)((*Pixel) >> 24);
      uint8 Green = (uint8)((*Pixel) >> 16);
      uint8 Blue = (uint8)((*Pixel) >> 8);
      fprintf(File, "%c%c%c", Red, Green, Blue);
      Pixel++;
    }
  
  fclose(File);
}

int main()
{
  Vector3f SphereCenter = {.X = -3, .Y = 0, .Z = -16};
  real32 SphereRadius = 2;
  render(&SphereCenter, SphereRadius);
  return 0;
}
