
#include "stdafx.h"
#include "global.h"
#include "view/3dview.h"


cGlobal::cGlobal()
	: m_3dView(nullptr)
	, m_editorView(nullptr)
	, m_physSync(nullptr)
	, m_state(eEditState::Normal)
{
}

cGlobal::~cGlobal()
{
	Clear();
}


bool cGlobal::Init(graphic::cRenderer &renderer)
{
	if (!m_physics.InitializePhysx())
		return false;

	m_physSync = new phys::cPhysicsSync();
	m_physSync->Create(&m_physics);
	const int id = m_physSync->SpawnPlane(renderer, Vector3(0, 1, 0));
	//if (phys::sActorInfo *info = m_physSync->FindActorInfo(id))
	//	((graphic::cGrid*)info->node)->m_mtrl.InitGray3();

	m_gizmo.Create(renderer);

	return true;
}


// pick rigidactor, selection actor
bool cGlobal::SelectRigidActor(const int actorId
	, const bool isToggle //= false
)
{
	if (isToggle)
	{
		if (m_selects.end() == std::find(m_selects.begin(), m_selects.end(), actorId))
			m_selects.push_back(actorId);
		else
			common::removevector(m_selects, actorId);
	}
	else
	{
		if (m_selects.end() == std::find(m_selects.begin(), m_selects.end(), actorId))
			m_selects.push_back(actorId);
	}
	return true;
}


bool cGlobal::ClearSelection()
{
	m_selects.clear();
	return true;
}


// store Modify RigidActor Dimension variable
// apply when RigidActor physics simulation and wakeup
// dimension changing is complicate work
bool cGlobal::ModifyRigidActorTransform(const int actorId, const Vector3 &dim)
{
	m_chDimensions[actorId] = dim;
	return true;
}


// return Modify actor dimension information
// if not found, return false
bool cGlobal::GetModifyRigidActorTransform(const int actorId, OUT Vector3 &out)
{
	auto it = m_chDimensions.find(actorId);
	if (it == m_chDimensions.end())
		return false;

	out = it->second;
	return true;
}


// remove modify rigid actor transform value
bool cGlobal::RemoveModifyRigidActorTransform(const int actorId)
{
	auto it = m_chDimensions.find(actorId);
	if (it == m_chDimensions.end())
		return false;
	m_chDimensions.erase(it);
	return true;
}


bool cGlobal::SetRigidActorColor(const int id, const graphic::cColor &color)
{
	using namespace graphic;

	phys::sActorInfo *info = m_physSync->FindActorInfo(id);
	if (!info)
		return false;

	switch (info->actor->m_shape)
	{
	case phys::cRigidActor::eShape::Box:
		if (cCube *cube = dynamic_cast<cCube*>(info->node))
			cube->SetColor(color);
		break;
	case phys::cRigidActor::eShape::Sphere:
		if (cSphere *sphere = dynamic_cast<cSphere*>(info->node))
			sphere->SetColor(color);
		break;
	case phys::cRigidActor::eShape::Capsule:
		if (cCapsule *capsule = dynamic_cast<cCapsule*>(info->node))
			capsule->SetColor(color);
		break;
	default:
		break;
	}
	return true;
}


// add joint, joint renderer
bool cGlobal::AddJoint(phys::cJoint *joint)
{
	m_physSync->AddJoint(joint);

	cJointRenderer *jointRenderer = new cJointRenderer();
	jointRenderer->Create(joint);
	m_jointRenderers.push_back(jointRenderer);
	return true;
}


// remove joint, joint renderer
bool cGlobal::RemoveJoint(phys::cJoint *joint)
{
	auto it = std::find_if(m_jointRenderers.begin(), m_jointRenderers.end()
		, [&](const auto &a) {return a->m_joint == joint; });
	if (m_jointRenderers.end() == it)
		return false;
	delete *it;
	m_jointRenderers.erase(it);

	m_physSync->RemoveJoint(joint);

	return true;
}


cJointRenderer* cGlobal::FindJointRenderer(phys::cJoint *joint)
{
	auto it = std::find_if(m_jointRenderers.begin(), m_jointRenderers.end()
		, [&](const auto &a) {return a->m_joint == joint; });
	if (m_jointRenderers.end() == it)
		return nullptr;
	return *it;
}


graphic::cRenderer& cGlobal::GetRenderer()
{
	return m_3dView->GetRenderer();
}


void cGlobal::Clear()
{
	for (auto &p : m_jointRenderers)
		delete p;
	m_jointRenderers.clear();

	m_chDimensions.clear();
	m_physics.Clear();
}
