#define group_size 64

struct RayCastTestIn
{
	float3		Origin;
	float3		Direction;
};

struct JobItem
{
	uint		RayIdx;
	uint		GroupIdx;
};

StructuredBuffer<float3> bounds : register(t0);

StructuredBuffer<float> vertices : register(t1);

StructuredBuffer<RayCastTestIn> rays : register(t2);

// Can't do atomics on float, so using int here (which will be fine as long as Distance > 0)
RWStructuredBuffer<int> output : register(u0);
