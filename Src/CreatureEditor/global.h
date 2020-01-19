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
	bool SelectRigidActor(const int id, const bool isToggle = false);
	bool ClearSelection();
	graphic::cRenderer& GetRenderer();
	void Clear();


protected:
	bool SetRigidActorColor(const int id, const graphic::cColor &color);


public:
	c3DView *m_3dView;
	cEditorView *m_editorView;
	phys::cPhysicsEngine m_physics;
	phys::cPhysicsSync *m_physSync;

	graphic::cGizmo m_gizmo;

	set<int> m_selects; // selection actor id
	
	// joint setting
	bool m_isShowJointOption;
	int m_jointActorId0;
	int m_jointActorId1;
};
