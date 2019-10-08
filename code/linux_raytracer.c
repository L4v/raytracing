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
RaySphereIntersect(vector3f* Origin, vector3f* Dir,
		   sphere* Sphere, real32* SphereDist)
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
  vector3f OC = Subtract3D(Origin, &Sphere->Center);
  vector3f UnitDir = Normalize3D(Dir);
  real32 A = Dot3D(&UnitDir, &UnitDir);
  real32 B = 2.0f * Dot3D(&OC, &UnitDir);
  real32 C = Dot3D(&OC, &OC) - (Sphere->Radius * Sphere->Radius);
  real32 Discriminant = B*B - 4*A*C;

  if(Discriminant < 0)
    {
      return 0;
    }

  *SphereDist = (-B - sqrt(Discriminant)) / (2.0f * A);
  return 1;
}

internal bool32
SceneIntersect(vector3f* Origin, vector3f* Dir,
	       vector3f* Normal, vector3f* Point,
	       material* Material, game_state* GameState)
{
  /* // NOTE(l4v): Sphere collisions */
  real32 ClosestSphere = FLT_MAX;
  for(size_t SphereIndex = 0;
      SphereIndex < GameState->SphereCount;
      ++SphereIndex)
    {
      real32 CurrSphereDist = 0;
      sphere* CurrSphere = &GameState->Spheres[SphereIndex];
      if(RaySphereIntersect(Origin, Dir, CurrSphere, &CurrSphereDist)
  	 && (CurrSphereDist < ClosestSphere))
  	{
  	  ClosestSphere = CurrSphereDist;

  	  vector3f ScaledDir = Scale3D(Dir, ClosestSphere);
  	  *Point = Add3D(Origin, &ScaledDir);
  	  *Normal = Subtract3D(Point, &CurrSphere->Center);
  	  *Normal = Normalize3D(Normal);
  	  *Material = CurrSphere->Material;
  	}
    }
  
  return (ClosestSphere < 1000);
}

internal vector3f
CastRay(vector3f* Origin, vector3f* Direction, game_state* GameState)
{
  vector3f Result = {};
  material Material = {};
  vector3f Point = (vector3f){};
  vector3f Normal = (vector3f){};

  if(!SceneIntersect(Origin, Direction, &Normal, &Point, &Material, GameState))
    {
      // NOTE(l4v): Return background color
      Result = (vector3f){.X = 0.8f, .Y = 0.8f, .Z = 0.8f};
      return Result;
    }

  // NOTE(l4v): Calculating lighting
  real32 DiffusionIndex = 0.0f;
  real32 SpecularIndex = 0.0f;
  for(size_t LightsIndex = 0;
      LightsIndex < GameState->LightCount;
      ++LightsIndex)
    {
      real32 Epsilon = 1e-3;
      vector3f Bias = Scale3D(&Normal, Epsilon);
      light* CurrLight = &GameState->Lights[LightsIndex];
      vector3f LightDir = Subtract3D(&CurrLight->Position, &Point);
      real32 LightLen = GetLen3D(LightDir);
      LightDir = Normalize3D(&LightDir);
      
#if 1
      // NOTE(l4v): Shadows
      // ------------------
      vector3f ShadowOrigin = {};
      if(Dot3D(&LightDir, &Normal) >= 0.0f)
	{
	  Bias = Scale3D(&Bias, -1.0f);
	}
      ShadowOrigin = Add3D(&Point, &Bias);
      vector3f ShadowPoint = {};
      vector3f ShadowNormal = {};
      material TmpMaterial = {};
      if(SceneIntersect(&ShadowOrigin, &LightDir, &ShadowNormal, &ShadowPoint, &TmpMaterial, GameState)
	 && GetLen3D(Subtract3D(&ShadowPoint, &ShadowOrigin)) < LightLen)
	{
	  continue;
	}
#endif
      // NOTE(l4v): Diffusion
      // --------------------
      DiffusionIndex += CurrLight->Intensity *
	MaxReal32(0.0f, Dot3D(&LightDir, &Normal));

      // NOTE(l4v): Specular
      // -------------------
      vector3f NegLightDir = Scale3D(&LightDir, -1);
      vector3f Reflected = Reflect3D(&NegLightDir, &Normal);
      Reflected = Scale3D(&Reflected, -1);
      SpecularIndex += pow(MaxReal32(0.0f, Dot3D(&Reflected, Direction)), Material.SpecularExponent) *
	CurrLight->Intensity;
      
    }
  vector3f DiffusionPart = Scale3D(&Material.DiffuseColor, DiffusionIndex * Material.Albedo.X);
  vector3f SpecularPart = {.X = SpecularIndex * Material.Albedo.Y,
			   .Y = SpecularIndex * Material.Albedo.Y,
			   .Z = SpecularIndex * Material.Albedo.Y};
  Result = (vector3f){};
  Result = Add3D(&DiffusionPart, &SpecularPart);
  return Result;
}

internal void
Render(game_state* GameState, linux_offscreen_buffer* Buffer)
{
  real32 FOV = Pi32 / 3.0f;
  uint32* Pixel = (uint32*)Buffer->Memory;
  for(size_t Row = 0;
      Row < Buffer->Height;
      ++Row)
    {
      for(size_t Column = 0;
	  Column < Buffer->Width;
	  ++Column)
	{
	  real32 PixelX = (2 * (Column + 0.5f) / (real32)Buffer->Width - 1) *
	    tan(FOV / 2.0f) * Buffer->Width / (real32)Buffer->Height;
	  real32 PixelY = -(2 * (Row + 0.5f) / (real32)Buffer->Height - 1) * tan(FOV / 2.0f);
	  real32 Distance = 1.0f;

	  vector3f Origin = {};
	  vector3f Camera = {.X = PixelX, .Y = PixelY, .Z = -Distance};
	  vector3f CameraDir = Normalize3D(&Camera);
	  
	  vector3f RealColor = CastRay(&Origin, &CameraDir, GameState);
	  real32 MaxValue = MaxReal32(RealColor.X, MaxReal32(RealColor.Y, RealColor.Z));
	  if(MaxValue > 1)
	    {
	      RealColor = Scale3D(&RealColor, (1.0f / MaxValue));
	    }
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
  GameState.Spheres[1].Material = RedRubber;
  
  GameState.Spheres[2].Center =
    (vector3f){.X = 1.5f , .Y = -0.5f, .Z = -18.0f};
  GameState.Spheres[2].Radius = 3;
  GameState.Spheres[2].Material = RedRubber;
  
  GameState.Spheres[3].Center =
    (vector3f){.X = 7 , .Y = 5, .Z = -18.0f};
  GameState.Spheres[3].Radius = 4;
  GameState.Spheres[3].Material = Ivory;
  
  GameState.Spheres[4].Center =
    (vector3f){.X = -5 , .Y = 5, .Z = -14.0f};
  GameState.Spheres[4].Radius = 4;
  GameState.Spheres[4].Material = Mirror;
  
  Render(&GameState, &GlobalBackBuffer);
  
  munmap(GlobalBackBuffer.Memory, FrameBufferSize);
  return 0;
}
