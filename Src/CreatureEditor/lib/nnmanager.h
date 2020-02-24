//
// 2020-02-23, jjuiddong
// neural network(NN) manager
//
#pragma once

enum class eNNEditMode { Normal, };


class cNNManager
{
public:
	cNNManager();
	virtual ~cNNManager();

	bool Init(graphic::cRenderer &renderer);

	// mode change
	void ChangeEditMode(const eNNEditMode state);
	eNNEditMode GetEditMode();

	// creature
	bool SetCurrentCreature(evc::cCreature *creature);

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

	// utility function
	bool SetAllLinkedNodeSelect(evc::cGNode *gnode);
	bool UpdateAllLinkedNodeTransform(evc::cGNode *gnode, const Transform &transform);
	bool AutoSave();

	graphic::cRenderer& GetRenderer();
	void Clear();


public:
	eNNEditMode m_mode;

	// gizmo & selection
	vector<int> m_selects; // select id array
	set<int> m_highLights; // hilight id array
	graphic::cNode m_multiSel; // multi selection moving transform
	Quaternion m_multiSelRot; // multi selection rotation offset (prev rotation)
	Vector3 m_multiSelPos; // multi selection position offset (prev position)
	int m_orbitId; // oribit moving focus genotype node id
	Vector3 m_orbitTarget;

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
	evc::cCreature* m_creature; // reference
};
