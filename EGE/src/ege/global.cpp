﻿#include "global.h"
#include "head.h" // for IMAGE;
#include <winbase.h>
#include <memory> // for std::unique_ptr;

namespace ege
{

bool
_graph_setting::_is_run() const
{
	return !(exit_window || exit_flag);
}

float
_graph_setting::_get_FPS(int add)
{
	static int fps = 0;
	static int fps_inv = 0;
	static double time = 0;
	static float flret = 0;
	static float fret = 0;
	static float fret_inv = 0;

	double cur = _get_highfeq_time_ls();
	if(add == 0x100)
		++fps;
	else if(add == -0x100)
		++fps, ++fps_inv;
	if(cur - time >= 0.5)
	{
		flret = fret;
		fret = (float)(fps / (cur - time));
		fret_inv = (float)((fps - fps_inv) / (cur - time));
		fps = 0;
		fps_inv = 0;
		time = cur;
	}
	return add > 0 ? (fret + flret) / 2 : fret_inv;
}

double
_graph_setting::_get_highfeq_time_ls()
{
	static ::LARGE_INTEGER llFeq; /* 此实为常数 */
	::LARGE_INTEGER llNow;

	if(get_highfeq_time_start.QuadPart == 0)
	{
		::SetThreadAffinityMask(::GetCurrentThread(), 0);
		::QueryPerformanceCounter(&get_highfeq_time_start);
		::QueryPerformanceFrequency(&llFeq);
		return 0;
	}
	else
	{
		::QueryPerformanceCounter(&llNow);
		llNow.QuadPart -= get_highfeq_time_start.QuadPart;
		return (double)llNow.QuadPart / llFeq.QuadPart;
	}
}

void
_graph_setting::_set_activepage(int page)
{
	active_page = page;
	if(!img_page[page])
	{
		img_page[page] = new IMAGE;
		img_page[page]->createimage(dc_w, dc_h);
		dc = img_page[page]->m_hDC;
	}
}

void
_graph_setting::_set_visualpage(int page)
{
	visual_page = page;
	if(!img_page[page])
	{
		img_page[page] = new IMAGE;
		img_page[page]->createimage(dc_w, dc_h);
	}
	update_mark_count = 0;
}

int
_graph_setting::_dealmessage(bool force_update)
{
	if(force_update || update_mark_count <= 0)
		_update();
	return !exit_window;
}

void
_graph_setting::_delay_ms(long ms)
{
	auto& root = egectrl_root;
	skip_timer_mark = true;
	if(ms == 0)
		_delay_update();
	else
	{
		double delay_time(ms);
		double dw(_get_highfeq_time_ls() * 1000.0);
		int f(ms >= 50 ? 0 : 100);

		delay_ms_dwLast = 0;
		if(delay_ms_dwLast == 0)
			delay_ms_dwLast = _get_highfeq_time_ls() * 1000.0;
		if(delay_ms_dwLast + 200.0 > dw)
			dw = delay_ms_dwLast;

		//ege_sleep(1);
		egectrl_root->draw(nullptr);
		while(dw + delay_time >= _get_highfeq_time_ls() * 1000.0)
		{
			if(f <= 0 || update_mark_count < UPDATE_MAX_CALL)
			{
				_dealmessage(FORCE_UPDATE);
				f = 256;
			}
			else
				ege_sleep((int)(dw + delay_time - _get_highfeq_time_ls()
					* 1000.0));
			--f;
		}
		_dealmessage(FORCE_UPDATE);
		dw = _get_highfeq_time_ls() * 1000.0;
		guiupdate(this, root);
		if(delay_ms_dwLast + 200.0 <= dw || delay_ms_dwLast > dw)
			delay_ms_dwLast = dw;
		else
			delay_ms_dwLast += delay_time;
	}
	skip_timer_mark = false;
}

void
_graph_setting::_delay_update()
{
	if(update_mark_count < UPDATE_MAX_CALL)
	{
		ege_sleep(1);
		egectrl_root->draw(nullptr);
		_dealmessage(FORCE_UPDATE);
		egectrl_root->update();

		int l, t, r, b, c;

		getviewport(&l, &t, &r, &b, &c);
		setviewport(l, t, r, b, c);
	}
	delay_ms_dwLast = _get_highfeq_time_ls() * 1000.0;
	skip_timer_mark = false;
}

void
_graph_setting::_flushkey()
{
	EGEMSG msg;

	if(msgkey_queue->empty())
		_dealmessage(NORMAL_UPDATE);
	if(!msgkey_queue->empty())
		while(msgkey_queue->pop(msg))
			;
}

void
_graph_setting::_flushmouse()
{
	EGEMSG msg;

	if(msgmouse_queue->empty())
		_dealmessage(NORMAL_UPDATE);
	if(!msgmouse_queue->empty())
		while(msgmouse_queue->pop(msg))
			;
}

int
_graph_setting::_getch_ex(int flag)
{
	if(exit_window)
		return grNoInitGraph;
	{
		int key;
		EGEMSG msg;
		::DWORD dw = GetTickCount();
		do
		{
			key = _kbhit_ex(flag);
			if(key < 0)
				break;
			if(key > 0)
			{
				key = _getkey_p();
				if(key)
				{
					msg = msgkey_queue->last();
					if(dw < msg.time + 1000)
					{
						int ogn_key = key;

						key &= 0xFFFF;

						int ret = key;

						if(flag)
							ret = ogn_key;
						else if(((ogn_key & KEYMSG_DOWN) && (msg.wParam >= 0x70
							&& msg.wParam < 0x80)) || (msg.wParam > ' '
							&& msg.wParam < '0'))
							ret |= 0x100;
						return ret;
					}
				}
			}
		} while(!exit_window && !exit_flag && _waitdealmessage());
	}
	return 0;
}

key_msg
_graph_setting::_getkey()
{
	key_msg ret{0, key_msg_none, 0};

	if(!exit_window)
	{
		int key = 0;

		do
		{
			if((key = _getkey_p()))
			{
				key_msg msg{0, key_msg_none, 0};

				if(key & KEYMSG_DOWN)
					msg.msg = key_msg_down;
				else if(key & KEYMSG_UP)
					msg.msg = key_msg_up;
				else if(key & KEYMSG_CHAR)
					msg.msg = key_msg_char;
				msg.key = key & 0xFFFF;
				if(keystate(VK_CONTROL))
					msg.flags |= key_flag_ctrl;
				if(keystate(VK_SHIFT))
					msg.flags |= key_flag_shift;
				return msg;
			}
		} while(!exit_window && !exit_flag && _waitdealmessage());
	}
	return ret;
}

int
_graph_setting::_getkey_p()
{
	EGEMSG msg;

	while(msgkey_queue->pop(msg))
		switch(msg.message)
		{
		case WM_CHAR:
			return KEYMSG_CHAR | (int(msg.wParam) & 0xFFFF);
		case WM_KEYDOWN:
			return KEYMSG_DOWN | (int(msg.wParam) & 0xFFFF)
				| (msg.lParam & 0x40000000 ? 0 : KEYMSG_FIRSTDOWN);
		case WM_KEYUP:
			return KEYMSG_UP | (int(msg.wParam) & 0xFFFF);
		default:
			break;
		}
	return 0;
}

mouse_msg
_graph_setting::_getmouse()
{
	auto mmsg = mouse_msg();

	if(exit_window)
		return mmsg;

	EGEMSG msg;

	do
	{
		msgmouse_queue->pop(msg);
		if(msg.hwnd)
		{
			mmsg.flags |= ((msg.wParam & MK_CONTROL) != 0
				? mouse_flag_ctrl : 0);
			mmsg.flags |= ((msg.wParam & MK_SHIFT) != 0 ? mouse_flag_shift : 0);
			mmsg.x = short(int(msg.lParam) & 0xFFFF);
			mmsg.y = short(unsigned(msg.lParam) >> 16);
			mmsg.msg = mouse_msg_move;
			if(msg.message == WM_LBUTTONDOWN)
			{
				mmsg.msg = mouse_msg_down;
				mmsg.flags |= mouse_flag_left;
			}
			else if(msg.message == WM_RBUTTONDOWN)
			{
				mmsg.msg = mouse_msg_down;
				mmsg.flags |= mouse_flag_right;
			}
			else if(msg.message == WM_MBUTTONDOWN)
			{
				mmsg.msg = mouse_msg_down;
				mmsg.flags |= mouse_flag_mid;
			}
			else if(msg.message == WM_LBUTTONUP)
			{
				mmsg.msg = mouse_msg_up;
				mmsg.flags |= mouse_flag_left;
			}
			else if(msg.message == WM_RBUTTONUP)
			{
				mmsg.msg = mouse_msg_up;
				mmsg.flags |= mouse_flag_right;
			}
			else if(msg.message == WM_MBUTTONUP)
			{
				mmsg.msg = mouse_msg_up;
				mmsg.flags |= mouse_flag_mid;
			}
			else if(msg.message == WM_MOUSEWHEEL)
			{
				mmsg.msg = mouse_msg_wheel;
				mmsg.wheel = short(unsigned(msg.wParam) >> 16);
			}
			return mmsg;
		}
	} while(!exit_window && !exit_flag && _waitdealmessage());
	return mmsg;
}

#if 0
EGEMSG
_graph_setting::_getmouse_p()
{
	auto msg = EGEMSG();

	msgmouse_queue->pop(msg);
	return msg;
}
#endif

int
_graph_setting::_kbhit_ex(int flag)
{
	if(exit_window)
		return grNoInitGraph;
	if(flag == 0)
		return _peekkey();
	else
		return _peekallkey(flag);
}

int
_graph_setting::_keystate(int key)
{
	if(key < 0 || key >= MAX_KEY_VCODE)
		return -1;
	if(!(::USHORT(::GetKeyState(key)) & 0x8000))
		keystatemap[key] = 0;
	return keystatemap[key];
}

void
_graph_setting::_on_paint(::HWND hwnd)
{
	if(!lock_window)
	{
		::PAINTSTRUCT ps;

		_on_repaint(hwnd, ::BeginPaint(hwnd, &ps));
	}
	else
	{
		::ValidateRect(hwnd, nullptr);
		--update_mark_count;
	}
}

void
_graph_setting::_on_repaint(::HWND hwnd, ::HDC dc)
{
	bool release = false;

	img_timer_update->copyimage(img_page[visual_page]);
	if(!dc)
	{
		dc = ::GetDC(hwnd);
		release = true;
	}

	int left = img_timer_update->m_vpt.left, top = img_timer_update->m_vpt.top;

	::BitBlt(dc, 0, 0, base_w, base_h, img_timer_update->m_hDC,
		base_x - left, base_y - top, SRCCOPY);
	if(release)
		::ReleaseDC(hwnd, dc);
}

int
_graph_setting::_peekkey()
{
	EGEMSG msg;

	while(msgkey_queue->pop(msg))
	{
		if(msg.message == WM_CHAR || msg.message == WM_KEYDOWN)
		{
			if(msg.message == WM_KEYDOWN)
				if(msg.wParam <= key_space || (msg.wParam >= key_0
					&& msg.wParam < key_f1) || (msg.wParam >= key_semicolon
					&& msg.wParam <= key_quote))
					continue;
			msgkey_queue->unpop();
			if(msg.message == WM_CHAR)
				return KEYMSG_CHAR | (int(msg.wParam) & 0xFFFF);
			if(msg.message == WM_KEYDOWN)
			{
				if(msg.wParam >= 0x70 && msg.wParam < 0x80)
					return (KEYMSG_DOWN | (int(msg.wParam) + 0x100));
				return (KEYMSG_DOWN | (int(msg.wParam) & 0xFFFF));
			}
			else if(msg.message == WM_KEYUP)
				return KEYMSG_UP | (int(msg.wParam) & 0xFFFF);
		}
	}
	return 0;
}

int
_graph_setting::_peekallkey(int flag)
{
	EGEMSG msg;

	while(msgkey_queue->pop(msg))
	{
		if((msg.message == WM_CHAR && (flag & KEYMSG_CHAR_FLAG)) ||
			(msg.message == WM_KEYUP && (flag & KEYMSG_UP_FLAG)) ||
			(msg.message == WM_KEYDOWN && (flag & KEYMSG_DOWN_FLAG)))
		{
			msgkey_queue->unpop();
			if(msg.message == WM_CHAR)
				return (KEYMSG_CHAR | (int(msg.wParam) & 0xFFFF));
			else if(msg.message == WM_KEYDOWN)
				return (KEYMSG_DOWN | (int(msg.wParam) & 0xFFFF)
					| (msg.lParam & 0x40000000 ? 0 : KEYMSG_FIRSTDOWN));
			else if(msg.message == WM_KEYUP)
				return KEYMSG_UP | (int(msg.wParam) & 0xFFFF);
		}
	}
	return 0;
}

EGEMSG
_graph_setting::_peekmouse()
{
	auto msg = EGEMSG();

	if(msgmouse_queue->empty())
		_dealmessage(NORMAL_UPDATE);
	while(msgmouse_queue->pop(msg))
	{
		msgmouse_queue->unpop();
		return msg;
	}
	return msg;
}

int
_graph_setting::_redraw_window(::HDC dc)
{
	int page = visual_page;
	::HDC hDC = img_page[page]->m_hDC;
	int left = img_page[page]->m_vpt.left,
		top = img_page[page]->m_vpt.top;
	//::HRGN rgn = img_page[page]->m_rgn;
	::BitBlt(dc, 0, 0, base_w, base_h, hDC, base_x - left, base_y - top,
		SRCCOPY);
	update_mark_count = UPDATE_MAX_CALL;
	return 0;
}

int
_graph_setting::_update()
{
	if(exit_window)
		return grNoInitGraph;

	::HDC hdc;
	if(IsWindowVisible(hwnd))
	{
		hdc = window_dc;

		if(!hdc)
			return grNullPointer;
		_redraw_window(hdc);
	}
	else
		update_mark_count = UPDATE_MAX_CALL;
	_get_FPS(0x100);

	::RECT rect, crect;
	::HWND h_wnd;
	int _dw, _dh;

	::GetClientRect(hwnd, &crect);
	::GetWindowRect(hwnd, &rect);

	int w = dc_w, h = dc_h;
	_dw = w - (crect.right - crect.left);
	_dh = h - (crect.bottom - crect.top);
	if(_dw != 0 || _dh != 0)
	{
		h_wnd = ::GetParent(hwnd);
		if(h_wnd)
		{
			::POINT pt{0, 0};

			::ClientToScreen(h_wnd, &pt);
			rect.left -= pt.x;
			rect.top -= pt.y;
			rect.right -= pt.x;
			rect.bottom -= pt.y;
		}
		::SetWindowPos(hwnd, nullptr, 0, 0, rect.right  + _dw - rect.left,
			rect.bottom + _dh - rect.top,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	}
	return grOk;
}

int
_graph_setting::_waitdealmessage()
{
//	MSG msg;

	if(update_mark_count < UPDATE_MAX_CALL)
	{
		egectrl_root->draw(nullptr);
		_update();
		guiupdate(this, egectrl_root);
	}
	ege_sleep(1);
	return !exit_window;
}


namespace
{

_graph_setting&
get_global_state()
{
	static std::unique_ptr<_graph_setting> p(new _graph_setting());

	return *p;
}

}

_graph_setting& graph_setting(get_global_state());

} // namespace ege

