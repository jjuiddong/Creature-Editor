//
// 2020-01-19, jjuiddong
// global variable class
//
#pragma once


class c3DView;
class cPhenoEditorView;
class cGenoEditorView;
class cResourceView;
class cSimulationView;
class cGenoView;
class cNNView;
class cEvolutionView;
class cPhenoTypeManager;
class cGenoTypeManager;
class cNNManager;

namespace evc { class cCreature; }

class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();

	bool Init(graphic::cRenderer &renderer);

	graphic::cRenderer& GetRenderer();
	void Clear();


public:
	c3DView *m_3dView; // reference
	cGenoView *m_genoView; // reference
	cNNView *m_nnView; // reference
	cPhenoEditorView *m_peditorView; // reference
	cGenoEditorView *m_geditorView; // reference
	cResourceView *m_resView; // reference
	cSimulationView *m_simView; // reference
	cEvolutionView *m_evoView; // reference
	phys::cPhysicsEngine m_physics;
	phys::cPhysicsSync *m_physSync;
	cPhenoTypeManager *m_pheno; // reference
	cGenoTypeManager *m_geno; // reference
	cNNManager *m_nn; // reference
	cEvolutionManager *m_evo; // reference
};
