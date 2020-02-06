//
// 2020-01-25, jjuiddong
// evc : evolved virtual creature
//		- pnode : phenotype node (karlsims94)
//		- tree architecture
//
#pragma once


namespace evc
{

	class cPNode
	{
	public:
		cPNode();
		virtual ~cPNode();

		bool Create(graphic::cRenderer &renderer, const sGenotypeNode &gnode);

		void Clear();


	public:
		int m_id;
		int m_gid; // genotype node id
		StrId m_name;
		phys::cRigidActor *m_actor; // reference
		graphic::cNode *m_node; // reference
		
		vector<cPNode*> m_children;
	};

}
