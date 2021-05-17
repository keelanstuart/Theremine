#pragma once

#include "Vec3.h"

class vec3
{
public:
	vec3();
	vec3(float _x, float _y, float _z);

	void add(const vec3 &b);
	void sub(const vec3 &b);
	void mul(float s);
	void mul(const vec3 &b);
	void div(float s);
	void div(const vec3 &b);
	float length() const;
	void normalize();
	float dot(const vec3 &b) const;
	void cross(const vec3 &b);
	void lerp(const vec3 &b, float t);

	vec3 &operator +(const vec3 &b) { add(b); }
	vec3 &operator -(const vec3 &b) { sub(b); }
	vec3 &operator *(float s) { mul(s); }
	vec3 &operator *(const vec3 &b) { mul(b); }
	vec3 &operator /(float s) { div(s); }
	vec3 &operator /(const vec3 &b) { div(b); }

	bool operator ==(const vec3 &b) { return (x == b.x) && (y == b.y) && (z == b.z); }

	union
	{
		float v[3];

		struct
		{
			float x, y, z;
		};
	};
};

