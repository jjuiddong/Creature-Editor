
#include "stdafx.h"
#include "global.h"
#include "view/3dview.h"


cGlobal::cGlobal()
	: m_3dView(nullptr)
	, m_editorView(nullptr)
	, m_physSync(nullptr)
	, m_state(eEditState::Normal)
	, m_showUIJoint(false)
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
	m_groundGridPlaneId = m_physSync->SpawnPlane(renderer, Vector3(0, 1, 0));
	
	m_gizmo.Create(renderer);
	//m_gizmo.LockEditType(graphic::eGizmoEditType::SCALE, true);

	// initialize ui joint
	m_uiJoint.CreateReferenceMode();
	m_uiJointRenderer.Create(&m_uiJoint);

	return true;
}


// pick rigidactor, selection actor
bool cGlobal::SelectObject(const int syncId
	, const bool isToggle //= false
)
{
	if (isToggle)
	{
		if (m_selects.end() == std::find(m_selects.begin(), m_selects.end(), syncId))
			m_selects.push_back(syncId);
		else
			common::removevector(m_selects, syncId);
	}
	else
	{
		if (m_selects.end() == std::find(m_selects.begin(), m_selects.end(), syncId))
			m_selects.push_back(syncId);
	}
	return true;
}


bool cGlobal::ClearSelection()
{
	m_selects.clear();
	m_highLight.clear();
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


// find joint renderer helper function
cJointRenderer* cGlobal::FindJointRenderer(phys::cJoint *joint)
{
	phys::sSyncInfo *syncJoint = m_physSync->FindSyncInfo(joint);
	if (!syncJoint)
		return nullptr;
	cJointRenderer *jointRenderer = dynamic_cast<cJointRenderer*>(syncJoint->node);
	return jointRenderer;
}


// find syncinfo wrapper cPhysSync
phys::sSyncInfo* cGlobal::FindSyncInfo(const int syncId)
{
	return m_physSync->FindSyncInfo(syncId);
}


bool cGlobal::SetRigidActorColor(const int syncId, const graphic::cColor &color)
{
	using namespace graphic;

	phys::sSyncInfo *info = m_physSync->FindSyncInfo(syncId);
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
