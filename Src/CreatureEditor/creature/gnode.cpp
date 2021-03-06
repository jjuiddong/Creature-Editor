
#include "stdafx.h"
#include "gnode.h"

using namespace evc;
using namespace graphic;


cGNode::cGNode()
	: graphic::cNode(common::GenerateId(), "gnode")
	, m_wname(L"gnode")
	, m_wnameId(L"-1")
	, m_txtColor(cColor::WHITE)
{
	m_prop.id = m_id;
	m_prop.color = cColor::WHITE;
	m_prop.density = 1.f;
	m_prop.iteration = -1;
	m_prop.maxGeneration = 0;
	m_prop.kinematic = false;
	m_prop.angularDamping = 0.5f;
	m_prop.linearDamping = 0.5f;
}

cGNode::~cGNode()
{
	Clear();
}


// create from sGenotypeNode struct
bool cGNode::Create(graphic::cRenderer &renderer, const sGenotypeNode &gnode)
{
	bool result = false;
	switch (gnode.shape)
	{
	case phys::eShapeType::Plane: break;
	case phys::eShapeType::Box: 
		result = CreateBox(renderer, gnode.transform);
		break;
	case phys::eShapeType::Sphere:
		result = CreateSphere(renderer, gnode.transform, gnode.transform.scale.x);
		break;
	case phys::eShapeType::Capsule:
	{
		const float radius = gnode.transform.scale.y;
		const float halfHeight = gnode.transform.scale.x - gnode.transform.scale.y;
		result = CreateCapsule(renderer, gnode.transform, radius, halfHeight);
	}
	break;
	case phys::eShapeType::Cylinder:
	{
		const float radius = gnode.transform.scale.y;
		const float height = gnode.transform.scale.x;
		result = CreateCylinder(renderer, gnode.transform, radius, height);
	}
	break;
	default: assert(0); break;
	}

	if (!result)
		return false;

	m_name = gnode.name;
	m_wname = gnode.name.wstr();
	m_wnameId.Format(L"%d", m_id);
	SetColor(gnode.color);
	m_txtColor = cColor::WHITE;
	m_gid = gnode.id;
	m_prop = gnode;

	if (gnode.iteration >= 0) // iteration node? alpha blending
	{
		const Vector4 vColor = gnode.color.GetColor();
		SetColor(cColor(vColor.x, vColor.y, vColor.z, 0.8f));
	}

	return true;
}


bool cGNode::CreateBox(graphic::cRenderer &renderer, const Transform &tfm)
{
	graphic::cCube *cube = new graphic::cCube();
	cube->Create(renderer);
	cube->SetCube(Transform());
	cube->SetRenderFlag(eRenderFlag::OUTLINE, true);
	cube->SetRenderFlag(eRenderFlag::TEXT, true);
	AddChild(cube);

	m_prop.shape = phys::eShapeType::Box;
	m_transform = tfm;
	m_name = "Box";
	m_wname = m_name.wstr();
	m_wnameId.Format(L"%d", m_id);
	m_prop.name = "Box";
	return true;
}


bool cGNode::CreateSphere(graphic::cRenderer &renderer, const Transform &tfm
	, const float radius)
{
	graphic::cSphere *sphere = new graphic::cSphere();
	sphere->Create(renderer, 1.f, 10, 10);
	sphere->SetRenderFlag(eRenderFlag::OUTLINE, true);
	sphere->SetRenderFlag(eRenderFlag::TEXT, true);
	AddChild(sphere);

	m_prop.shape = phys::eShapeType::Sphere;
	m_transform = tfm;
	m_transform.scale = Vector3::Ones * radius;
	m_name = "Sphere";
	m_wname = m_name.wstr();
	m_wnameId.Format(L"%d", m_id);
	m_prop.name = "Sphere";
	SetSphereRadius(radius);
	return true;
}


bool cGNode::CreateCapsule(graphic::cRenderer &renderer, const Transform &tfm
	, const float radius, const float halfHeight)
{
	graphic::cCapsule *capsule = new graphic::cCapsule();
	capsule->Create(renderer, radius, halfHeight, 16, 8);
	capsule->SetRenderFlag(eRenderFlag::OUTLINE, true);
	capsule->SetRenderFlag(eRenderFlag::TEXT, true);
	AddChild(capsule);

	m_prop.shape = phys::eShapeType::Capsule;
	m_transform = tfm;
	m_name = "Capsule";
	m_wname = m_name.wstr();
	m_wnameId.Format(L"%d", m_id);
	m_prop.name = "Capsule";
	SetCapsuleDimension(radius, halfHeight);
	return true;
}


bool cGNode::CreateCylinder(graphic::cRenderer &renderer, const Transform &tfm
	, const float radius, const float height)
{
	graphic::cCylinder *cylinder = new graphic::cCylinder();
	cylinder->Create(renderer, 1.f, 2.f, 8);
	cylinder->SetRenderFlag(eRenderFlag::OUTLINE, true);
	cylinder->SetRenderFlag(eRenderFlag::TEXT, true);
	AddChild(cylinder);

	m_prop.shape = phys::eShapeType::Cylinder;
	m_transform = tfm;
	m_transform.scale = Vector3(height/2.f, radius, radius);
	m_name = "Cylinder";
	m_wname = m_name.wstr();
	m_wnameId.Format(L"%d", m_id);
	m_prop.name = "Cylinder";
	SetCylinderDimension(radius, height);
	return true;
}


bool cGNode::Render(graphic::cRenderer &renderer
	, const XMMATRIX &parentTm //= XMIdentity
	, const int flags //= 1
)
{
	__super::Render(renderer, parentTm, flags);

	if (flags & graphic::eRenderFlag::TEXT)
	{
		Transform tfm;
		tfm.pos = m_transform.pos + Vector3(0, 0.2f, 0);
		tfm.scale = Vector3::Ones * 0.12f;
		renderer.m_textMgr.AddTextRender(renderer
			, m_id, m_wname.c_str(), m_txtColor, cColor::BLACK
			, BILLBOARD_TYPE::DYN_SCALE, tfm, true, 16, 1
			, 0.5f, 200.f, 7.f);
	}

	if (!(flags & graphic::eRenderFlag::OUTLINE))
	{
		// y-axis line
		renderer.m_dbgLine.m_isSolid = true;
		renderer.m_dbgLine.SetColor(cColor::BLACK);
		renderer.m_dbgLine.SetLine(m_transform.pos
			, m_transform.pos + Vector3(0, -m_transform.pos.y, 0), 0.005f);
		renderer.m_dbgLine.Render(renderer);
	}

	return true;
}


// picking, tricky code
graphic::cNode* cGNode::Picking(const Ray &ray, const graphic::eNodeType::Enum type
	, const bool isSpherePicking //= true
	, OUT float *dist //= NULL
)
{
	if (m_children.empty())
		return nullptr;

	Transform temp = m_transform;
	m_transform.scale = m_children[0]->m_transform.scale * m_transform.scale;
	graphic::cNode *ret = __super::Picking(ray, type, isSpherePicking, dist);
	m_transform = temp;
	return ret;
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
	if (m_prop.shape != phys::eShapeType::Sphere)
		return;
	m_transform.scale = Vector3::Ones * radius;
}


float cGNode::GetSphereRadius()
{
	if (m_prop.shape != phys::eShapeType::Sphere)
		return 0.f;
	return m_transform.scale.x;
}


void cGNode::SetCapsuleDimension(const float radius, const float halfHeight)
{
	if (m_prop.shape != phys::eShapeType::Capsule)
		return;
	if (cCapsule *p = (cCapsule*)dynamic_cast<cCapsule*>(m_children[0]))
		p->SetDimension(radius, halfHeight);
	m_transform.scale = Vector3::Ones;
}


// return vector2(radius, halfHeight)
Vector2 cGNode::GetCapsuleDimension()
{
	if (m_prop.shape != phys::eShapeType::Capsule)
		return Vector2(0,0);
	if (cCapsule *p = (cCapsule*)dynamic_cast<cCapsule*>(m_children[0]))
		return Vector2(p->m_radius, p->m_halfHeight);
	return Vector2();
}


void cGNode::SetCylinderDimension(const float radius, const float height)
{
	if (m_prop.shape != phys::eShapeType::Cylinder)
		return;
	m_transform.scale = Vector3(height / 2.f, radius, radius);
}


// return Vector2(radius, height)
Vector2 cGNode::GetCylinderDimension()
{
	if (m_prop.shape != phys::eShapeType::Cylinder)
		return Vector2(0,0);
	return Vector2(m_transform.scale.y, m_transform.scale.x * 2.f);
}


// update genotype node color
void cGNode::SetColor(const graphic::cColor &color)
{
	m_prop.color = color;

	if (m_children.empty())
		return;

	switch (m_prop.shape)
	{
	case phys::eShapeType::Box:
		if (cCube *p = dynamic_cast<cCube*>(m_children[0]))
			p->SetColor(color);
		break;
	case phys::eShapeType::Sphere:
		if (cSphere *p= dynamic_cast<cSphere*>(m_children[0]))
			p->SetColor(color);
		break;
	case phys::eShapeType::Capsule:
		if (cCapsule *p = dynamic_cast<cCapsule*>(m_children[0]))
			p->SetColor(color);
		break;
	case phys::eShapeType::Cylinder:
		if (cCylinder *p = dynamic_cast<cCylinder*>(m_children[0]))
			p->SetColor(color);
		break;
	default: assert(0); break;
	}
}


// clone iteration node
cGNode* cGNode::Clone(graphic::cRenderer &renderer)
{
	cGNode *gnode = new cGNode();
	gnode->m_prop = m_prop;

	switch (m_prop.shape)
	{
	case phys::eShapeType::Box:
		gnode->CreateBox(renderer, m_transform);
		break;
	case phys::eShapeType::Sphere:
		gnode->CreateSphere(renderer, m_transform, GetSphereRadius());
		break;
	case phys::eShapeType::Capsule:
	{
		const Vector2 dim = GetCapsuleDimension();
		gnode->CreateCapsule(renderer, m_transform, dim.x, dim.y);
	}
	break;
	case phys::eShapeType::Cylinder:
	{
		const Vector2 dim = GetCylinderDimension();
		gnode->CreateCylinder(renderer, m_transform, dim.x, dim.y);
	}
	break;
	default: assert(0); break;
	}

	gnode->m_name = m_name + " iter";
	gnode->m_wname = m_wname + L" iter";
	const Vector3 scale = gnode->m_transform.scale * 2.f;
	gnode->m_transform.pos += Vector3(scale.x, 0, scale.z);
	const Vector4 vColor = m_prop.color.GetColor();
	gnode->SetColor(cColor(vColor.x, vColor.y, vColor.z, 0.8f));
	gnode->m_prop.iteration = m_id;
	return gnode;
}


void cGNode::Clear()
{
	__super::Clear();
}
