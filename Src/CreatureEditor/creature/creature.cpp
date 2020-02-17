
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

	GenerationGenoType2(generation);

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
		cPNode *pnode0 = nodeMap[p->parent->id];
		cPNode *pnode1 = nodeMap[p->child->id];
		if (!pnode0 || !pnode1)
			continue;

		evc::CreatePhenoTypeJoint(*p, pnode0, pnode1);
	}
}


// Genotype node generation
bool cCreature::GenerationGenoType(const uint generation)
{
	if (generation <= 0)
	{
		// complete generation?
		// move final node to last generation node
		MoveAllFinalNode();
		return true; // complete
	}

	// 1. find iteration node
	// 2. add new genotype node where iteration node
	// 3. add iteration node where new genotype node
	// 4. decreament generation
	set<sGenotypeNode*> iterNodes; // already generate node?
	set<sGenotypeNode*> addNodes;
	for (uint i=0; i < m_gnodes.size(); ++i)
	{
		sGenotypeNode *gnode = m_gnodes[i];
		if (gnode->iteration < 0 || gnode->generation)
			continue;
		if (iterNodes.end() != iterNodes.find(gnode))
			continue; // already generated node, ignore
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
			if (glink->child->iteration < 0 || glink->child->generation)
				continue;

			sGenotypeNode *itnode = glink->child; // iterator node

			// new iteration node
			const Matrix44 tm0 = parent->transform.GetMatrix();
			const Matrix44 tm1 = itnode->transform.GetMatrix();
			const Matrix44 localTm = tm1 * tm0.Inverse();
			const Matrix44 tm = localTm * tm1;
		
			const Vector3 scale(itnode->transform.scale.x / parent->transform.scale.x
				, itnode->transform.scale.y / parent->transform.scale.y
				, itnode->transform.scale.z / parent->transform.scale.z);
			const Vector3 newScale = itnode->transform.scale * scale;

			if (newScale.Length() < 0.1f)
				continue; // ignore too small dimension object

			sGenotypeNode *newIter = new sGenotypeNode;
			*newIter = *itnode;
			newIter->id = common::GenerateId();
			newIter->iteration = itnode->id;
			newIter->transform.pos = tm.GetPosition();
			newIter->transform.scale = newScale;
			newIter->transform.rot = tm.GetQuaternion();
			newIter->generation = false;
			m_gnodes.push_back(newIter);
			m_gmap[newIter->id] = newIter;
			iterNodes.insert(newIter);
			itnode->generation = true;
			addNodes.insert(itnode);

			// if iteration node has link another node or final node?
			// move to new iteration node
			{
				set<sGenotypeNode*> visit; // ignore node
				visit.insert(parent);
				visit.insert(itnode);
				visit.insert(newIter);

				set<sGenotypeLink*> &itLinks = m_linkMap[itnode];
				for (auto &p : itLinks)
				{
					// find final node
					if ((p->parent != parent) && (p->child == itnode))
					{
						// if find another link node, move to newIter position
						MoveFinalNodeWithCalcTm(parent, itnode, newIter, p->parent, visit);
					}
				}
			}

			// new link, iter node - new node
			sGenotypeLink *curLink = glink;
			//for (auto &glink : m_glinks)
			//{
			//	if ((glink->gnode0 == parent)
			//		&& (glink->child == gnode))
			//	{
			//		curLink = glink;
			//		break;
			//	}		
			//}
			//if (!curLink)
			//	break; // error occurred

			sGenotypeLink *newLink = new sGenotypeLink;
			*newLink = *curLink;
			newLink->parent = itnode;
			newLink->child = newIter;
			newLink->nodeLocal0 = itnode->transform;
			newLink->nodeLocal1 = newIter->transform;
			m_glinks.push_back(newLink);

		}




	}

	// create link added nodes
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


// Genotype node generation
bool cCreature::GenerationGenoType2(const uint generation)
{
	if (generation <= 0)
		return true; // complete

	// 1. find iteration node
	// 2. add new genotype node where iteration node
	// 3. add iteration node where new genotype node
	// 4. decreament generation
	set<sGenotypeNode*> iterNodes; // already generate node?
	set<sGenotypeNode*> addNodes;
	vector<sGenotypeNode*> nodes = m_gnodes;
	for (uint i = 0; i < nodes.size(); ++i)
	{
		sGenotypeNode *gnode = nodes[i];
		if (gnode->iteration < 0 || gnode->generation)
			continue;
		if (iterNodes.end() != iterNodes.find(gnode))
			continue; // already generated node, ignore
		auto it = m_gmap.find(gnode->iteration);
		if (m_gmap.end() == it) // find parent node?
			continue;

		sGenotypeNode *parent = it->second;

		sGenotypeLink *glink = FindLink(parent, gnode);
		if (!glink)
			continue;

		sGenotypeNode *clone = CloneGenoTypeNode(eHierachy::Parent, glink, parent);
		if (!clone)
			continue; // error occurred

		// remove iterator node, link
		RemoveGenoTypeNode(gnode);
	}

	return GenerationGenoType2(generation - 1);
}


// copy genotype node and link
bool cCreature::CopyGenoType(sGenotypeNode *src, sGenotypeLink *srcLink, sGenotypeNode *dst
	, set<sGenotypeNode*> &ignore)
{
	sGenotypeNode *clone = nullptr;

	// clone parent genotype node
	if (sGenotypeLink *glink = FindParentLink(src))
	{
		clone = CloneGenoType(glink, dst);
	}
	else
	{

	}

	set<sGenotypeLink*> &glinks = m_linkMap[src];
	for (auto &glink : glinks)
	{
		sGenotypeNode *parent = glink->parent;
		sGenotypeNode *child = glink->child;
		sGenotypeNode *other = (parent == src) ? child : parent;

		if (ignore.end() != ignore.find(other))
			continue;





		//// new iteration node
		//const Matrix44 tm0 = parent->transform.GetMatrix();
		//const Matrix44 tm1 = itnode->transform.GetMatrix();
		//const Matrix44 localTm = tm1 * tm0.Inverse();
		//const Matrix44 tm = localTm * tm1;

		//const Vector3 scale(itnode->transform.scale.x / parent->transform.scale.x
		//	, itnode->transform.scale.y / parent->transform.scale.y
		//	, itnode->transform.scale.z / parent->transform.scale.z);
		//const Vector3 newScale = itnode->transform.scale * scale;

		//if (newScale.Length() < 0.1f)
		//	continue; // ignore too small dimension object

		//sGenotypeNode *newIter = new sGenotypeNode;
		//*newIter = *itnode;
		//newIter->id = common::GenerateId();
		//newIter->iteration = itnode->id;
		//newIter->transform.pos = tm.GetPosition();
		//newIter->transform.scale = newScale;
		//newIter->transform.rot = tm.GetQuaternion();
		//newIter->generation = false;
		//m_gnodes.push_back(newIter);
		//m_gmap[newIter->id] = newIter;
		//iterNodes.insert(newIter);
		//itnode->generation = true;
		//addNodes.insert(itnode);

	}

	return true;
}


// clone genotype node src -> dst
sGenotypeNode* cCreature::CloneGenoTypeNode(sGenotypeNode *src, sGenotypeNode *dst)
{
	sGenotypeLink *glink = FindParentLink(src);
	if (!glink)
		return nullptr;

	set<sGenotypeNode*> ignoreNodes;
	set<std::pair<sGenotypeNode*, sGenotypeNode*>> ignoreLinks;
	sGenotypeNode *clone = CloneGenoTypeNode(eHierachy::Child, glink, glink->parent, glink->child, dst
		, ignoreNodes, ignoreLinks);
	UpdateIterationId();
	return clone;
}


// clone genotype node src -> dst
sGenotypeNode* cCreature::CloneGenoTypeNode(const eHierachy cloneType
	, sGenotypeLink *srcLink, sGenotypeNode *dst)
{
	set<sGenotypeNode*> ignoreNodes;
	set<std::pair<sGenotypeNode*, sGenotypeNode*>> ignoreLinks;
	sGenotypeNode *clone = CloneGenoTypeNode(cloneType, srcLink, srcLink->parent, srcLink->child, dst
		, ignoreNodes, ignoreLinks);
	UpdateIterationId();
	return clone;
}


// update iteration id from generation genotypenode
void cCreature::UpdateIterationId()
{
	// update iteration node id
	for (auto &gnode : m_gnodes)
	{
		if (gnode->iteration_internal >= 0)
		{
			gnode->iteration = m_clonemap[gnode->iteration_internal]->id;
			gnode->iteration_internal = -1;
		}
	}
}


// clone child genotype node, from srcChild to dstParent
// cloneType : clone node type
sGenotypeNode* cCreature::CloneGenoTypeNode(const eHierachy cloneType
	, sGenotypeLink *srcLink
	, sGenotypeNode *srcParent, sGenotypeNode *srcChild, sGenotypeNode *dst
	, INOUT set<sGenotypeNode*> &ignoreNodes
	, INOUT set<std::pair<sGenotypeNode*, sGenotypeNode*>> &ignoreLinks)
{
	const bool isVisitLink = (ignoreLinks.end() != ignoreLinks.find(std::make_pair(srcParent, srcChild)))
		|| (ignoreLinks.end() != ignoreLinks.find(std::make_pair(srcChild, srcParent)));
	if (isVisitLink)
		return nullptr;

	const bool isFirstIteration = (cloneType == eHierachy::Parent) && (srcChild->iteration >= 0);
	if (!isFirstIteration)
		ignoreLinks.insert(std::make_pair(srcParent, srcChild));

	bool isVisitNode = false;
	if (cloneType == eHierachy::Parent)
	{
		isVisitNode = ignoreNodes.end() != ignoreNodes.find(srcParent);
	}
	else
	{
		isVisitNode = ignoreNodes.end() != ignoreNodes.find(srcChild);
	}

	if ((cloneType == eHierachy::Parent)
		//&& (srcChild->iteration == srcParent->id)
		&& srcParent->generation)
		return nullptr; // already generation

	sGenotypeNode *clone = nullptr;
	sGenotypeNode *dstParent = nullptr;
	sGenotypeNode *dstChild = nullptr;

	if (isVisitNode)
	{
		dstParent = (cloneType == eHierachy::Child) ? dst : m_clonemap[srcParent->id];
		dstChild = (cloneType == eHierachy::Child) ? m_clonemap[srcChild->id] : dst;
	}
	else
	{
		ignoreNodes.insert((cloneType == eHierachy::Parent) ? srcParent : srcChild);
		sGenotypeNode *from = (cloneType == eHierachy::Child) ? srcParent : srcChild;
		sGenotypeNode *to = (cloneType == eHierachy::Child) ? srcChild : srcParent;

		// new iteration node
		const Matrix44 tm0 = from->transform.GetMatrix();
		const Matrix44 tm1 = to->transform.GetMatrix();
		const Matrix44 tm2 = dst->transform.GetMatrix();
		Matrix44 localTm = tm1 * tm0.Inverse();
		if (isFirstIteration) // iteration node?
			localTm = tm0 * tm1.Inverse(); // parent -> child transform
		const Matrix44 tm = localTm * tm2;

		//const Vector3 scale(srcChild->transform.scale.x / srcParent->transform.scale.x
		//	, srcChild->transform.scale.y / srcParent->transform.scale.y
		//	, srcChild->transform.scale.z / srcParent->transform.scale.z);
		//const Vector3 newScale = srcChild->transform.scale * scale;
		//if (newScale.Length() < 0.1f)
		//	return nullptr; // ignore too small dimension object

		clone = new sGenotypeNode;
		*clone = *to;
		clone->id = common::GenerateId();
		clone->transform.pos = tm.GetPosition();
		//clone->transform.scale = newScale;
		clone->transform.rot = tm.GetQuaternion();
		clone->iteration_internal = -1;
		m_gnodes.push_back(clone);
		m_gmap[clone->id] = clone;
		//iterNodes.insert(clone);
		//addNodes.insert(srcChild);

		dstParent = (cloneType == eHierachy::Child) ? dst : clone;
		dstChild = (cloneType == eHierachy::Child) ? clone : dst;
		if (isFirstIteration) // iteration node?
		{
			to->generation = true; // generate!!, no more generation
			clone->iteration = -1;
			clone->generation = false;
			dstParent = dst;
			dstChild = clone;
		}
		else if (to->iteration >= 0)
		{
			clone->iteration_internal = to->iteration;
			clone->generation = false;
			//to->generation = true;
		}

		m_clonemap[srcChild->id] = dstChild;
		m_clonemap[srcParent->id] = dstParent;
	}

	if (!isVisitNode)
	{
		sGenotypeNode *srcNode = (cloneType == eHierachy::Child) ? srcChild : srcParent;

		set<sGenotypeLink*> &glinks = m_linkMap[srcNode];
		for (auto &glink : glinks)
		{
			//const eHierachy type = (glink->parent == srcNode) ? eHierachy::Child : eHierachy::Parent;
			//CloneGenoTypeNode(type, glink, glink->parent, glink->child, clone
			//	, ignoreNodes, ignoreLinks);

			if (glink->parent == srcNode)
			{
				CloneGenoTypeNode(eHierachy::Child, glink, glink->parent, glink->child, clone
					, ignoreNodes, ignoreLinks);
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


// clone genotype node
// srcLink parent, child -> dstParent, clone node
sGenotypeNode* cCreature::CloneGenoType(sGenotypeLink *srcLink, sGenotypeNode *dstParent)
{
	sGenotypeNode *parent = srcLink->parent;
	sGenotypeNode *child = srcLink->child;

	// new iteration node
	const Matrix44 tm0 = parent->transform.GetMatrix();
	const Matrix44 tm1 = child->transform.GetMatrix();
	const Matrix44 localTm = tm1 * tm0.Inverse();
	const Matrix44 tm2 = dstParent->transform.GetMatrix();
	const Matrix44 tm = localTm * tm2;

	const Vector3 scale(child->transform.scale.x / parent->transform.scale.x
		, child->transform.scale.y / parent->transform.scale.y
		, child->transform.scale.z / parent->transform.scale.z);
	const Vector3 newScale = child->transform.scale * scale;

	if (newScale.Length() < 0.1f)
		return nullptr; // ignore too small dimension object

	sGenotypeNode *genNode = new sGenotypeNode;
	*genNode = *child;
	genNode->id = common::GenerateId();
	genNode->iteration = child->id;
	genNode->transform.pos = tm.GetPosition();
	genNode->transform.scale = newScale;
	genNode->transform.rot = tm.GetQuaternion();
	genNode->generation = false;
	m_gnodes.push_back(genNode);
	m_gmap[genNode->id] = genNode;
	//iterNodes.insert(genNode);
	child->generation = true;
	//addNodes.insert(child);
	return genNode;
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


bool cCreature::RemoveGenoTypeNode(sGenotypeNode *gnode)
{
	set<sGenotypeLink*> rml;
	for (auto &glink : m_glinks)
	{
		if (glink->parent == gnode)
			rml.insert(glink);
		if (glink->child == gnode)
			rml.insert(glink);
	}

	for (auto &glink : rml)
	{
		for (auto &kv : m_linkMap)
			kv.second.erase(glink);
		common::removevector(m_glinks, glink);
		delete glink;
	}

	common::removevector(m_gnodes, gnode);
	delete gnode;
	return true;
}


// generation genotype node and link from source genotype node
// src: source genotype node
// gen: new genotype node
void cCreature::GenerationGenotypeLink(sGenotypeNode *src, sGenotypeNode *gen)
{
	// new genotype link
	// find iteration link
	set<sGenotypeLink*> &glinks = m_linkMap[src];
	for (auto &glink : glinks)
	{
		if ((glink->parent == src) && (glink->child != gen))
		{
			if ((glink->child->iteration >= 0)
				&& (glink->child->iteration != glink->parent->id))
				continue; // iteratio node? link another parent node, skip generation

			// new iteration node
			const Vector3 p0 = glink->parent->transform.pos;
			const Vector3 p1 = glink->child->transform.pos;
			const Vector3 dir = (p1 - p0).Normal();
			const float len = p1.Distance(p0);
			const Vector3 nextPos = dir * len + gen->transform.pos;

			sGenotypeNode *newNode = new sGenotypeNode;
			*newNode = *glink->child;
			newNode->id = common::GenerateId();
			newNode->transform.pos = nextPos;
			if (glink->child->iteration >= 0)
				newNode->iteration = gen->id;
			m_gnodes.push_back(newNode);
			m_gmap[newNode->id] = newNode;

			sGenotypeLink *newLink = new sGenotypeLink;
			*newLink = *glink;
			newLink->parent = gen;
			newLink->child = newNode;
			newLink->nodeLocal0 = gen->transform;
			newLink->nodeLocal1 = newNode->transform;
			m_glinks.push_back(newLink);
			m_linkMap[gen].insert(newLink);
			m_linkMap[newNode].insert(newLink);

			// generate next link node
			GenerationGenotypeLink(glink->child, newNode);
		}
	}
}


// move final node to new iteration node
// parent: parent of iteration node
// iter: iteration node
// addIter: added iteration node
// finalNode: move node. move to addIter position
void cCreature::MoveFinalNodeWithCalcTm(sGenotypeNode *parent, sGenotypeNode *iter
	, sGenotypeNode *addIter, sGenotypeNode *finalNode
	, INOUT set<sGenotypeNode*> &visit)
{
	const Matrix44 tm0 = iter->transform.GetMatrix();
	const Matrix44 tm1 = addIter->transform.GetMatrix();
	const Matrix44 localTm = tm1 * tm0.Inverse();
	const Matrix44 tm = localTm * tm1;

	const Vector3 scale(addIter->transform.scale.x / iter->transform.scale.x
		, addIter->transform.scale.y / iter->transform.scale.y
		, addIter->transform.scale.z / iter->transform.scale.z);
	const Vector3 newScale = addIter->transform.scale * scale;

	if (newScale.Length() < 0.3f)
		return; // ignore too small dimension object

	// create new link to addIter
	for (auto &glink : m_glinks)
	{
		if ((glink->parent == finalNode)
			&& (glink->child == iter))
		{
			glink->child = addIter;
			break;
		}
	}

	// move linked node
	const Matrix44 movTm = tm1.Inverse() * tm;
	MoveNode(addIter, movTm, visit);
}


// move node
// linkNode: all link has linkNode, move tm
// localTm: move local transform
void cCreature::MoveNode(sGenotypeNode *linkNode, const Matrix44 &tm
	, INOUT set<sGenotypeNode*> &visit)
{
	for (auto &glink : m_glinks)
	{
		if ((glink->parent == linkNode)
			|| (glink->child == linkNode))
		{
			sGenotypeNode *gnode = (glink->parent == linkNode)? glink->child : glink->parent;

			if (visit.end() != visit.find(gnode))
				continue;
			visit.insert(gnode);

			const Matrix44 m = gnode->transform.GetMatrix() * tm;

			// move node
			gnode->transform.pos = m.GetPosition();
			gnode->transform.rot = m.GetQuaternion();
			glink->origPos *= tm;
			if (glink->parent == linkNode)
			{
				glink->nodeLocal0 = linkNode->transform;
				glink->nodeLocal1 = gnode->transform;
			}
			else
			{
				glink->nodeLocal0 = gnode->transform;
				glink->nodeLocal1 = linkNode->transform;
			}

			// move next
			MoveNode(gnode, tm, visit);
		}
	}
}


// move final node, curLinkNode to movLinkNode
// because curLinkNode was hide current state
void cCreature::MoveFinalNode(sGenotypeNode *curLinkNode, sGenotypeNode *movLinkNode
	, sGenotypeNode *finalNode)
{
	for (auto &glink : m_glinks)
	{
		if ((glink->parent == finalNode)
			&& (glink->child == curLinkNode))
		{
			const Matrix44 tm0 = curLinkNode->transform.GetMatrix();
			const Matrix44 tm1 = finalNode->transform.GetMatrix();
			const Matrix44 localTm = tm1 * tm0.Inverse();
			const Matrix44 tm = localTm * movLinkNode->transform.GetMatrix();

			// move final node
			finalNode->transform.pos = tm.GetPosition();
			finalNode->transform.rot = tm.GetQuaternion();

			// move linked node
			set<sGenotypeNode*> visit;
			visit.insert(curLinkNode); // ignore node
			visit.insert(finalNode); // ignore node
			const Matrix44 movTm = tm1.Inverse() * tm;
			MoveNode(finalNode, movTm, visit);

			// create new link to addIter
			glink->child = movLinkNode;
			glink->nodeLocal0 = finalNode->transform;
			glink->nodeLocal1 = movLinkNode->transform;

			break;
		}
	}
}


// move all final node to last visible generation node
void cCreature::MoveAllFinalNode()
{
	for (auto &gnode : m_gnodes)
	{
		// add iteration node and no generation node
		if ((gnode->iteration >= 0)
			&& !gnode->generation)
		{
			sGenotypeNode *parent = m_gmap[gnode->iteration];
			if (!parent)
				continue; // error occurred

			sGenotypeNode *curLinkNode = gnode;
			sGenotypeNode *movLinkNode = parent;

			// find link node and move to movLinkNode
			for (auto &glink : m_glinks)
			{
				if ((glink->child == curLinkNode)
					&& (glink->parent != parent))
				{
					sGenotypeNode *finalNode = glink->parent;
					MoveFinalNode(curLinkNode, movLinkNode, finalNode);
				}
			}
		}
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
