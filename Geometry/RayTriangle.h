#pragma once

// Intersect ray with triangle, returns closest point or FLT_MAX if no hit (unoptimized version)
// Taken from: http://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
f_inline float RayTriangleOriginal(const Vec3 &inOrigin, const Vec3 &inDirection, const Vec3 &inV0, const Vec3 &inV1, const Vec3 &inV2)
{
	float epsilon = 1.0e-12f;

	// Find vectors for two edges sharing inV0
	Vec3 e1 = inV1 - inV0;
	Vec3 e2 = inV2 - inV0;

	// Begin calculating determinant - also used to calculate u parameter
	Vec3 p = inDirection.Cross(e2);

	// if determinant is near zero, ray lies in plane of triangle
	float det = e1.Dot(p);

	// NOT CULLING
	if (det > -epsilon && det < epsilon)
		return FLT_MAX;

	// Calculate distance from inV0 to ray origin
	Vec3 s = inOrigin - inV0;

	// Calculate u parameter and test bound
	float u = s.Dot(p) / det;
	
	// The intersection lies outside of the triangle
	if (u < 0.0f || u > 1.0f) 
		return FLT_MAX;

	// Prepare to test v parameter
	Vec3 q = s.Cross(e1);

	// Calculate V parameter and test bound
	float v = inDirection.Dot(q) / det;

	// The intersection lies outside of the triangle
	if (v < 0.0f || u + v  > 1.0f) 
		return FLT_MAX;

	float t = e2.Dot(q) / det;

	if (t < 0.0f)
		return FLT_MAX;

	return t;
}

// Intersect ray with triangle, returns closest point or FLT_MAX if no hit (branch less version)
// Adapted from: http://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
f_inline float RayTriangle(const Vec3 &inOrigin, const Vec3 &inDirection, const Vec3 &inV0, const Vec3 &inV1, const Vec3 &inV2)
{
	// Epsilon
	Vec3 epsilon = Vec3::sReplicate(1.0e-12f);

	// Zero & one
	Vec3 zero = Vec3::sZero();
	Vec3 one = Vec3::sReplicate(1.0f);

	// Find vectors for two edges sharing inV0
	Vec3 e1 = inV1 - inV0;
	Vec3 e2 = inV2 - inV0;

	// Begin calculating determinant - also used to calculate u parameter
	Vec3 p = inDirection.Cross(e2);

	// if determinant is near zero, ray lies in plane of triangle
	Vec3 det = Vec3::sReplicate(e1.Dot(p));

	// Calculate distance from inV0 to ray origin
	Vec3 s = inOrigin - inV0;

	// Calculate u parameter
	Vec3 u = Vec3::sReplicate(s.Dot(p)) / det;

	// Prepare to test v parameter
	Vec3 q = s.Cross(e1);

	// Calculate v parameter
	Vec3 v = Vec3::sReplicate(inDirection.Dot(q)) / det;

	// Get intersection point
	Vec3 t = Vec3::sReplicate(e2.Dot(q)) / det;

	// Check if there is an intersection
	UVec4 no_intersection = 
		UVec4::sOr
		(
			UVec4::sOr
			(
				UVec4::sOr
				(
					Vec3::sLess(det.Abs(), epsilon), 
					Vec3::sLess(u, zero)
				),
				UVec4::sOr
				(
					Vec3::sLess(v, zero), 
					Vec3::sGreater(u + v, one)
				)
			), 
			Vec3::sLess(t, zero)
		);

	// Select intersection point or FLT_MAX based on if there is an intersection or not
	return Vec3::sSelect(t, Vec3::sReplicate(FLT_MAX), no_intersection).GetX();
}

// Same as above but will update ioClosest if hit is closer
f_inline void RayTriangle(const Vec3 &inOrigin, const Vec3 &inDirection, const Vec3 &inV0, const Vec3 &inV1, const Vec3 &inV2, float &ioClosest)
{
	// Epsilon
	Vec3 epsilon = Vec3::sReplicate(1.0e-12f);

	// Zero & one
	Vec3 zero = Vec3::sZero();
	Vec3 one = Vec3::sReplicate(1.0f);

	// Find vectors for two edges sharing inV0
	Vec3 e1 = inV1 - inV0;
	Vec3 e2 = inV2 - inV0;

	// Begin calculating determinant - also used to calculate u parameter
	Vec3 p = inDirection.Cross(e2);

	// if determinant is near zero, ray lies in plane of triangle
	Vec3 det = Vec3::sReplicate(e1.Dot(p));

	// Calculate distance from inV0 to ray origin
	Vec3 s = inOrigin - inV0;

	// Calculate u parameter
	Vec3 u = Vec3::sReplicate(s.Dot(p)) / det;

	// Prepare to test v parameter
	Vec3 q = s.Cross(e1);

	// Calculate v parameter
	Vec3 v = Vec3::sReplicate(inDirection.Dot(q)) / det;

	// Get intersection point
	Vec3 t = Vec3::sReplicate(e2.Dot(q)) / det;

	// Check if there is an intersection
	UVec4 no_intersection = 
		UVec4::sOr
		(
			UVec4::sOr
			(
				UVec4::sOr
				(
					Vec3::sLess(det.Abs(), epsilon), 
					Vec3::sLess(u, zero)
				),
				UVec4::sOr
				(
					Vec3::sLess(v, zero), 
					Vec3::sGreater(u + v, one)
				)
			), 
			Vec3::sLess(t, zero)
		);

	// Select intersection point or FLT_MAX based on if there is an intersection or not
	Vec3 distance = Vec3::sSelect(t, Vec3::sReplicate(FLT_MAX), no_intersection);

	// Update minimum
	ioClosest = Vec3::sMin(distance, Vec3::sReplicate(ioClosest)).GetX();
}

// Intersect ray with 4 triangles in SOA format, returns 4 vector of closest points or FLT_MAX if no hit (uses bit tricks to do less divisions)
f_inline Vec4 RayTriangle4(const Vec3 &inOrigin, const Vec3 &inDirection, const Vec4 &inV0X, const Vec4 &inV0Y, const Vec4 &inV0Z, const Vec4 &inV1X, const Vec4 &inV1Y, const Vec4 &inV1Z, const Vec4 &inV2X, const Vec4 &inV2Y, const Vec4 &inV2Z)
{
	// Epsilon
	Vec4 epsilon = Vec4::sReplicate(1.0e-12f);

	// Zero
	Vec4 zero = Vec4::sZero();

	// Find vectors for two edges sharing inV0
	Vec4 e1x = inV1X - inV0X;
	Vec4 e1y = inV1Y - inV0Y;
	Vec4 e1z = inV1Z - inV0Z;
	Vec4 e2x = inV2X - inV0X;
	Vec4 e2y = inV2Y - inV0Y;
	Vec4 e2z = inV2Z - inV0Z;

	// Get direction vector components
	Vec4 dx = inDirection.SplatX();
	Vec4 dy = inDirection.SplatY();
	Vec4 dz = inDirection.SplatZ();

	// Begin calculating determinant - also used to calculate u parameter
	Vec4 px = dy * e2z - dz * e2y;
	Vec4 py = dz * e2x - dx * e2z;
	Vec4 pz = dx * e2y - dy * e2x;

	// if determinant is near zero, ray lies in plane of triangle
	Vec4 det = e1x * px + e1y * py + e1z * pz;

	// Get sign bit for determinant and make positive
	Vec4 det_sign = Vec4::sAnd(det, UVec4::sReplicate(0x80000000).ReinterpretAsFloat());
	det = Vec4::sXor(det, det_sign);

	// Calculate distance from inV0 to ray origin
	Vec4 sx = inOrigin.SplatX() - inV0X;
	Vec4 sy = inOrigin.SplatY() - inV0Y;
	Vec4 sz = inOrigin.SplatZ() - inV0Z;

	// Calculate u parameter and flip sign if determinant was negative
	Vec4 u = Vec4::sXor(sx * px + sy * py + sz * pz, det_sign);

	// Prepare to test v parameter
	Vec4 qx = sy * e1z - sz * e1y;
	Vec4 qy = sz * e1x - sx * e1z;
	Vec4 qz = sx * e1y - sy * e1x;

	// Calculate v parameter and flip sign if determinant was negative
	Vec4 v = Vec4::sXor(dx * qx + dy * qy + dz * qz, det_sign);

	// Get intersection point and flip sign if determinant was negative
	Vec4 t = Vec4::sXor(e2x * qx + e2y * qy + e2z * qz, det_sign);

	// Check if there is an intersection
	UVec4 no_intersection =
		UVec4::sOr
		(
			UVec4::sOr
			(
				UVec4::sOr
				(
					Vec4::sLess(det, epsilon),
					Vec4::sLess(u, zero)
				),
				UVec4::sOr
				(
					Vec4::sLess(v, zero),
					Vec4::sGreater(u + v, det)
				)
			),
			Vec4::sLess(t, zero)
		);

	// Select intersection point or FLT_MAX based on if there is an intersection or not
	return Vec4::sSelect(t / det, Vec4::sReplicate(FLT_MAX), no_intersection);
}
