//
// 2020-01-25, jjuiddong
// evc : evolved virtual creature
//		- pnode : phenotype node (karlsims94)
//		- tree architecture
//
#pragma once


namespace evc
{

	class cJointRenderer;

	class cPNode
	{
	public:
		cPNode();
		virtual ~cPNode();


		void Clear();


	public:
		int m_id;
		phys::cRigidActor *m_actor; // reference
		//vector<phys::cJoint*> m_joints; // reference
		//vector<cJointRenderer*> m_jointRenderers; // reference
		graphic::cNode *m_node; // render object, reference
		vector<cPNode*> m_children;
	};

}
