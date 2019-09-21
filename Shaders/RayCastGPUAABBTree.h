#include "RayCastGPUTree.h"
#include "RayAABox.h"

// Tree properties
static const uint HAS_TRIANGLES = 0x80000000;
static const uint MASK_OUT_HAS_TRIANGLES = 0x7fffffff;

