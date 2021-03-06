//
// 2020-02-03, jjuiddong
// phenotype data manager
//		- global phenotype data manager
//
#pragma once


enum class ePhenoEditMode { Normal, JointEdit, Pivot0, Pivot1, Revolute, SpawnLocation };


class cPhenoTypeManager
{
public:
	cPhenoTypeManager();
	virtual ~cPhenoTypeManager();

	bool Init(graphic::cRenderer &renderer
		, phys::cPhysicsEngine *physics
		, phys::cPhysicsSync *sync);

	// mode change
	void ChangeEditMode(const ePhenoEditMode state);
	ePhenoEditMode GetEditMode();

	// creature
	bool AddCreature(evc::cCreature *creature);
	evc::cCreature* FindCreatureContainNode(const int syncId);
	bool RemoveCreature(evc::cCreature *creature);
	bool ReadPhenoTypeFile(const StrPath &fileName, const Vector3 &pos);

	enum eSpawnFlag {Lock=0x01, Unlock=0x02, UnSelect=0x04};
	evc::cCreature* ReadCreatureFile(const StrPath &fileName, const Vector3 &pos
		, const int spawnFlags = 0);

	// spawn object
	void SpawnBox(const Vector3 &pos);
	void SpawnSphere(const Vector3 &pos);
	void SpawnCapsule(const Vector3 &pos);
	void SpawnCylinder(const Vector3 &pos);
	void SetSelectionAndKinematic(const int syncId);

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
	bool SetAllConnectionActorWakeUp(phys::cRigidActor *actor);
	bool SetAllConnectionActorSelect(phys::cRigidActor *actor);
	bool UpdateAllConnectionActorDimension(phys::cRigidActor *actor, const bool isKinematic);
	bool UpdateAllConnectionActorTransform(phys::cRigidActor *actor, const Transform &transform);
	bool RefreshResourceView();

	graphic::cRenderer& GetRenderer();
	void ClearJointEdit();
	void ClearCreature();
	void Clear();


protected:
	bool SetRigidActorColor(const int syncId, const graphic::cColor &color);


public:
	ePhenoEditMode m_mode;
	phys::cPhysicsEngine *m_physics; // reference
	phys::cPhysicsSync *m_physSync; // reference

	bool m_isSpawnLock; // default: true
	Transform m_spawnTransform;

	// gizmo & selection
	int m_groundGridPlaneId; // ground plane sync id
	vector<int> m_selects; // select syncId array
	set<int> m_highLights; // hilight syncId array
	graphic::cNode m_multiSel; // multi selection moving transform
	Quaternion m_multiSelRot; // multi selection rotation offset (prev rotation)
	Vector3 m_multiSelPos; // multi selection position offset (prev position)
	int m_orbitId;
	Vector3 m_orbitTarget;

	// manage Modify RigidActor information
	graphic::cGizmo m_gizmo;
	map<int, Vector3> m_chDimensions; // key:actorid, value:dimension

	// save filename
	StrPath m_saveFileName;
	StrPath m_saveGenomeFileName;

	// joint
	phys::cJoint *m_selJoint; // reference
	bool m_showUIJoint;
	phys::cJoint m_uiJoint; // joint edit mode ui
	cJointRenderer m_uiJointRenderer; // joint edit mode ui
	bool m_fixJointSelection;
	int m_pairSyncId0; // joint edit mode, actor0
	int m_pairSyncId1; // joint edit mode, actor1

	// creatures
	vector<evc::cCreature*> m_creatures;
	map<int, evc::cCreature*> m_creatureMap; // fast search map, reference

	// generation
	int m_generationCnt;
	int m_nnLayerCnt;
};
