#include <pch.h>

#include <Stripify/Stripify.h>

void Stripify::sStripify(const IndexedTriangleList &inTriangles, TriangleStrip &outStrips)
{
	// Build database edge -> triangle index
	EdgeToTriangles edge_to_triangles;
	for (uint t = 0; t < inTriangles.size(); ++t)
		for (int v = 0; v < 3; ++v)
		{
			Edge edge(inTriangles[t].mIdx[v], inTriangles[t].mIdx[(v + 1) % 3]);
			edge_to_triangles[edge].push_back(t);
		}

	// Calculate the amount of edges that triangles share with other triangles
	TriangleExList triangles_ex;
	triangles_ex.reserve(inTriangles.size());
	for (auto &t : inTriangles)
	{
		TriangleEx tex;
		tex.mUsedInStrip = false;
		tex.mSharedEdgeCount = 0;
		for (int v = 0; v < 3; ++v)
		{
			Edge edge(t.mIdx[v], t.mIdx[(v + 1) % 3]);
			tex.mSharedEdgeCount += (uint)edge_to_triangles[edge].size();
		}
		triangles_ex.push_back(tex);
	}

	// Sort triangles so least connected triangle goes first
	vector<uint32> ordered_triangles;
	ordered_triangles.resize(inTriangles.size());
	for (uint t = 0; t < inTriangles.size(); ++t)
		ordered_triangles[t] = t;
	sort(ordered_triangles.begin(), ordered_triangles.end(), [&triangles_ex](uint32 inLHS, uint32 inRHS) -> bool { return triangles_ex[inLHS].mSharedEdgeCount < triangles_ex[inRHS].mSharedEdgeCount; });
	
	// This would be the least amount of memory needed if all triangles fall in the same strip
	outStrips.reserve(inTriangles.size() + 2); 

	Vertex vtx;

	// Loop over all triangles
	for (uint t = 0; t < ordered_triangles.size(); ++t)
	{
		uint tri_idx = ordered_triangles[t];
		IndexedTriangle tri = inTriangles[tri_idx];

		// Check if the triangle has been used yet
		if (!triangles_ex[tri_idx].mUsedInStrip)
		{
			// Start a new strip
			vtx.mIndex = tri.mIdx[0];
			vtx.mMaterialIndex = tri.mMaterialIndex;
			vtx.mFlags = STRIPIFY_FLAG_START_STRIP_V1;
			outStrips.push_back(vtx);
			vtx.mIndex = tri.mIdx[1];
			vtx.mFlags = STRIPIFY_FLAG_START_STRIP_V2;
			outStrips.push_back(vtx);
			vtx.mIndex = tri.mIdx[2];
			vtx.mFlags = STRIPIFY_FLAG_A_IS_V1 | STRIPIFY_FLAG_B_IS_A_PLUS_1;
			outStrips.push_back(vtx);

			// Loop until no more triangles found
			for (;;)
			{
				// Mark triangle as used
				triangles_ex[tri_idx].mUsedInStrip = true;

				bool next_tri_found = false;

				// Loop over all edges of triangle
				for (uint v = 0; !next_tri_found && v < 3; ++v)
				{
					Edge edge(tri.mIdx[v], tri.mIdx[(v + 1) % 3]);

					// Loop over all triangles that share this edge, consider only the first one that isn't used yet
					EdgeToTriangles::const_iterator list = edge_to_triangles.find(edge);
					if (list != edge_to_triangles.end())
						for (auto next_tri_idx : list->second)
							if (!triangles_ex[next_tri_idx].mUsedInStrip)
							{
								// Get next triangle so that the new vertex is in mIdx[2]
								IndexedTriangle next_tri = inTriangles[next_tri_idx];
								while (next_tri.mIdx[2] == edge.mIdx[0] || next_tri.mIdx[2] == edge.mIdx[1])
									next_tri.Rotate();
				
								// Determine which vertex from tri to take for the first vertex
								uint a = 0;
								while (a < 3 && next_tri.mIdx[0] != tri.mIdx[a])
									++a;
								assert(a < 3);
					
								// Determine which vertex from tri to take for the second vertex
								uint b_count = 0;
								uint b = a;
								while (b_count < 3 && next_tri.mIdx[1] != tri.mIdx[b])
								{
									b = (b + 1) % 3;
									++b_count;
								}
								assert(b_count == 1 || b_count == 2);

								// Add vertex to strip
								vtx.mIndex = next_tri.mIdx[2];
								vtx.mMaterialIndex = next_tri.mMaterialIndex;
								vtx.mFlags = a | (b_count == 1? STRIPIFY_FLAG_B_IS_A_PLUS_1 : STRIPIFY_FLAG_B_IS_A_PLUS_2);
								outStrips.push_back(vtx);

								// Start again from the next triangle
								tri = next_tri;
								tri_idx = next_tri_idx;
								next_tri_found = true;
								break;
							}
				}

				// Strip ends when no more triangles are found
				if (!next_tri_found)
					break;
			}
		}
	}
	
#ifdef _DEBUG
	IndexedTriangleList triangles_left(inTriangles);
	IndexedTriangleList de_stripified;
	sDeStripify(outStrips, de_stripified);
	for (const IndexedTriangle &t : de_stripified)
	{
		IndexedTriangleList::iterator found = triangles_left.end();
		for (IndexedTriangleList::iterator i = triangles_left.begin(); i != triangles_left.end(); ++i)
			if (i->IsEquivalent(t))
			{
				found = i;
				break;
			}
		if (found != triangles_left.end())
			triangles_left.erase(found);
		else
			assert(false); // Triangle should have existed
	}
	assert(triangles_left.empty()); // All triangles should have been found	
#endif
}

void Stripify::sDeStripify(const TriangleStrip &inStrips, IndexedTriangleList &outTriangles)
{
	assert(outTriangles.empty());

	auto t = inStrips.begin();
	auto t_end = inStrips.end();

	uint32 v[3];
	
	assert(t == t_end || t->mFlags == STRIPIFY_FLAG_START_STRIP_V1);
	while (t < t_end)
	{
		if (t->mFlags == STRIPIFY_FLAG_START_STRIP_V1)
		{
			// Start of new strip, load first 2 vertices
			v[0] = t->mIndex;
			t++;
			v[1] = t->mIndex;
			assert(t->mFlags == STRIPIFY_FLAG_START_STRIP_V2);
			t++;
			
			// Assume vertices are already in the right order
			assert(t->mFlags == (STRIPIFY_FLAG_A_IS_V1 | STRIPIFY_FLAG_B_IS_A_PLUS_1));
		}
		else
		{
			// Continuation of strip, determine first 2 vertices
			uint32 a = t->mFlags & STRIPIFY_FLAG_A_MASK;
			uint32 b = (a + ((t->mFlags & STRIPIFY_FLAG_B_MASK) >> STRIPIFY_FLAG_B_SHIFT) + 1) % 3;
			uint32 va = v[a];
			uint32 vb = v[b];
			v[0] = va;
			v[1] = vb;
		}

		// Load third vertex
		v[2] = t->mIndex;

		// Test if this triangle exists
		IndexedTriangle triangle(v[0], v[1], v[2], t->mMaterialIndex);
		outTriangles.push_back(triangle);

		// Next vertex/triangle
		t++;
	}
}
