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

float3 operator-(const float3 a, const float3 b)
{
  float3 c = {a.x-b.x, a.y-b.y, a.z-b.z};
  return c;
}

float3 operator*(const float3 a, const float b)
{
  float3 c = {a.x*b, a.y*b, a.z*b};
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

union float4
{
  __m128 abcd;
  struct { float a,b,c,d; };
};

bool operator<=(const float4 a, const float4 b)
{
  return _mm_movemask_ps(_mm_cmple_ps(a.abcd,b.abcd)) == 0xF;
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

float4 operator*(const float4 a, const float4 b)
{
  float4 c;
  c.abcd = _mm_mul_ps(a.abcd,b.abcd);
  return c;
}

float4 operator+(const float4 a, const float4 b)
{
  float4 c;
  c.abcd = _mm_add_ps(a.abcd,b.abcd);
  return c;
}

float4 operator-(const float4 a, const float4 b)
{
  float4 c;
  c.abcd = _mm_sub_ps(a.abcd,b.abcd);
  return c;
}

typedef float4 AABT;
typedef float4 BoundingSphere;

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
    const float x = random(0.5f,1.f);
    const float y = random(0.5f,1.f);
    const float z = random(0.5f,1.f);
    m_point.resize(points);
    for(int p = 0; p < points; ++p)
    {
      do
      {
        m_point[p].x = x * random(-radius, radius);
        m_point[p].y = y * random(-radius, radius);
        m_point[p].z = z * random(-radius, radius);
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
  void CalculateBoundingSphere(BoundingSphere* sphere) const
  {
    AABB aabb;
    CalculateAABB(&aabb);
    const float3 center = (aabb.m_min + aabb.m_max) * 0.5f;
    float maxRadius = 0.f;
    for(int p = 0; p < m_mesh->m_point.size(); ++p)
    {
      const float3 xyz = m_position + m_mesh->m_point[p];
      const float radius = length(xyz - center);
      if(radius > maxRadius)
	maxRadius = radius;
    }
    sphere->a = center.x;
    sphere->b = center.y;
    sphere->c = center.z;
    sphere->d = maxRadius;
  }
};

int main(int argc, char* argv[])
{
  const int kMeshes = 100;
  std::vector<Mesh> mesh(kMeshes);
  for(int m = 0; m < kMeshes; ++m)
    mesh[m].Generate(100, 1.0f);

  const int kTests = 100;
  
  const int kObjects = 10000000;
  std::vector<Object> objects(kObjects);
  for(int o = 0; o < kObjects; ++o)
  {
    objects[o].m_mesh = &mesh[rand() % kMeshes];
    objects[o].m_position.x = random(-50.f, 50.f);
    objects[o].m_position.y = random(-50.f, 50.f);
    objects[o].m_position.z = random(-50.f, 50.f);
  }
  
  std::vector<BoundingSphere> boundingSphere(kObjects);
  for(int a = 0; a < kObjects; ++a)
    objects[a].CalculateBoundingSphere(&boundingSphere[a]);
  
  std::vector<AABT> aabtMin(kObjects);
  std::vector<AABT> aabtMax(kObjects);
  for(int a = 0; a < kObjects; ++a)
    objects[a].CalculateAABT(&aabtMin[a], &aabtMax[a]);
  
  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const BoundingSphere probe = boundingSphere[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const BoundingSphere target = boundingSphere[t];
	const float4 sub = probe - target;
	const float4 add = probe + target;
	const float4 subSquared = sub * sub;
	const float4 addSquared = add * add;
	const __m128 xxxx = _mm_shuffle_ps(subSquared.abcd, subSquared.abcd, _MM_SHUFFLE(0,0,0,0));
	const __m128 yyyy = _mm_shuffle_ps(subSquared.abcd, subSquared.abcd, _MM_SHUFFLE(1,1,1,1));
	const __m128 zzzz = _mm_shuffle_ps(subSquared.abcd, subSquared.abcd, _MM_SHUFFLE(2,2,2,2));
	const __m128 squaredDistance        = _mm_add_ps(xxxx, _mm_add_ps(yyyy, zzzz));
	const __m128 squaredMaximumDistance = _mm_shuffle_ps(addSquared.abcd, addSquared.abcd, _MM_SHUFFLE(3,3,3,3));
	if(_mm_movemask_ps(_mm_cmple_ps(squaredDistance, squaredMaximumDistance)) == 0xF)
  	  ++intersections;
      }
    }
    const float seconds = clock.seconds();
    
    printf("Bounding Sphere reported %d intersections in %f seconds\n", intersections, seconds);
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
	  {
	    ++intersections;
	  }
        }
      }
    }
    const float seconds = clock.seconds();
    
    printf("AABO SIMD reported %d intersections in %f seconds\n", intersections, seconds);
  }
  return 0;
}
