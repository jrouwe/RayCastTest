cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

# 64 bit architecture
set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE "x64")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

project(TestRayCast CXX)

# Set general compiler flags
set(CMAKE_CXX_FLAGS "/std:c++17 /GR- /Gm- /Gv /Wall /WX /EHsc /nologo /diagnostics:classic /FC /arch:AVX2 /fp:except- /fp:fast /Zc:inline /Zi /MP /DWIN32 /D_WINDOWS /DUNICODE /D_UNICODE /I${CMAKE_CURRENT_SOURCE_DIR}")	

# Set compiler flags for various configurations
set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /GS /ZI /Od /Ob0 /MTd /RTC1")
set(CMAKE_CXX_FLAGS_RELEASE "/DNDEBUG /GS- /GL /Gy /O2 /Oi /Ot /MT")

# Set linker flags
set(CMAKE_EXE_LINKER_FLAGS "/machine:x64 /SUBSYSTEM:CONSOLE /ignore:4221")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO /LTCG:incremental /OPT:ICF /OPT:REF")
set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "/LTCG")

# ISPC Compiler path
set(ISPC ${CMAKE_CURRENT_SOURCE_DIR}/ispc.exe)

# Source files
set(TEST_RAY_CAST_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/AABBTree/AABBTreeBuilder.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/AABBTree/AABBTreeBuilder.h
	${CMAKE_CURRENT_SOURCE_DIR}/AABBTree/AABBTreeToBuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/Core/AlignedAllocator.h
	${CMAKE_CURRENT_SOURCE_DIR}/Core/ByteBuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/Core/Color.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Core/Color.h
	${CMAKE_CURRENT_SOURCE_DIR}/Core/Core.h
	${CMAKE_CURRENT_SOURCE_DIR}/Core/TickCounter.h
	${CMAKE_CURRENT_SOURCE_DIR}/Core/TickCounter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Core/HashCombine.h
	${CMAKE_CURRENT_SOURCE_DIR}/Core/Memory.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Core/Memory.h
	${CMAKE_CURRENT_SOURCE_DIR}/Core/progressindicator.h
	${CMAKE_CURRENT_SOURCE_DIR}/Core/Reference.h
	${CMAKE_CURRENT_SOURCE_DIR}/Core/StringTools.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Core/StringTools.h
	${CMAKE_CURRENT_SOURCE_DIR}/Core/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Core/Utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/Geometry/AABox.h
	${CMAKE_CURRENT_SOURCE_DIR}/Geometry/IndexedTriangle.h
	${CMAKE_CURRENT_SOURCE_DIR}/Geometry/Indexify.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Geometry/Indexify.h
	${CMAKE_CURRENT_SOURCE_DIR}/Geometry/MortonCode.h
	${CMAKE_CURRENT_SOURCE_DIR}/Geometry/RayAABox.h
	${CMAKE_CURRENT_SOURCE_DIR}/Geometry/RayAABox8.h
	${CMAKE_CURRENT_SOURCE_DIR}/Geometry/RayTriangle.h
	${CMAKE_CURRENT_SOURCE_DIR}/Geometry/RayTriangle8.h
	${CMAKE_CURRENT_SOURCE_DIR}/Geometry/Triangle.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Float2.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Float3.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Float4.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/HalfFloat.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Mat44.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Mat44.inl
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Math.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Swizzle.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/UVec4.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Math/UVec4.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/UVec4.inl
	${CMAKE_CURRENT_SOURCE_DIR}/Math/UVec8.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/UVec8.inl
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Vec3.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Vec3.inl
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Vec4.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Vec4.inl
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Vec8.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/Vec8.inl
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleGrouper/TriangleGrouper.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleGrouper/TriangleGrouperClosestCentroid.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleGrouper/TriangleGrouperClosestCentroid.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleGrouper/TriangleGrouperMorton.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleGrouper/TriangleGrouperMorton.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitter.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitterBinning.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitterBinning.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitterFixedLeafSize.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitterFixedLeafSize.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitterLongestAxis.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitterLongestAxis.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitterMean.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitterMean.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitterMorton.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/TriangleSplitterMorton.h
	${CMAKE_CURRENT_SOURCE_DIR}/Application/Application.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Application/Application.h
	${CMAKE_CURRENT_SOURCE_DIR}/Application/EntryPoint.h
	${CMAKE_CURRENT_SOURCE_DIR}/Input/Keyboard.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Input/Keyboard.h
	${CMAKE_CURRENT_SOURCE_DIR}/Input/Mouse.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Input/Mouse.h
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/linerenderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/LineRenderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/trianglerenderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/TriangleRenderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/ConstantBuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/ConstantBuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/FatalErrorIfFailed.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/FatalErrorIfFailed.h
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/LineRendererImp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/LineRendererImp.h
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/Renderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/Renderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/RenderInstances.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/RenderInstances.h
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/RenderPrimitive.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/RenderPrimitive.h
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/StructuredBuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/StructuredBuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/Texture.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/Texture.h
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/TriangleRendererImp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Renderer/TriangleRendererImp.h
	${CMAKE_CURRENT_SOURCE_DIR}/Stripify/Stripify.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Stripify/Stripify.h
	${CMAKE_CURRENT_SOURCE_DIR}/Stripify/StripifyFlags.h
	${CMAKE_CURRENT_SOURCE_DIR}/ISPC/Math.h
	${CMAKE_CURRENT_SOURCE_DIR}/ISPC/RayAABox.h
	${CMAKE_CURRENT_SOURCE_DIR}/ISPC/RayTriangle.h
	${CMAKE_CURRENT_SOURCE_DIR}/NodeCodec/NodeCodecAABBTree.h
	${CMAKE_CURRENT_SOURCE_DIR}/NodeCodec/NodeCodecAABBTreeCompressed.h
	${CMAKE_CURRENT_SOURCE_DIR}/NodeCodec/NodeCodecAABBTreePNS.h
	${CMAKE_CURRENT_SOURCE_DIR}/NodeCodec/NodeCodecAABBTreeSplitAxis.h
	${CMAKE_CURRENT_SOURCE_DIR}/NodeCodec/NodeCodecQuadTree.h
	${CMAKE_CURRENT_SOURCE_DIR}/NodeCodec/NodeCodecSKDTree.h
	${CMAKE_CURRENT_SOURCE_DIR}/NodeCodec/NodeCodecQuadTreeHalfFloat.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBList.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTree1.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTree2.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTree3.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTree4.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTree5.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTreeCompressed.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTreeISPC.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTreeISPC.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTreePNS.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTreeSplitAxis.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTreeStripISPC.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUAABBTreeStripISPC.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUBruteForce.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUQuadTree.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUQuadTreeHalfFloat.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUQuadTreeHalfFloat2.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastCPUSKDTree.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastGPUAABBList.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastGPUAABBList.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastGPUBruteForce.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastGPUBruteForce.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastGPUBruteForceBitPack.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastGPUBruteForceBitPack.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastGPUTree.h
	${CMAKE_CURRENT_SOURCE_DIR}/RayCastTest/RayCastTest.h
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/VertexConstants.h
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Constants.h
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ParallelMin.h
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayAABox.h
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUAABBList.h
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUAABBTree.h
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUTree.h
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayTriangle.h
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/StripifyFlags.h
	${CMAKE_CURRENT_SOURCE_DIR}/TestRaycast.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecBitPack.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecBitPackSOA4.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecFloat3.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecFloat3ISPC.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecFloat3Original.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecFloat3SOA4.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecFloat3SOA4Packed.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecFloat3SOA8.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecIndexed.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecIndexed8BitPackSOA4.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecIndexedBitPackSOA4.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecIndexedSOA4.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangleCodec/TriangleCodecStrip.h
	${CMAKE_CURRENT_SOURCE_DIR}/Utils/CacheTrasher.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utils/CacheTrasher.h
	${CMAKE_CURRENT_SOURCE_DIR}/Utils/Model.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utils/Model.h
	${CMAKE_CURRENT_SOURCE_DIR}/Utils/PerfTimer.h
	${CMAKE_CURRENT_SOURCE_DIR}/Utils/StatsWriter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utils/StatsWriter.h
	${CMAKE_CURRENT_SOURCE_DIR}/pch.h
)

# Vertex shaders
set(TEST_RAY_CAST_SRC_FILES_VERTEX_SHADERS
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/LineVertexShader.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TriangleDepthVertexShader.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TriangleVertexShader.hlsl
)

# Pixel shaders
set(TEST_RAY_CAST_SRC_FILES_PIXEL_SHADERS
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/LinePixelShader.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TriangleDepthPixelShader.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TrianglePixelShader.hlsl
)

# All compute shaders
set(TEST_RAY_CAST_SRC_FILES_COMPUTE_SHADERS
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUAABBListStage1.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUAABBListStage2.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUAABBTree1.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUAABBTree2.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUAABBTree3.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUAABBTree4.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUAABBTreeStrip.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUBruteForce.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUBruteForceBitPack.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUBruteForceClear.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/RayCastGPUSKDTree.hlsl
)

set_source_files_properties(${TEST_RAY_CAST_SRC_FILES_VERTEX_SHADERS} PROPERTIES VS_SHADER_FLAGS "/WX /T vs_5_0")
set(TEST_RAY_CAST_SRC_FILES ${TEST_RAY_CAST_SRC_FILES} ${TEST_RAY_CAST_SRC_FILES_VERTEX_SHADERS})

set_source_files_properties(${TEST_RAY_CAST_SRC_FILES_PIXEL_SHADERS} PROPERTIES VS_SHADER_FLAGS "/WX /T ps_5_0")
set(TEST_RAY_CAST_SRC_FILES ${TEST_RAY_CAST_SRC_FILES} ${TEST_RAY_CAST_SRC_FILES_PIXEL_SHADERS})

set_source_files_properties(${TEST_RAY_CAST_SRC_FILES_COMPUTE_SHADERS} PROPERTIES VS_SHADER_FLAGS "/WX /T cs_5_0")
set(TEST_RAY_CAST_SRC_FILES ${TEST_RAY_CAST_SRC_FILES} ${TEST_RAY_CAST_SRC_FILES_COMPUTE_SHADERS})
	
# Files that need to be compiled with ISPC
set(TEST_RAY_CAST_SRC_FILES_ISPC
	${CMAKE_CURRENT_SOURCE_DIR}/ISPC/RayVsAABBTree.ispc
	${CMAKE_CURRENT_SOURCE_DIR}/ISPC/RayVsAABBTreeStripped.ispc
	${CMAKE_CURRENT_SOURCE_DIR}/ISPC/RayVsTriangleList.ispc
)
set(TEST_RAY_CAST_SRC_FILES ${TEST_RAY_CAST_SRC_FILES} ${TEST_RAY_CAST_SRC_FILES_ISPC})

# Compile ISPC files
set(TEST_RAY_CAST_COMPILED_FILES_ISPC)
foreach(in_f ${TEST_RAY_CAST_SRC_FILES_ISPC})
	file(RELATIVE_PATH out_f ${CMAKE_CURRENT_SOURCE_DIR} ${in_f})
	set(out_f "${CMAKE_CURRENT_BINARY_DIR}/${out_f}.obj")
	add_custom_command(OUTPUT ${out_f}
		COMMAND ${ISPC} ${in_f} -o ${out_f} --target=avx2-i32x8 --arch=x86-64 --wno-perf
		DEPENDS ${in_f}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Compiling ${out_f}"
		VERBATIM
	)
	list(APPEND TEST_RAY_CAST_COMPILED_FILES_ISPC ${out_f})
endforeach()

# Enable Precompiled Headers for TestRayCast
set_source_files_properties(${TEST_RAY_CAST_SRC_FILES} PROPERTIES COMPILE_FLAGS "/Yupch.h")
set(TEST_RAY_CAST_SRC_FILES ${TEST_RAY_CAST_SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/pch.cpp)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")

# Group source files
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TEST_RAY_CAST_SRC_FILES})	

# Create TestRayCast executable
add_executable(TestRayCast  ${TEST_RAY_CAST_SRC_FILES} ${TEST_RAY_CAST_COMPILED_FILES_ISPC})
target_include_directories(TestRayCast PUBLIC TestRayCast)
target_link_libraries (TestRayCast LINK_PUBLIC d3d11.lib shcore.lib)
set_property(TARGET TestRayCast PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Source files for PlyConverter
set(PLY_CONVERTER_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/PlyConverter/PlyConverter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Core/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utils/Model.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Geometry/Indexify.cpp
)

# Enable Precompiled Headers for PlyConverter
set_source_files_properties(${PLY_CONVERTER_SRC_FILES} PROPERTIES COMPILE_FLAGS "/Yupch.h")
set(PLY_CONVERTER_SRC_FILES ${PLY_CONVERTER_SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/pch.cpp)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")

# Create PlyConverter executable
add_executable(PlyConverter  ${PLY_CONVERTER_SRC_FILES})
target_link_options(PlyConverter PUBLIC "/SUBSYSTEM:CONSOLE")
set_property(TARGET PlyConverter PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

