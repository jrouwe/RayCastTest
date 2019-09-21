#include <pch.h>

#include <Renderer/FatalErrorIfFailed.h>
#include <Core/StringTools.h>

void FatalErrorIfFailed(HRESULT inHResult)
{
	if (FAILED(inHResult))
		FatalError("DirectX exception thrown: %s", ConvertToString(inHResult).c_str());
}
