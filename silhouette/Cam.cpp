#include "Cam.h"
#include "stdafx.h"

using namespace Egg::Math;

Cam::Cam::Cam()
{
	position = float3::zero;
	ahead = float3::zUnit;
	right = float3::xUnit;
	yaw = 0.0;
	pitch = 0.0;

	fov = 1.0f;
	nearPlane = 0.1f;
	farPlane = 1000.0f;
	setAspect(1.33f);

	lastMousePos = int2::zero;
	mouseDelta = float2::zero;

	speed = 0.5f;

	viewMatrix = float4x4::view(position, ahead, float3::yUnit);
	viewDirMatrix = (float4x4::view(float3::zero, ahead, float3::yUnit) * projMatrix).invert();

	wPressed = false;
	aPressed = false;
	sPressed = false;
	dPressed = false;
	qPressed = false;
	ePressed = false;
	shiftPressed = false;
}

Cam::Cam::P Cam::Cam::setView(Egg::Math::float3 position, Egg::Math::float3 ahead)
{
	this->position = position;
	this->ahead = ahead;
	updateView();
	return getShared();
}

Cam::Cam::P Cam::Cam::setProj(float fov, float aspect, float nearPlane, float farPlane)
{
	this->fov = fov;
	this->aspect = aspect;
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
	return getShared();
}

const float3& Cam::Cam::getEyePosition()
{
	return position;
}

const float3& Cam::Cam::getAhead()
{
	return ahead;
}

const float4x4& Cam::Cam::getViewDirMatrix()
{
	return viewDirMatrix;
}

const float4x4& Cam::Cam::getViewMatrix()
{
	return viewMatrix;
}

const float4x4& Cam::Cam::getProjMatrix()
{
	return projMatrix;
}

void Cam::Cam::updateView()
{
	viewMatrix = float4x4::view(position, ahead, float3::yUnit);
	viewDirMatrix = (float4x4::view(float3::zero, ahead, float3::yUnit) * projMatrix).invert();

	right = float3::yUnit.cross(ahead).normalize();
	yaw = atan2f(ahead.x, ahead.z);
	pitch = -atan2f(ahead.y, ahead.xz.length());
}

void Cam::Cam::updateProj()
{
	projMatrix = float4x4::proj(fov, aspect, nearPlane, farPlane);
}

void Cam::Cam::animate(double dt)
{
	if (wPressed)
		position += ahead * (shiftPressed ? speed * 5.0 : speed) * dt;
	if (sPressed)
		position -= ahead * (shiftPressed ? speed * 5.0 : speed) * dt;
	if (aPressed)
		position -= right * (shiftPressed ? speed * 5.0 : speed) * dt;
	if (dPressed)
		position += right * (shiftPressed ? speed * 5.0 : speed) * dt;
	if (qPressed)
		position -= float3(0, 1, 0) * (shiftPressed ? speed * 5.0 : speed) * dt;
	if (ePressed)
		position += float3(0, 1, 0) * (shiftPressed ? speed * 5.0 : speed) * dt;

	yaw += mouseDelta.x * 0.02f;
	pitch += mouseDelta.y * 0.02f;
	pitch = float1(pitch).clamp(-3.14 / 2, +3.14 / 2);

	mouseDelta = float2::zero;

	ahead = float3(sin(yaw) * cos(pitch), -sin(pitch), cos(yaw) * cos(pitch));

	updateView();
}

void Cam::Cam::processMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN)
	{
		if (wParam == 'W')
			wPressed = true;
		else if (wParam == 'A')
			aPressed = true;
		else if (wParam == 'S')
			sPressed = true;
		else if (wParam == 'D')
			dPressed = true;
		else if (wParam == 'Q')
			qPressed = true;
		else if (wParam == 'E')
			ePressed = true;
		else if (wParam == VK_SHIFT)
			shiftPressed = true;
	}
	else if (uMsg == WM_KEYUP)
	{
		if (wParam == 'W')
			wPressed = false;
		else if (wParam == 'A')
			aPressed = false;
		else if (wParam == 'S')
			sPressed = false;
		else if (wParam == 'D')
			dPressed = false;
		else if (wParam == 'Q')
			qPressed = false;
		else if (wParam == 'E')
			ePressed = false;
		else if (wParam == VK_SHIFT)
			shiftPressed = false;
		else if (wParam == VK_ADD)
			speed *= 2;
		else if (wParam == VK_SUBTRACT)
			speed *= 0.5;
	}
	else if (uMsg == WM_KILLFOCUS)
	{
		wPressed = false;
		aPressed = false;
		sPressed = false;
		dPressed = false;
		qPressed = false;
		ePressed = false;
		shiftPressed = false;
	}
	else if (uMsg == WM_LBUTTONDOWN)
	{
		lastMousePos = int2(LOWORD(lParam), HIWORD(lParam));
	}
	else if (uMsg == WM_LBUTTONUP)
	{
		mouseDelta = float2::zero;
	}
	else if (uMsg == WM_MOUSEMOVE && (wParam & MK_LBUTTON))
	{
		int2 mousePos(LOWORD(lParam), HIWORD(lParam));
		mouseDelta = mousePos - lastMousePos;

		lastMousePos = mousePos;
	}
}

void Cam::Cam::setAspect(float aspect)
{
	this->aspect = aspect;
	updateProj();
}