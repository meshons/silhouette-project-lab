#pragma once
#include <boost/shared_ptr.hpp>
#include <windows.h>

#include "../Math/math.h"
#include "../Shared.h"

namespace Cam {

	GG_CLASS(Base)
public:

	virtual const Egg::Math::float3& getEyePosition() = 0;

	virtual const Egg::Math::float3& getAhead() = 0;

	virtual const Egg::Math::float4x4& getViewDirMatrix() = 0;

	virtual const Egg::Math::float4x4& getViewMatrix() = 0;

	virtual const Egg::Math::float4x4& getProjMatrix() = 0;

	virtual void animate(double dt) {}

	virtual void processMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {}

	virtual void setAspect(float aspect) = 0;
	GG_ENDCLASS

}