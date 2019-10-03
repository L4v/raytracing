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

#include "linux_raytracer.h"

global_variable linux_offscreen_buffer GlobalBackBuffer;

internal bool32
RayIntersect(Vector3f* RayOrigin, Vector3f* RayDirection, Vector3f* SphereCenter,
	     real32 SphereRadius, real32* SphereDistance)
{

  /* NOTE(l4v):
     Sphere formula: dot((P - C), (P - C)) = r^2;
     P - Point on sphere
     C - Center of sphere
     r - Radius of sphere

     Ray formula: P(t) = A + t*B
     P - Point on ray
     A - Origin of ray
     B - Unit direction vector of ray
     t - Parameter used to move along the Ray vector

     Combined formulas: dot((A + t*B - C), (A + t*B - C)) = r^2
     When solved, you get: a*t^2 + b*t + c = 0
     a = dot(B, B);
     b = 2 * dot(B, A - C);
     c = dot((A - C), (A - C)) - r^2

     Then just check whether the determinant
     D = b^2 - 4ac
     Is < 0 - No intersection
     Is = 0 - One point intersection
     Is > 0 - Two point intersection (take the lesser positive point)

     The point of intersection represents the distance
     from the ray origin to the sphere
  */
  
  // NOTE(l4v): Here I used uppercase A, B, C, they are
  // the lowercase equivalent from above
  Vector3f OC = Difference3D(RayOrigin, SphereCenter);
  Vector3f Dir = Normalize3D(RayDirection);
  real32 A = Dot3D(&Dir, &Dir);
  real32 B = 2.0f * Dot3D(&OC, &Dir);
  real32 C = Dot3D(&OC, &OC) - SphereRadius * SphereRadius;
  real32 Discriminant = B*B - 4*A*C;

  if(Discriminant < 0)
    {
      // NOTE(l4v): No intersection
      return 0;
    }
  
  // NOTE(l4v): If the quadratic has a solution, set the distance
  // as the lesser positive point
  *SphereDistance = ((-B - sqrt(Discriminant)) / (2.0f * A));
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
render(Vector3f* SphereCenter, real32 SphereRadius, linux_offscreen_buffer* Buffer)
{
  real32 FOV = Pi32 / 2.0f;
  // NOTE(l4v): Generating gradient image
  uint32* Pixel = (uint32*)Buffer->Memory;
  for(int32 Y = 0;
      Y < Buffer->Height;
      ++Y)
    {
      for(int32 X = 0;
	  X < Buffer->Width;
	  ++X)
	{
	  uint8 Blue = 0;
	  uint8 Green = 255 * (X / (real32)Buffer->Width);
	  uint8 Red = 255 * (Y / (real32)Buffer->Height);
	  // NOTE(l4v): Stored as RR GG BB XX
	  *Pixel++ = ((Red << 24) | (Green << 16) | (Blue << 8));
	}
    }

  Pixel = (uint32*)Buffer->Memory;
  for(size_t Row = 0;
      Row < Buffer->Height;
      ++Row)
    {
      for(size_t Column = 0;
	  Column < Buffer->Width;
	  ++Column)
	{
	  real32 X = (2 * (Column + 0.5f) / (real32)Buffer->Width - 1) *
	    tan(FOV / 2.0f) * Buffer->Width / (real32)Buffer->Height;
	  real32 Y = -(2 * (Row + 0.5f) / (real32)Buffer->Height - 1) * tan(FOV / 2.0f);
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
  fprintf(File, "P6\n%d %d\n255\n", Buffer->Width, Buffer->Height);
  Pixel = (uint32*)Buffer->Memory;
  for(int32 PixelIndex = 0;
      PixelIndex < Buffer->Width * Buffer->Height;
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
  GlobalBackBuffer.Width = 1024;
  GlobalBackBuffer.Height = 768;
  GlobalBackBuffer.BytesPerPixel = 4;
  int32 FrameBufferSize = GlobalBackBuffer.Width * GlobalBackBuffer.Height *
    GlobalBackBuffer.BytesPerPixel;
  
  GlobalBackBuffer.Memory = mmap(0,
				 FrameBufferSize,
				 PROT_READ | PROT_WRITE,
				 MAP_ANONYMOUS | MAP_PRIVATE,
				 -1,
				 0);
  Vector3f SphereCenter = {.X = -3, .Y = 0, .Z = -16};
  real32 SphereRadius = 2;
  render(&SphereCenter, SphereRadius, &GlobalBackBuffer);

  munmap(GlobalBackBuffer.Memory, FrameBufferSize);
  return 0;
}
