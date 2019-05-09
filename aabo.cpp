#include "stdio.h"
#include <vector>
#include <time.h>
#include <math.h>
#include <immintrin.h>

struct Clock
{
  const clock_t m_start;
  Clock() : m_start(clock())
  {
  }
  float seconds() const
  {
    const clock_t end = clock();
    const float seconds = ((float)(end - m_start)) / CLOCKS_PER_SEC;
    return seconds;
  }
};

struct float2
{
  float x,y;
};

struct float3
{
  float x,y,z;
};
  
float3 operator+(const float3 a, const float3 b)
{
  float3 c = {a.x+b.x, a.y+b.y, a.z+b.z};
  return c;
}

float dot(const float3 a, const float3 b)
{
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

float length(const float3 a)
{
  return sqrtf(dot(a,a));
}

float3 min(const float3 a, const float3 b)
{
  float3 c = {std::min(a.x,b.x), std::min(a.y,b.y), std::min(a.z,b.z)};
  return c;
}

float3 max(const float3 a, const float3 b)
{
  float3 c = {std::max(a.x,b.x), std::max(a.y,b.y), std::max(a.z,b.z)};
  return c;
}

union float4
{
  __m128 m;
  struct { float a,b,c,d; };
};

float4 min(const float4 a, const float4 b)
{
  float4 c = {std::min(a.a,b.a), std::min(a.b,b.b), std::min(a.c,b.c), std::min(a.d,b.d)};
  return c;
}

float4 max(const float4 a, const float4 b)
{
  float4 c = {std::max(a.a,b.a), std::max(a.b,b.b), std::max(a.c,b.c), std::max(a.d,b.d)};
  return c;
}

float random(float lo, float hi)
{
  const int grain = 10000;
  const float t = (rand() % grain) * 1.f/(grain-1);
  return lo + (hi - lo) * t;
}

struct Mesh
{
  std::vector<float3> m_point;
  void Generate(int points, float radius)
  {
    m_point.resize(points);
    for(int p = 0; p < points; ++p)
    {
      do
      {
        m_point[p].x = random(-radius, radius);
        m_point[p].y = random(-radius, radius);
        m_point[p].z = random(-radius, radius);
      } while(length(m_point[p]) > radius);
    }
  }
};

struct Object
{
  Mesh *m_mesh;
  float3 m_position;
  void CalculateAABB(float3* mini, float3* maxi) const
  {
    const float3 xyz = m_position + m_mesh->m_point[0];
    *mini = *maxi = xyz;
    for(int p = 1; p < m_mesh->m_point.size(); ++p)
    {
      const float3 xyz = m_position + m_mesh->m_point[p];
      *mini = min(*mini, xyz);
      *maxi = max(*maxi, xyz);
    }
  }
  void CalculateAABO(float4* mini, float4* maxi) const
  { 
    const float3 xyz = m_position + m_mesh->m_point[0];
    const float4 abcd = {xyz.x, xyz.y, xyz.z, -(xyz.x + xyz.y + xyz.z)};
    *mini = *maxi = abcd;
    for(int p = 1; p < m_mesh->m_point.size(); ++p)
    {
      const float3 xyz = m_position + m_mesh->m_point[p];
      const float4 abcd = {xyz.x, xyz.y, xyz.z, -(xyz.x + xyz.y + xyz.z)};
      *mini = min(*mini, abcd);
      *maxi = max(*maxi, abcd);
    }
  };
};

int main(int argc, char* argv[])
{
  const int kMeshes = 100;
  Mesh mesh[kMeshes];
  for(int m = 0; m < kMeshes; ++m)
    mesh[m].Generate(50, 1.0f);

  const int kTests = 100;
  
  const int kObjects = 10000000;
  Object* objects = new Object[kObjects];
  for(int o = 0; o < kObjects; ++o)
  {
    objects[o].m_mesh = &mesh[rand() % kMeshes];
    objects[o].m_position.x = random(-50.f, 50.f);
    objects[o].m_position.y = random(-50.f, 50.f);
    objects[o].m_position.z = random(-50.f, 50.f);
  }
  
  float3* aabbMin = new float3[kObjects];
  float3* aabbMax = new float3[kObjects];
  for(int a = 0; a < kObjects; ++a)
    objects[a].CalculateAABB(&aabbMin[a], &aabbMax[a]);

  float2* aabbX = new float2[kObjects];
  float2* aabbY = new float2[kObjects];
  float2* aabbZ = new float2[kObjects];
  for(int a = 0; a < kObjects; ++a)
  {
    aabbX[a].x = aabbMin[a].x;
    aabbX[a].y = aabbMax[a].x;
    aabbY[a].x = aabbMin[a].y;
    aabbY[a].y = aabbMax[a].y;
    aabbZ[a].x = aabbMin[a].z;
    aabbZ[a].y = aabbMax[a].z;
  }

  float4 *aabbXY = new float4[kObjects]; 
  float4 *aabbZZ = new float4[kObjects/2];
  {
    float2* ZZ = (float2*)aabbZZ;
    for(int o = 0; o < kObjects; ++o)
    {
      aabbXY[o].a =  aabbMin[o].x;
      aabbXY[o].b = -aabbMax[o].x; // so SIMD tests are <= x4
      aabbXY[o].c =  aabbMin[o].y;
      aabbXY[o].d = -aabbMax[o].y; // so SIMD tests are <= x4
          ZZ[o].x =  aabbMin[o].z; 
          ZZ[o].y = -aabbMax[o].z; // so SIMD tests are <= x4
    }
  }
  
  float4* aabtMin = new float4[kObjects];
  float4* aabtMax = new float4[kObjects];
  for(int a = 0; a < kObjects; ++a)
    objects[a].CalculateAABO(&aabtMin[a], &aabtMax[a]);

  float4* heptaMin = new float4[kObjects];
  float4* heptaMax = new float4[kObjects];
  for(int a = 0; a < kObjects; ++a)
  {
    heptaMin[a].a = aabbMin[a].x;
    heptaMin[a].b = aabbMin[a].y;
    heptaMin[a].c = aabbMin[a].z;
    heptaMin[a].d = -(aabbMax[a].x + aabbMax[a].y + aabbMax[a].z);
    heptaMax[a].a = aabbMax[a].x;
    heptaMax[a].b = aabbMax[a].y;
    heptaMax[a].c = aabbMax[a].z;
    heptaMax[a].d = -(aabbMin[a].x + aabbMin[a].y + aabbMin[a].z);
  }

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const float3 queryMin = aabbMin[test];
      const float3 queryMax = aabbMax[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const float3 objectMin = aabbMin[t];
        if(objectMin.x <= queryMax.x
        && objectMin.y <= queryMax.y
        && objectMin.z <= queryMax.z)
	{
	  const float3 objectMax = aabbMax[t];
	  if(queryMin.x <= objectMax.x
          && queryMin.y <= objectMax.y
          && queryMin.z <= objectMax.z)
  	    ++intersections;
	}
      }
    }
    const float seconds = clock.seconds();
    
    printf("AABB min/max accepted %d objects in %f seconds\n", intersections, seconds);
  }

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const float2 queryX = aabbX[test];
      const float2 queryY = aabbY[test];
      const float2 queryZ = aabbZ[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const float2 objectX = aabbX[t];
        if(objectX.x <= queryX.y && queryX.x <= objectX.y)
	{
          const float2 objectY = aabbY[t];
          if(objectY.x <= queryY.y && queryY.x <= objectY.y)
    	  {
            const float2 objectZ = aabbZ[t];
	    if(objectZ.x <= queryZ.y && queryZ.x <= objectZ.y)
    	      ++intersections;
	  }
	}
      }
    }
    const float seconds = clock.seconds();
    
    printf("AABB x/y/z accepted %d objects in %f seconds\n", intersections, seconds);
  }

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const float4 queryMax = aabtMax[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const float4 objectMin = aabtMin[t];
        if(objectMin.a <= queryMax.a
        && objectMin.b <= queryMax.b
        && objectMin.c <= queryMax.c
        && objectMin.d <= queryMax.d)
        {
          ++intersections;
        }
      }
    }
    const float seconds = clock.seconds();
    
    printf("tetrahedron accepted %d objects in %f seconds\n", intersections, seconds);
  }

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const float4 queryMin = aabtMin[test];
      const float4 queryMax = aabtMax[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const float4 objectMin = aabtMin[t];
        if(objectMin.a <= queryMax.a
        && objectMin.b <= queryMax.b
        && objectMin.c <= queryMax.c
        && objectMin.d <= queryMax.d)
        {
	  const float4 objectMax = aabtMax[t];
	  if(queryMin.a <= objectMax.a
	  && queryMin.b <= objectMax.b
	  && queryMin.c <= objectMax.c
          && queryMin.d <= objectMax.d)
	  {
	    ++intersections;
	  }
        }
      }
    }
    const float seconds = clock.seconds();
    
    printf("octahedron accepted %d objects in %f seconds\n", intersections, seconds);
  }

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const float4 queryMin = heptaMin[test];
      const float4 queryMax = heptaMax[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const float4 objectMin = heptaMin[t];
        if(objectMin.a <= queryMax.a
        && objectMin.b <= queryMax.b
        && objectMin.c <= queryMax.c
        && objectMin.d <= queryMax.d)
        {
	  const float4 objectMax = heptaMax[t];
	  if(queryMin.a <= objectMax.a
	  && queryMin.b <= objectMax.b
          && queryMin.c <= objectMax.c)
	  {
	    ++intersections;
	  }
        }
      }
    }
    const float seconds = clock.seconds();
    
    printf("7-plane AABB accepted %d objects in %f seconds\n", intersections, seconds);
  }

  printf("\n");

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      float4 queryXY, queryZZ;
      queryXY   = aabbXY[test];
      queryZZ.m = _mm_loadu_ps((float*)aabbZZ + test * 2);

      queryXY.m = _mm_sub_ps(_mm_setzero_ps(), queryXY.m);    
      queryZZ.m = _mm_sub_ps(_mm_setzero_ps(), queryZZ.m);

      queryXY.m = _mm_shuffle_ps(queryXY.m, queryXY.m, _MM_SHUFFLE(2,3,0,1));
      queryZZ.m = _mm_shuffle_ps(queryZZ.m, queryZZ.m, _MM_SHUFFLE(0,1,0,1));
      for(int t = 0; t < kObjects; ++t)
      {
        const float4 objectXY = aabbXY[t];
        if(_mm_movemask_ps(_mm_cmplt_ps(queryXY.m, objectXY.m)) == 0x0)
        {
	  float4 objectZZ;
	  objectZZ.m = _mm_loadu_ps((float*)aabbZZ + t * 2);
	  objectZZ.m = _mm_movelh_ps(objectZZ.m, objectZZ.m);
	  if(_mm_movemask_ps(_mm_cmplt_ps(queryZZ.m, objectZZ.m)) == 0x0)
	  {
	      ++intersections;
	  }
	}
      }
    }
    const float seconds = clock.seconds();
    
    printf("6-plane AABB SIMD XY,Z accepted %d objects in %f seconds\n", intersections, seconds);
  }

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      float4 queryXY, queryZZ;
      queryXY   = aabbXY[test];
      queryZZ.m = _mm_loadu_ps((float*)aabbZZ + test * 2);

      queryXY.m = _mm_sub_ps(_mm_setzero_ps(), queryXY.m);    
      queryZZ.m = _mm_sub_ps(_mm_setzero_ps(), queryZZ.m);

      queryXY.m = _mm_shuffle_ps(queryXY.m, queryXY.m, _MM_SHUFFLE(2,3,0,1));
      queryZZ.m = _mm_shuffle_ps(queryZZ.m, queryZZ.m, _MM_SHUFFLE(0,1,0,1));
      for(int t = 0; t < kObjects; ++t)
      {
	  float4 objectZZ;
	  objectZZ.m = _mm_loadu_ps((float*)aabbZZ + t * 2);
	  objectZZ.m = _mm_movelh_ps(objectZZ.m, objectZZ.m);
	  if(_mm_movemask_ps(_mm_cmplt_ps(queryZZ.m, objectZZ.m)) == 0x0)
	  {
            const float4 objectXY = aabbXY[t];
            if(_mm_movemask_ps(_mm_cmplt_ps(queryXY.m, objectXY.m)) == 0x0)
            {
	      ++intersections;
	    }
	  }
      }
    }
    const float seconds = clock.seconds();
    
    printf("6-plane AABB SIMD Z,XY accepted %d objects in %f seconds\n", intersections, seconds);
  }

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const float4 queryMin = heptaMin[test];
      const float4 queryMax = heptaMax[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const float4 objectMin = heptaMin[t];
        if(_mm_movemask_ps(_mm_cmplt_ps(queryMax.m, objectMin.m)) == 0x0)
        {
	  const float4 objectMax = heptaMax[t];
	  if(_mm_movemask_ps(_mm_cmplt_ps(objectMax.m, queryMin.m)) == 0x0)
	  {
	    ++intersections;
	  }
        }
      }
    }
    const float seconds = clock.seconds();
    
    printf("7-plane AABB SIMD accepted %d objects in %f seconds\n", intersections, seconds);
  }

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const float4 queryMin = aabtMin[test];
      const float4 queryMax = aabtMax[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const float4 objectMin = aabtMin[t];
        if(_mm_movemask_ps(_mm_cmplt_ps(queryMax.m, objectMin.m)) == 0x0)
        {
	  const float4 objectMax = aabtMax[t];
	  if(_mm_movemask_ps(_mm_cmplt_ps(objectMax.m, queryMin.m)) == 0x0)
	  {
	    ++intersections;
	  }
        }
      }
    }
    const float seconds = clock.seconds();
    
    printf("octahedron SIMD accepted %d objects in %f seconds\n", intersections, seconds);
  }

  return 0;
}
