#include "pch.h"

#include "Vec3.h"

vec3::vec3()
{
	x = y = z = 0.0f;
}

vec3::vec3(float _x, float _y, float _z)
{
	x = _x;
	y = _y;
	z = _z;
}

void vec3::add(const vec3 &b)
{
	x += b.x;
	y += b.y;
	z += b.z;
}

void vec3::sub(const vec3 &b)
{
	x -= b.x;
	y -= b.y;
	z -= b.z;
}

void vec3::mul(float s)
{
	x *= s;
	y *= s;
	z *= s;
}

void vec3::mul(const vec3 &b)
{
	x *= b.x;
	y *= b.y;
	z *= b.z;
}

void vec3::div(float s)
{
	x /= s;
	y /= s;
	z /= s;
}

void vec3::div(const vec3 &b)
{
	x /= b.x;
	y /= b.y;
	z /= b.z;
}

float vec3::length() const
{
	return sqrt((x * x) + (y * y) + (z * z));
}

void vec3::normalize()
{
	float q = length();
	x /= q;
	y /= q;
	z /= q;
}

float vec3::dot(const vec3 &b) const
{
	return ((b.x * x) + (b.y * y) + (b.z * z));
}

void vec3::cross(const vec3 &b)
{
	x = (y * b.z) - (z * b.y),
	y = (z * b.x) - (x * b.z),
	z = (x * b.y) - (y * b.x);
}

void vec3::lerp(const vec3 &b, float t)
{
	x += (b.x - x) * t;
	y += (b.y - y) * t;
	z += (b.z - z) * t;
}

