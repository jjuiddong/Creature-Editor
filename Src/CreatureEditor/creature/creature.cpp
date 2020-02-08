
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
	for (auto &gnode : gnodes)
		m_gmap[gnode->id] = gnode;

	LoadFromGenoType(renderer, g_pheno->m_generationCnt);
	return true;
}


// create phenotype node from genotype node
void cCreature::LoadFromGenoType(graphic::cRenderer &renderer
	, const uint generation)
{
	// clear phenotype node
	for (auto &p : m_nodes)
		delete p;
	m_nodes.clear();

	GenerationGenoType(generation);

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


// Genotype node generation
bool cCreature::GenerationGenoType(const uint generation)
{
	if (generation <= 0)
		return true; // complete

	// 1. find iteration node
	// 2. add new genotype node where iteration node
	// 3. add iteration node where new genotype node
	// 4. decreament generation
	set<sGenotypeNode*> iterNodes;
	set<sGenotypeNode*> addNodes;
	for (uint i=0; i < m_gnodes.size(); ++i)
	{
		sGenotypeNode *gnode = m_gnodes[i];
		if (gnode->iteration < 0 || gnode->generation)
			continue;
		if (iterNodes.end() != iterNodes.find(gnode))
			continue; // generated node, ignore
		auto it = m_gmap.find(gnode->iteration);
		if (m_gmap.end() == it)
			continue;

		sGenotypeNode *parent = it->second;

		// new iteration node
		const Matrix44 tm0 = parent->transform.GetMatrix();
		const Matrix44 tm1 = gnode->transform.GetMatrix();
		const Matrix44 localTm = tm1 * tm0.Inverse();
		const Matrix44 tm = localTm * tm1;
		
		const Vector3 scale(gnode->transform.scale.x / parent->transform.scale.x
			, gnode->transform.scale.y / parent->transform.scale.y
			, gnode->transform.scale.z / parent->transform.scale.z);
		const Vector3 newScale = gnode->transform.scale * scale;

		if (newScale.Length() < 0.3f)
			continue; // ignore too small dimension object
		
		sGenotypeNode *newIter = new sGenotypeNode;
		*newIter = *gnode;
		newIter->id = common::GenerateId();
		newIter->iteration = gnode->id;
		newIter->transform.pos = tm.GetPosition();
		newIter->transform.scale = newScale;
		newIter->transform.rot = tm.GetQuaternion();
		newIter->generation = false;
		m_gnodes.push_back(newIter);
		m_gmap[newIter->id] = newIter;
		iterNodes.insert(newIter);
		gnode->generation = true;
		addNodes.insert(gnode);

		// if iteration node has link another node or final node?
		// copy to new iteration node
		for (auto &glink : m_glinks)
		{
			if (glink->gnode0 == parent)
				continue; // ignore parent iteration node
			if (glink->gnode1 == gnode)
			{
				// find another link node



			}
		}

		// new link, iter node - new node
		sGenotypeLink *parentLink = nullptr;
		for (auto &glink : m_glinks)
		{
			if ((glink->gnode0 == parent)
				&& (glink->gnode1 == gnode))
			{
				parentLink = glink;
				break;
			}		
		}
		if (!parentLink)
			break; // error occurred

		sGenotypeLink *newLink = new sGenotypeLink;
		*newLink = *parentLink;
		newLink->gnode0 = gnode;
		newLink->gnode1 = newIter;
		newLink->nodeLocal0 = gnode->transform;
		newLink->nodeLocal1 = newIter->transform;
		m_glinks.push_back(newLink);
	}

	for (sGenotypeNode *gnode : addNodes)
	{
		auto it = m_gmap.find(gnode->iteration);
		if (m_gmap.end() == it)
			continue;

		// new genotype link
		// find iteration link
		sGenotypeNode *parent = it->second;
		GenerationGenotypeLink(parent, gnode);
	}

	return GenerationGenoType(generation - 1);
}


// generation genotype node and link from source genotype node
// src: source genotype node
// gen: new genotype node
void cCreature::GenerationGenotypeLink(sGenotypeNode *src, sGenotypeNode *gen)
{
	// new genotype link
	// find iteration link
	for (uint i=0; i < m_glinks.size(); ++i)
	{
		// find parent link
		// find only parent role link
		sGenotypeLink *glink = m_glinks[i];
		if (glink->gnode0 != src)
			continue;
		if (glink->gnode1 == gen)
			continue;

		// new iteration node
		const Vector3 p0 = glink->gnode0->transform.pos;
		const Vector3 p1 = glink->gnode1->transform.pos;
		const Vector3 dir = (p1 - p0).Normal();
		const float len = p1.Distance(p0);
		const Vector3 nextPos = dir * len + gen->transform.pos;

		sGenotypeNode *newNode = new sGenotypeNode;
		*newNode = *glink->gnode1;
		newNode->id = common::GenerateId();
		newNode->transform.pos = nextPos;
		if (glink->gnode1->iteration >= 0)
			newNode->iteration = gen->id;
		m_gnodes.push_back(newNode);
		m_gmap[newNode->id] = newNode;

		sGenotypeLink *newLink = new sGenotypeLink;
		*newLink = *glink;
		newLink->gnode0 = gen;
		newLink->gnode1 = newNode;
		newLink->nodeLocal0 = gen->transform;
		newLink->nodeLocal1 = newNode->transform;
		m_glinks.push_back(newLink);

		// generate next link node
		GenerationGenotypeLink(glink->gnode1, newNode);
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

	m_gmap.clear();
}
