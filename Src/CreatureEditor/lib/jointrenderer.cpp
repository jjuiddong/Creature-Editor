
#include "stdafx.h"
#include "jointrenderer.h"

using namespace graphic;
using namespace phys;
using namespace physx;


cJointRenderer::cJointRenderer()
{
}

cJointRenderer::~cJointRenderer()
{
	Clear();
}


bool cJointRenderer::Create(phys::cJoint *joint) 
{
	m_joint = joint;

	phys::cRigidActor *actor0 = joint->m_actor0;
	phys::cRigidActor *actor1 = joint->m_actor1;
	if (!actor0 || !actor1)
		return false;

	phys::sSyncInfo *sync0 = g_global->m_physSync->FindSyncInfo(actor0);
	phys::sSyncInfo *sync1 = g_global->m_physSync->FindSyncInfo(actor1);
	if (!sync0 || !sync1)
		return false;

	m_sync0 = sync0;
	m_sync1 = sync1;

	m_transform.pos = (GetPivotPos(0) + GetPivotPos(1)) / 2.f;

	return true;
}


bool cJointRenderer::Update(graphic::cRenderer &renderer, const float deltaSeconds)
{
	return true;
}


bool cJointRenderer::Render(graphic::cRenderer &renderer
	, const XMMATRIX &parentTm //= XMIdentity
	, const int flags //= 1
)
{
	RETV(!m_joint, false);
	if (m_techniqueName == "BuildShadowMap")
		return true;

	if (m_joint->m_joint) // broken joint?
	{
		const bool broken = (m_joint->m_joint->getConstraintFlags()
			& physx::PxConstraintFlag::eBROKEN);
		if (broken)
			return true;
	}

	renderer.m_dbgCube.SetTechnique(m_techniqueName.c_str());
	renderer.m_dbgBox.SetTechnique(m_techniqueName.c_str());
	renderer.m_dbgLine.SetTechnique(m_techniqueName.c_str());

	cNode *node0 = m_sync0->node;
	cNode *node1 = m_sync1->node;
	if (!node0 || !node1)
		return false;

	renderer.m_dbgCube.SetColor(cColor::GREEN);

	// render actor0 pivot
	Vector3 pivotPos0 = GetPivotPos(0);
	{
		Transform tfm;
		tfm.pos = pivotPos0;
		tfm.scale = Vector3::Ones * 0.05f;
		renderer.m_dbgCube.SetCube(tfm);
		renderer.m_dbgCube.Render(renderer);
	}

	// render actor1 pivot
	Vector3 pivotPos1 = GetPivotPos(1);
	{
		Transform tfm;
		tfm.pos = pivotPos1;
		tfm.scale = Vector3::Ones * 0.05f;
		renderer.m_dbgCube.SetCube(tfm);
		renderer.m_dbgCube.Render(renderer);
	}

	// render joint position
	Vector3 jointPos = (pivotPos0 + pivotPos1) / 2.f;
	Transform tfm;
	tfm.pos = jointPos;
	tfm.scale = Vector3::Ones * 0.05f;
	renderer.m_dbgBox.SetColor(cColor::GREEN);
	renderer.m_dbgBox.SetBox(tfm);
	renderer.m_dbgBox.Render(renderer);
	//~render joint position

	// render pivot - joint pos
	if (1)
	{
		renderer.m_dbgLine.SetColor(cColor::GREEN);
		renderer.m_dbgLine.m_isSolid = true;
		renderer.m_dbgLine.SetLine(pivotPos0, jointPos, 0.01f);
		renderer.m_dbgLine.Render(renderer);
		renderer.m_dbgLine.SetLine(pivotPos1, jointPos, 0.01f);
		renderer.m_dbgLine.Render(renderer);
	}
	//~

	// render revolution axis
	if (phys::eJointType::Revolute == m_joint->m_type)
	{
		m_transform.pos = jointPos;
		
		Vector3 p0, p1;
		GetRevoluteAxis(p0, p1);
		renderer.m_dbgLine.SetColor(cColor::BLUE);
		renderer.m_dbgLine.m_isSolid = true;
		renderer.m_dbgLine.SetLine(p0, p1, 0.02f);
		renderer.m_dbgLine.Render(renderer);
	}

	// render pivot - revolution axis
	if (phys::eJointType::Revolute == m_joint->m_type)
	{
		Vector3 p0, p1;
		GetRevoluteAxis(p0, p1);
		const Line line(p0, p1);
		const Vector3 p2 = line.Projection(node0->m_transform.pos);
		const Vector3 p3 = line.Projection(node1->m_transform.pos);

		renderer.m_dbgLine.SetColor(cColor::YELLOW);
		renderer.m_dbgLine.SetLine(node0->m_transform.pos, p2, 0.02f);
		renderer.m_dbgLine.Render(renderer);
		renderer.m_dbgLine.SetLine(node1->m_transform.pos, p3, 0.02f);
		renderer.m_dbgLine.Render(renderer);
	}

	// recovery
	renderer.m_dbgCube.SetTechnique("Unlit");
	renderer.m_dbgBox.SetTechnique("Unlit");
	renderer.m_dbgLine.SetTechnique("Unlit");

	return true;
}


Vector3 cJointRenderer::GetPivotPos(const int actorIndex)
{
	if ((actorIndex != 0) && (actorIndex != 1))
		return Vector3::Zeroes;

	Vector3 pivotPos;
	phys::cJoint::sPivot *pivot = m_joint->m_pivots;
	cNode *node = (actorIndex == 0) ? m_sync0->node : m_sync1->node;
	if (pivot[actorIndex].len != 0.f)
	{
		const Vector3 localPos = pivot[actorIndex].dir * node->m_transform.rot
			* pivot[actorIndex].len;
		pivotPos = node->m_transform.pos + localPos;
	}
	else
	{
		pivotPos = node->m_transform.pos;
	}
	return pivotPos;
	//return m_joint->GetPivotPos(actorIndex);
}


// actorIndex: 0=actor0, 1=actor1
// pos : pivot global pos
void cJointRenderer::SetPivotPos(const int actorIndex, const Vector3 &pos)
{
	//if ((actorIndex != 0) && (actorIndex != 1))
	//{
	//	assert(0);
	//	return;
	//}

	//// change local coordinate system
	//phys::sSyncInfo *sync = (actorIndex == 0)? m_sync0 : m_sync1;
	//const Vector3 dir = (pos - sync->node->m_transform.pos);
	//m_pivots[actorIndex].dir = dir.Normal() * sync->node->m_transform.rot.Inverse();
	//m_pivots[actorIndex].len = dir.Length();
	m_joint->SetPivotPos(actorIndex, pos);
}


// return pivot world transform
// actorIndex: 0=actor0, 1=actor1
Transform cJointRenderer::GetPivotWorldTransform(const int actorIndex)
{
	//if ((actorIndex != 0) && (actorIndex != 1))
	//{
	//	assert(0);
	//	return Transform();
	//}

	//cNode *node = (actorIndex == 0) ? m_sync0->node : m_sync1->node;

	//Transform tfm;
	//tfm.pos = GetPivotPos(actorIndex);
	//tfm.scale = node->m_transform.scale;
	//tfm.rot = node->m_transform.rot;
	//return tfm;
	return m_joint->GetPivotWorldTransform(actorIndex);
}


// revoluteAxis : revolute axis direction
// aixsPos : revolute axis position
void cJointRenderer::SetRevoluteAxis(const Vector3 &revoluteAxis, const Vector3 &axisPos)
{
	const Vector3 r0 = revoluteAxis * 2.f + axisPos;
	const Vector3 r1 = revoluteAxis * -2.f + axisPos;
	const Vector3 pos = axisPos;
	const Line line(r0, r1);

	// update actor0 pivot pos
	const Vector3 p0 = line.Projection(m_joint->m_actorLocal0.pos);
	const float len0 = pos.Distance(p0);
	const Vector3 toP0 = (p0 - pos).Normal();
	const Vector3 newPivotPos0 = toP0 * len0 + pos;

	// update actor1 pivot pos
	const Vector3 p1 = line.Projection(m_joint->m_actorLocal1.pos);
	const float len1 = pos.Distance(p1);
	const Vector3 toP1 = (p1 - pos).Normal();
	const Vector3 newPivotPos1 = toP1 * len1 + pos;

	SetPivotPos(0, newPivotPos0);
	SetPivotPos(1, newPivotPos1);
	m_joint->m_origPos = pos;
}


// pos : revolute axis world pos
// revolute axis pos store relative center pos
void cJointRenderer::SetRevoluteAxisPos(const Vector3 &pos)
{
	Vector3 r0, r1;
	if (!GetRevoluteAxis(r0, r1, pos))
		return;

	const Line line(r0, r1);

	// update actor0 pivot pos
	const Vector3 p0 = line.Projection(m_joint->m_actorLocal0.pos);
	const float len0 = pos.Distance(p0);
	const Vector3 toP0 = (p0 - pos).Normal();
	const Vector3 newPivotPos0 = toP0 * len0 + pos;

	// update actor1 pivot pos
	const Vector3 p1 = line.Projection(m_joint->m_actorLocal1.pos);
	const float len1 = pos.Distance(p1);
	const Vector3 toP1 = (p1 - pos).Normal();
	const Vector3 newPivotPos1 = toP1 * len1 + pos;

	SetPivotPos(0, newPivotPos0);
	SetPivotPos(1, newPivotPos1);
	m_joint->m_origPos = pos;
}


// calc revolute axis from pivot0,1
// axisPos : revolute axis position if update
//			if empty, default setting joint position
// return revolute axis position p0, p1
bool cJointRenderer::GetRevoluteAxis(OUT Vector3 &out0, OUT Vector3 &out1
	, const Vector3 &axisPos //= Vector3::Zeroes
)
{
	RETV(!m_joint, false);
	cNode *node0 = m_sync0->node;
	cNode *node1 = m_sync1->node;
	if (!node0 || !node1)
		return false;

	const Vector3 pivotPos0 = GetPivotPos(0);
	const Vector3 pivotPos1 = GetPivotPos(1);
	const Vector3 jointPos = axisPos.IsEmpty() ? ((pivotPos0 + pivotPos1) / 2.f) : axisPos;

	Vector3 dir;
	if (pivotPos1.Distance(pivotPos0) < 0.2f)
	{
		const Quaternion rot = node0->m_transform.rot * m_joint->m_actorLocal0.rot.Inverse();
		dir = m_joint->m_revoluteAxis * rot;
	}
	else
	{
		dir = (pivotPos1 - pivotPos0).Normal();
	}

	const Vector3 p0 = dir * 2.f + jointPos;
	const Vector3 p1 = dir * -2.f + jointPos;

	const common::Line line(p0, p1);
	const Vector3 p2 = line.Projection(node0->m_transform.pos);
	const Vector3 p3 = line.Projection(node1->m_transform.pos);

	if (p2.Distance(p3) < 2.f)
	{
		out0 = dir + jointPos;
		out1 = -dir + jointPos;
	}
	else
	{
		out0 = p2;
		out1 = p3;
	}
	return true;
}


// override picking function
// picking revolute joint
cNode* cJointRenderer::Picking(const Ray &ray, const eNodeType::Enum type
	, const bool isSpherePicking //= true
	, OUT float *dist //= NULL
)
{
	if (phys::eJointType::Revolute != m_joint->m_type)
		return nullptr;
	RETV(!m_joint, nullptr);

	Vector3 p0, p1;
	GetRevoluteAxis(p0, p1);
	const Vector3 jointPos = (p0 + p1) / 2.f;

	cBoundingBox bbox;
	bbox.SetLineBoundingBox(p0, p1, 0.02f);
	if (bbox.Pick(ray))
	{
		if (dist)
			*dist = jointPos.Distance(ray.orig);
		return this;
	}

	return nullptr;
}


// apply pivot position
bool cJointRenderer::ApplyPivot()
{
	RETV(!m_joint, false);

	cNode *node0 = m_sync0->node;
	cNode *node1 = m_sync1->node;
	if (!node0 || !node1)
		return false;

	cJoint::sPivot *pivots = m_joint->m_pivots;

	Transform tfm0;
	if (pivots[0].len != 0) // update pivot?
	{
		const Vector3 localPos = pivots[0].dir * node0->m_transform.rot
			* pivots[0].len;

		tfm0.pos = node0->m_transform.pos + localPos;
		tfm0.scale = node0->m_transform.scale;
		tfm0.rot = node0->m_transform.rot;
	}
	else
	{
		tfm0 = node0->m_transform;
	}

	Transform tfm1;
	if (pivots[1].len != 0) // update pivot?
	{
		const Vector3 localPos = pivots[1].dir * node1->m_transform.rot
			* pivots[1].len;

		tfm1.pos = node1->m_transform.pos + localPos;
		tfm1.scale = node1->m_transform.scale;
		tfm1.rot = node1->m_transform.rot;
	}
	else
	{
		tfm1 = node1->m_transform;
	}

	return m_joint->ModifyPivot(g_global->m_physics
		, node0->m_transform, tfm0.pos
		, node1->m_transform, tfm1.pos
		, m_joint->m_revoluteAxis);
}


void cJointRenderer::Clear()
{
	m_joint = nullptr;
	m_sync0 = nullptr;
	m_sync1 = nullptr;
}
