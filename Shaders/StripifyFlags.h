#ifndef _STRIPIFY_FLAGS_H_
#define _STRIPIFY_FLAGS_H_

// Each new vertex C in the list forms a new triangle (V1, V2, V3) = (A, B, C), A and B come from the previous triangle (V1, V2, V3). 
// The following flags indicate where A and B come from (the flags are ORred together)

// Where to take the first vertex for the new triangle from
// OR this flag with STRIPIFY_FLAG_B_*
static const uint STRIPIFY_FLAG_A_IS_V1			= 0;
static const uint STRIPIFY_FLAG_A_IS_V2			= 1;
static const uint STRIPIFY_FLAG_A_IS_V3			= 2;
static const uint STRIPIFY_FLAG_A_MASK			= 3;

// Where to take the second vertex for the new triangle from (note that it wraps: V3 + 1 = V1)
// OR this flag with STRIPIFY_FLAG_A_*
static const uint STRIPIFY_FLAG_B_IS_A_PLUS_1	= 0;
static const uint STRIPIFY_FLAG_B_IS_A_PLUS_2	= 4;
static const uint STRIPIFY_FLAG_B_MASK			= 4;
static const uint STRIPIFY_FLAG_B_SHIFT			= 2;

// Start new strip with 2 vertices
static const uint STRIPIFY_FLAG_START_STRIP_V1	= 3;
static const uint STRIPIFY_FLAG_START_STRIP_V2	= 7;

#endif