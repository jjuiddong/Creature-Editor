
#include "stdafx.h"
#include "nnmanager.h"
#include "../view/nnview.h"
#include "../creature/creature.h"


cNNManager::cNNManager()
	: m_mode(eNNEditMode::Normal)
	, m_orbitId(-1)
	, m_saveFileName("filename.gnt")
	, m_creature(nullptr)
{
}

cNNManager::~cNNManager()
{
	Clear();
}


bool cNNManager::Init(graphic::cRenderer &renderer)
{
	m_gizmo.Create(renderer, false);

	m_uiLink.m_autoDelete = false;
	return true;
}


// change edit state
void cNNManager::ChangeEditMode(const eNNEditMode state)
{
	m_mode = state;
}


eNNEditMode cNNManager::GetEditMode()
{
	return m_mode;
}


// read creature phenotype node
bool cNNManager::SetCurrentCreature(evc::cCreature *creature)
{
	m_creature = creature;
	return true;
}


evc::cGNode* cNNManager::FindGNode(const int id)
{
	//auto it = std::find_if(m_gnodes.begin(), m_gnodes.end()
	//	, [&](const auto &a) { return a->m_id == id; });
	//if (m_gnodes.end() == it)
	//	return nullptr;
	//return *it;
	return nullptr;
}


evc::cGLink* cNNManager::FindGLink(const int id)
{
	//auto it = std::find_if(m_glinks.begin(), m_glinks.end()
	//	, [&](const auto &a) { return a->m_id == id; });
	//if (m_glinks.end() == it)
	//	return nullptr;
	//return *it;
	return nullptr;
}


bool cNNManager::AddGNode(evc::cGNode *gnode)
{
	//if (m_gnodes.end() != find(m_gnodes.begin(), m_gnodes.end(), gnode))
	//	return false; // already exist
	//m_gnodes.push_back(gnode);
	//return true;
	return false;
}


bool cNNManager::RemoveGNode(evc::cGNode *gnode)
{
	//// if has iterator? remove all iterator gnode
	//set<evc::cGNode*> rms;
	//for (auto &p : m_gnodes)
	//	if (p->m_cloneId == gnode->m_id)
	//		rms.insert(p);
	//rms.insert(gnode);

	//for (auto &p : rms)
	//{
	//	for (int i = (int)p->m_links.size() - 1; i >= 0; --i) // back traverse
	//		RemoveGLink(p->m_links[i]);
	//	common::removevector(m_gnodes, p);
	//	delete p;
	//}

	//return true;
	return false;
}


bool cNNManager::AddGLink(evc::cGLink *glink)
{
	//if (m_glinks.end() != find(m_glinks.begin(), m_glinks.end(), glink))
	//	return false; // already exist
	//m_glinks.push_back(glink);
	//return true;
	return false;
}


bool cNNManager::RemoveGLink(evc::cGLink *glink)
{
	//RETV(!glink, false);

	//if (glink->m_gnode0)
	//	glink->m_gnode0->RemoveLink(glink);
	//if (glink->m_gnode1)
	//	glink->m_gnode1->RemoveLink(glink);

	//common::removevector(m_glinks, glink);
	//if (glink->m_autoDelete)
	//{
	//	delete glink;
	//}
	//else
	//{
	//	glink->m_gnode0 = nullptr;
	//	glink->m_gnode1 = nullptr;
	//}
	//return true;
	return false;
}


// clear select and then select id
void cNNManager::SetSelection(const int id)
{
	evc::cGNode *gnode = FindGNode(id);
	evc::cGLink *glink = FindGLink(id);

	// select spawn genotype node
	ChangeEditMode(eNNEditMode::Normal);
	ClearSelection();
	SelectObject(id);

	// change focus
	//g_global->m_genoView->SetFocus((framework::cDockWindow*)g_global->m_genoView);

	if (gnode)
	{
		m_gizmo.SetControlNode(gnode);
		m_gizmo.LockEditType(graphic::eGizmoEditType::SCALE, false);

	}
	else if (glink)
	{
		m_gizmo.SetControlNode(glink);
		m_gizmo.LockEditType(graphic::eGizmoEditType::SCALE, true);
	}
}


// pick genotype node, selection node
// id: genotype node, link
bool cNNManager::SelectObject(const int id
	, const bool isToggle //= false
)
{
	const graphic::cColor &selColor = graphic::cColor::RED;
	const graphic::cColor &noSelColor = graphic::cColor::WHITE;

	evc::cGNode *gnode = g_geno->FindGNode(id);

	auto it = std::find(m_selects.begin(), m_selects.end(), id);
	if (isToggle)
	{
		if (m_selects.end() == it)
		{
			if (gnode)
				gnode->m_txtColor = selColor;
			m_selects.push_back(id);
		}
		else
		{
			if (gnode)
				gnode->m_txtColor = noSelColor;
			common::removevector(m_selects, id);
		}
	}
	else
	{
		if (m_selects.end() == it)
		{
			if (gnode)
				gnode->m_txtColor = selColor;
			m_selects.push_back(id);
		}
	}

	// update multi selection transform
	if (m_selects.size() > 1)
	{
		Vector3 center;
		for (auto &id : m_selects)
		{
			evc::cGNode *p = g_geno->FindGNode(id);
			if (p)
				center += p->m_transform.pos;
		}
		center /= (float)m_selects.size();
		center.y += 0.5f;
		m_multiSelPos = center;
		m_multiSelRot = Quaternion();
		m_multiSel.m_transform.rot = Quaternion();
		m_multiSel.m_transform.pos = center;
		m_gizmo.SetControlNode(&m_multiSel);
	}

	if (m_selects.size() == 1)
	{
		evc::cGNode *gnode = g_geno->FindGNode(m_selects[0]);
		if (gnode)
			m_gizmo.SetControlNode(gnode);
	}

	return true;
}


bool cNNManager::ClearSelection()
{
	m_orbitId = -1;
	m_selects.clear();
	m_highLights.clear();
	for (auto &p : m_gnodes)
		p->m_txtColor = graphic::cColor::WHITE;
	return true;
}


// traverse all linked genotype node
template<typename Fn>
bool TraverseAllLinkedNode(evc::cGNode *gnode, Fn fn)
{
	set<evc::cGNode*> visit;
	queue<evc::cGNode*> q;
	q.push(gnode);
	while (!q.empty())
	{
		evc::cGNode *n = q.front();
		q.pop();

		if (visit.end() != std::find(visit.begin(), visit.end(), n))
			continue; // already visit

		if (!fn(n))
			continue;

		visit.insert(n);

		for (auto &p : n->m_links)
		{
			q.push(p->m_gnode0);
			q.push(p->m_gnode1);
		}
	}
	return true;
}


bool cNNManager::SetAllLinkedNodeSelect(evc::cGNode *gnode)
{
	ClearSelection();
	TraverseAllLinkedNode(gnode,
		[&](evc::cGNode *p)
	{
		SelectObject(p->m_id);
		return true;
	}
	);
	return true;
}


bool cNNManager::UpdateAllLinkedNodeTransform(evc::cGNode *gnode, const Transform &transform)
{
	TraverseAllLinkedNode(gnode,
		[&](evc::cGNode *p)
	{
		p->m_transform.pos += transform.pos;
		return true;
	}
	);
	return true;
}


// auto save all genotype node, link
bool cNNManager::AutoSave()
{
	if (m_gnodes.empty())
		return false;
	return evc::WriteGenoTypeFileFrom_Node("autosave_genotype.gnt", m_gnodes);
}


graphic::cRenderer& cNNManager::GetRenderer()
{
	return g_global->m_nnView->GetRenderer();
}


void cNNManager::Clear()
{
	m_creature = nullptr;

	for (auto &p : m_glinks)
		if (p->m_autoDelete)
			delete p;
	m_glinks.clear();

	for (auto &p : m_gnodes)
		delete p;
	m_gnodes.clear();
}
