#type vertex
#version 450 core

layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec4 a_Color; // Tint
layout(location = 2) in float a_TextureSlot; // Texture Unit or -1 for no texture
layout(location = 3) in float a_TextureFilter; // Linear / Point
layout(location = 4) in float a_TextureAlphaClippingThreshold;
layout(location = 5) in vec2 a_TextureWrapping; // Repeat, MirroredRepeat, ...
layout(location = 6) in vec4 a_TextureBorderColor; // Used for ClampToBorder
layout(location = 7) in vec4 a_TextureRect; // 0 -> 1 Rect from Atlas
layout(location = 8) in vec2 a_TextureSize; // Texture size in pixels
layout(location = 9) in vec2 a_TextureCoord; // UV Coord

flat out vec4 v_Color;
flat out int v_TextureSlot;
flat out int v_TextureFilter;
flat out float v_TextureAlphaClippingThreshold;
flat out int v_TextureSWrapping;
flat out int v_TextureTWrapping;
flat out vec4 v_TextureBorderColor;
flat out vec4 v_TextureRect;
flat out vec2 v_TextureSize;
out vec2 v_TextureCoord;

void main()
{
	gl_Position = a_Position;
	v_Color = a_Color;
	v_TextureSlot = int(a_TextureSlot);
	v_TextureSWrapping = int(a_TextureWrapping.x);
	v_TextureTWrapping = int(a_TextureWrapping.y);
	v_TextureFilter = int(a_TextureFilter);
	v_TextureAlphaClippingThreshold = a_TextureAlphaClippingThreshold;
	v_TextureBorderColor = a_TextureBorderColor;
	v_TextureRect = a_TextureRect;
	v_TextureSize = a_TextureSize;
	v_TextureCoord = vec2(a_TextureCoord.x, 1 - a_TextureCoord.y);
}

#type fragment
#version 450 core
#pragma precision highp float

layout(location = 0) out vec4 o_Color;

uniform sampler2D[32] u_Slots;

flat in vec4 v_Color;
flat in int v_TextureSlot;
flat in int v_TextureFilter;
flat in float v_TextureAlphaClippingThreshold;
flat in int v_TextureSWrapping;
flat in int v_TextureTWrapping;
flat in vec4 v_TextureBorderColor;
flat in vec4 v_TextureRect;
flat in vec2 v_TextureSize;
in vec2 v_TextureCoord;

vec4 PointSampleFromAtlas(sampler2D slot, vec2 coord);

vec4 BiLinearSampleFromAtlas(sampler2D slot, vec2 coord)
{
	vec2 texelSize = vec2(1 / v_TextureSize.x, 1 / v_TextureSize.y);

	float a = mod(coord.x, texelSize.x) * v_TextureSize.x;
	float b = mod(coord.y, texelSize.y) * v_TextureSize.y;

	if (a > 0.5)
		a -= 0.5;
	else if (a < 0.5)
		a += 0.5;

	if (b > 0.5)
		b -= 0.5;
	else if (b < 0.5)
		b += 0.5;

	float xmin = coord.x - texelSize.x / 2;
	float xmax = coord.x + texelSize.x / 2;

	float ymin = coord.y - texelSize.y / 2;
	float ymax = coord.y + texelSize.y / 2;

	vec4 upperLeft = PointSampleFromAtlas(slot, vec2(xmin, ymin));
	vec4 upperRight = PointSampleFromAtlas(slot, vec2(xmax, ymin));
	vec4 lowerLeft = PointSampleFromAtlas(slot, vec2(xmin, ymax));
	vec4 lowerRight = PointSampleFromAtlas(slot, vec2(xmax, ymax));

	return mix(mix(upperLeft, upperRight, a), mix(lowerLeft, lowerRight, a), b);
}

float Wrap(float value, int wrapping)
{
	int valueInt = int(value);
	bool valueIsRound = value == valueInt;
	int valueIntMinusOne = valueInt - 1;
	int valueIntMinusTwo = valueInt - 2;
	int commonExpr1 = valueIsRound ? valueIntMinusOne : valueInt;
	int commonExpr2 = valueIsRound ? valueIntMinusTwo : valueIntMinusOne;

	switch (wrapping)
	{
	case 1: // Repeat
		if (value > 1)
			value -= commonExpr1;
		else if (value < 0)
			value -= commonExpr2;
		break;
	case 2: // MirroredRepeat
		bool commonCondition = valueInt % 2 == 0;
		if (value > 1)
		{
			if (commonCondition)
				value -= commonExpr1;
			else
				value = (valueIsRound ? valueInt : valueInt + 1) - value;
		}
		else if (value < 0)
		{
			if (commonCondition)
				value = commonExpr1 - value;
			else
				value -= commonExpr2;
		}
		break;
	case 3: // ClampToEdge
		value = clamp(value, 0.001, 0.999);
		break;
	}

	return value;
}

vec4 PointSampleFromAtlas(sampler2D slot, vec2 coord)
{
	if ((v_TextureSWrapping == 4 && (coord.x > 1 || coord.x < 0)) || (v_TextureTWrapping == 4 && (coord.y > 1 || coord.y < 0)))
		return v_TextureBorderColor; // ClampToBorder

	coord.x = Wrap(coord.x, v_TextureSWrapping);
	coord.y = Wrap(coord.y, v_TextureTWrapping);

	coord.x = clamp(coord.x, 0.001, 0.999);
	coord.y = clamp(coord.y, 0.001, 0.999);
	return texture(slot, vec2(v_TextureRect.x + coord.x * v_TextureRect.z, v_TextureRect.y + coord.y * v_TextureRect.w));
}

vec4 Sample(sampler2D slot)
{
	vec4 color;
	if (v_TextureFilter == 1)
		color = PointSampleFromAtlas(slot, v_TextureCoord) * v_Color;
	else
		color = BiLinearSampleFromAtlas(slot, v_TextureCoord) * v_Color;

	if (color.a <= v_TextureAlphaClippingThreshold) // Handle Alpha clipping
		discard;

	return color;
}

void main()
{
	switch (v_TextureSlot)
	{
	case -1 : o_Color = v_Color; return; // Alpha clipping handled by Renderer2D class in C++
	case  0 : o_Color = Sample(u_Slots[0 ]); return;
	case  1 : o_Color = Sample(u_Slots[1 ]); return;
	case  2 : o_Color = Sample(u_Slots[2 ]); return;
	case  3 : o_Color = Sample(u_Slots[3 ]); return;
	case  4 : o_Color = Sample(u_Slots[4 ]); return;
	case  5 : o_Color = Sample(u_Slots[5 ]); return;
	case  6 : o_Color = Sample(u_Slots[6 ]); return;
	case  7 : o_Color = Sample(u_Slots[7 ]); return;
	case  8 : o_Color = Sample(u_Slots[8 ]); return;
	case  9 : o_Color = Sample(u_Slots[9 ]); return;
	case 10 : o_Color = Sample(u_Slots[10]); return;
	case 11 : o_Color = Sample(u_Slots[11]); return;
	case 12 : o_Color = Sample(u_Slots[12]); return;
	case 13 : o_Color = Sample(u_Slots[13]); return;
	case 14 : o_Color = Sample(u_Slots[14]); return;
	case 15 : o_Color = Sample(u_Slots[15]); return;
	case 16 : o_Color = Sample(u_Slots[16]); return;
	case 17 : o_Color = Sample(u_Slots[17]); return;
	case 18 : o_Color = Sample(u_Slots[18]); return;
	case 19 : o_Color = Sample(u_Slots[19]); return;
	case 20 : o_Color = Sample(u_Slots[20]); return;
	case 21 : o_Color = Sample(u_Slots[21]); return;
	case 22 : o_Color = Sample(u_Slots[22]); return;
	case 23 : o_Color = Sample(u_Slots[23]); return;
	case 24 : o_Color = Sample(u_Slots[24]); return;
	case 25 : o_Color = Sample(u_Slots[25]); return;
	case 26 : o_Color = Sample(u_Slots[26]); return;
	case 27 : o_Color = Sample(u_Slots[27]); return;
	case 28 : o_Color = Sample(u_Slots[28]); return;
	case 29 : o_Color = Sample(u_Slots[29]); return;
	case 30 : o_Color = Sample(u_Slots[30]); return;
	case 31 : o_Color = Sample(u_Slots[31]); return;
	}
}
