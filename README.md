```
       Bounding Volume |  trivials |  trivials | accepts | seconds
------------------------------------------------------------------
          AABB MIN,MAX |         0 | 152349412 |   39229 | 4.7276
            AABB X,Y,Z |  34310232 |   1154457 |   39229 | 4.2566
           Tetrahedron |         0 |         0 |   67752 | 0.3066
            Octahedron |         0 |     67752 |   33793 | 2.0796
          7-Sided AABB |         0 |    172382 |   39229 | 2.9394

6-Sided AABB XY,Z SIMD |         0 |   1154457 |   39229 | 1.8369
6-Sided AABB Z,XY SIMD |         0 |  33819438 |   39229 | 2.5227
     7-Sided AABB SIMD |         0 |    172382 |   39229 | 1.6998
       Octahedron SIMD |         0 |     67752 |   33793 | 1.7737
```

The Axis-Aligned Bounding Octahedron and The 7-Sided AABB
=========================================================

>In computer graphics and computational geometry, a bounding volume for a set of objects is a closed 
>volume that completely contains the union of the objects in the set. Bounding volumes are used to 
>improve the efficiency of geometrical operations by using simple volumes to contain more complex objects. 
>Normally, simpler volumes have simpler ways to test for overlap.

The axis-aligned bounding box and bounding sphere are considered to be the simplest bounding volumes, and therefore are ubiquitous in realtime and large-scale applications.

There is a simpler bounding volume unknown to industry and literature. By virtue of this simplicity it has nice properties, such as high performance in space and time. It is the Axis-Aligned Bounding Simplex.

Simplex
-------

In two dimensions a simplex is a triangle, and in three it is a tetrahedron. Generally speaking, a simplex is the fewest half-spaces necessary to enclose space: one more than the number of dimensions, or N+1. By contrast, a bounding box is 2N half-spaces.

In three dimensions a simplex has four (3+1) half-spaces and a bounding box has six (3*2). That’s 50% more work in order to determine intersection.

Axis-Aligned Bounding Triangle
------------------------------

We will work in two dimensions first, since it is simpler and extends trivially to three dimensions and beyond.

In two dimensions, a simplex is a triangle. The following are data structures for an axis-aligned bounding box and axis-aligned bounding triangle in two dimensions:

```
struct Box
{ 
  float minX, minY, maxX, maxY; 
}; 

struct Triangle
{ 
  float A, B, C; 
}; 
```

2D AABB is well-understood. Here is an example of an object and its 2D AABB, where X bounds are red and Y are green:

![A horse enclosed in a 2D AABB](images/horse_box.png)

2D AABT (axis-aligned bounding triangle) is not as well known. It does not use the X and Y axes - it uses the three axes ABC, which could have the values {X, Y, -(X+Y)}, but for simplicity’s sake let’s say they are at 120 degree angles to each other:

![The ABC axes point at vertices of an equilateral triangle](images/abc_axes.png)

The points from the horse image above can each be projected onto the ABC axes, and the minimum and maximum values for A, B, and C can be found, just as with AABB and XY:

![A horse enclosed in opposing bounding triangles](images/horse_dual_triangle.png)

Interestingly, however, it is possible to perform an intersection test without looking at both the min and max values, unlike with AABB. Because {minA, minB, minC} define a triangle, we can trivially reject against those three values in isolation, without considering {maxA, maxB, maxC}:

![A horse enclosed in opposing bounding triangles](images/horse_triangle.png)

That is why the data structure for an axis-aligned bounding triangle requires only (minA, minB, minC): in 2D, three values are sufficient to perform trivial intersection rejection tests:

![A bounding triangle of minimum axis values](images/triangle_min.png)

To perform a trivial rejection against a group of {minA, minB, minC} target objects, your probe object would need to have the form {maxA, maxB, maxC}:

![A bounding triangle of maximum axis values](images/triangle_max.png)

And for each rejection test, if the probe’s maxA < the object’s minA (or B or C), they do not intersect. This is true of the two above triangles: they do not intersect.

There is no need for each object to store a {maxA, maxB, maxC} in addition to a {minA, minB, minC} simply to do intersection tests - only the probe needs {maxA, maxB, maxC}. So if we stop here, we have a novel bounding volume with roughly the same characteristics as AABB, but 25% cheaper in 2D and 33% cheaper in 3D than AABB.

But if both min and max are stored in each object, an axis-aligned bounding hexagon appears: 
 
Axis-Aligned Bounding Hexagons
------------------------------

The axis-aligned bounding hexagon (AABH) has six planes, which makes it 50% bigger than a 2D AABB with four planes: 

```
struct Box
{ 
  float minX, minY, maxX, maxY; 
}; 

struct Triangle
{ 
  float A, B, C; 
}; 

struct Hexagons 
{ 
  Triangle *minABC; // triangles that point up
  Triangle *maxABC; // triangles that point down
};
```

However, the hexagon has the nice property that it is made of two independent axis-aligned bounding triangles (minABC and maxABC), and unless two hexagons are nearly overlapping, a check of one hexagon’s minABC triangle vs the other’s maxABC triangle (or vice versa) is sufficient for initial trivial intersection rejection. 

Therefore, If the minABC triangles are stored separately from the maxABC triangles (as above,) a bounding hexagon check is usually as cheap as a bounding triangle check, since the second triangle is rarely needed.

No three of a 2D AABB's four planes describe a closed shape. If you were to try to do an intersection check with less than four of an AABB's planes, the shape would have infinite area. This is larger than the finite area of an AABH's first triangle. That is the essential advantage of AABH.

For example, {minX, minY, maxX} is not a closed shape - it is unbounded in the direction of +Y. The same is true of any three of a 2D AABB's four values. The {minA, minB, minC} of an AABH, however, is always an equilateral triangle, which is a closed shape.

AABH has a larger memory footprint than AABB, but (usually) uses less memory bandwidth and computation than AABB.

Axis-Aligned Bounding Octahedron
--------------------------------

Everything above extends trivially to three and higher dimensions. In three dimensions, an axis-aligned bounding box, axis-aligned bounding tetrahedron, and axis-aligned bounding octahedron have the following structure:

```
struct Box
{ 
  float minX, minY, minZ, maxX, maxY, maxZ; 
}; 

struct Tetrahedron
{ 
  float A, B, C, D; 
}; 

struct Octahedra
{ 
  Tetrahedron *minABCD; // tetrahedrons that point up
  Tetrahedron *maxABCD; // tetrahedrons that point down
};
```

AABO uses 33% more memory than AABB, but since only the one of the two tetrahedra need be checked for initial trivial rejection, an AABO check is usually four comparisons, while a 3D AABB check is six. AABO uses 33% less bandwidth and computation than AABB, and has 33% more planes than AABB, for making tighter bounding shapes.

Comparison to k-DOP
-------------------

Christer Ericson’s book “Real-Time Collision Detection” has the following to say about k-DOP, whose DOP8 is similar to Axis Aligned Bounding Octahedron:

![Christer Ericon's book, talking about k-DOP](images/kdop.png)

k-DOP is different from the ideas in this paper, in the following ways:

* An Axis-Aligned Bounding Simplex does not have opposing planes, so it is not a k-DOP
* A k-DOP does not have dual polyhedra if there exists a hemisphere that contains none of its axes. A 6DOP always has a hemisphere that contains no axes, and so there can not be dual polyhedra in a 6DOP. Nowhere can we find discussion of how choice of axes affects a k-DOP’s ability to have dual polyhedra (one of which can be used for a trivial rejection or acceptance test, in isolation.)
* k-DOP is about opposing planes, and AABO is about opposing pairs of bounding polyhedra. a 6DOP doesn’t have opposing polyhedra - it has only one rectangular solid - but still qualifies as a k-DOP. An 8DOP can have opposing tetrahedra, but nowhere in literature can we find anyone mentioning this or making use of it, despite its large performance advantage.
* k-DOP has faces aligned with [+-1,+-1,+-1] but AABO have axes that point at the vertices of a simplex

Comparison To Bounding Sphere
-----------------------------

A bounding sphere has four scalar values - the same as an Axis Aligned Bounding Tetrahedron:

```
struct AABB { float minX, minY, minZ, maxX, maxY, maxZ; }; // bounding box
struct AABT { float A, B, C, D; }; // bounding tetrahedron
struct BS { float x, y, z, radius; }; // bounding sphere
```

In terms of storage a sphere can be just as efficient as a tetrahedron, but a sphere-sphere check is inherently more expensive, as it requires multiplication and its expression has a deeper dependency graph than an AABB or AABT check.

If the data are stored in very low precision such as uint8_t, the sphere-sphere check will overflow the data precision while performing its calculation, which necessitates expansion to a wider precision before performing the check.

AABB has no such problem, and neither does AABT (triangle or tetrahedron.) Their runtime check requires only comparisons, which can be performed by individual machine instructions in a variety of data precisions.

A bounding sphere can have exactly one shape, but each AABO can be wide and flat, or tall and skinny, or roughly spherical, etc. So, in comparison to an AABO, a bounding sphere may not have very tight bounds. 

The Pragmatic Axes
------------------

Though axes ABC that point at the vertices of an equilateral triangle are elegant and unbiased:

![Elegant axes for Axis Aligned Bounding Triangle](images/abc_axes.png)

Transforming between ABC and XY coordinates is costly, and can be avoided by choosing these more pragmatic axes:

```
A=X
B=Y
C=-(X+Y)
```

![Pragmatic axes for Axis Aligned Bounding Triangle](images/pragmatic.png)

The pragmatic axes look worse, and are worse, but still make triangles that enclose objects pretty well. With these axes, it is possible to construct an AABH from a pre-existing AABB, that has exactly the same shape as the AABB, and where the 6th check is unnecessary:

```
{minX, minY, -(maxX + maxY)}
{maxX, maxY, -(minX + minY)}
```

![Pre-existing AABB to AABO](images/pragmatic_post.png)

This AABH won't trivially reject any more objects than the original AABB, but the AABH will take less time to reject objects, because there are (usually) 3 checks instead of 4. 

If you construct the AABH from the object's vertices instead, you can trivially reject more objects than an AABB can:

```
{minX, minY, -max(X+Y)}
{maxX, maxY, -min(X+Y)}
```

![Pragmatic axis AABO](images/pragmatic_pre.png)

If it's unclear how AABH is superior to AABB when doing a 3 check initial trivial rejection test, the image below may help to 
explain. Even if you were to do 3 checks first with an AABB, no matter which 3 of the 4 checks you pick, the resulting shape is not closed. It fails to exclude an infinite area from the rejection test. 

![Inifinite Volume](images/infinity.png)
