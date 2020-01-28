//
// 2020-01-25, jjuiddong
// evc : evolved virtual creature
//		- generate creature
//
#pragma once


namespace evc
{

	class cCreature;

	
	cCreature* GenerateCreatureFrom_RigidActor(graphic::cRenderer &renderer
		, phys::cRigidActor *actor);

	bool WritePhenoTypeFileFrom_RigidActor(const StrPath &fileName
		, phys::cRigidActor *actor);

	bool WritePhenoTypeFileFrom_RigidActor(const StrPath &fileName
		, vector<phys::cRigidActor*> actors);

	cCreature* ReadPhenoTypeFile(graphic::cRenderer &renderer
		, const StrPath &fileName
		, OUT vector<int> *outSyncIds = nullptr);

}
