
#include "stdafx.h"
#include "phenotypemanager.h"
#include "../view/3dview.h"
#include "../view/phenoeditorview.h"
#include "../view/resourceview.h"
#include "../creature/creature.h"


cPhenoTypeManager::cPhenoTypeManager()
	: m_spawnTransform(Vector3(0, 3, 0), Vector3(0.5f, 0.5f, 0.5f))
	, m_physics(nullptr)
	, m_physSync(nullptr)
	, m_mode(ePhenoEditMode::Normal)
	, m_showUIJoint(false)
	, m_isSpawnLock(true)
	, m_pairSyncId0(-1)
	, m_pairSyncId1(-1)
	, m_fixJointSelection(false)
{
}

cPhenoTypeManager::~cPhenoTypeManager()
{
	Clear();
}


bool cPhenoTypeManager::Init(graphic::cRenderer &renderer
	, phys::cPhysicsEngine *physics
	, phys::cPhysicsSync *sync)
{
	m_physics = physics;
	m_physSync = sync;

	m_groundGridPlaneId = m_physSync->SpawnPlane(renderer, Vector3(0, 1, 0));

	// wall
	const float wall = 100.f;
	const float h = 2.5f;
	m_physSync->SpawnBox(renderer, Transform(Vector3(wall, h, 0), Vector3(0.5f, h, wall))
		, 1.f, true, "wall");
	m_physSync->SpawnBox(renderer, Transform(Vector3(0, h, wall), Vector3(wall, h, 0.5f))
		, 1.f, true, "wall");
	m_physSync->SpawnBox(renderer, Transform(Vector3(-wall, h, 0), Vector3(0.5f, h, wall))
		, 1.f, true, "wall");
	m_physSync->SpawnBox(renderer, Transform(Vector3(0, h, -wall), Vector3(wall, h, 0.5f))
		, 1.f, true, "wall");
	//~wall

	m_gizmo.Create(renderer);

	// initialize ui joint
	m_uiJoint.CreateReferenceMode();
	m_uiJointRenderer.Create(*m_physSync, &m_uiJoint);

	return true;
}


// change edit state
void cPhenoTypeManager::ChangeEditMode(const ePhenoEditMode state)
{
	m_mode = state;
}


ePhenoEditMode cPhenoTypeManager::GetEditMode()
{
	return m_mode;
}


// add creature ptr
bool cPhenoTypeManager::AddCreature(evc::cCreature *creature)
{
	m_creatures.push_back(creature);
	return true;
}


// set syncId rigidactor selection and set kinematic true or false
void cPhenoTypeManager::SetSelectionAndKinematic(const int syncId)
{
	phys::sSyncInfo *sync = m_physSync->FindSyncInfo(syncId);
	if (sync && sync->actor)
	{
		sync->actor->SetKinematic(g_pheno->m_isSpawnLock);

		// select spawn rigidactor
		ChangeEditMode(ePhenoEditMode::Normal);
		ClearSelection();
		SelectObject(syncId);
		m_gizmo.SetControlNode(sync->node);

		// change focus
		g_global->m_3dView->SetFocus((framework::cDockWindow*)g_global->m_3dView);
	}
}


void cPhenoTypeManager::SpawnBox(const Vector3 &pos)
{
	graphic::cRenderer &renderer = GetRenderer();

	Transform tfm = m_spawnTransform;
	tfm.pos = pos;
	tfm.pos.y = m_spawnTransform.pos.y;
	const float density = g_global->m_peditorView->m_density;

	const int syncId = m_physSync->SpawnBox(renderer, tfm, density);
	SetSelectionAndKinematic(syncId);
}


void cPhenoTypeManager::SpawnSphere(const Vector3 &pos)
{
	graphic::cRenderer &renderer = GetRenderer();

	Transform tfm = m_spawnTransform;
	tfm.pos = pos;
	tfm.pos.y = m_spawnTransform.pos.y;
	const float density = g_global->m_peditorView->m_density;
	const float radius = g_global->m_peditorView->m_radius;

	const int syncId = m_physSync->SpawnSphere(renderer, tfm, radius, density);
	SetSelectionAndKinematic(syncId);
}


void cPhenoTypeManager::SpawnCapsule(const Vector3 &pos)
{
	graphic::cRenderer &renderer = GetRenderer();

	Transform tfm(pos, m_spawnTransform.rot);
	tfm.pos.y = m_spawnTransform.pos.y;
	const float density = g_global->m_peditorView->m_density;
	const float radius = g_global->m_peditorView->m_radius;
	const float halfHeight = g_global->m_peditorView->m_halfHeight;

	const int syncId = m_physSync->SpawnCapsule(renderer, tfm, radius, halfHeight, density);
	SetSelectionAndKinematic(syncId);
}


void cPhenoTypeManager::SpawnCylinder(const Vector3 &pos)
{
	graphic::cRenderer &renderer = GetRenderer();

	Transform tfm(pos, m_spawnTransform.rot);
	tfm.pos.y = m_spawnTransform.pos.y;
	const float density = g_global->m_peditorView->m_density;
	const float radius = g_global->m_peditorView->m_radius;
	const float halfHeight = g_global->m_peditorView->m_halfHeight;

	const int syncId = m_physSync->SpawnCylinder(renderer, tfm, radius, halfHeight, density);
	SetSelectionAndKinematic(syncId);
}


// pick rigidactor, selection actor
bool cPhenoTypeManager::SelectObject(const int syncId
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

	// update multi selection transform
	if (m_selects.size() > 1)
	{
		Vector3 center;
		for (auto &id : m_selects)
		{
			phys::sSyncInfo *sync = FindSyncInfo(id);
			if (sync)
				center += sync->node->m_transform.pos;
		}
		center /= (float)m_selects.size();
		center.y += 0.5f;
		m_multiSelPos = center;
		m_multiSelRot = Quaternion();
		m_multiSel.m_transform.rot = Quaternion();
		m_multiSel.m_transform.pos = center;
		m_gizmo.SetControlNode(&m_multiSel);
	}

	return true;
}


bool cPhenoTypeManager::ClearSelection()
{
	m_selects.clear();
	m_highLights.clear();
	return true;
}


// store Modify RigidActor Dimension variable
// apply when RigidActor physics simulation and wakeup
// dimension changing is complicate work
bool cPhenoTypeManager::ModifyRigidActorTransform(const int syncId, const Vector3 &dim)
{
	m_chDimensions[syncId] = dim;
	return true;
}


// return Modify actor dimension information
// if not found, return false
bool cPhenoTypeManager::GetModifyRigidActorTransform(const int syncId, OUT Vector3 &out)
{
	auto it = m_chDimensions.find(syncId);
	if (it == m_chDimensions.end())
		return false;

	out = it->second;
	return true;
}


// remove modify rigid actor transform value
bool cPhenoTypeManager::RemoveModifyRigidActorTransform(const int syncId)
{
	auto it = m_chDimensions.find(syncId);
	if (it == m_chDimensions.end())
		return false;
	m_chDimensions.erase(it);
	return true;
}


// find joint renderer helper function
cJointRenderer* cPhenoTypeManager::FindJointRenderer(phys::cJoint *joint)
{
	phys::sSyncInfo *syncJoint = m_physSync->FindSyncInfo(joint);
	if (!syncJoint)
		return nullptr;
	cJointRenderer *jointRenderer = dynamic_cast<cJointRenderer*>(syncJoint->node);
	return jointRenderer;
}


// find syncinfo wrapper cPhysSync
phys::sSyncInfo* cPhenoTypeManager::FindSyncInfo(const int syncId)
{
	return m_physSync->FindSyncInfo(syncId);
}

// find rigid actor from syncid
phys::cRigidActor* cPhenoTypeManager::FindRigidActorFromSyncId(const int syncId)
{
	phys::sSyncInfo *sync = m_physSync->FindSyncInfo(syncId);
	if (!sync)
		return nullptr;
	return sync->actor;
}


// traverse all connection actor
template<typename Fn>
bool TraverseAllConnectionActor(phys::cRigidActor *actor, Fn fn)
{
	set<phys::cRigidActor*> actors;
	queue<phys::cRigidActor*> q;
	q.push(actor);
	while (!q.empty())
	{
		phys::cRigidActor *a = q.front();
		q.pop();

		if (actors.end() != std::find(actors.begin(), actors.end(), a))
			continue; // already visit

		if (!fn(a))
			continue;

		actors.insert(a);

		for (auto &p : a->m_joints)
		{
			q.push(p->m_actor0);
			q.push(p->m_actor1);
		}
	}
	return true;
}


// update actor dimension if changed
// actor dimension delay update, before operation to actor object
// update dimension information to cRigidActor
bool cPhenoTypeManager::UpdateActorDimension(phys::cRigidActor *actor, const bool isKinematic)
{
	phys::sSyncInfo *sync = m_physSync->FindSyncInfo(actor);
	if (sync)
	{
		// is change dimension?
		// apply modify dimension
		Vector3 dim;
		if (GetModifyRigidActorTransform(sync->id, dim))
		{
			// apply physics shape
			actor->ChangeDimension(*m_physics, dim);
			RemoveModifyRigidActorTransform(sync->id);
		}
	}

	if (actor->IsKinematic() != isKinematic)
		actor->SetKinematic(isKinematic);
	if (!isKinematic)
		actor->WakeUp();

	return true;
}


// update kinematic all connection actor
bool cPhenoTypeManager::SetAllConnectionActorKinematic(phys::cRigidActor *actor
	, const bool isKinematic)
{
	TraverseAllConnectionActor(actor,
		[&](phys::cRigidActor *a) {
		if (a->IsKinematic() != isKinematic)
			a->SetKinematic(isKinematic);
		if (!isKinematic)
			a->WakeUp();
		return true;
	});

	return true;
}


// select all connection actor
bool cPhenoTypeManager::SetAllConnectionActorSelect(phys::cRigidActor *actor)
{
	ClearSelection();

	TraverseAllConnectionActor(actor,
		[&](phys::cRigidActor *a) {
		phys::sSyncInfo *sync = m_physSync->FindSyncInfo(a);
		if (sync)
			SelectObject(sync->id);
		return true;
	});
	return true;
}


// update all connection actor dimension value
// update rigidactor dimension
// actor dimension delay update, before operation to actor object
// update dimension information to cRigidActor
bool cPhenoTypeManager::UpdateAllConnectionActorDimension(phys::cRigidActor *actor
	, const bool isKinematic)
{
	TraverseAllConnectionActor(actor,
		[&](phys::cRigidActor *a) {
		UpdateActorDimension(a, isKinematic);

		phys::sSyncInfo *sync = m_physSync->FindSyncInfo(a);
		if (!sync)
			return true;

		// update actor localFrame
		for (auto &joint : a->m_joints)
		{
			if (joint->m_actor0 == a)
				joint->m_actorLocal0 = sync->node->m_transform;
			if (joint->m_actor1 == a)
				joint->m_actorLocal1 = sync->node->m_transform;
		}
		return true;
	}
	);

	return true;
}


// update all actor transform
bool cPhenoTypeManager::UpdateAllConnectionActorTransform(phys::cRigidActor *actor
	, const Transform &transform)
{
	TraverseAllConnectionActor(actor,
		[&](phys::cRigidActor *a) {
		using namespace physx;
		if (phys::sSyncInfo *sync = m_physSync->FindSyncInfo(a))
		{
			sync->node->m_transform.pos += transform.pos;
			a->SetGlobalPose(sync->node->m_transform);
		}
		return true;
	}
	);

	return true;
}


// resource view wrapping function
bool cPhenoTypeManager::RefreshResourceView()
{
	g_global->m_resourceView->UpdateResourceFiles();
	return true;
}


bool cPhenoTypeManager::SetRigidActorColor(const int syncId, const graphic::cColor &color)
{
	using namespace graphic;

	phys::sSyncInfo *info = m_physSync->FindSyncInfo(syncId);
	if (!info)
		return false;

	switch (info->actor->m_shape)
	{
	case phys::eShapeType::Box:
		if (cCube *cube = dynamic_cast<cCube*>(info->node))
			cube->SetColor(color);
		break;
	case phys::eShapeType::Sphere:
		if (cSphere *sphere = dynamic_cast<cSphere*>(info->node))
			sphere->SetColor(color);
		break;
	case phys::eShapeType::Capsule:
		if (cCapsule *capsule = dynamic_cast<cCapsule*>(info->node))
			capsule->SetColor(color);
		break;
	default:
		break;
	}
	return true;
}


graphic::cRenderer& cPhenoTypeManager::GetRenderer()
{
	return g_global->m_3dView->GetRenderer();
}


void cPhenoTypeManager::Clear()
{
	for (auto &p : m_creatures)
		delete p;
	m_creatures.clear();

	m_chDimensions.clear();
}