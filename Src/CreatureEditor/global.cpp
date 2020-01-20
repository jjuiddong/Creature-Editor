
#include "stdafx.h"
#include "global.h"
#include "view/3dview.h"


cGlobal::cGlobal()
	: m_3dView(nullptr)
	, m_editorView(nullptr)
	, m_physSync(nullptr)
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
	//if (phys::cPhysicsSync::sActorInfo *info = m_physSync->FindActorInfo(id))
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
		if (m_selects.end() == m_selects.find(actorId))
			m_selects.insert(actorId);
		else
			m_selects.erase(actorId);
	}
	else
	{
		m_selects.insert(actorId);
	}

	//using namespace graphic;
	//if (m_selectActorId == id)
	//	return true;
	//SetRigidActorColor(m_selectActorId, cColor::WHITE);
	//SetRigidActorColor(id, cColor::RED);
	//m_selectActorId = id;
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

	phys::cPhysicsSync::sActorInfo *info = m_physSync->FindActorInfo(id);
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


graphic::cRenderer& cGlobal::GetRenderer()
{
	return m_3dView->GetRenderer();
}


void cGlobal::Clear()
{
	m_chDimensions.clear();
	m_physics.Clear();
}
