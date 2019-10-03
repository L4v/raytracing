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

internal inline real32
Dot3D(Vector3f* A, Vector3f* B)
{
  return A->X * B->X + A->Y * B->Y + A->Z * B->Z;
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
