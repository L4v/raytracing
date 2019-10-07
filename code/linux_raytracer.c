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
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#include "linux_raytracer.h"

global_variable linux_offscreen_buffer GlobalBackBuffer;

internal bool32
RaySphereIntersect(vector3f* Origin, vector3f* Dir, vector3f* Normal,
		   sphere* Sphere, real32* SphereDist)
{
  // dot(P - C, P - C) = r^2
  //
  vector3f OC = Subtract3D(Origin, &Sphere->Center);
  vector3f NormalDir = Normalize3D(Dir);
  real32 A = Dot3D(&Dir, &Dir);
  real32 B = 2.0f * Dot3D(&OC, &Dir);
  real32 C = Dot3D(&OC, &OC) - (Sphere->Radius * Sphere->Radius);
  real32 Discriminant = B*B - 4*A*C;

  if(Discriminant < 0)
    {
      return 0;
    }

  // TODO(l4v): Calc point and normal here?
  *Normal = Normalize3D();
  *SphereDist = (-B - sqrt(Discriminant)) / (2.0f * A);
  return 1;
}

internal vector3f
CastRay(vector3f* Origin, vector3f* Direction, game_state* GameState)
{
  vector3f Result = (vector3f){};
  
  return Result;
}

internal void
Render(game_state* GameState, linux_offscreen_buffer* Buffer)
{
  real32 FOV = Pi32 / 2.0f;
  uint32* Pixel = (uint32*)Buffer->Memory;
  for(size_t Row = 0;
      Row < Buffer->Height;
      ++Row)
    {
      for(size_t Column = 0;
	  Column < Buffer->Width;
	  ++Column)
	{
	  real32 PixelX = ((2 * (Column + 0.5f)) / (Buffer->Width)) *
	    tan(FOV / 2.0f - 1) * ((Buffer->Width) / (Buffer->Height));
	  real32 PixelY = ((2 * (Row + 0.5f)) / (Buffer->Height)) *
	    tan(FOV / 2.0f - 1);
	  real32 Distance = 1.0f;

	  vector3f Origin = {};
	  vector3f Camera = {.X = PixelX, .Y = PixelY, .Z = -Distance};
	  vector3f CameraDir = Normalize3D(&Camera);
	  
	  vector3f RealColor = CastRay(&Origin, &CameraDir, GameState);
	  *Pixel++ = ((RoundReal32ToUInt32(RealColor.X * 255.0f) << 24) |
		      (RoundReal32ToUInt32(RealColor.Y * 255.0f) << 16) |
		      (RoundReal32ToUInt32(RealColor.Z * 255.0f) << 8));
	}
    }
  
  // NOTE(l4v): Writing pixel info to file
  char* Filename = "../data/generated.ppm";
  FILE* File = fopen(Filename, "w");
  if(!File)
    {
      printf("ERROR: COULD NOT OPEN FILE %s\n", Filename);
      return;
    }
  fprintf(File, "P6\n%d %d\n255\n", Buffer->Width, Buffer->Height);
  Pixel = (uint32*)Buffer->Memory;
  for(size_t PixelIndex = 0;
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
  material Ivory = {};
  Ivory.Albedo = (vector3f){.X = 0.6f, .Y = 0.3f, .Z = 0.1f};
  Ivory.DiffuseColor = (vector3f){.X = 0.4f, .Y = 0.4f, .Z = 0.3f};
  Ivory.SpecularExponent = 50.0f;
  material RedRubber = {};
  RedRubber.Albedo = (vector3f){.X = 0.9f, .Y = 0.1f, .Z = 0.0f};
  RedRubber.DiffuseColor = (vector3f){.X = 0.3f, .Y = 0.1f, .Z = 0.1f};
  RedRubber.SpecularExponent = 10.0f;
  material Mirror = {};
  Mirror.Albedo = (vector3f){.X = 0.0f, .Y = 10.0f, .Z = 0.8f};
  Mirror.DiffuseColor = (vector3f){.X = 1.0f, .Y = 1.0f, .Z = 1.0f};
  Mirror.SpecularExponent = 10.0f;
  
  
  game_state GameState = {};
  GameState.SphereCount = ArrayCount(GameState.Spheres);
  GameState.LightCount = ArrayCount(GameState.Lights);

  GameState.Lights[0].Position = (vector3f){.X = -20, .Y = 20, .Z = 20};
  GameState.Lights[0].Intensity = 1.5f;

  GameState.Lights[1].Position = (vector3f){.X = 30, .Y = 50, .Z = -25};
  GameState.Lights[1].Intensity = 1.8f;
  
  GameState.Lights[2].Position = (vector3f){.X = 30, .Y = 20, .Z = 30};
  GameState.Lights[2].Intensity = 1.7f;
  
  GameState.Spheres[0].Center =
    (vector3f){.X = -3 , .Y = 0, .Z = -16};
  GameState.Spheres[0].Radius = 2;
  GameState.Spheres[0].Material = Ivory;
  
  GameState.Spheres[1].Center =
    (vector3f){.X = -1.0f , .Y = -1.5f, .Z = -12.0f};
  GameState.Spheres[1].Radius = 2;
  GameState.Spheres[1].Material = Mirror;
  
  GameState.Spheres[2].Center =
    (vector3f){.X = 1.5f , .Y = -0.5f, .Z = -18.0f};
  GameState.Spheres[2].Radius = 3;
  GameState.Spheres[2].Material = RedRubber;
  
  GameState.Spheres[3].Center =
    (vector3f){.X = 7 , .Y = 5, .Z = -18.0f};
  GameState.Spheres[3].Radius = 4;
  GameState.Spheres[3].Material = Mirror;
  
  Render(&GameState, &GlobalBackBuffer);
  
  munmap(GlobalBackBuffer.Memory, FrameBufferSize);
  return 0;
}
