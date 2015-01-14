﻿#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include "ege.h"
#include "global.h"
#include "image.h"


namespace ege
{

namespace
{

::LRESULT CALLBACK
wndproc(::HWND hWnd, ::UINT message, ::WPARAM wParam, ::LPARAM lParam)
{
	auto& gstate(FetchEGEApplication());
	//int wmId, wmEvent;
	const auto pg_w(reinterpret_cast<EGEApplication*>(
		::GetWindowLongPtrW(hWnd, GWLP_USERDATA)));

	if(!pg_w)
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	switch(message)
	{
	case WM_PAINT:
		if(&gstate == pg_w)
			gstate._on_paint(hWnd);
		break;
	case WM_CLOSE:
		if(&gstate == pg_w)
		{
			if(gstate.callback_close)
				gstate.callback_close();
			else
				return ::DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		if(&gstate == pg_w)
			gstate._on_destroy();
		break;
	case WM_ERASEBKGND:
		if(&gstate == pg_w)
			return TRUE;
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
	//	if(hWnd == gstate.hwnd)
		gstate._on_key(message, static_cast<unsigned long>(wParam), lParam);
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		gstate.mouse_lastclick_x = short(lParam & 0xFFFF);
		gstate.mouse_lastclick_y = short(::UINT(lParam) >> 16);
		gstate.keystatemap[VK_LBUTTON] = 1;
		::SetCapture(hWnd);
		gstate.mouse_state_l = 1;
		if(hWnd == gstate._get_hwnd())
			gstate._push_mouse_msg( message, wParam, lParam);
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		gstate.mouse_lastclick_x = short(lParam & 0xFFFF);
		gstate.mouse_lastclick_y = short(::UINT(lParam) >> 16);
		gstate.keystatemap[VK_MBUTTON] = 1;
		::SetCapture(hWnd);
		gstate.mouse_state_m = 1;
		if(hWnd == gstate._get_hwnd())
			gstate._push_mouse_msg(message, wParam, lParam);
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		gstate.mouse_lastclick_x = short(lParam & 0xFFFF);
		gstate.mouse_lastclick_y = short(::UINT(lParam) >> 16);
		gstate.keystatemap[VK_RBUTTON] = 1;
		::SetCapture(hWnd);
		gstate.mouse_state_r = 1;
		if(hWnd == gstate._get_hwnd())
			gstate._push_mouse_msg( message, wParam, lParam);
		break;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		gstate._on_mouse_button_up(hWnd, message, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		gstate.mouse_last_x = short(lParam & 0xFFFF);
		gstate.mouse_last_y = short(::UINT(lParam) >> 16);
		if(hWnd == gstate._get_hwnd() && (gstate.mouse_lastup_x
			!= gstate.mouse_last_x || gstate.mouse_lastup_y
			!= gstate.mouse_last_y))
			gstate._push_mouse_msg(message, wParam, lParam);
		break;
	case WM_MOUSEWHEEL:
		{
			::POINT pt{short(lParam & 0xFFFF), short(::UINT(lParam) >> 16)};

			::ScreenToClient(gstate._get_hwnd(), &pt);
			gstate.mouse_last_x = pt.x;
			gstate.mouse_last_y = pt.y;
			lParam = static_cast<unsigned short>(short(gstate.mouse_last_y))
				<< 16 | static_cast<unsigned short>(short(gstate.mouse_last_x));
		}
		if(hWnd == gstate._get_hwnd())
			gstate._push_mouse_msg(message, wParam, lParam);
		break;
	case WM_SETCURSOR:
		if(&gstate == pg_w)
		{
			gstate._on_setcursor(hWnd);
			return TRUE;
		}
		break;
	case WM_USER + 1:
		EGEApplication::_window_handle_wm_user_1(lParam, wParam);
		break;
	case WM_USER + 2:
		::SetFocus(::HWND(lParam));
		break;
	case WM_CTLCOLOREDIT:
		return (reinterpret_cast<egeControlBase*>(::GetWindowLongPtrW(
			::HWND(lParam), GWLP_USERDATA)))
			->onMessage(message, wParam, lParam);
		break;
	default:
		if(&gstate == pg_w)
			return ::DefWindowProc(hWnd, message, wParam, lParam);
	}
	if(&gstate != pg_w)
		return (reinterpret_cast<egeControlBase*>(pg_w))
			->onMessage(message, wParam, lParam);
	return 0;
}

}

::WNDPROC
getProcfunc()
{
	return wndproc;
}

} // namespace ege;

