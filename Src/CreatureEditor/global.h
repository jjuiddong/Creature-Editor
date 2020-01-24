//
// 2020-01-19, jjuiddong
// global variable class
//
#pragma once


enum class eEditState {Normal, Pivot0, Pivot1, Revolute};

class c3DView;
class cEditorView;

class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();

	bool Init(graphic::cRenderer &renderer);
	bool SelectObject(const int syncId, const bool isToggle = false);
	bool ClearSelection();

	// transform edit function
	bool ModifyRigidActorTransform(const int actorId, const Vector3 &dim);
	bool GetModifyRigidActorTransform(const int actorId, OUT Vector3 &out);
	bool RemoveModifyRigidActorTransform(const int actorId);

	// utility function
	cJointRenderer* FindJointRenderer(phys::cJoint *joint);
	phys::sSyncInfo* FindSyncInfo(const int syncId);

	graphic::cRenderer& GetRenderer();
	void Clear();


protected:
	bool SetRigidActorColor(const int syncId, const graphic::cColor &color);


public:
	eEditState m_state;
	c3DView *m_3dView;
	cEditorView *m_editorView;
	phys::cPhysicsEngine m_physics;
	phys::cPhysicsSync *m_physSync;

	int m_groundGridPlaneId; // ground plane sync id
	vector<int> m_selects; // select syncId array
	vector<int> m_highLight; // highlight syncId array

	// manage Modify RigidActor information
	graphic::cGizmo m_gizmo;
	map<int, Vector3> m_chDimensions; // key:actorid, value:dimension

	// joint
	phys::cJoint *m_selJoint; // reference
	bool m_showUIJoint;
	phys::cJoint m_uiJoint;
	cJointRenderer m_uiJointRenderer;
};
