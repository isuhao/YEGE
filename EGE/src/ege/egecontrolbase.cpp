﻿#include "head.h"
#include "global.h"
#include "image.h"
#include <algorithm>
#include <set>
#include <vector>

namespace ege
{

typedef std::set<egeControlBase*> egectlmap;
typedef std::vector<egeControlBase*> egectlvec;

int egeControlBase::s_maxchildid = 1024;

static egectlvec s_egeCtlParent;

egeControlBase::InitObject::InitObject(egeControlBase* pThis, int inherit_level)
{
	m_this = pThis;
	m_inherit_level = inherit_level;
}
egeControlBase::InitObject::~InitObject()
{
	if(m_this->m_inheritlevel == m_inherit_level)
		m_this->initok();
}
egeControlBase::egeControlBase()
{
	init(nullptr);
}

egeControlBase::egeControlBase(int inherit, egeControlBase* pParent)
{
	if(s_egeCtlParent.size() > 0 && !pParent)
		pParent = *s_egeCtlParent.rbegin();
	m_inheritlevel = inherit;
	s_egeCtlParent.push_back(this);
	init(pParent);
}

void egeControlBase::initok()
{
	s_egeCtlParent.pop_back();
}

egeControlBase::~egeControlBase()
{
	if(m_parent)
	{
		m_parent->delchild(this);
		if(const auto cmap = (egectlmap*)m_childmap)
			for(auto p_ctl : *cmap)
				m_parent->addchild(p_ctl);
	}
	delimage(m_mainbuf);
	delimage(m_mainFilter);
}

void
egeControlBase::init(egeControlBase* parent)
{
	auto pg = &graph_setting;
	egeControlBase*& root = pg->egectrl_root;
	m_parent = nullptr;
	m_mainbuf = newimage();
	m_mainFilter = newimage();
	if(!root)
	{
		root = this;
		m_parent = nullptr;
		m_bVisable = 0;
		m_bEnable = 0;
		m_bAutoDraw = 0;
		m_bDirectDraw = 1;
		m_zOrder = 0;
		m_w = getwidth();
		m_h = getheight();
	}
	else
	{
		if(parent)
		{
			parent->addchild(this);
			m_parent = parent;
		}
		else
		{
			root->addchild(this);
			m_parent = root;
		}
		m_bVisable = 1;
		m_bEnable = 1;
		m_bAutoDraw = 1;
		m_bDirectDraw = 0;
		m_zOrder = m_parent->allocZorder(); // 待处理
		m_w = m_h = 1;
	}
	m_zOrderLayer = 0;
	m_allocId = 0x10000;
	m_allocZorder = 1;
	m_bCapture = 0;
	m_bCapMouse = 0;
	m_bInputFocus = 0;
	m_childmap = nullptr;
	m_childzorder = nullptr;
	m_x = m_y = 0;
	m_rop = SRCCOPY;
	m_AlphablendMode = 0;
}

int egeControlBase::allocZorder()
{
	if(m_allocZorder > 0x800000)
		fixzorder();
	return m_allocZorder++;
}

int egeControlBase::allocId()
{
	return ++m_allocId;
}

bool ctlcmp(const egeControlBase* pa, const egeControlBase* pb)
{
	return *pa < *pb;
}

void egeControlBase::sortzorder()
{
	if(const auto cvec = (egectlvec*)m_childzorder)
		std::sort(cvec->begin(), cvec->end(), ctlcmp);
}

int egeControlBase::addchild(egeControlBase* pChild)
{
	egectlmap*& cmap = (egectlmap*&)m_childmap;
	egectlvec*& cvec = (egectlvec*&)m_childzorder;
	if(!cmap)
	{
		cmap = new egectlmap;
		cvec = new egectlvec;
	}
	if(pChild->m_parent)
		pChild->m_parent->delchild(pChild);
	++s_maxchildid;
	cmap->insert(pChild);
	cvec->push_back(pChild);
	sortzorder();
	pChild->m_parent = this;
	pChild->m_zOrder = allocZorder();
	onAddChild(pChild);
	return 0;
}

int egeControlBase::delchild(egeControlBase* pChild)
{
	auto& cmap = (egectlmap*&)m_childmap;

	if(!cmap)
		return 0;

	auto& cvec = (egectlvec*&)m_childzorder;
	auto it = cmap->find(pChild);

	if(it != cmap->end())
	{
		auto itv = cvec->begin();

		for(; itv != cvec->end(); itv++)
			if(*itv == *it) break;
		onDelChild(*it);
		cmap->erase(it);
		pChild->m_parent = nullptr;
		if(itv != cvec->end())
		{
			cvec->erase(itv);
			sortzorder();
		}
		return 1;
	}
	return 0;
}

void egeControlBase::fixzorder()
{
	if(const auto cmap = (egectlmap*)m_childmap)
	{
		egectlvec*& cvec = (egectlvec*&)m_childzorder;
		int z = 1;

		for(auto p_ctl : *cvec)
			p_ctl->m_zOrder = z++;
		m_allocZorder = z;
	}
}

void egeControlBase::zorderup()
{
	m_zOrder = m_parent->allocZorder();
	parent()->sortzorder();
}

void egeControlBase::zorderdown()
{
	m_zOrder = -m_parent->allocZorder();
	parent()->sortzorder();
}

void egeControlBase::zorderset(int z)
{
	m_zOrder = z;
	parent()->sortzorder();
}

void egeControlBase::mouse(int x, int y, int flag)
{
	auto pg = &graph_setting;
	int ret = 0;

	x -= m_x, y -= m_y;
	PushTarget _target;
	settarget(buf());
	ret = onMouse(x, y, flag);
	if(ret == 0)
	{
		egectlmap*& cmap = (egectlmap*&)m_childmap;
		egectlvec*& cvec = (egectlvec*&)m_childzorder;
		if(cmap)
		{
			egectlvec vec = *cvec;
			auto it = vec.rbegin();

			for(; it != vec.rend(); ++it)
				if((*it)->iscapmouse())
				{
					(*it)->mouse(x, y, flag);
					break;
				}
			if(it == vec.rend())
				it = vec.rbegin();
			for(; it != vec.rend(); ++it)
			{
				egeControlBase* pc = *it;
				if(!pc->isvisable() || !pc->isenable()) continue;
				if(x >= pc->getx() && y >= pc->gety() &&
					x < pc->getx() + pc->getw() && y < pc->gety() + pc->geth())
				{
					if(pc->m_AlphablendMode == 0 || pc->isdirectdraw()
						|| getpixel(x - pc->getx(), y - pc->gety(),
						pc->filter()))
					{
						if((flag & mouse_msg_down))
						{
							int ret = pc->onGetFocus();

							if(ret == 0)
							{
								pc->capture(true);
								pc->m_zOrder = allocZorder();
								sortzorder();
								if(pg->egectrl_focus && pg->egectrl_focus != pc
									&& pg->egectrl_focus != pc->parent())
								{
									for(egeControlBase* pcb = pg->egectrl_focus;
										pcb != pc && pcb->parent();
										pcb = pcb->parent())
									{
										pcb->onLostFocus();
										pcb->capture(false);
									}
								}
								pg->egectrl_focus = pc;
							}
						}
						pc->mouse(x, y, flag);
						break;
					}
					else
						continue;
				}
			}
		}
	}
}

void egeControlBase::keymsgdown(unsigned key, int flag)
{
	int ret = 0;
	{
		PushTarget _target;
		settarget(buf());
		ret = onKeyDown((int)key, flag);
	}
	if(ret == 0)
	{
		egectlmap*& cmap = (egectlmap*&)m_childmap;
		if(cmap)
		{
			for(egectlmap::reverse_iterator it = cmap->rbegin() ; it != cmap->rend(); ++it)
			{
				if(!(*it)->isvisable() || !(*it)->isenable()) continue;
				if((*it)->iscapture())
				{
					(*it)->keymsgdown(key, flag);
				}
			}
		}
	}
}

void egeControlBase::keymsgup(unsigned key, int flag)
{
	int ret = 0;
	{
		PushTarget _target;
		settarget(buf());
		onKeyUp((int)key, flag);
	}
	if(ret == 0)
	{
		egectlmap*& cmap = (egectlmap*&)m_childmap;
		if(cmap)
		{
			for(egectlmap::reverse_iterator it = cmap->rbegin() ; it != cmap->rend(); ++it)
			{
				if(!(*it)->isvisable() || !(*it)->isenable()) continue;
				if((*it)->iscapture())
				{
					(*it)->keymsgup(key, flag);
				}
			}
		}
	}
}

void egeControlBase::keymsgchar(unsigned key, int flag)
{
	int ret = 0;
	{
		PushTarget _target;
		settarget(buf());
		onKeyChar((int)key, flag);
	}
	if(ret == 0)
	{
		egectlmap*& cmap = (egectlmap*&)m_childmap;
		if(cmap)
		{
			for(egectlmap::reverse_iterator it = cmap->rbegin() ; it != cmap->rend(); ++it)
			{
				if(!(*it)->isvisable() || !(*it)->isenable()) continue;
				if((*it)->iscapture())
				{
					(*it)->keymsgchar(key, flag);
				}
			}
		}
	}
}

void egeControlBase::update()
{
	egectlmap*& cmap = (egectlmap*&)m_childmap;
	if(!m_parent)
	{
		m_w = getwidth();
		m_h = getheight();
	}
	if(cmap)
	{
		for(egectlmap::iterator it = cmap->begin() ; it != cmap->end(); ++it)
		{
			(*it)->update();
		}
	}
	PushTarget _target;
	settarget(buf());
	onUpdate();
}

void egeControlBase::draw(IMAGE* pimg)
{
	IMAGE* pmain = m_mainbuf;
	if(!m_parent || m_bDirectDraw)
		pmain = pimg;
	{
		PushTarget _target;
		settarget(pmain);
		onDraw(pmain);
	}
	egectlmap*& cmap = (egectlmap*&)m_childmap;
	egectlvec*& cvec = (egectlvec*&)m_childzorder;
	if(cmap)
	{
		for(egectlvec::iterator it = cvec->begin() ; it != cvec->end(); it++)
		{
			(*it)->draw(pmain);
		}
	}
	if(!m_bDirectDraw && m_bVisable)
	{
		if(m_AlphablendMode == SOLIDCOPY)
			//putimage(pimg, m_x, m_y, &m_mainbuf, m_rop);
			m_mainbuf->putimage(pimg, m_x, m_y, m_rop);
		else if(m_AlphablendMode == ALPHABLEND)
			m_mainbuf->putimage_alphafilter(pimg, m_x, m_y, m_mainFilter);
	}
}

} // namespace ege
