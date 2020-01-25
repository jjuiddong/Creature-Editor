//
// 2020-01-24, jjuiddong
// evc : evolved virtual creature
//		- creature
//
#pragma once


namespace evc
{

	class cPNode;

	class cCreature
	{
	public:
		cCreature();
		virtual ~cCreature();

		bool Read(const StrPath &fileName);
		bool Write(const StrPath &fileName);
		void Clear();


	public:
		cPNode *m_root;
	};

}
