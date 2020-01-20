//
// 2020-01-19, jjuiddong
// global variable class
//
#pragma once


enum class eEditState {
	Normal, Pivot0, Pivot1
};


class c3DView;
class cEditorView;

class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();

	bool Init(graphic::cRenderer &renderer);
	bool SelectRigidActor(const int actorId, const bool isToggle = false);
	bool ClearSelection();

	// transform edit function
	bool ModifyRigidActorTransform(const int actorId, const Vector3 &dim);
	bool GetModifyRigidActorTransform(const int actorId, OUT Vector3 &out);
	bool RemoveModifyRigidActorTransform(const int actorId);

	// joint manage function
	bool AddJoint(phys::cJoint *joint);
	bool RemoveJoint(phys::cJoint *joint);
	cJointRenderer* FindJointRenderer(phys::cJoint *joint);

	void Clear();

	graphic::cRenderer& GetRenderer();


protected:
	bool SetRigidActorColor(const int id, const graphic::cColor &color);


public:
	eEditState m_state;

	c3DView *m_3dView;
	cEditorView *m_editorView;
	phys::cPhysicsEngine m_physics;
	phys::cPhysicsSync *m_physSync;

	vector<int> m_selects; // selection actor id

	// manage Modify RigidActor information
	graphic::cGizmo m_gizmo;
	map<int, Vector3> m_chDimensions; // key:actorid, value:dimension

	// joint
	phys::cJoint *m_selJoint; // reference
	vector<cJointRenderer*> m_jointRenderers;
};
