
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

	//genotype_parser::cParser parser;
	//if (!parser.Parse(fileName.c_str()))
	//	return false;

	//// create genotype node
	//for (auto &kv : parser.m_symTable)
	//{
	//	genotype_parser::sExpr *expr = kv.second;

	//	cGNode *gnode = new cGNode();
	//	gnode->m_name = expr->id;
	//	gnode->m_wname = StrId(expr->id).wstr();
	//	gnode->m_shape = phys::eShapeType::FromString(expr->shape);
	//	gnode->m_density = expr->density;
	//	gnode->m_dimension = expr->dimension;
	//	gnode->m_color = graphic::cColor(expr->material);
	//	m_gnodes.push_back(gnode);
	//}

	//// create genotype link
	//for (auto &kv : parser.m_symTable)
	//{
	//	genotype_parser::sExpr *expr = kv.second;

	//	auto it0 = std::find_if(m_gnodes.begin(), m_gnodes.end()
	//		, [&](const auto &a) { return a->m_name == expr->id; });
	//	if (m_gnodes.end() == it0)
	//		continue; // error occurred

	//	cGNode *gnode0 = *it0;

	//	// create link connection
	//	genotype_parser::sConnectionList *con = expr->connection;
	//	while (con)
	//	{
	//		genotype_parser::sConnection *connect = con->connect;
	//		const phys::eJointType::Enum jointType = phys::eJointType::FromString(connect->type);

	//		auto it1 = std::find_if(m_gnodes.begin(), m_gnodes.end()
	//			, [&](const auto &a) { return a->m_name == connect->exprName; });
	//		if (m_gnodes.end() == it0)
	//			break; // error occurred!!

	//		cGNode *gnode1 = *it1;

	//		// update transform
	//		gnode0->m_transform = connect->conTfm0;
	//		gnode1->m_transform = connect->conTfm1;

	//		cGLink *link = new cGLink();
	//		switch (jointType)
	//		{
	//		case phys::eJointType::Fixed:
	//			link->CreateFixed(gnode0, connect->pivot0, gnode1, connect->pivot1);
	//			break;
	//		case phys::eJointType::Spherical:
	//			link->CreateSpherical(gnode0, connect->pivot0, gnode1, connect->pivot1);
	//			break;
	//		case phys::eJointType::Revolute:
	//			link->CreateRevolute(gnode0, connect->pivot0, gnode1, connect->pivot1
	//				, connect->jointAxis);
	//			break;
	//		case phys::eJointType::Prismatic:
	//			link->CreatePrismatic(gnode0, connect->pivot0, gnode1, connect->pivot1
	//				, connect->jointAxis);
	//			break;
	//		case phys::eJointType::Distance:
	//			link->CreateDistance(gnode0, connect->pivot0, gnode1, connect->pivot1);
	//			break;
	//		case phys::eJointType::D6:
	//			link->CreateD6(gnode0, connect->pivot0, gnode1, connect->pivot1);
	//			break;
	//		}
	//		m_glinks.push_back(link);
	//		con = con->next;
	//	}
	//}
	return true;
}


void cCreature::LoadFromGenoType()
{
	// clear phenotype node
	for (auto &p : m_nodes)
		delete p;
	m_nodes.clear();






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
