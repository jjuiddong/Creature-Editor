
#include "stdafx.h"
#include "pnode.h"

using namespace evc;


cPNode::cPNode()
{
}

cPNode::~cPNode()
{
	Clear();
}



void cPNode::Clear()
{
	for (auto &p : m_children)
		delete p;
	m_children.clear();
}
