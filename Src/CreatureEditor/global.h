//
// 2020-01-19, jjuiddong
// global variable class
//
#pragma once


enum class eEditMode {Normal, JointEdit, Pivot0, Pivot1, Revolute, SpawnLocation};

class c3DView;
class cEditorView;
class cResourceView;
class cSimulationView;

class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();

	bool Init(graphic::cRenderer &renderer);

	// mode change
	void ChangeEditMode(const eEditMode state);
	eEditMode GetEditMode();

	// selection
	bool SelectObject(const int syncId, const bool isToggle = false);
	bool ClearSelection();

	// transform edit function
	bool ModifyRigidActorTransform(const int syncId, const Vector3 &dim);
	bool GetModifyRigidActorTransform(const int syncId, OUT Vector3 &out);
	bool RemoveModifyRigidActorTransform(const int syncId);

	// utility function
	cJointRenderer* FindJointRenderer(phys::cJoint *joint);
	phys::sSyncInfo* FindSyncInfo(const int syncId);
	phys::cRigidActor* FindRigidActorFromSyncId(const int syncId);
	bool UpdateActorDimension(phys::cRigidActor *actor, const bool isKinematic);
	bool SetAllConnectionActorKinematic(phys::cRigidActor *actor, const bool isKinematic);
	bool SetAllConnectionActorSelect(phys::cRigidActor *actor);
	bool UpdateAllConnectionActorDimension(phys::cRigidActor *actor, const bool isKinematic);
	bool UpdateAllConnectionActorTransform(phys::cRigidActor *actor, const Transform &transform);	
	bool RefreshResourceView();

	graphic::cRenderer& GetRenderer();
	void Clear();


protected:
	bool SetRigidActorColor(const int syncId, const graphic::cColor &color);


public:
	eEditMode m_mode;
	c3DView *m_3dView;
	cEditorView *m_editorView;
	cResourceView *m_resourceView;
	cSimulationView *m_simView;
	phys::cPhysicsEngine m_physics;
	phys::cPhysicsSync *m_physSync;

	// spawn control
	bool m_isSpawnLock; // default: true
	Transform m_spawnTransform;

	// gizmo & selection
	int m_groundGridPlaneId; // ground plane sync id
	vector<int> m_selects; // select syncId array
	set<int> m_highLights; // hilight syncId array
	graphic::cNode m_multiSel; // multi selection moving transform
	Quaternion m_multiSelRot; // multi selection rotation offset (prev rotation)
	Vector3 m_multiSelPos; // multi selection position offset (prev position)

	// manage Modify RigidActor information
	graphic::cGizmo m_gizmo;
	map<int, Vector3> m_chDimensions; // key:actorid, value:dimension

	// joint
	phys::cJoint *m_selJoint; // reference
	bool m_showUIJoint;
	phys::cJoint m_uiJoint; // joint edit mode ui
	cJointRenderer m_uiJointRenderer; // joint edit mode ui
	bool m_fixJointSelection;
	int m_pairSyncId0; // joint edit mode, actor0
	int m_pairSyncId1; // joint edit mode, actor1
};
