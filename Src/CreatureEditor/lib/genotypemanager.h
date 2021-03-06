//
// 2020-02-03, jjuiddong
// genotype manager
//		- global genotype data manager
//
#pragma once

enum class eGenoEditMode { Normal, JointEdit, Pivot0, Pivot1, Revolute, SpawnLocation };

namespace evc {
	class cGNode;
	class cGLink;
}


class cGenoTypeManager
{
public:
	cGenoTypeManager();
	virtual ~cGenoTypeManager();

	bool Init(graphic::cRenderer &renderer
		, phys::cPhysicsEngine *physics
		, phys::cPhysicsSync *sync);

	// mode change
	void ChangeEditMode(const eGenoEditMode state);
	eGenoEditMode GetEditMode();

	// spawn object
	int SpawnBox(const Vector3 &pos);
	int SpawnSphere(const Vector3 &pos);
	int SpawnCapsule(const Vector3 &pos);
	int SpawnCylinder(const Vector3 &pos);

	// creature
	bool AddCreature(evc::cCreature *creature);
	bool ReadGenoTypeNodeFile(const StrPath &fileName, const Vector3 &pos);
	bool ReadCreatureFile(const StrPath &fileName, const Vector3 &pos);

	evc::cGNode* FindGNode(const int id);
	evc::cGLink* FindGLink(const int id);
	bool AddGNode(evc::cGNode *gnode);
	bool RemoveGNode(evc::cGNode *gnode);
	bool AddGLink(evc::cGLink *glink);
	bool RemoveGLink(evc::cGLink *glink);

	// selection
	void SetSelection(const int id);
	bool SelectObject(const int id, const bool isToggle = false);
	bool ClearSelection();

	// group
	void SetGroupBySelection();
	void AddGroup(const int id);
	void RemoveGroup(const int id);
	void ClearGroup();

	// utility function
	bool SetAllLinkedNodeSelect(evc::cGNode *gnode);
	bool UpdateAllLinkedNodeTransform(evc::cGNode *gnode, const Transform &transform);
	bool AutoSave();

	graphic::cRenderer& GetRenderer();
	void Clear();


public:
	eGenoEditMode m_mode;
	phys::cPhysicsEngine *m_physics; // reference
	phys::cPhysicsSync *m_physSync; // reference

	Transform m_spawnTransform;

	// gizmo & selection
	vector<int> m_selects; // select id array
	set<int> m_highLights; // hilight id array
	graphic::cNode m_multiSel; // multi selection moving transform
	Quaternion m_multiSelRot; // multi selection rotation offset (prev rotation)
	Vector3 m_multiSelPos; // multi selection position offset (prev position)
	int m_orbitId; // oribit moving focus genotype node id
	Vector3 m_orbitTarget;

	// group
	set<int> m_group; // group id set

	// manage Modify RigidActor information
	graphic::cGizmo m_gizmo;

	// save filename
	StrPath m_saveFileName;

	// link
	evc::cGLink *m_selLink; // reference
	bool m_showUILink;
	evc::cGLink m_uiLink; // link edit mode ui

	bool m_fixJointSelection;
	int m_pairId0; // link edit mode, gnode0
	int m_pairId1; // link edit mode, gnode1

	// genotype node, link 
	vector<evc::cGNode*> m_gnodes;
	vector<evc::cGLink*> m_glinks;

	// creatures
	vector<evc::cCreature*> m_creatures;
};
