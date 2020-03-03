//
// 2020-03-01, jjuiddong
// genome management class
//		- multiple genome management
//		- *.gen
//
// Data Hierarchy
//	- Genome
//		- Chromo array

//
// format (binary)
//  - field name (byte size)
//		- "GEN" file format (3)
//		- name (64)
//		- genome count (4)
//		- genome 1 chromo count (4)
//		- genome 1 chromo data double type (chromo count * 8)
//		- ....
//		- genome N chromo count (4)
//		- genome N chromo data double type (chromo count * 8)
//
#pragma once


namespace evc
{

	class cGenome
	{
	public:
		cGenome();
		virtual ~cGenome();

		bool Read(const StrPath &fileName);
		bool Write(const StrPath &fileName);
		void Clear();


	public:
		struct sGenome
		{
			uint layerCnt;
			uint inputCnt;
			uint outputCnt;
			vector<double> chromo;
		};
		StrPath m_fileName;
		StrId m_name;
		vector<sGenome> m_genomes;
	};

}
