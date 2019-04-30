#include "stdio.h"
#include <vector>
#include <time.h>
#include <math.h>

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
  float3 m_min;
  float3 m_max;
};

struct float4
{
  float a,b,c,d;
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
    aabb->m_min = aabb->m_max = xyz;
    for(int p = 1; p < m_mesh->m_point.size(); ++p)
    {
      const float3 xyz = m_position + m_mesh->m_point[p];
      aabb->m_min = min(aabb->m_min, xyz);
      aabb->m_max = max(aabb->m_max, xyz);
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
  
  // test how fast AABB is for kObjects * kObjects tests
  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const AABB probe = aabb[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const AABB target = aabb[t];
        if(target.m_min.x <= probe.m_max.x
        && target.m_min.y <= probe.m_max.y
        && target.m_min.z <= probe.m_max.z
        && target.m_max.x >= probe.m_min.x
        && target.m_max.y >= probe.m_min.y
        && target.m_max.z >= probe.m_min.z)  
  	  ++intersections;
      }
    }
    const float seconds = clock.seconds();
    
    printf("AABB reported %d intersections in %f seconds\n", intersections, seconds);
  }
  
  // test how fast AABO is for kObjects * kObjects tests
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
        if(targetMin.a <= probeMax.a
        && targetMin.b <= probeMax.b
        && targetMin.c <= probeMax.c
        && targetMin.d <= probeMax.d)
        {
	  const AABT targetMax = aabtMax[t];
	  if(targetMax.a >= probeMin.a
	  && targetMax.b >= probeMin.b
	  && targetMax.c >= probeMin.c
	  && targetMax.d >= probeMin.d)
	  {
	    ++intersections;
	  }
        }
      }
    }
    const float seconds = clock.seconds();
    
    printf("AABO reported %d intersections in %f seconds\n", intersections, seconds);
  }
  return 0;
}
