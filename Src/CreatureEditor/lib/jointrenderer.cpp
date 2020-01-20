
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

	phys::sActorInfo *info0 = g_global->m_physSync->FindActorInfo(actor0);
	phys::sActorInfo *info1 = g_global->m_physSync->FindActorInfo(actor1);
	if (!info0 || !info1)
		return false;

	m_info0 = info0;
	m_info1 = info1;

	return true;
}


bool cJointRenderer::Update(const float deltaSeconds)
{

	return true;
}


bool cJointRenderer::Render(graphic::cRenderer &renderer
	, const XMMATRIX &tm //= graphic::XMIdentity
)
{
	RETV(!m_joint, false);

	cNode *node0 = m_info0->node;
	cNode *node1 = m_info1->node;
	if (!node0 || !node1)
		return false;

	const Vector3 &p0 = node0->m_transform.pos;
	const Vector3 &p1 = node1->m_transform.pos;
	renderer.m_dbgLine.SetLine(p0, p1, 0.01f);
	renderer.m_dbgLine.Render(renderer);

	// treacky code, to find joint pivot position
	PxTransform localFrame;
	localFrame = m_joint->m_joint->getLocalPose(physx::PxJointActorIndex::eACTOR0);
	const float len = localFrame.p.magnitude();
	const Vector3 pos = (p1 - p0).Normal() * len + p0;

	// joint pos
	Transform tfm;
	tfm.pos = pos;
	tfm.scale = Vector3::Ones * 0.05f;
	renderer.m_dbgBox.SetBox(tfm);
	renderer.m_dbgBox.Render(renderer);

	// actor0 pivot
	if (m_pivots[0].len != 0.f)
	{
		const Vector3 localPos = m_pivots[0].dir * node0->m_transform.rot
			* m_pivots[0].len;

		Transform tfm;
		tfm.pos = node0->m_transform.pos + localPos;
		tfm.scale = Vector3::Ones * 0.05f;
		renderer.m_dbgBox.SetBox(tfm);
		renderer.m_dbgBox.SetColor(cColor::RED);
		renderer.m_dbgBox.Render(renderer);
	}

	// actor1 pivot
	if (m_pivots[1].len != 0.f)
	{
		const Vector3 localPos = m_pivots[1].dir * node1->m_transform.rot
			* m_pivots[1].len;

		Transform tfm;
		tfm.pos = node1->m_transform.pos + localPos;
		tfm.scale = Vector3::Ones * 0.05f;
		renderer.m_dbgBox.SetBox(tfm);
		renderer.m_dbgBox.SetColor(cColor::RED);
		renderer.m_dbgBox.Render(renderer);
	}

	return true;
}


// actorIndex: 0=actor0, 1=actor1
void cJointRenderer::SetPivotPos(const int actorIndex, const Vector3 &pos)
{
	if ((actorIndex != 0) && (actorIndex != 1))
	{
		assert(0);
		return;
	}

	phys::sActorInfo *info = (actorIndex == 0)? m_info0 : m_info1;
	const Vector3 dir = (pos - info->node->m_transform.pos);
	m_pivots[actorIndex].dir = dir.Normal() * info->node->m_transform.rot.Inverse();
	m_pivots[actorIndex].len = dir.Length();
}


void cJointRenderer::Clear()
{
	m_joint = nullptr;
	m_info0 = nullptr;
	m_info1 = nullptr;
}
