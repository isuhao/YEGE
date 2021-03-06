﻿#ifndef Inc_ege_pushtarget_h_
#define Inc_ege_pushtarget_h_

#include "ege/gapi.h"

namespace ege
{

class EGEAPI PushTarget
{
public:
	PushTarget()
	{
		m_target = gettarget();
	}
	PushTarget(IMAGE* target)
	{
		m_target = gettarget();
		settarget(target);
	}
	~PushTarget()
	{
		settarget(m_target);
	}
private:
	IMAGE* m_target;
};

}

#endif

