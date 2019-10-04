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
RayIntersect(vector3f* RayOrigin, vector3f* RayDirection,
	     sphere* Sphere, real32* SphereDistance)
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
  vector3f OC = Difference3D(RayOrigin, &Sphere->Center);
  vector3f Dir = Normalize3D(RayDirection);
  real32 A = Dot3D(&Dir, &Dir);
  real32 B = 2.0f * Dot3D(&OC, &Dir);
  real32 C = Dot3D(&OC, &OC) - Sphere->Radius * Sphere->Radius;
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

internal bool32
SceneIntersect(vector3f* Origin, vector3f* Dir,
	       vector3f* Hit, vector3f* Normal,
	       material* Material, game_state* GameState)
{
  real32 SphereDistance = FLT_MAX;
  for(size_t SphereIndex = 0;
      SphereIndex < GameState->SphereCount;
      ++SphereIndex)
    {
      real32 DistI;
      if(RayIntersect(Origin, Dir, &GameState->Spheres[SphereIndex], &DistI) && DistI < SphereDistance)
	{
	  SphereDistance = DistI;
	  vector3f ScaledDir = Scale3D(Dir, DistI);
	  *Hit = Add3D(Origin, &ScaledDir);
	  vector3f HitToCenter = Difference3D(Hit, &GameState->Spheres[SphereIndex].Center);
	  *Normal = Normalize3D(&HitToCenter);
	  *Material = GameState->Spheres[SphereIndex].Material;
	}
    }
  return (SphereDistance < 1000);
}

internal vector3f
CastRay(vector3f* Origin, vector3f* Dir, game_state* GameState)
{
  vector3f Point = {};
  vector3f Normal = {};
  vector3f Result = {};
  material Material = {};
  // NOTE(l4v): Background color
  Material.DiffuseColor.X = 0.2f;
  Material.DiffuseColor.Y = 0.7f;
  Material.DiffuseColor.Z = 0.8f;
  Result = Material.DiffuseColor;

  if(SceneIntersect(Origin, Dir, &Point, &Normal, &Material, GameState))
    {
      // NOTE(l4v): Return sphere color
      real32 Diffusion = 0;
      real32 Specular = 0;
      for(size_t LightsIndex = 0;
	  LightsIndex < GameState->LightCount;
	  ++LightsIndex)
	{
	  vector3f LightDir = Difference3D(&GameState->Lights[LightsIndex].Position, &Point);
	  LightDir = Normalize3D(&LightDir);

#if SHADOWS
	  // TODO NOTE(l4v): Shadows
	  // ------------------
	  real32 LightDistance = GetLen3D(LightDir);
	  vector3f ShadowOrigin = {};
	  vector3f ScaledNormal = Scale3D(&Normal, 1e-3);
	  ShadowOrigin = Dot3D(&LightDir, &Normal) < 0 ?
	    Difference3D(&Point, &ScaledNormal) : Add3D(&Point, &ScaledNormal);
	  vector3f ShadowPoint = {};
	  vector3f ShadowNormal = {};
	  material TmpMaterial = {};
	  if(SceneIntersect(&ShadowOrigin, &LightDir, &ShadowPoint, &ShadowNormal, &TmpMaterial, GameState) &&
	     GetLen3D(Difference3D(&ShadowPoint, &ShadowOrigin)) < LightDistance)
	    {
	      continue;
	    }
#endif
	    
	  // NOTE(l4v): Diffusion
	  // --------------------
	  Diffusion += GameState->Lights[LightsIndex].Intensity * MaxReal(0.0f, Dot3D(&LightDir, &Normal));

	  // NOTE(l4v): Specular
	  // -------------------
	  vector3f NegLightDir = Scale3D(&LightDir, -1);
	  vector3f Reflected = Reflect3D(&NegLightDir, &Normal);
	  Reflected = Scale3D(&Reflected, -1.0f);
	  Specular += (real32)pow(MaxReal(0.0f, Dot3D(&Reflected, Dir)), Material.SpecularExponent) *
	    GameState->Lights[LightsIndex].Intensity;
	}
      Result = Scale3D(&Material.DiffuseColor, Diffusion * Material.Albedo.X);
      vector3f Identity = {.X = 1.0f, .Y = 1.0f, .Z = 1.0f};
      Identity = Scale3D(&Identity, Specular * Material.Albedo.Y);
      Result = Add3D(&Result, &Identity);
    }
  return Result;
}

internal void
Render(game_state* GameState, linux_offscreen_buffer* Buffer)
{
  real32 FOV = Pi32 / 2.0f;
  uint32* Pixel = (uint32*)Buffer->Memory;
  
  // NOTE(l4v): Drawing the scene
  for(int32 SphereIndex = 0;
      SphereIndex < GameState->SphereCount;
      ++SphereIndex)
    {
      sphere* Sphere = &GameState->Spheres[SphereIndex];
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
	      vector3f Temp = {.X = X, .Y = Y, .Z = -1};
	      vector3f Dir = Normalize3D(&Temp);
	      vector3f Zero3D = {};
	      vector3f RealColor = CastRay(&Zero3D, &Dir, GameState);
	      real32 MaxValue = MaxReal(RealColor.X, MaxReal(RealColor.Y, RealColor.Z));
	      if(MaxValue > 1)
		{
		  RealColor = Scale3D(&RealColor, 1.0f / MaxValue);
		}
	      *Pixel++ = ((RoundReal32ToUInt32(RealColor.X * 255.0f) << 24) |
			  (RoundReal32ToUInt32(RealColor.Y * 255.0f) << 16) |
			  (RoundReal32ToUInt32(RealColor.Z * 255.0f) << 8));
	    }
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
  material Ivory = {};
  Ivory.Albedo = (vector2f){.X = 0.6f, .Y = 0.3f};
  Ivory.DiffuseColor = (vector3f){.X = 0.4f, .Y = 0.4f, .Z = 0.3f};
  Ivory.SpecularExponent = 50.0f;
  material RedRubber = {};
  RedRubber.Albedo = (vector2f){.X = 0.9f, .Y = 0.1f};
  RedRubber.DiffuseColor = (vector3f){.X = 0.3f, .Y = 0.1f, .Z = 0.1f};
  RedRubber.SpecularExponent = 10.0f;
  
  
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
  
  Render(&GameState, &GlobalBackBuffer);

  munmap(GlobalBackBuffer.Memory, FrameBufferSize);
  return 0;
}
