#include "Constants.h"

// Taken from: http://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
inline float RayTriangle(float3 O, float3 D, float3 V0, float3 V1, float3 V2)
{
	// Find vectors for two edges sharing V0
	float3 e1 = V1 - V0;
	float3 e2 = V2 - V0;

	// Begin calculating determinant - also used to calculate u parameter
	float3 p = cross(D, e2);

	// If determinant is near zero, ray lies in plane of triangle
	float det = dot(e1, p);
	float inv_det = rcp(det);

	// Calculate distance from V0 to ray origin
	float3 s = O - V0;

	// Calculate u parameter 
	float u = dot(s, p) * inv_det;

	// Calculate v parameter
	float3 q = cross(s, e1);
	float v = dot(D, q) * inv_det;

	// Get intersection point 
	float t = dot(e2, q) * inv_det;

	// Calculate collision distance
	return abs(det) >= EPSILON && u >= 0 && v >= 0 && u + v <= 1.0f && t >= 0? t : FLT_MAX;
}

// Taken from: http://www.graphicon.ru/proceedings/2012/conference/EN2%20-%20Graphics/gc2012Shumskiy.pdf
inline float RayTriangle4(float3 O, float3 D, float3 V01, float3 V11, float3 V21, float3 V02, float3 V12, float3 V22, float3 V03, float3 V13, float3 V23, float3 V04, float3 V14, float3 V24)
{
	float3 e11 = V11 - V01;
	float3 e21 = V21 - V01;
	float3 e12 = V12 - V02;
	float3 e22 = V22 - V02;
	float3 e13 = V13 - V03;
	float3 e23 = V23 - V03;
	float3 e14 = V14 - V04;
	float3 e24 = V24 - V04;

	float4 v0x = float4(V01.x, V02.x, V03.x, V04.x);
	float4 v0y = float4(V01.y, V02.y, V03.y, V04.y);
	float4 v0z = float4(V01.z, V02.z, V03.z, V04.z);
	float4 e1x = float4(e11.x, e12.x, e13.x, e14.x);
	float4 e1y = float4(e11.y, e12.y, e13.y, e14.y);
	float4 e1z = float4(e11.z, e12.z, e13.z, e14.z);
	float4 e2x = float4(e21.x, e22.x, e23.x, e24.x);
	float4 e2y = float4(e21.y, e22.y, e23.y, e24.y);
	float4 e2z = float4(e21.z, e22.z, e23.z, e24.z);

	float4 dir4x = D.xxxx;
	float4 dir4y = D.yyyy;
	float4 dir4z = D.zzzz;

	float4 pvecx = dir4y * e2z - dir4z * e2y;
	float4 pvecy = dir4z * e2x - dir4x * e2z;
	float4 pvecz = dir4x * e2y - dir4y * e2x;
	
	float4 det = pvecx * e1x + pvecy * e1y + pvecz * e1z;
	float4 inv_det = rcp(det);
	
	float4 orig4x = O.xxxx;
	float4 orig4y = O.yyyy;
	float4 orig4z = O.zzzz;
	
	float4 tvecx = orig4x - v0x;
	float4 tvecy = orig4y - v0y;
	float4 tvecz = orig4z - v0z;
	
	float4 u4 = (tvecx * pvecx + tvecy * pvecy + tvecz * pvecz) * inv_det;

	float4 qvecx = tvecy * e1z - tvecz * e1y;
	float4 qvecy = tvecz * e1x - tvecx * e1z;
	float4 qvecz = tvecx * e1y - tvecy * e1x;
	
	float4 v4 = (dir4x * qvecx + dir4y * qvecy + dir4z * qvecz) * inv_det;
	
	float4 t4 = (e2x * qvecx + e2y * qvecy + e2z * qvecz) * inv_det;

	t4 = abs(det) >= epsilon4 && u4 >= 0 && v4 >= 0 && u4 + v4 <= 1.0f && t4 >= 0? t4 : flt_max4;
	
	// Return minimum of 4 components
	t4.xy = min(t4.xy, t4.zw);
	return min(t4.x, t4.y);
}
