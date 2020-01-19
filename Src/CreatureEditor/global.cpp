
#include "stdafx.h"
#include "global.h"
#include "view/3dview.h"


cGlobal::cGlobal()
	: m_3dView(nullptr)
	, m_editorView(nullptr)
	, m_physSync(nullptr)
	, m_isShowJointOption(false)
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
bool cGlobal::SelectRigidActor(const int id
	, const bool isToggle //= false
)
{
	if (isToggle)
	{
		if (m_selects.end() == m_selects.find(id))
		{
			m_selects.insert(id);
		}
		else
		{
			m_selects.erase(id);
		}
	}
	else
	{
		m_selects.insert(id);
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
	m_physics.Clear();
}
