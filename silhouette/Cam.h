#pragma once
#include "Shared.h"
#include "Math/math.h"
#include "Cam/Base.h"

namespace Cam {

	GG_SUBCLASS(Cam, Cam::Base)
		Egg::Math::float3 position;
	Egg::Math::float3 ahead;
	Egg::Math::float3 right;
	float yaw;
	float pitch;

	float fov;
	float aspect;
	float nearPlane;
	float farPlane;

	float speed;

	Egg::Math::float4x4 viewMatrix;
	Egg::Math::float4x4 projMatrix;
	Egg::Math::float4x4 viewDirMatrix;

	bool wPressed;
	bool aPressed;
	bool sPressed;
	bool dPressed;
	bool qPressed;
	bool ePressed;
	bool shiftPressed;

	Egg::Math::int2 lastMousePos;
	Egg::Math::float2 mouseDelta;

	void updateView();
	void updateProj();

protected:
	Cam();
public:

	static P create() { return P(new Cam()); }
	P setView(Egg::Math::float3 position, Egg::Math::float3 ahead);
	P setProj(float fov, float aspect, float nearPlane, float farPlane);
	P setSpeed(float speed);

	const Egg::Math::float3& getEyePosition();
	const Egg::Math::float3& getAhead();
	const Egg::Math::float4x4& getViewDirMatrix();
	const Egg::Math::float4x4& getViewMatrix();
	const Egg::Math::float4x4& getProjMatrix();

	virtual void animate(double dt);

	virtual void processMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual void setAspect(float aspect);
	GG_ENDCLASS
}

