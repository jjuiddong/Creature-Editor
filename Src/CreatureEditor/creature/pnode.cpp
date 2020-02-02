
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
	g_evc->m_sync->RemoveSyncInfo(m_actor);

	for (auto &p : m_children)
		delete p;
	m_children.clear();
	m_actor = nullptr;
	m_node = nullptr;
}
