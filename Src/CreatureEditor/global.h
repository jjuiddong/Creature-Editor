//
// 2020-01-19, jjuiddong
// global variable class
//
#pragma once


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

	void Clear();

	graphic::cRenderer& GetRenderer();


protected:
	bool SetRigidActorColor(const int id, const graphic::cColor &color);


public:
	c3DView *m_3dView;
	cEditorView *m_editorView;
	phys::cPhysicsEngine m_physics;
	phys::cPhysicsSync *m_physSync;

	set<int> m_selects; // selection actor id

	// edit RigidActor information from selection
	graphic::cGizmo m_gizmo;
	map<int, Vector3> m_chDimensions; // key:actorid, value:dimension
};
