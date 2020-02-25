
#include "stdafx.h"
#include "creature.h"
#include "pnode.h"
#include "parser/GenotypeParser.h"

using namespace evc;


cCreature::cCreature(const StrId &name //="creature"
)
	: m_id(common::GenerateId())
	, m_name(name)
	, m_generation(0)
	, m_isNN(true)
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



void cCreature::Update(const float deltaSeconds)
{
	if (m_isNN)
		for (auto &p : m_nodes)
			p->Update(deltaSeconds);
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
	for (auto &glink : glinks)
	{
		m_linkMap[glink->parent].insert(glink);
		m_linkMap[glink->child].insert(glink);
	}

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

	m_generation = generation;
	GenerationGenoType(0, generation);

	map<int, cPNode*> nodeMap;
	for (auto &p : m_gnodes)
	{
		cPNode *pnode = evc::CreatePhenoTypeNode(renderer, *p, generation);
		if (!pnode)
			continue;
		m_nodes.push_back(pnode);
		nodeMap[p->id] = pnode;
	}

	for (auto &p : m_glinks)
	{
		cPNode *pnode0 = nodeMap[p->parent->id];
		cPNode *pnode1 = nodeMap[p->child->id];
		if (!pnode0 || !pnode1)
			continue;

		evc::CreatePhenoTypeJoint(*p, pnode0, pnode1, generation);
	}

	// initialize neural network
	if (!m_nodes.empty())
	{
		cPNode *pnode = m_nodes[0];

		set<phys::cJoint*> joints;
		for (auto &p : m_nodes)
			for (auto &j : p->m_actor->m_joints)
				joints.insert(j);

		for (auto &j : joints)
		{
			cAngularSensor *AngleSen = new cAngularSensor();
			AngleSen->Create(j);
			pnode->AddSensor(AngleSen);

			cLimitSensor *limitSen = new cLimitSensor();
			limitSen->Create(j);
			pnode->AddSensor(limitSen);
			
			cMuscleEffector *effector = new cMuscleEffector();
			effector->Create(j);
			pnode->AddEffector(effector);
		}

		vector<double> weights;
		pnode->InitializeNN(3, joints.size(), weights);
	}
}


// generate genotype 
bool cCreature::GenerationGenoType(const uint generation, const uint maxGeneration
	, const bool isRecursive //=true
)
{
	if (generation >= maxGeneration)
	{
		MoveAllFinalNode(generation);
		return true; // complete
	}

	vector<sGenotypeNode*> nodes = m_gnodes;
	for (uint i = 0; i < nodes.size(); ++i)
	{
		sGenotypeNode *gnode = nodes[i];
		if ((gnode->iteration < 0) || (gnode->generation != (generation + 1)))
			continue;
		if ((gnode->maxGeneration > 0) && (gnode->maxGeneration <= gnode->generation))
			continue; // over max generation
		if (IsAlreadyGenerated(gnode))
			continue;
		auto it = m_gmap.find(gnode->iteration);
		if (m_gmap.end() == it) // find parent node?
			continue;

		sGenotypeNode *parent = it->second;

		// loop parent - child iterator node
		set<sGenotypeLink*> &parentLinks = m_linkMap[parent];
		for (auto &glink : parentLinks)
		{
			if (glink->parent != parent)
				continue; // only parent role
			if ((glink->child->iteration < 0)
				|| (glink->child->iteration != parent->id))
				continue; // iteration node?

			sGenotypeNode *itnode = glink->child; // iterator node

			// clone iterator
			CloneGenoTypeNode(eClone::Generation, eHierarchy::Child
				, generation + 1, glink, gnode);

			// clone parent child node except iterator
			for (auto &glink2 : parentLinks)
			{
				if ((glink2->parent == parent)
					&& (glink2->child->iteration < 0))
				{
					CloneGenoTypeNode(eClone::Copy, eHierarchy::Child
						, UINT_MAX, glink2, gnode);
				}
			}
		}
	}

	if (!isRecursive)
		return true;

	// one more generate final node and copy iteration (no recursive)
	GenerationGenoType(generation, generation + 1, false);

	return GenerationGenoType(generation + 1, maxGeneration);
}


// check already generation
bool cCreature::IsAlreadyGenerated(sGenotypeNode *gnode)
{
	const set<sGenotypeLink*> &glinks = m_linkMap[gnode];
	for (auto &glink : glinks)
	{
		if ((glink->parent == gnode) // has iteration node?
			&& (glink->child->iteration == gnode->id))
			return true;
	}
	return false;
}


// clone genotype node src -> dst
sGenotypeNode* cCreature::CloneGenoTypeNode(const eClone cloneType
	, const uint generation, sGenotypeNode *src, sGenotypeNode *dst)
{
	sGenotypeLink *glink = FindParentLink(src);
	if (!glink)
		return nullptr;

	set<sGenotypeNode*> ignoreNodes;
	set<std::pair<sGenotypeNode*, sGenotypeNode*>> ignoreLinks;
	sGenotypeNode *clone = CloneGenoTypeNode(cloneType, eHierarchy::Child
		, generation, glink, glink->parent, glink->child, dst, ignoreNodes
		, ignoreLinks);
	return clone;
}


// clone genotype node src -> dst
sGenotypeNode* cCreature::CloneGenoTypeNode(const eClone cloneType
	, const eHierarchy target, const uint generation
	, sGenotypeLink *srcLink, sGenotypeNode *dst)
{
	set<sGenotypeNode*> ignoreNodes;
	set<std::pair<sGenotypeNode*, sGenotypeNode*>> ignoreLinks;
	sGenotypeNode *clone = CloneGenoTypeNode(cloneType, target, generation
		, srcLink, srcLink->parent, srcLink->child, dst, ignoreNodes, ignoreLinks);
	for (auto &gnode : m_gnodes)
		gnode->clonable = true;
	return clone;
}


// clone child genotype node, from srcChild to dstParent
// target : clone target node type
sGenotypeNode* cCreature::CloneGenoTypeNode(
	const eClone cloneType, const eHierarchy target, const uint generation
	, sGenotypeLink *srcLink, sGenotypeNode *srcParent, sGenotypeNode *srcChild
	, sGenotypeNode *dst
	, INOUT set<sGenotypeNode*> &ignoreNodes
	, INOUT set<std::pair<sGenotypeNode*, sGenotypeNode*>> &ignoreLinks)
{
	// check clonable, generation
	if (((eHierarchy::Parent == target)
			&& ((srcParent->generation > generation) || (!srcParent->clonable)))
		|| ((eHierarchy::Child == target)
			&& ((srcChild->generation > generation) || (!srcChild->clonable))))
		return nullptr;

	// already visit link?
	const bool isVisitLink = (ignoreLinks.end() != ignoreLinks.find(std::make_pair(srcParent, srcChild)))
		|| (ignoreLinks.end() != ignoreLinks.find(std::make_pair(srcChild, srcParent)));
	if (isVisitLink)
		return nullptr;

	const bool isFirstIteration = (target == eHierarchy::Parent)
		&& (srcChild->iteration == srcParent->id);
	if (!isFirstIteration)
		ignoreLinks.insert(std::make_pair(srcParent, srcChild));

	bool isVisitNode = false;
	if (target == eHierarchy::Parent)
	{
		isVisitNode = ignoreNodes.end() != ignoreNodes.find(srcParent);
	}
	else
	{
		isVisitNode = ignoreNodes.end() != ignoreNodes.find(srcChild);
	}

	sGenotypeNode *clone = nullptr;
	sGenotypeNode *dstParent = nullptr;
	sGenotypeNode *dstChild = nullptr;

	if (isVisitNode)
	{
		dstParent = (target == eHierarchy::Child) ? dst : m_clonemap[srcParent->id];
		dstChild = (target == eHierarchy::Child) ? m_clonemap[srcChild->id] : dst;
	}
	else
	{
		ignoreNodes.insert((target == eHierarchy::Parent) ? srcParent : srcChild);
		sGenotypeNode *from = (target == eHierarchy::Child) ? srcParent : srcChild;
		sGenotypeNode *to = (target == eHierarchy::Child) ? srcChild : srcParent;

		// new iteration node
		const Matrix44 tm0 = from->transform.GetMatrix();
		const Matrix44 tm1 = to->transform.GetMatrix();
		const Matrix44 tm2 = dst->transform.GetMatrix();
		Matrix44 localTm = tm1 * tm0.Inverse();
		if (isFirstIteration) // iteration node?
			localTm = tm0 * tm1.Inverse(); // parent -> child transform
		const Matrix44 tm = localTm * tm2;

		// calc next node dimension
		const Vector3 scale(to->transform.scale.x / from->transform.scale.x
			, to->transform.scale.y / from->transform.scale.y
			, to->transform.scale.z / from->transform.scale.z);
		Vector3 newScale = to->transform.scale;
		if (isFirstIteration && (cloneType == eClone::Generation))
		{
			// reverse scale (child -> parent)
			const Vector3 nscale = Vector3(1.f / scale.x, 1.f / scale.y, 1.f / scale.z);
			newScale = dst->transform.scale * nscale;
			if (newScale.Length() < 0.3f)
				return nullptr; // ignore too small dimension object
		}
		else if ((to->iteration >= 0) && (cloneType == eClone::Generation))
		{
			newScale = dst->transform.scale * scale;
			if (newScale.Length() < 0.3f)
				return nullptr; // ignore too small dimension object
		}

		clone = new sGenotypeNode;
		*clone = *to;
		clone->id = common::GenerateId();
		clone->transform.pos = tm.GetPosition();
		clone->transform.scale = newScale;
		clone->transform.rot = tm.GetQuaternion();
		clone->clonable = false;
		if (eClone::Generation == cloneType)
			clone->generation += 1;
		m_gnodes.push_back(clone);
		m_gmap[clone->id] = clone;

		dstParent = (target == eHierarchy::Child) ? dst : clone;
		dstChild = (target == eHierarchy::Child) ? clone : dst;
		if (isFirstIteration) // iteration node?
		{
			clone->iteration = from->iteration;
			dstParent = dst;
			dstChild = clone;
		}
		else if (to->iteration >= 0)
		{
			clone->iteration = dstParent->id;
		}

		m_clonemap[srcParent->id] = dstParent;
		m_clonemap[srcChild->id] = dstChild;
	}

	if (!isVisitNode)
	{
		sGenotypeNode *srcNode = (target == eHierarchy::Child) ? srcChild : srcParent;

		set<sGenotypeLink*> &glinks = m_linkMap[srcNode];
		for (auto &glink : glinks)
		{
			if ((glink->parent == srcNode) // child clone (no another iter node)
				&& ((glink->child->iteration < 0)
					|| (glink->child->iteration == glink->parent->id)))
			{
				CloneGenoTypeNode(cloneType, eHierarchy::Child, generation
					, glink, glink->parent, glink->child, clone, ignoreNodes, ignoreLinks);
			}
			else if ((glink->child == srcNode) 
				&& (glink->child->iteration >= 0)
				&& (glink->child->iteration != glink->parent->id)
				&& (glink->parent->iteration < 0)) // final node clone?
			{
				CloneGenoTypeNode(eClone::Copy, eHierarchy::Parent, UINT_MAX
					, glink, glink->parent, glink->child, clone, ignoreNodes, ignoreLinks);
			}
		}
	}

	sGenotypeLink *newLink = new sGenotypeLink;
	*newLink = *srcLink;
	newLink->parent = dstParent;
	newLink->child = dstChild;
	newLink->nodeLocal0 = dstParent->transform;
	newLink->nodeLocal1 = dstChild->transform;
	m_glinks.push_back(newLink);
	m_linkMap[dstParent].insert(newLink);
	m_linkMap[dstChild].insert(newLink);

	return clone;
}


// find genotype link parent from child node
sGenotypeLink* cCreature::FindParentLink(sGenotypeNode *child)
{
	set<sGenotypeLink*> &glinks = m_linkMap[child];
	for (auto &glink : glinks)
		if (glink->child == child)
			return glink;
	return nullptr;
}


// find link, parent, child contain node
sGenotypeLink* cCreature::FindLink(sGenotypeNode *parent, sGenotypeNode *child)
{
	set<sGenotypeLink*> &glinks = m_linkMap[child];
	for (auto &glink : glinks)
		if ((glink->child == child) && (glink->parent == parent))
			return glink;
	return nullptr;
}


// collect all connection final node
void cCreature::CollectLinkedNode(sGenotypeNode *gnode
	, const set<sGenotypeNode*> &ignores
	, OUT set<sGenotypeNode*> &out)
{
	queue<sGenotypeNode*> q;
	q.push(gnode);

	while (!q.empty())
	{
		sGenotypeNode *p = q.front();
		q.pop();
		if (out.end() != out.find(p))
			continue;
		out.insert(p);

		auto &glinks2 = m_linkMap[p];
		for (auto &glink2 : glinks2)
		{
			if (ignores.end() != ignores.find(glink2->child))
				continue;
			q.push((glink2->parent == p) ? glink2->child : glink2->parent);
		}
	}
}


// remove genotype node and connection final node
bool cCreature::RemoveGenoTypeNode(sGenotypeNode *gnode
	, const bool removeFinalNode //=false
)
{
	if (m_gnodes.end() == std::find(m_gnodes.begin(), m_gnodes.end(), gnode))
		return false; // not exist, ignore

	set<sGenotypeNode*> rms; // remove nodes

	if (removeFinalNode && (gnode->iteration >= 0))
	{
		// collect all final node
		auto &glinks = m_linkMap[gnode];
		for (auto &glink : glinks)
		{
			// final node?
			if ((glink->child == gnode)
				&& (gnode->iteration != glink->parent->id) // no iteration parent node
				&& (glink->parent->iteration < 0)) 
			{
				set<sGenotypeNode*> ignores;
				ignores.insert(gnode);
				CollectLinkedNode(glink->parent, ignores, rms);
			}
		}
	}
	else if (removeFinalNode && (gnode->iteration < 0))
	{
		// collect all final node
		set<sGenotypeNode*> ignores;
		auto &glinks = m_linkMap[gnode];
		for (auto &glink : glinks)
		{
			// find final node link iteration node (no remove node)
			if ((glink->parent == gnode)
				&& (glink->child->iteration >= 0)
				&& (glink->child->iteration != gnode->id))
				ignores.insert(glink->child);
		}
		CollectLinkedNode(gnode, ignores, rms);
	}

	rms.insert(gnode);

	for (auto &p : rms)
	{
		set<sGenotypeLink*> rml;
		for (auto &glink : m_glinks)
		{
			if (glink->parent == p)
				rml.insert(glink);
			if (glink->child == p)
				rml.insert(glink);
		}

		for (auto &glink : rml)
		{
			for (auto &kv : m_linkMap)
				kv.second.erase(glink);
			common::removevector(m_glinks, glink);
			delete glink;
		}

		common::removevector(m_gnodes, p);
		delete p;
	}
	return true;
}


// move final node if generation = 0
// or remove final node
void cCreature::MoveAllFinalNode(const uint generation)
{
	set<sGenotypeLink*> finalLinks; // final node link
	for (auto &glink : m_glinks)
	{
		// find final node
		if ((glink->child->iteration >= 0)
			&& (glink->parent->iteration < 0)
			&& (glink->child->iteration != glink->parent->id))
		{
			finalLinks.insert(glink);
		}
	}

	// move to child iteration parent node
	if (generation == 0)
	{
		for (auto &glink : finalLinks)
		{
			sGenotypeNode *parent = m_gmap[glink->child->iteration];
			if (!parent)
				continue; // error occurred!!

			sGenotypeNode *finalNode = glink->parent;
			CloneGenoTypeNode(eClone::Copy, eHierarchy::Parent, UINT_MAX, glink, parent);
			RemoveGenoTypeNode(finalNode, true);
		}
	}
	else
	{
		for (auto &glink : finalLinks)
			if (glink->child->generation != generation)
				RemoveGenoTypeNode(glink->parent, true);
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
	m_clonemap.clear();
	m_linkMap.clear();
}
