
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

	m_transform.pos = GetJointTransform().pos;
	//m_axisLen = sync0->node->m_transform.pos.Distance(sync1->node->m_transform.pos);

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
	Vector3 jointPos;
	if (m_joint->m_joint)
	{
		const Transform tm = GetJointTransform();
		jointPos = tm.pos;
	}
	else
	{
		const Vector3 center = (pivotPos0 + pivotPos1) / 2.f;
		jointPos = center;
	}

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
	if (phys::cJoint::eType::Revolute == m_joint->m_type)
	{
		m_transform.pos = GetJointTransform().pos;
		const float axisLen = node0->m_transform.pos.Distance(node1->m_transform.pos);

		Vector3 dir;
		if (m_joint->m_joint)
		{
			const Transform tm = GetJointTransform();
			dir = m_joint->m_revoluteAxis * tm.rot;
		}
		else
		{
			dir = m_joint->m_revoluteAxis;
		}

		const Vector3 p0 = dir * axisLen * 0.5f + jointPos;
		const Vector3 p1 = dir * -axisLen * 0.5f + jointPos;

		renderer.m_dbgLine.SetColor(cColor::BLUE);
		renderer.m_dbgLine.m_isSolid = true;
		renderer.m_dbgLine.SetLine(p0, p1, 0.02f);
		renderer.m_dbgLine.Render(renderer);
	}

	// render pivot - revolution axis
	if (phys::cJoint::eType::Revolute == m_joint->m_type)
	{
		Vector3 dir;

		if (m_joint->m_joint)
		{
			const Transform tm = GetJointTransform();
			dir = m_joint->m_revoluteAxis * tm.rot;
		}
		else
		{
			dir = m_joint->m_revoluteAxis;
		}

		const Vector3 p0 = dir * 2.f + jointPos;
		const Vector3 p1 = dir * -2.f + jointPos;
		const common::Line line(p0, p1);
		const Vector3 p2 = line.Projection(pivotPos0);
		const Vector3 p3 = line.Projection(pivotPos1);

		renderer.m_dbgLine.SetColor(cColor::YELLOW);
		renderer.m_dbgLine.SetLine(pivotPos0, p2, 0.02f);
		renderer.m_dbgLine.Render(renderer);
		renderer.m_dbgLine.SetLine(pivotPos1, p3, 0.02f);
		renderer.m_dbgLine.Render(renderer);
	}


	return true;
}


// return joint transform
// return : localFrame * actor0 transform
Transform cJointRenderer::GetJointTransform()
{
	cNode *node0 = m_sync0->node;
	const Vector3 pos = m_joint->m_origPos - m_joint->m_actorLocal0.pos;
	const Transform tm = Transform(pos)
		* Transform(node0->m_transform.pos, node0->m_transform.rot);
	return tm;
}


Vector3 cJointRenderer::GetPivotPos(const int actorIndex)
{
	if ((actorIndex != 0) && (actorIndex != 1))
		return Vector3::Zeroes;

	Vector3 pivotPos;
	cNode *node = (actorIndex == 0) ? m_sync0->node : m_sync1->node;
	if (m_pivots[actorIndex].len != 0.f)
	{
		const Vector3 localPos = m_pivots[actorIndex].dir * node->m_transform.rot
			* m_pivots[actorIndex].len;
		pivotPos = node->m_transform.pos + localPos;
	}
	else
	{
		pivotPos = node->m_transform.pos;
	}
	return pivotPos;
}


// actorIndex: 0=actor0, 1=actor1
// pos : pivot global pos
void cJointRenderer::SetPivotPos(const int actorIndex, const Vector3 &pos)
{
	if ((actorIndex != 0) && (actorIndex != 1))
	{
		assert(0);
		return;
	}

	// change local coordinate system
	phys::sSyncInfo *sync = (actorIndex == 0)? m_sync0 : m_sync1;
	const Vector3 dir = (pos - sync->node->m_transform.pos);
	m_pivots[actorIndex].dir = dir.Normal() * sync->node->m_transform.rot.Inverse();
	m_pivots[actorIndex].len = dir.Length();
}


// return pivot world transform
// actorIndex: 0=actor0, 1=actor1
Transform cJointRenderer::GetPivotWorldTransform(const int actorIndex)
{
	if ((actorIndex != 0) && (actorIndex != 1))
	{
		assert(0);
		return Transform();
	}

	cNode *node = (actorIndex == 0) ? m_sync0->node : m_sync1->node;

	Transform tfm;
	tfm.pos = GetPivotPos(actorIndex);
	tfm.scale = node->m_transform.scale;
	tfm.rot = node->m_transform.rot;
	return tfm;
}


// pos : revolute axis world pos
// revolute axis pos store relative center pos
void cJointRenderer::SetRevoluteAxisPos(const Vector3 &pos)
{
	//m_revoluteRelativePos
	RET(!m_joint);

	cNode *node0 = m_sync0->node;
	cNode *node1 = m_sync1->node;
	if (!node0 || !node1)
		return;

	//Vector3 center = (node0->m_transform.pos + node1->m_transform.pos) / 2.f;
	//Vector3 p = pos - center;
	//const Vector3 pivot0 = GetPivotPos(0);
	//const Vector3 pivot1 = GetPivotPos(1);
	//Vector3 p0 = m_joint->m_origPos + m_joint->m_toActor0;
	//Vector3 p1 = m_joint->m_origPos + m_joint->m_toActor1;

	Vector3 p0 = m_joint->m_actorLocal0.pos;
	Vector3 p1 = m_joint->m_actorLocal1.pos;
	const Vector3 origDir = (p1 - p0).Normal();
	const Vector3 revoluteAxisDir = origDir * m_joint->m_rotRevolute;

	// update actor0 pivot pos
	const Vector3 pivot0 = GetPivotPos(0);
	const float len0 = pivot0.Distance(m_joint->m_origPos);
	const Vector3 newPivotPos0 = revoluteAxisDir * len0 * 0.5f + pos;

	// update actor1 pivot pos
	const Vector3 pivot1 = GetPivotPos(1);
	const float len1 = pivot1.Distance(m_joint->m_origPos);
	const Vector3 newPivotPos1 = revoluteAxisDir * len1 * 0.5f + pos;

	SetPivotPos(0, newPivotPos0);
	SetPivotPos(1, newPivotPos1);
	m_joint->m_origPos = pos;
}


// override picking function
// picking revolute joint
cNode* cJointRenderer::Picking(const Ray &ray, const eNodeType::Enum type
	, const bool isSpherePicking //= true
	, OUT float *dist //= NULL
)
{
	if (phys::cJoint::eType::Revolute != m_joint->m_type)
		return nullptr;
	RETV(!m_joint, nullptr);

	const Transform tm = GetJointTransform();
	const Vector3 dir = m_joint->m_revoluteAxis * tm.rot;
	const Vector3 p0 = dir * 2.f + tm.pos;
	const Vector3 p1 = dir * -2.f + tm.pos;

	cBoundingBox bbox;
	bbox.SetLineBoundingBox(p0, p1, 0.02f);
	if (bbox.Pick(ray))
	{
		if (dist)
			*dist = tm.pos.Distance(ray.orig);
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

	Transform tfm0;
	if (m_pivots[0].len != 0) // update pivot?
	{
		const Vector3 localPos = m_pivots[0].dir * node0->m_transform.rot
			* m_pivots[0].len;

		tfm0.pos = node0->m_transform.pos + localPos;
		tfm0.scale = node0->m_transform.scale;
		tfm0.rot = node0->m_transform.rot;
	}
	else
	{
		tfm0 = node0->m_transform;
	}

	Transform tfm1;
	if (m_pivots[1].len != 0) // update pivot?
	{
		const Vector3 localPos = m_pivots[1].dir * node1->m_transform.rot
			* m_pivots[1].len;

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
