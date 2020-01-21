
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

	// render actor0 pivot
	renderer.m_dbgCube.SetColor(cColor::GREEN);

	Vector3 pivotPos0;
	if (m_pivots[0].len != 0.f)
	{
		const Vector3 localPos = m_pivots[0].dir * node0->m_transform.rot
			* m_pivots[0].len;

		Transform tfm;
		tfm.pos = node0->m_transform.pos + localPos;
		tfm.scale = Vector3::Ones * 0.05f;
		renderer.m_dbgCube.SetCube(tfm);
		renderer.m_dbgCube.Render(renderer);

		pivotPos0 = tfm.pos;
	}
	else
	{
		pivotPos0 = node0->m_transform.pos;
	}

	// render actor1 pivot
	Vector3 pivotPos1;
	if (m_pivots[1].len != 0.f)
	{
		const Vector3 localPos = m_pivots[1].dir * node1->m_transform.rot
			* m_pivots[1].len;

		Transform tfm;
		tfm.pos = node1->m_transform.pos + localPos;
		tfm.scale = Vector3::Ones * 0.05f;
		renderer.m_dbgCube.SetCube(tfm);
		renderer.m_dbgCube.Render(renderer);

		pivotPos1 = tfm.pos;
	}
	else
	{
		pivotPos1 = node1->m_transform.pos;
	}

	// render joint direct line (pivot - pivot)
	//renderer.m_dbgLine.SetColor(cColor::GREEN);
	//renderer.m_dbgLine.SetLine(pivotPos0, pivotPos1, 0.01f);
	//renderer.m_dbgLine.Render(renderer);

	// render joint position
	// tricky code, to find joint pivot position
	Vector3 jointPos;
	if (m_joint->m_joint)
	{
		//const PxTransform localFrame
		//	= m_joint->m_joint->getLocalPose(physx::PxJointActorIndex::eACTOR0);
		//const Transform local = Transform(*(Vector3*)&localFrame.p)
		//	* Transform(*(Quaternion*)&localFrame.q);
		//Transform tm = local * Transform(node0->m_transform.pos, node0->m_transform.rot);
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
		Vector3 dir;
		
		if (m_joint->m_joint)
		{
			//const PxTransform localFrame
			//	= m_joint->m_joint->getLocalPose(physx::PxJointActorIndex::eACTOR0);
			//const Transform local = Transform(*(Vector3*)&localFrame.p)
			//	 * Transform(*(Quaternion*)&localFrame.q);
			//Transform tm = Transform(local.pos)
			//	* Transform(node0->m_transform.pos, node0->m_transform.rot);
			const Transform tm = GetJointTransform();
			dir = m_joint->m_revoluteAxis * tm.rot;
		}
		else
		{
			dir = m_joint->m_revoluteAxis;
		}

		const Vector3 p0 = dir * 2.f + jointPos;
		const Vector3 p1 = dir * -2.f + jointPos;

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
			//const PxTransform localFrame
			//	= m_joint->m_joint->getLocalPose(physx::PxJointActorIndex::eACTOR0);
			//const Transform local = Transform(*(Vector3*)&localFrame.p)
			//	* Transform(*(Quaternion*)&localFrame.q);
			//Transform tm = local * Transform(node0->m_transform.pos, node0->m_transform.rot);
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
	cNode *node0 = m_info0->node;
	const Transform tm = Transform(-m_joint->m_toActor0)
		* Transform(node0->m_transform.pos, node0->m_transform.rot);
	return tm;


	//PxTransform gpose = m_joint->m_actor0->m_actor->getGlobalPose();
	//const PxTransform localFrame = m_joint->m_joint->getLocalPose(
	//	PxJointActorIndex::eACTOR0);
	//PxTransform tt = gpose * localFrame.getInverse();
	//PxTransform a = PxTransform(tt.q) * PxTransform(tt.p); // correct

	//const Transform local = Transform(*(Vector3*)&localFrame.p)
	//	//* Transform(*(Quaternion*)&localFrame.q);
	//	;
	//Transform orgTm;
	//if (phys::cJoint::eType::Revolute == m_joint->m_type)
	//{
	//	// recovery to original direction
	//	Transform rotate;
	//	rotate.rot.SetRotationArc(Vector3(1, 0, 0)
	//		, m_joint->m_revoluteAxis);

	//	orgTm = local * rotate;
	//}
	//else
	//{
	//	orgTm = local;
	//}

	//cNode *node0 = m_info0->node;
	//const Transform tm = orgTm
	//	* Transform(node0->m_transform.pos, node0->m_transform.rot); // ignore scale
	//return tm;
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


// return pivot world transform
// actorIndex: 0=actor0, 1=actor1
Transform cJointRenderer::GetPivotWorldTransform(const int actorIndex)
{
	if ((actorIndex != 0) && (actorIndex != 1))
	{
		assert(0);
		return Transform();
	}

	phys::sActorInfo *infos[2] = { m_info0, m_info1 };
	graphic::cNode *node = infos[actorIndex]->node;

	if (m_pivots[actorIndex].len == 0) // update pivot?
		return node->m_transform;

	const Vector3 localPos = m_pivots[actorIndex].dir * node->m_transform.rot
		* m_pivots[actorIndex].len;

	Transform tfm;
	tfm.pos = node->m_transform.pos + localPos;
	tfm.scale = node->m_transform.scale;
	tfm.rot = node->m_transform.rot;
	return tfm;
}


// apply pivot position
bool cJointRenderer::ApplyPivot()
{
	RETV(!m_joint, false);

	cNode *node0 = m_info0->node;
	cNode *node1 = m_info1->node;
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
	m_info0 = nullptr;
	m_info1 = nullptr;
}
