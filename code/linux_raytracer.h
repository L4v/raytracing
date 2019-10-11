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
    real32 E[4];
    struct{
      real32 X;
      real32 Y;
      real32 Z;
      real32 W;
    };
  };
} vector4f;

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
  vector4f Albedo;
  real32 SpecularExponent;
  real32 RefractiveIndex;
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
  sphere Spheres[5];
  int32 SphereCount;

  light Lights[3];
  int32 LightCount;
} game_state;

internal inline real32
ClampReal32(real32 Real32, real32 Min, real32 Max)
{
  real32 Result = Real32;
  if(Result < Min)
    {
      Result = Min;
    }
  if(Result > Max)
    {
      Result = Max;
    }
  return Result;
}

internal inline int32
RoundReal32ToInt32(real32 Real32)
{
  int32 Result = (int32)(Real32 + 0.5f);
  
  return Result;
}

internal inline uint32
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
Subtract3D(vector3f* A, vector3f* B)
{
  // NOTE(l4v): A - B
  vector3f Result = {};
  Result.X = A->X - B->X;
  Result.Y = A->Y - B->Y;
  Result.Z = A->Z - B->Z;

  return Result;
}

internal vector2f
Subtract2D(vector2f* A, vector2f* B)
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

internal real32
Dot3D(vector3f* A, vector3f* B)
{
  real32 Result = 0.0f;
  for(size_t i = 0;
      i < 3;
      ++i)
    {
      Result += A->E[i] * B->E[i];
    }
  return Result;
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
  real32 Length = GetLen3D(*A);
  Result = Scale3D(A, 1 / Length);

  return Result;
}

internal vector3f
Reflect3D(vector3f* Incoming, vector3f* Normal)
{
  vector3f Result = {};
  /* real32 ABDot = Dot3D(A, B); */
  /* vector3f ScaledB = Scale3D(B, 2.0f * ABDot); */
  /* Result = Subtract3D(A, &ScaledB);  */
  real32 IncomingDotNormal = Dot3D(Incoming, Normal);
  vector3f ScaledNormal = Scale3D(Normal, 2.0f * IncomingDotNormal);
  Result = Subtract3D(Incoming, &ScaledNormal);
  
  return Result;
}

internal inline real32
MaxReal32(real32 X, real32 Y)
{
  return X > Y ? X : Y;
}

internal inline real32
MinReal32(real32 X, real32 Y)
{
  return X < Y ? X : Y;
}

#define LINUX_RAYTACER_H
#endif
