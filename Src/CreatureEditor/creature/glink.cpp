
#include "stdafx.h"
#include "glink.h"

using namespace evc;


cGLink::cGLink()
	: graphic::cNode(common::GenerateId(), "glink")
	, m_autoDelete(true)
	, m_revoluteAxis(Vector3(1, 0, 0))
	, m_breakForce(2000.f)
	//, m_breakForce(0.f)
	, m_gnode0(nullptr)
	, m_gnode1(nullptr)
	, m_highlightRevoluteAxis(false)
	, m_type(phys::eJointType::Fixed)
{
}

cGLink::~cGLink()
{
	Clear();
}


// create genotype link from genotype struct
bool cGLink::Create(const sGenotypeLink &glink, cGNode *gnode0, cGNode *gnode1)
{
	const Vector3 pivot0 = glink.pivots[0].dir * glink.nodeLocal0.rot * glink.pivots[0].len
		+ glink.nodeLocal0.pos;
	const Vector3 pivot1 = glink.pivots[1].dir * glink.nodeLocal1.rot * glink.pivots[1].len
		+ glink.nodeLocal1.pos;

	bool result = false;
	switch (glink.type)
	{
	case phys::eJointType::Fixed:
		result = CreateFixed(gnode0, pivot0, gnode1, pivot1);
		break;
	case phys::eJointType::Spherical:
		result = CreateSpherical(gnode0, pivot0, gnode1, pivot1);
		m_limit.cone = glink.limit.cone;
		break;
	case phys::eJointType::Revolute:
		result = CreateRevolute(gnode0, pivot0, gnode1, pivot1, glink.revoluteAxis);
		m_limit.angular = glink.limit.angular;
		m_drive = glink.drive;
		break;
	case phys::eJointType::Prismatic:
		result = CreatePrismatic(gnode0, pivot0, gnode1, pivot1, glink.revoluteAxis);
		m_limit.linear = glink.limit.linear;
		break;
	case phys::eJointType::Distance:
		result = CreateDistance(gnode0, pivot0, gnode1, pivot1);
		m_limit.distance = glink.limit.distance;
		break;
	case phys::eJointType::D6:
		result = CreateD6(gnode0, pivot0, gnode1, pivot1);
		m_limit.d6 = glink.limit.d6;
		break;
	}
	if (!result)
		return false;

	m_drive = glink.drive;
	return true;
}


bool cGLink::CreateFixed(cGNode *gnode0 , const Vector3 &pivot0
	, cGNode *gnode1, const Vector3 &pivot1)
{
	const Transform &worldTfm0 = gnode0->m_transform;
	const Transform &worldTfm1 = gnode1->m_transform;
	const Vector3 linkPos = (pivot0 + pivot1) / 2.f;

	m_type = phys::eJointType::Fixed;
	m_gnode0 = gnode0;
	m_gnode1 = gnode1;
	gnode0->AddLink(this);
	gnode1->AddLink(this);
	SetRevoluteAxis(Vector3(1, 0, 0));
	m_origPos = linkPos;
	m_nodeLocal0 = worldTfm0;
	m_nodeLocal1 = worldTfm1;
	// world -> local space
	m_pivots[0].dir = (pivot0 - worldTfm0.pos).Normal() * worldTfm0.rot.Inverse();
	m_pivots[0].len = (pivot0 - worldTfm0.pos).Length();
	m_pivots[1].dir = (pivot1 - worldTfm1.pos).Normal() * worldTfm1.rot.Inverse();
	m_pivots[1].len = (pivot1 - worldTfm1.pos).Length();
	return true;
}


bool cGLink::CreateSpherical(cGNode *gnode0, const Vector3 &pivot0
	, cGNode *gnode1, const Vector3 &pivot1)
{
	const Transform &worldTfm0 = gnode0->m_transform;
	const Transform &worldTfm1 = gnode1->m_transform;
	const Vector3 linkPos = (pivot0 + pivot1) / 2.f;

	m_type = phys::eJointType::Spherical;
	m_gnode0 = gnode0;
	m_gnode1 = gnode1;
	gnode0->AddLink(this);
	gnode1->AddLink(this);
	SetRevoluteAxis(Vector3(1, 0, 0));
	m_origPos = linkPos;
	m_nodeLocal0 = worldTfm0;
	m_nodeLocal1 = worldTfm1;
	// world -> local space
	m_pivots[0].dir = (pivot0 - worldTfm0.pos).Normal() * worldTfm0.rot.Inverse();
	m_pivots[0].len = (pivot0 - worldTfm0.pos).Length();
	m_pivots[1].dir = (pivot1 - worldTfm1.pos).Normal() * worldTfm1.rot.Inverse();
	m_pivots[1].len = (pivot1 - worldTfm1.pos).Length();
	return true;
}


bool cGLink::CreateRevolute(cGNode *gnode0, const Vector3 &pivot0
	, cGNode *gnode1, const Vector3 &pivot1, const Vector3 &revoluteAxis)
{
	const Transform &worldTfm0 = gnode0->m_transform;
	const Transform &worldTfm1 = gnode1->m_transform;
	const Vector3 linkPos = (pivot0 + pivot1) / 2.f;

	m_type = phys::eJointType::Revolute;
	m_gnode0 = gnode0;
	m_gnode1 = gnode1;
	gnode0->AddLink(this);
	gnode1->AddLink(this);
	SetRevoluteAxis(revoluteAxis);
	m_origPos = linkPos;
	m_nodeLocal0 = worldTfm0;
	m_nodeLocal1 = worldTfm1;
	// world -> local space
	m_pivots[0].dir = (pivot0 - worldTfm0.pos).Normal() * worldTfm0.rot.Inverse();
	m_pivots[0].len = (pivot0 - worldTfm0.pos).Length();
	m_pivots[1].dir = (pivot1 - worldTfm1.pos).Normal() * worldTfm1.rot.Inverse();
	m_pivots[1].len = (pivot1 - worldTfm1.pos).Length();
	return true;
}


bool cGLink::CreatePrismatic(cGNode *gnode0, const Vector3 &pivot0
	, cGNode *gnode1, const Vector3 &pivot1, const Vector3 &revoluteAxis)
{
	const Transform &worldTfm0 = gnode0->m_transform;
	const Transform &worldTfm1 = gnode1->m_transform;
	const Vector3 linkPos = (pivot0 + pivot1) / 2.f;

	m_type = phys::eJointType::Prismatic;
	m_gnode0 = gnode0;
	m_gnode1 = gnode1;
	gnode0->AddLink(this);
	gnode1->AddLink(this);
	SetRevoluteAxis(revoluteAxis);
	m_origPos = linkPos;
	m_nodeLocal0 = worldTfm0;
	m_nodeLocal1 = worldTfm1;
	// world -> local space
	m_pivots[0].dir = (pivot0 - worldTfm0.pos).Normal() * worldTfm0.rot.Inverse();
	m_pivots[0].len = (pivot0 - worldTfm0.pos).Length();
	m_pivots[1].dir = (pivot1 - worldTfm1.pos).Normal() * worldTfm1.rot.Inverse();
	m_pivots[1].len = (pivot1 - worldTfm1.pos).Length();
	return true;
}


bool cGLink::CreateDistance(cGNode *gnode0, const Vector3 &pivot0
	, cGNode *gnode1, const Vector3 &pivot1)
{
	const Transform &worldTfm0 = gnode0->m_transform;
	const Transform &worldTfm1 = gnode1->m_transform;
	const Vector3 linkPos = (pivot0 + pivot1) / 2.f;

	m_type = phys::eJointType::Distance;
	m_gnode0 = gnode0;
	m_gnode1 = gnode1;
	gnode0->AddLink(this);
	gnode1->AddLink(this);
	SetRevoluteAxis(Vector3(1, 0, 0));
	m_origPos = linkPos;
	m_nodeLocal0 = worldTfm0;
	m_nodeLocal1 = worldTfm1;
	// world -> local space
	m_pivots[0].dir = (pivot0 - worldTfm0.pos).Normal() * worldTfm0.rot.Inverse();
	m_pivots[0].len = (pivot0 - worldTfm0.pos).Length();
	m_pivots[1].dir = (pivot1 - worldTfm1.pos).Normal() * worldTfm1.rot.Inverse();
	m_pivots[1].len = (pivot1 - worldTfm1.pos).Length();
	return true;
}


bool cGLink::CreateD6(cGNode *gnode0, const Vector3 &pivot0
	, cGNode *gnode1, const Vector3 &pivot1)
{
	const Transform &worldTfm0 = gnode0->m_transform;
	const Transform &worldTfm1 = gnode1->m_transform;
	const Vector3 linkPos = (pivot0 + pivot1) / 2.f;

	m_type = phys::eJointType::D6;
	m_gnode0 = gnode0;
	m_gnode1 = gnode1;
	gnode0->AddLink(this);
	gnode1->AddLink(this);
	SetRevoluteAxis(Vector3(1, 0, 0));
	m_origPos = linkPos;
	m_nodeLocal0 = worldTfm0;
	m_nodeLocal1 = worldTfm1;
	// world -> local space
	m_pivots[0].dir = (pivot0 - worldTfm0.pos).Normal() * worldTfm0.rot.Inverse();
	m_pivots[0].len = (pivot0 - worldTfm0.pos).Length();
	m_pivots[1].dir = (pivot1 - worldTfm1.pos).Normal() * worldTfm1.rot.Inverse();
	m_pivots[1].len = (pivot1 - worldTfm1.pos).Length();
	return true;
}


bool cGLink::Render(graphic::cRenderer &renderer
	, const XMMATRIX &parentTm //= XMIdentity
	, const int flags //= 1
)
{
	using namespace graphic;

	Vector3 pivotPos0 = GetPivotPos(0);
	Vector3 pivotPos1 = GetPivotPos(1);
	Vector3 linkPos = (pivotPos0 + pivotPos1) / 2.f;

	// render link position
	Transform tfm;
	tfm.pos = linkPos;
	tfm.scale = Vector3::Ones * 0.05f;
	renderer.m_dbgBox.SetColor(cColor::GREEN);
	renderer.m_dbgBox.SetBox(tfm);
	renderer.m_dbgBox.Render(renderer);
	//~render link position

	// render pivot - link pos
	if (1)
	{
		renderer.m_dbgLine.SetColor(cColor(0.f, 1.f, 0.f, 0.5f));
		renderer.m_dbgLine.m_isSolid = true;
		renderer.m_dbgLine.SetLine(pivotPos0, linkPos, 0.01f);
		renderer.m_dbgLine.Render(renderer);
		renderer.m_dbgLine.SetLine(pivotPos1, linkPos, 0.01f);
		renderer.m_dbgLine.Render(renderer);
	}
	//~

	// render revolution axis
	if ((phys::eJointType::Revolute == m_type)
		|| (phys::eJointType::Prismatic == m_type))
	{
		m_transform.pos = linkPos;

		Vector3 p0, p1;
		GetRevoluteAxis(p0, p1);
		renderer.m_dbgLine.SetColor(m_highlightRevoluteAxis ?
			cColor(0.f, 0.f, 1.f, 0.9f) : cColor(0.f, 0.f, 1.f, 0.7f));
		renderer.m_dbgLine.m_isSolid = true;
		renderer.m_dbgLine.SetLine(p0, p1, m_highlightRevoluteAxis? 0.03f : 0.01f);
		renderer.m_dbgLine.Render(renderer);
	}

	// render pivot - revolution axis
	if ((phys::eJointType::Revolute == m_type)
		|| (phys::eJointType::Prismatic == m_type))
	{
		Vector3 p0, p1;
		GetRevoluteAxis(p0, p1);
		const Line line(p0, p1);
		const Vector3 p2 = line.Projection(m_gnode0->m_transform.pos);
		const Vector3 p3 = line.Projection(m_gnode1->m_transform.pos);

		renderer.m_dbgLine.SetColor(cColor::YELLOW);
		renderer.m_dbgLine.SetLine(m_gnode0->m_transform.pos, p2, 0.02f);
		renderer.m_dbgLine.Render(renderer);
		renderer.m_dbgLine.SetLine(m_gnode1->m_transform.pos, p3, 0.02f);
		renderer.m_dbgLine.Render(renderer);
	}

	// render angular limit (origin axis)
	if (phys::eJointType::Revolute == m_type)
	{
		const float axisLen = 0.2f;
		const float axisSize = 0.01f;

		Vector3 r0, r1;
		GetRevoluteAxis(r0, r1);
		const Vector3 dir = (r1 - r0).Normal();
		const Quaternion rot(Vector3(1, 0, 0), dir);

		Vector3 dirOrig = Vector3(0, 1, 0) * rot;
		const Vector3 b0 = Vector3(-0.2f, 0, 0) * rot + linkPos;
		const Vector3 b1 = Vector3(0.2f, 0, 0) * rot + linkPos;
		const Vector3 p0 = dirOrig * axisLen + b0;
		const Vector3 p1 = dirOrig * axisLen + b1;

		renderer.m_dbgLine.SetColor(cColor::BLUE);
		renderer.m_dbgLine.SetLine(b0, p0, axisSize);
		renderer.m_dbgLine.Render(renderer);
		renderer.m_dbgLine.SetLine(b1, p1, axisSize);
		renderer.m_dbgLine.Render(renderer);
	}

	// render cone limit (y,z angle)
	if ((phys::eJointType::Spherical == m_type) && (m_limit.cone.isLimit))
	{
		const float r = 0.2f;
		const float ry = (float)sin(m_limit.cone.yAngle) * r;
		const float hy = (float)cos(m_limit.cone.yAngle) * r;
		const float rz = (float)sin(m_limit.cone.zAngle) * r;
		const float hz = (float)cos(m_limit.cone.zAngle) * r;
		const float h = max(max(hy, hz), 0.1f);
		renderer.m_cone.SetDimension(r, h);
		renderer.m_cone.SetRadiusXZ(ry, rz);

		Quaternion q;
		q.SetRotationArc(Vector3(0, -1, 0), m_revoluteAxis);
		renderer.m_cone.m_transform.pos = m_origPos + m_revoluteAxis * h;
		renderer.m_cone.m_transform.rot = q;
		renderer.m_cone.Render(renderer, XMIdentity, eRenderFlag::WIREFRAME);

		q.SetRotationArc(Vector3(0, -1, 0), -m_revoluteAxis);
		renderer.m_cone.m_transform.pos = m_origPos + -m_revoluteAxis * h;
		renderer.m_cone.m_transform.rot = q;
		renderer.m_cone.Render(renderer, XMIdentity, eRenderFlag::WIREFRAME);
	}

	// render linear limit (lower, upper)
	if ((phys::eJointType::Prismatic == m_type) && (m_limit.linear.isLimit))
	{
		Vector3 r0, r1;
		GetRevoluteAxis(r0, r1);
		const Vector3 dir = (r1 - r0).Normal();
		const Quaternion rot(Vector3(1, 0, 0), dir);
		const float distance = abs(m_revoluteAxis.DotProduct(
			m_gnode1->m_transform.pos - m_gnode0->m_transform.pos));
		const float lower = distance + m_limit.linear.lower;
		const float upper = distance + m_limit.linear.upper;
		const Vector3 p0 = Vector3(1, 0, 0) * lower * rot + m_gnode0->m_transform.pos;
		const Vector3 p1 = Vector3(1, 0, 0) * upper * rot + m_gnode0->m_transform.pos;

		Transform tfm;
		tfm.pos = p0;
		tfm.scale = Vector3::Ones * 0.02f;
		renderer.m_dbgBox.SetColor(cColor::YELLOW);
		renderer.m_dbgBox.SetBox(tfm);
		renderer.m_dbgBox.Render(renderer);

		tfm.pos = p1;
		renderer.m_dbgBox.SetBox(tfm);
		renderer.m_dbgBox.SetColor(cColor::RED);
		renderer.m_dbgBox.Render(renderer);
	}

	__super::Render(renderer, parentTm, flags);
	return true;
}


// revolute axis picking
graphic::cNode* cGLink::Picking(const Ray &ray, const graphic::eNodeType::Enum type
	, const bool isSpherePicking //= true
	, OUT float *dist //= NULL
)
{
	if (phys::eJointType::Revolute != m_type)
		return nullptr;

	Vector3 p0, p1;
	GetRevoluteAxis(p0, p1);
	const Vector3 linkPos = (p0 + p1) / 2.f;

	cBoundingBox bbox;
	bbox.SetLineBoundingBox(p0, p1, 0.05f);
	if (bbox.Pick(ray))
	{
		if (dist)
			*dist = linkPos.Distance(ray.orig);
		return this;
	}

	return nullptr;
}


// nodeIndex: 0=node0, 1=node1
// pos : pivot global pos
void cGLink::SetPivotPos(const int nodeIndex, const Vector3 &pos)
{
	if ((nodeIndex != 0) && (nodeIndex != 1))
	{
		assert(0);
		return;
	}

	// change local coordinate system
	const Transform tfm = (nodeIndex == 0) ? m_nodeLocal0 : m_nodeLocal1;
	const Vector3 dir = (pos - tfm.pos);
	m_pivots[nodeIndex].dir = dir.Normal() * tfm.rot.Inverse();
	m_pivots[nodeIndex].len = dir.Length();
}


// return pivot world transform
// nodeIndex: 0=node0, 1=node1
Transform cGLink::GetPivotWorldTransform(const int nodeIndex)
{
	if ((nodeIndex != 0) && (nodeIndex != 1))
	{
		assert(0);
		return Transform();
	}

	const Transform tfm = (nodeIndex == 0) ? m_nodeLocal0 : m_nodeLocal1;

	Transform ret;
	ret.pos = GetPivotPos(nodeIndex);
	ret.scale = tfm.scale;
	ret.rot = tfm.rot;
	return ret;
}


// return pivot pos
Vector3 cGLink::GetPivotPos(const int nodeIndex)
{
	if ((nodeIndex != 0) && (nodeIndex != 1))
		return Vector3::Zeroes;

	Vector3 pivotPos;
	cGNode *gnode = (nodeIndex == 0) ? m_gnode0 : m_gnode1;

	if (m_pivots[nodeIndex].len != 0.f)
	{
		const Vector3 localPos = m_pivots[nodeIndex].dir * gnode->m_transform.rot
			* m_pivots[nodeIndex].len;
		pivotPos = gnode->m_transform.pos + localPos;
	}
	else
	{
		pivotPos = gnode->m_transform.pos;
	}
	return pivotPos;
}


void cGLink::SetRevoluteAxis(const Vector3 &revoluteAxis)
{
	m_revoluteAxis = revoluteAxis;
	m_rotRevolute.SetRotationArc(Vector3(1, 0, 0), revoluteAxis);
}


// update pivot position
// revoluteAxis : revolute axis direction
// aixsPos : revolute axis position
void cGLink::SetPivotPosByRevoluteAxis(const Vector3 &revoluteAxis, const Vector3 &axisPos)
{
	const Vector3 r0 = revoluteAxis * 2.f + axisPos;
	const Vector3 r1 = revoluteAxis * -2.f + axisPos;
	const Vector3 pos = axisPos;
	const Line line(r0, r1);

	// update actor0 pivot pos
	const Vector3 p0 = line.Projection(m_nodeLocal0.pos);
	const float len0 = pos.Distance(p0);
	const Vector3 toP0 = (p0 - pos).Normal();
	const Vector3 newPivotPos0 = toP0 * len0 + pos;

	// update actor1 pivot pos
	const Vector3 p1 = line.Projection(m_nodeLocal1.pos);
	const float len1 = pos.Distance(p1);
	const Vector3 toP1 = (p1 - pos).Normal();
	const Vector3 newPivotPos1 = toP1 * len1 + pos;

	SetPivotPos(0, newPivotPos0);
	SetPivotPos(1, newPivotPos1);
	m_origPos = pos;
}


// pos : revolute axis world pos
// revolute axis pos store relative center pos
void cGLink::SetPivotPosByRevolutePos(const Vector3 &pos)
{
	Vector3 r0, r1;
	if (!GetRevoluteAxis(r0, r1, pos))
		return;

	const Line line(r0, r1);

	// update actor0 pivot pos
	const Vector3 p0 = line.Projection(m_nodeLocal0.pos);
	const float len0 = pos.Distance(p0);
	const Vector3 toP0 = (p0 - pos).Normal();
	const Vector3 newPivotPos0 = toP0 * len0 + pos;

	// update actor1 pivot pos
	const Vector3 p1 = line.Projection(m_nodeLocal1.pos);
	const float len1 = pos.Distance(p1);
	const Vector3 toP1 = (p1 - pos).Normal();
	const Vector3 newPivotPos1 = toP1 * len1 + pos;

	SetPivotPos(0, newPivotPos0);
	SetPivotPos(1, newPivotPos1);
	m_origPos = pos;
}



// calc revolute axis from pivot0,1
// axisPos : revolute axis position if update
//			if empty, default setting joint position
// return revolute axis position p0, p1
bool cGLink::GetRevoluteAxis(OUT Vector3 &out0, OUT Vector3 &out1
	, const Vector3 &axisPos //= Vector3::Zeroes
)
{
	if (!m_gnode0 || !m_gnode1)
		return false;

	const Vector3 pivotPos0 = GetPivotPos(0);
	const Vector3 pivotPos1 = GetPivotPos(1);
	const Vector3 jointPos = axisPos.IsEmpty() ? ((pivotPos0 + pivotPos1) / 2.f) : axisPos;

	Vector3 dir;
	if (pivotPos1.Distance(pivotPos0) < 0.2f)
	{
		const Quaternion rot = m_gnode0->m_transform.rot * m_nodeLocal0.rot.Inverse();
		dir = m_revoluteAxis * rot;
	}
	else
	{
		dir = (pivotPos1 - pivotPos0).Normal();
	}

	const Vector3 p0 = dir * 2.f + jointPos;
	const Vector3 p1 = dir * -2.f + jointPos;

	const common::Line line(p0, p1);
	const Vector3 p2 = line.Projection(m_gnode0->m_transform.pos);
	const Vector3 p3 = line.Projection(m_gnode1->m_transform.pos);

	if (p2.Distance(p3) < 2.f)
	{
		out0 = dir * 0.5f + jointPos;
		out1 = -dir * 0.5f + jointPos;
	}
	else
	{
		out0 = p2;
		out1 = p3;
	}
	return true;
}


void cGLink::Clear()
{
	m_gnode0 = nullptr;
	m_gnode1 = nullptr;
}
