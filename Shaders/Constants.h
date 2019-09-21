#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

// Constants
#define EPSILON 1.0e-20f
#define FLT_MAX asfloat(0x7f7fffff)

static const float3 zero3 = float3(0, 0, 0);
static const float4 zero4 = float4(0, 0, 0, 0);

static const float3 epsilon3 = float3(EPSILON, EPSILON, EPSILON);
static const float4 epsilon4 = float4(EPSILON, EPSILON, EPSILON, EPSILON);

static const float3 flt_min3 = float3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
static const float4 flt_min4 = float4(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

static const float3 flt_max3 = float3(FLT_MAX, FLT_MAX, FLT_MAX);
static const float4 flt_max4 = float4(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);

static const uint3 uint3_one = uint3(1, 1, 1);
static const uint3 uint3_all_but_lowest_bit_set = uint3(0xfffffff7, 0xfffffff7, 0xfffffff7);

#endif //_CONSTANTS_H_