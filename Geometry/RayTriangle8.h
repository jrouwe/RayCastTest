#pragma once

#include <Math/Vec8.h>

// Intersect ray with 8 triangles in SOA format, returns 8 vector of closest points or FLT_MAX if no hit
f_inline Vec8 RayTriangle8(const Vec3 &inOrigin, const Vec3 &inDirection, const Vec8 &inV0X, const Vec8 &inV0Y, const Vec8 &inV0Z, const Vec8 &inV1X, const Vec8 &inV1Y, const Vec8 &inV1Z, const Vec8 &inV2X, const Vec8 &inV2Y, const Vec8 &inV2Z)
{
	// Epsilon
	Vec8 epsilon = Vec8::sReplicate(1.0e-12f);

	// Zero & one
	Vec8 zero = Vec8::sZero();
	Vec8 one = Vec8::sReplicate(1.0f);

	// Find vectors for two edges sharing inV0
	Vec8 e1x = inV1X - inV0X;
	Vec8 e1y = inV1Y - inV0Y;
	Vec8 e1z = inV1Z - inV0Z;
	Vec8 e2x = inV2X - inV0X;
	Vec8 e2y = inV2Y - inV0Y;
	Vec8 e2z = inV2Z - inV0Z;

	// Get direction vector components
	Vec8 dx = Vec8::sSplatX(Vec4(inDirection));
	Vec8 dy = Vec8::sSplatY(Vec4(inDirection));
	Vec8 dz = Vec8::sSplatZ(Vec4(inDirection));

	// Begin calculating determinant - also used to calculate u parameter
	Vec8 px = dy * e2z - dz * e2y;
	Vec8 py = dz * e2x - dx * e2z;
	Vec8 pz = dx * e2y - dy * e2x;
	
	// if determinant is near zero, ray lies in plane of triangle
	Vec8 det = e1x * px + e1y * py + e1z * pz;

	// Calculate distance from inV0 to ray origin
	Vec8 sx = Vec8::sSplatX(Vec4(inOrigin)) - inV0X;
	Vec8 sy = Vec8::sSplatY(Vec4(inOrigin)) - inV0Y;
	Vec8 sz = Vec8::sSplatZ(Vec4(inOrigin)) - inV0Z;

	// Calculate u parameter and flip sign if determinant was negative
	Vec8 u = (sx * px + sy * py + sz * pz) / det;

	// Prepare to test v parameter
	Vec8 qx = sy * e1z - sz * e1y;
	Vec8 qy = sz * e1x - sx * e1z;
	Vec8 qz = sx * e1y - sy * e1x;
				    
	// Calculate v parameter and flip sign if determinant was negative
	Vec8 v = (dx * qx + dy * qy + dz * qz) / det;

	// Get intersection point and flip sign if determinant was negative
	Vec8 t = (e2x * qx + e2y * qy + e2z * qz) / det;

	// Check if there is an intersection
	UVec8 no_intersection = 
		UVec8::sOr
		(
			UVec8::sOr
			(
				UVec8::sOr
				(
					Vec8::sLess(det.Abs(), epsilon), 
					Vec8::sLess(u, zero)
				),
				UVec8::sOr
				(
					Vec8::sLess(v, zero), 
					Vec8::sGreater(u + v, one)
				)
			), 
			Vec8::sLess(t, zero)
		);

	// Select intersection point or FLT_MAX based on if there is an intersection or not
	return Vec8::sSelect(t, Vec8::sReplicate(FLT_MAX), no_intersection);
}
