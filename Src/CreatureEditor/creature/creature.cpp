
#include "stdafx.h"
#include "creature.h"
#include "pnode.h"
#include "parser/GenotypeParser.h"

using namespace evc;


cCreature::cCreature(const StrId &name //="creature"
)
	: m_id(common::GenerateId())
	, m_name(name)
{
}

cCreature::~cCreature()
{
	Clear();
}


// read *.pnt, *.gnt
// pnt : phenotype script (json format)
// gnt : genotype script
bool cCreature::Read(graphic::cRenderer &renderer, const StrPath &fileName
	, const Transform &tfm //=Transform()
)
{
	Clear();

	const string ext = fileName.GetFileExt();
	if ((ext == ".pnt") || (ext == ".PNT"))
	{
		vector<int> syncIds;
		evc::ReadPhenoTypeFile(renderer, fileName, &syncIds);

		if (syncIds.empty())
			return false; // error occurred

		m_nodes.reserve(syncIds.size());
		for (auto &id : syncIds)
		{
			phys::sSyncInfo *sync = g_evc->m_sync->FindSyncInfo(id);
			if (!sync)
				continue;

			cPNode *node = new cPNode();
			node->m_actor = sync->actor;
			node->m_node = sync->node;
			m_nodes.push_back(node);
		}
	}
	else  if ((ext == ".gnt") || (ext == ".GNT"))
	{
		if (!ReadGenoTypeFile(renderer, fileName))
			return false;
	}

	SetTransform(tfm);

	return true;
}


bool cCreature::Write(const StrPath &fileName)
{
	return true;
}



bool cCreature::SetKinematic(const bool isKinematic)
{
	for (auto &p : m_nodes)
		p->m_actor->SetKinematic(isKinematic);
	return true;
}


// update transform
// todo : rotation, scaling
void cCreature::SetTransform(const Transform &tfm)
{
	Vector3 center;
	for (auto &p : m_nodes)
		center += p->m_node->m_transform.pos;
	center /= (float)m_nodes.size();

	const Vector3 targetPos = tfm.pos;
	Vector3 movePos = targetPos - center;
	movePos.y = 0;

	for (auto &p : m_nodes)
	{
		p->m_node->m_transform.pos += movePos;
		p->m_actor->SetGlobalPose(p->m_node->m_transform);
	}
}


// return all pnode syncid
bool cCreature::GetSyncIds(OUT vector<int> &out)
{
	out.reserve(m_nodes.size());
	for (auto &p : m_nodes)
		if (phys::sSyncInfo *sync = g_evc->m_sync->FindSyncInfo(p->m_actor))
			out.push_back(sync->id);
	return !out.empty();
}


// read genotype file
bool cCreature::ReadGenoTypeFile(graphic::cRenderer &renderer, const StrPath &fileName)
{
	Clear();

	vector<evc::sGenotypeNode*> gnodes;
	vector<evc::sGenotypeLink*> glinks;
	map<int, evc::sGenotypeNode*> gnodeMap;
	if (!evc::ReadGenoTypeFile(fileName, gnodes, glinks, gnodeMap))
		return false;

	m_gnodes = gnodes;
	m_glinks = glinks;
	LoadFromGenoType(renderer);
	return true;
}


// create phenotype node from genotype node
void cCreature::LoadFromGenoType(graphic::cRenderer &renderer)
{
	// clear phenotype node
	for (auto &p : m_nodes)
		delete p;
	m_nodes.clear();

	map<int, cPNode*> nodeMap;
	for (auto &p : m_gnodes)
	{
		cPNode *pnode = evc::CreatePhenoTypeNode(renderer, *p);
		if (!pnode)
			continue;
		m_nodes.push_back(pnode);
		nodeMap[p->id] = pnode;
	}

	for (auto &p : m_glinks)
	{
		cPNode *pnode0 = nodeMap[p->gnode0->id];
		cPNode *pnode1 = nodeMap[p->gnode1->id];
		if (!pnode0 || !pnode1)
			continue;

		evc::CreatePhenoTypeJoint(*p, pnode0, pnode1);
	}
}


void cCreature::Clear()
{
	for (auto &p : m_nodes)
		delete p;
	m_nodes.clear();

	for (auto &p : m_glinks)
		delete p;
	m_glinks.clear();

	for (auto &p : m_gnodes)
		delete p;
	m_gnodes.clear();
}
