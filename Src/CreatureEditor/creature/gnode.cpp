
#include "stdafx.h"
#include "gnode.h"

using namespace evc;
using namespace graphic;


cGNode::cGNode()
	: graphic::cNode(common::GenerateId(), "gnode")
	, m_wname(L"gnode")
	, m_cloneId(-1)
	, m_density(1.f)
{
}

cGNode::~cGNode()
{
	Clear();
}


bool cGNode::CreateBox(graphic::cRenderer &renderer, const Transform &tfm)
{
	graphic::cCube *cube = new graphic::cCube();
	cube->Create(renderer);
	cube->SetCube(Transform());
	cube->SetRenderFlag(eRenderFlag::OUTLINE, true);
	AddChild(cube);

	m_shape = phys::eShapeType::Box;
	m_transform = tfm;
	m_name = "Box";
	m_wname = m_name.wstr();
	return true;
}


bool cGNode::CreateSphere(graphic::cRenderer &renderer, const Transform &tfm
	, const float radius)
{
	graphic::cSphere *sphere = new graphic::cSphere();
	sphere->Create(renderer, 1.f, 10, 10);
	sphere->SetRenderFlag(eRenderFlag::OUTLINE, true);
	AddChild(sphere);

	m_shape = phys::eShapeType::Sphere;
	m_transform = tfm;
	m_transform.scale = Vector3::Ones * radius;
	m_name = "Sphere";
	m_wname = m_name.wstr();
	return true;
}


bool cGNode::CreateCapsule(graphic::cRenderer &renderer, const Transform &tfm
	, const float radius, const float halfHeight)
{
	graphic::cCapsule *capsule = new graphic::cCapsule();
	capsule->Create(renderer, radius, halfHeight, 16, 8);
	capsule->SetRenderFlag(eRenderFlag::OUTLINE, true);
	AddChild(capsule);

	m_shape = phys::eShapeType::Capsule;
	m_transform.pos = tfm.pos;
	m_name = "Capsule";
	m_wname = m_name.wstr();
	return true;
}


bool cGNode::CreateCylinder(graphic::cRenderer &renderer, const Transform &tfm
	, const float radius, const float height)
{
	graphic::cCylinder *cylinder = new graphic::cCylinder();
	cylinder->Create(renderer, 1.f, 2.f, 8);
	cylinder->SetRenderFlag(eRenderFlag::OUTLINE, true);
	AddChild(cylinder);

	m_shape = phys::eShapeType::Cylinder;
	m_transform = tfm;
	m_transform.scale = Vector3(height/2.f, radius, radius);
	m_name = "Cylinder";
	m_wname = m_name.wstr();
	return true;
}


bool cGNode::Render(graphic::cRenderer &renderer
	, const XMMATRIX &parentTm //= XMIdentity
	, const int flags //= 1
)
{
	__super::Render(renderer, parentTm, flags);

	if (!(flags & graphic::eRenderFlag::OUTLINE))
	{
		Transform tfm;
		tfm.pos = m_transform.pos + Vector3(0, 0.2f,0);
		tfm.scale = Vector3::Ones * 0.2f;
		renderer.m_textMgr.AddTextRender(renderer
			, m_id, m_wname.c_str(), cColor::WHITE, cColor::BLACK
			, BILLBOARD_TYPE::ALL_AXIS, tfm, true);

		// y-axis line
		renderer.m_dbgLine.m_isSolid = true;
		renderer.m_dbgLine.SetColor(cColor::BLACK);
		renderer.m_dbgLine.SetLine(m_transform.pos
			, m_transform.pos + Vector3(0, -m_transform.pos.y, 0), 0.01f);
		renderer.m_dbgLine.Render(renderer);
	}

	return true;
}


// add genotype link referenece
bool cGNode::AddLink(cGLink *glink)
{
	if (m_links.end() == std::find(m_links.begin(), m_links.end(), glink))
	{
		m_links.push_back(glink);
		return true;
	}
	return false;
}


// remove genotype link reference from link ptr
bool cGNode::RemoveLink(cGLink *glink)
{
	if (m_links.end() != std::find(m_links.begin(), m_links.end(), glink))
	{
		common::removevector(m_links, glink);
		return true;
	}
	return false;
}


// remove genotype link from link id
bool cGNode::RemoveLink(const int linkId)
{
	auto it = std::find_if(m_links.begin(), m_links.end()
		, [&](const auto &a) { return a->m_id == linkId;});
	if (m_links.end() != it)
	{
		m_links.erase(it);
		return true;
	}

	return true;
}


void cGNode::SetSphereRadius(const float radius)
{
	if (m_shape != phys::eShapeType::Sphere)
		return;
	m_transform.scale = Vector3::Ones * radius;
}


float cGNode::GetSphereRadius()
{
	if (m_shape != phys::eShapeType::Sphere)
		return 0.f;
	return m_transform.scale.x;
}


void cGNode::SetCapsuleDimension(const float radius, const float halfHeight)
{
	if (m_shape != phys::eShapeType::Capsule)
		return;
	m_transform.scale = Vector3(halfHeight + radius, radius, radius);
}


// return vector2(radius, halfHeight)
Vector2 cGNode::GetCapsuleDimension()
{
	if (m_shape != phys::eShapeType::Capsule)
		return Vector2(0,0);
	return Vector2(m_transform.scale.y, m_transform.scale.x - m_transform.scale.y);
}


void cGNode::SetCylinderDimension(const float radius, const float height)
{
	if (m_shape != phys::eShapeType::Cylinder)
		return;
	m_transform.scale = Vector3(height / 2.f, radius, radius);
}


// return Vector2(radius, height)
Vector2 cGNode::GetCylinderDimension()
{
	if (m_shape != phys::eShapeType::Cylinder)
		return Vector2(0,0);
	return Vector2(m_transform.scale.y, m_transform.scale.x * 2.f);
}


cGNode* cGNode::Clone(graphic::cRenderer &renderer)
{
	cGNode *gnode = new cGNode();
	
	switch (m_shape)
	{
	case phys::eShapeType::Box:
		gnode->CreateBox(renderer, m_transform);
		break;
	case phys::eShapeType::Sphere:
		gnode->CreateSphere(renderer, m_transform, m_transform.scale.x);
		break;
	case phys::eShapeType::Capsule:
		gnode->CreateCapsule(renderer, m_transform
			, m_transform.scale.y, m_transform.scale.x);
		break;
	case phys::eShapeType::Cylinder:
		gnode->CreateCylinder(renderer, m_transform
			, m_transform.scale.y, m_transform.scale.x);
		break;
	default: assert(0); break;
	}

	gnode->m_name = m_name + " iter";
	gnode->m_wname = m_wname + L" iter";
	const Vector3 scale = gnode->m_transform.scale * 2.f;
	gnode->m_transform.pos += Vector3(scale.x, 0, scale.z);
	gnode->m_cloneId = m_id;
	return gnode;
}


void cGNode::Clear()
{
	__super::Clear();
}
