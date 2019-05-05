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

struct AABB
{
  float minx, maxx;
  float miny, maxy;
  float minz, maxz;
  void set(const float3 a)
  {
    minx =  a.x;
    miny =  a.y;
    minz =  a.z;
    maxx = -a.x;
    maxy = -a.y;
    maxz = -a.z;
  }
  void add(const float3 a)
  {
    minx = std::min(minx,  a.x);
    miny = std::min(miny,  a.y);
    minz = std::min(minz,  a.z);
    maxx = std::min(maxx, -a.x);
    maxy = std::min(maxy, -a.y);
    maxz = std::min(maxz, -a.z);
  }
};

union float4
{
  __m128 abcd;
  struct { float a,b,c,d; };
};

bool operator<=(const float4 a, const float4 b)
{
  return _mm_movemask_ps(_mm_cmple_ps(b.abcd,a.abcd)) == 0;
}

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

struct AABBSimd
{
  float4 xy;
  float4 zz;
  AABBSimd(const AABB* a)
  {
    xy.abcd = _mm_sub_ps(_mm_setzero_ps(),_mm_loadu_ps((float*)a + 0)); // negate to test vs target with <=
    zz.abcd = _mm_sub_ps(_mm_setzero_ps(),_mm_loadu_ps((float*)a + 4)); // negate to test vs target with <=
    xy.abcd = _mm_shuffle_ps(xy.abcd, xy.abcd, _MM_SHUFFLE(2, 3, 0, 1)); // swap min and max to test vs target with <=
    zz.abcd = _mm_shuffle_ps(zz.abcd, zz.abcd, _MM_SHUFFLE(0, 1, 0, 1)); // swap min and max, make 2 copies of z
  }
};

typedef float4 AABT;

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

const float3 abcdInXyz[4] =
{
 {-1,0,-1/sqrtf(2)}, // A
 {+1,0,-1/sqrtf(2)}, // B
 {0,-1, 1/sqrtf(2)}, // C
 {0,+1, 1/sqrtf(2)}, // D
};

float4 xyzToAbcd(const float3 xyz)
{
  float4 abcd;
  abcd.a = dot(xyz, abcdInXyz[0]);
  abcd.b = dot(xyz, abcdInXyz[1]);
  abcd.c = dot(xyz, abcdInXyz[2]);
  abcd.d = dot(xyz, abcdInXyz[3]);
  return abcd;
}

struct Object
{
  Mesh *m_mesh;
  float3 m_position;
  void CalculateAABB(AABB* aabb) const
  {
    const float3 xyz = m_position + m_mesh->m_point[0];
    aabb->set(xyz);
    for(int p = 1; p < m_mesh->m_point.size(); ++p)
    {
      const float3 xyz = m_position + m_mesh->m_point[p];
      aabb->add(xyz);
    }
  }
  void CalculateAABT(AABT* mini, AABT* maxi) const
  { 
    const float3 xyz = m_position + m_mesh->m_point[0];
    *mini = *maxi = xyzToAbcd(xyz);
    for(int p = 1; p < m_mesh->m_point.size(); ++p)
    {
      const float3 xyz = m_position + m_mesh->m_point[p];
      const float4 abcd = xyzToAbcd(xyz);
      *mini = min(*mini, abcd);
      *maxi = max(*maxi, abcd);
    }
  };
};

int main(int argc, char* argv[])
{
  Mesh mesh;
  mesh.Generate(100, 1.0f);

  const int kTests = 100;
  
  const int kObjects = 10000000;
  std::vector<Object> objects(kObjects);
  for(int o = 0; o < kObjects; ++o)
  {
    objects[o].m_mesh = &mesh;
    objects[o].m_position.x = random(-50.f, 50.f);
    objects[o].m_position.y = random(-50.f, 50.f);
    objects[o].m_position.z = random(-50.f, 50.f);
  }
  
  std::vector<AABB> aabb(kObjects);
  for(int a = 0; a < kObjects; ++a)
    objects[a].CalculateAABB(&aabb[a]);
  
  std::vector<AABT> aabtMin(kObjects);
  std::vector<AABT> aabtMax(kObjects);
  for(int a = 0; a < kObjects; ++a)
    objects[a].CalculateAABT(&aabtMin[a], &aabtMax[a]);
  
  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const AABBSimd probe(&aabb[test]);
      for(int t = 0; t < kObjects; ++t)
      {
        float4 targetxy;
        targetxy.abcd = _mm_loadu_ps((float*)&aabb[t] + 0);
        if(targetxy <= probe.xy)
        {
          float4 targetzz;
          targetzz.abcd = _mm_loadu_ps((float*)&aabb[t] + 4);
          targetzz.abcd = _mm_movelh_ps(targetzz.abcd, targetzz.abcd); // make 2 copies of z
          if(targetzz <= probe.zz)
            ++intersections;
        }
      }
    }
    const float seconds = clock.seconds();
    
    printf("AABB early-out SIMD reported %d intersections in %f seconds\n", intersections, seconds);
  }
  
  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const AABT probeMin = aabtMin[test];
      const AABT probeMax = aabtMax[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const AABT targetMin = aabtMin[t];
        if(targetMin <= probeMax)
        {
	  const AABT targetMax = aabtMax[t];
	  if(probeMin <= targetMax)
	    ++intersections;
        }
      }
    }
    const float seconds = clock.seconds();
    
    printf("AABO SIMD reported %d intersections in %f seconds\n", intersections, seconds);
  }
  return 0;
}
