﻿#ifndef Inc_ege_colorbase_h_
#define Inc_ege_colorbase_h_

#include "ege/def.h"
#if YEGE_Use_YSLib
#	include <ysbuild.h>
#else
#	include <cstdint>
#endif

using PROPID = unsigned long;

namespace ege
{

#if YEGE_Use_YSLib
using color_t = YSLib::Drawing::Pixel;
using color_int_t = color_t::Trait::IntegerType;
using YSLib::Drawing::AlphaType;
using YSLib::Drawing::MonoType;
using YSLib::Drawing::BitmapPtr;
using YSLib::Drawing::ConstBitmapPtr;
#else
using color_t = int;
using color_int_t = std::uint32_t;
using AlphaType = octet;
using MonoType = octet;
using BitmapPtr = color_int_t*;
using ConstBitmapPtr = const color_int_t*;
#endif

yconstfn color_int_t
RGBTOBGR(MonoType color)
{
	return ((color & 0xFF) << 16) | ((color & 0xFF0000) >> 16)
		   | (color & 0xFF00FF00);
}

yconstfn color_int_t
EGERGB(MonoType r, MonoType g, MonoType b)
{
	return (b << 16) | (g << 8) | r;
}

yconstfn color_int_t
EGERGBA(MonoType r, MonoType g, MonoType b, MonoType a)
{
	return EGERGB(r, g, b) | a << 24;
}

yconstfn color_int_t
EGEARGB(MonoType a, MonoType r, MonoType g,
	MonoType b)
{
	return EGERGB(r, g, b) | a << 24;
}


yconstfn color_int_t
EGEACOLOR(MonoType a, color_int_t color)
{
	return (color & 0xFFFFFF) | (a << 24);
}
yconstfn color_int_t
EGECOLORA(MonoType a, color_int_t color)
{
	return (color & 0xFFFFFF) | (a << 24);
}


yconstfn MonoType
EGEGET_R(color_int_t c)
{
	return (c >> 16) & 0xFF;
}

yconstfn MonoType
EGEGET_G(color_int_t c)
{
	return (c >> 8) & 0xFF;
}

yconstfn MonoType
EGEGET_B(color_int_t c)
{
	return c & 0xFF;
}

yconstfn MonoType
EGEGET_A(color_int_t c)
{
	return (c >> 24) & 0xFF;
}

yconstfn color_int_t
EGEGRAY(color_int_t gray)
{
	return (gray << 16) | (gray << 8) | gray;
}

yconstfn color_int_t
EGEGRAYA(color_int_t gray, MonoType a)
{
	return EGEGRAY(gray) | (a << 24);
}

yconstfn color_int_t
EGEAGRAY(MonoType a, color_int_t gray)
{
	return EGEGRAY(gray) | (a << 24);
}

} // namespace ege;

#endif

