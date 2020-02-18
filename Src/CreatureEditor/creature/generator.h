//
// 2020-01-25, jjuiddong
// evc : evolved virtual creature
//		- generate creature
//
#pragma once


namespace evc
{

	class cCreature;
	class cGNode;
	class cGLink;
	class cPNode;
	
	cCreature* GenerateCreatureFrom_RigidActor(graphic::cRenderer &renderer
		, phys::cRigidActor *actor);


	//----------------------------------------------------------------------------
	// PhenoType
	bool WritePhenoTypeFileFrom_RigidActor(const StrPath &fileName
		, phys::cRigidActor *actor);

	bool WritePhenoTypeFileFrom_RigidActor(const StrPath &fileName
		, const vector<phys::cRigidActor*> &actors);

	cCreature* ReadPhenoTypeFile(graphic::cRenderer &renderer
		, const StrPath &fileName
		, OUT vector<int> *outSyncIds = nullptr);

	cPNode* CreatePhenoTypeNode(graphic::cRenderer &renderer
		, const sGenotypeNode &gnode, const uint generation);

	phys::cJoint* CreatePhenoTypeJoint(const sGenotypeLink &glink
		, cPNode *pnode0, cPNode *pnode1, const uint generation);


	//----------------------------------------------------------------------------
	// GenoType
	bool WriteGenoTypeFileFrom_Node(const StrPath &fileName
		, cGNode *gnode);

	bool WriteGenoTypeFileFrom_Node(const StrPath &fileName
		, const vector<cGNode*> &gnodes);

	bool ReadGenoTypeFile(const StrPath &fileName
		, OUT vector<sGenotypeNode*> &outNode
		, OUT vector<sGenotypeLink*> &outLink
		, OUT map<int, sGenotypeNode*> &outMap);

}
