//
// 2020-02-03, jjuiddong
// GenoType Node Link
//		- between genotype node connection link
//
#pragma once


namespace evc
{
	class cGNode;

	class cGLink
	{
	public:
		cGLink();
		virtual ~cGLink();

		void Clear();


	public:
		int m_id;
		phys::eJointType::Enum m_type;
		cGNode *m_gnode0;
		cGNode *m_gnode1;
	};

}
