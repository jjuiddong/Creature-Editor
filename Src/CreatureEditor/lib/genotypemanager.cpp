
#include "stdafx.h"
#include "genotypemanager.h"
#include "../view/genoview.h"
#include "../view/genoeditorview.h"
#include "../view/resourceview.h"
#include "../creature/creature.h"
#include "../creature/gnode.h"
#include "../creature/glink.h"


cGenoTypeManager::cGenoTypeManager()
	: m_mode(eGenoEditMode::Normal)
	, m_spawnTransform(Vector3(0, 3, 0), Vector3(0.5f, 0.5f, 0.5f))
	, m_physics(nullptr)
	, m_physSync(nullptr)
	, m_orbitId(-1)
{
}

cGenoTypeManager::~cGenoTypeManager()
{
	Clear();
}


bool cGenoTypeManager::Init(graphic::cRenderer &renderer
	, phys::cPhysicsEngine *physics
	, phys::cPhysicsSync *sync)
{
	m_physics = physics;
	m_physSync = sync;

	m_gizmo.Create(renderer, false);

	m_uiLink.m_autoDelete = false;
	return true;
}


// change edit state
void cGenoTypeManager::ChangeEditMode(const eGenoEditMode state)
{
	m_mode = state;
}


eGenoEditMode cGenoTypeManager::GetEditMode()
{
	return m_mode;
}


// add creature ptr
bool cGenoTypeManager::AddCreature(evc::cCreature *creature)
{
	m_creatures.push_back(creature);
	return true;
}


// read genotype node file
bool cGenoTypeManager::ReadGenoTypeNodeFile(const StrPath &fileName
	, const Vector3 &pos)
{
	vector<evc::sGenotypeNode*> gnodes;
	vector<evc::sGenotypeLink*> glinks;
	map<int, evc::sGenotypeNode*> gnodeMap;
	if (!evc::ReadGenoTypeFile(fileName, gnodes, glinks, gnodeMap))
		return false;

	// create genotype node
	vector<evc::cGNode*> addNodes;
	map<int, evc::cGNode*> gmap;
	for (auto &p : gnodes)
	{
		evc::cGNode *node = new evc::cGNode();
		if (node->Create(GetRenderer(), *p))
		{
			g_geno->AddGNode(node);
			gmap[p->id] = node;
			addNodes.push_back(node);
		}
		else
		{
			delete node;
		}
	}

	// update iteration id (genotype nod id -> cGNode id)
	for (auto &p : addNodes)
	{
		if (p->m_cloneId < 0)
			continue;
		if (gmap.find(p->m_cloneId) == gmap.end())
		{
			if (!FindGNode(p->m_cloneId))
				p->m_cloneId = -1; // not found parent clone id
		}
		else
		{
			p->m_cloneId = gmap[p->m_cloneId]->m_id;
		}
	}

	// create genotype link
	for (auto &p : glinks)
	{
		evc::cGNode *node0 = gmap[p->parent->id];
		evc::cGNode *node1 = gmap[p->child->id];
		evc::cGLink *link = new evc::cGLink();
		if (link->Create(*p, node0, node1))
		{
			g_geno->AddGLink(link);
		}
		else
		{
			delete link;
		}
	}

	for (auto &p : gnodes)
		delete p;
	gnodes.clear();
	for (auto &p : glinks)
		delete p;
	glinks.clear();

	// moving actor position to camera center
	if (!gmap.empty())
	{
		Vector3 spawnPos = pos - gmap.begin()->second->m_transform.pos;
		spawnPos.y = 0;

		g_geno->ClearSelection();

		for (auto &kv : gmap)
		{
			evc::cGNode *gnode = kv.second;
			gnode->m_transform.pos += spawnPos;
			g_geno->SelectObject(gnode->m_id);
		}
	}

	return true;
}


// read creature file from genotype file
bool cGenoTypeManager::ReadCreatureFile(const StrPath &fileName, const Vector3 &pos)
{
	return ReadGenoTypeNodeFile(fileName, pos);
}


// create box genotype
int cGenoTypeManager::SpawnBox(const Vector3 &pos)
{
	graphic::cRenderer &renderer = GetRenderer();

	Transform tfm = m_spawnTransform;
	tfm.pos = pos;
	tfm.pos.y = m_spawnTransform.pos.y;

	evc::cGNode *node = new evc::cGNode();
	node->CreateBox(renderer, tfm);
	m_gnodes.push_back(node);

	SetSelection(node->m_id);
	AutoSave();
	return node->m_id;
}


// create sphere genotype
int cGenoTypeManager::SpawnSphere(const Vector3 &pos)
{
	graphic::cRenderer &renderer = GetRenderer();

	Transform tfm = m_spawnTransform;
	tfm.pos = pos;
	tfm.pos.y = m_spawnTransform.pos.y;
	const float radius = g_global->m_geditorView->m_radius;

	evc::cGNode *node = new evc::cGNode();
	node->CreateSphere(renderer, tfm, radius);
	m_gnodes.push_back(node);

	SetSelection(node->m_id);
	AutoSave();
	return node->m_id;
}


// create capsule genotype
int cGenoTypeManager::SpawnCapsule(const Vector3 &pos)
{
	graphic::cRenderer &renderer = GetRenderer();

	Transform tfm(pos, m_spawnTransform.rot);
	tfm.pos.y = m_spawnTransform.pos.y;
	const float density = g_global->m_geditorView->m_density;
	const float radius = g_global->m_geditorView->m_radius;
	const float halfHeight = g_global->m_geditorView->m_halfHeight;

	evc::cGNode *node = new evc::cGNode();
	node->CreateCapsule(renderer, tfm, radius, halfHeight);
	m_gnodes.push_back(node);

	SetSelection(node->m_id);
	AutoSave();
	return node->m_id;
}


// create cylinder genotype
int cGenoTypeManager::SpawnCylinder(const Vector3 &pos)
{
	graphic::cRenderer &renderer = GetRenderer();

	Transform tfm(pos, m_spawnTransform.rot);
	tfm.pos.y = m_spawnTransform.pos.y;
	const float density = g_global->m_geditorView->m_density;
	const float radius = g_global->m_geditorView->m_radius;
	const float height = g_global->m_geditorView->m_halfHeight;

	evc::cGNode *node = new evc::cGNode();
	node->CreateCylinder(renderer, tfm, radius, height);
	m_gnodes.push_back(node);

	SetSelection(node->m_id);
	AutoSave();
	return node->m_id;
}


evc::cGNode* cGenoTypeManager::FindGNode(const int id)
{
	auto it = std::find_if(m_gnodes.begin(), m_gnodes.end()
		, [&](const auto &a) { return a->m_id == id; });
	if (m_gnodes.end() == it)
		return nullptr;
	return *it;
}


evc::cGLink* cGenoTypeManager::FindGLink(const int id)
{
	auto it = std::find_if(m_glinks.begin(), m_glinks.end()
		, [&](const auto &a) { return a->m_id == id; });
	if (m_glinks.end() == it)
		return nullptr;
	return *it;
}


bool cGenoTypeManager::AddGNode(evc::cGNode *gnode)
{
	if (m_gnodes.end() != find(m_gnodes.begin(), m_gnodes.end(), gnode))
		return false; // already exist
	m_gnodes.push_back(gnode);
	return true;
}


bool cGenoTypeManager::RemoveGNode(evc::cGNode *gnode)
{
	// if has iterator? remove all iterator gnode
	set<evc::cGNode*> rms;
	for (auto &p : m_gnodes)
		if (p->m_cloneId == gnode->m_id)
			rms.insert(p);
	rms.insert(gnode);

	for (auto &p : rms)
	{
		for (int i = (int)p->m_links.size()-1; i>=0; --i) // back traverse
			RemoveGLink(p->m_links[i]);
		common::removevector(m_gnodes, p);
		delete p;
	}

	return true;
}


bool cGenoTypeManager::AddGLink(evc::cGLink *glink)
{
	if (m_glinks.end() != find(m_glinks.begin(), m_glinks.end(), glink))
		return false; // already exist
	m_glinks.push_back(glink);
	return true;
}


bool cGenoTypeManager::RemoveGLink(evc::cGLink *glink)
{
	RETV(!glink, false);

	if (glink->m_gnode0)
		glink->m_gnode0->RemoveLink(glink);
	if (glink->m_gnode1)
		glink->m_gnode1->RemoveLink(glink);

	common::removevector(m_glinks, glink);
	if (glink->m_autoDelete)
	{
		delete glink;
	}
	else
	{
		glink->m_gnode0 = nullptr;
		glink->m_gnode1 = nullptr;
	}
	return true;
}


// clear select and then select id
void cGenoTypeManager::SetSelection(const int id)
{
	evc::cGNode *gnode = FindGNode(id);
	evc::cGLink *glink = FindGLink(id);

	// select spawn genotype node
	ChangeEditMode(eGenoEditMode::Normal);
	ClearSelection();
	SelectObject(id);

	// change focus
	g_global->m_genoView->SetFocus((framework::cDockWindow*)g_global->m_genoView);

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
bool cGenoTypeManager::SelectObject(const int id
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


bool cGenoTypeManager::ClearSelection()
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


bool cGenoTypeManager::SetAllLinkedNodeSelect(evc::cGNode *gnode)
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


bool cGenoTypeManager::UpdateAllLinkedNodeTransform(evc::cGNode *gnode, const Transform &transform)
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
bool cGenoTypeManager::AutoSave()
{
	if (m_gnodes.empty())
		return false;
	return evc::WriteGenoTypeFileFrom_Node("autosave_genotype.gnt", m_gnodes);
}


graphic::cRenderer& cGenoTypeManager::GetRenderer()
{
	return g_global->m_genoView->GetRenderer();
}


void cGenoTypeManager::Clear()
{
	for (auto &p : m_creatures)
		delete p;
	m_creatures.clear();

	for (auto &p : m_glinks)
		if (p->m_autoDelete)
			delete p;
	m_glinks.clear();

	for (auto &p : m_gnodes)
		delete p;
	m_gnodes.clear();
}
