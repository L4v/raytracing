#ifndef LINUX_RAYTRACER_H

typedef struct
{
  void* Memory;
  int32 Width;
  int32 Height;
  int32 BytesPerPixel;
} linux_offscreen_buffer;

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
} vector3f;

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
} vector2f;

typedef struct
{
  vector3f DiffuseColor;
  vector2f Albedo;
  real32 SpecularExponent;
} material;

typedef struct
{
  vector3f Center;
  real32 Radius;
  
  material Material;
} sphere;

typedef struct
{
  vector3f Position;
  real32 Intensity;
} light;

typedef struct
{
  sphere Spheres[4];
  int32 SphereCount;

  light Lights[3];
  int32 LightCount;
} game_state;

internal int32
RoundReal32ToInt32(real32 Real32)
{
  int32 Result = (int32)(Real32 + 0.5f);
  
  return Result;
}

internal uint32
RoundReal32ToUInt32(real32 Real32)
{
  uint32 Result = (uint32)(Real32 + 0.5f);
  
  return Result;
}

internal vector3f
Add3D(vector3f* A, vector3f* B)
{
  vector3f Result = {};
  Result.X = A->X + B->X;
  Result.Y = A->Y + B->Y;
  Result.Z = A->Z + B->Z;

  return Result;
}

internal vector3f
Difference3D(vector3f* A, vector3f* B)
{
  // NOTE(l4v): A - B
  vector3f Result = {};
  Result.X = A->X - B->X;
  Result.Y = A->Y - B->Y;
  Result.Z = A->Z - B->Z;

  return Result;
}

internal vector2f
Difference2D(vector2f* A, vector2f* B)
{
  // NOTE(l4v): A - B
  vector2f Result = {};
  Result.X = A->X - B->X;
  Result.Y = A->Y - B->Y;

  return Result;
}

internal vector3f
Scale3D(vector3f* A, real32 Scale)
{
  vector3f Result = {};
  Result.X = Scale * A->X;
  Result.Y = Scale * A->Y;
  Result.Z = Scale * A->Z;

  return Result;
}

internal inline real32
Dot3D(vector3f* A, vector3f* B)
{
  return A->X * B->X + A->Y * B->Y + A->Z * B->Z;
}

internal inline real32
GetLen3D(vector3f A)
{
  return sqrt(A.X * A.X + A.Y * A.Y + A.Z * A.Z);
}

internal vector3f
Normalize3D(vector3f* A)
{
  vector3f Result = {};
  real32 Length = sqrt(Dot3D(A, A));
  Result.X = A->X / Length;
  Result.Y = A->Y / Length;
  Result.Z = A->Z / Length;

  return Result;
}

internal vector3f
Reflect3D(vector3f* A, vector3f* B)
{
  vector3f Result = {};
  real32 ABDot = Dot3D(A, B);
  vector3f ScaledB = Scale3D(B, 2.0f * ABDot);
  Result = Difference3D(A, &ScaledB); 
  
  return Result;
}

internal inline real32
MaxReal(real32 X, real32 Y)
{
  return X > Y ? X : Y;
}

internal inline real32
MinReal(real32 X, real32 Y)
{
  return X < Y ? X : Y;
}

#define LINUX_RAYTACER_H
#endif
