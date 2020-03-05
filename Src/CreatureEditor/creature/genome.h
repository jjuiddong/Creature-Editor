//
// 2020-03-01, jjuiddong
// genome management class
//		- multiple dna management
//		- *.gen
//
// Data Hierarchy
//	- Genome
//		- DNA Array
//			- Chromo array

//
// format (binary)
//  - field name (byte size)
//		- "GEN" file format (3)
//		- name (64)
//		- dna count (4)
//		- dna 1 chromo count (4)
//		- dna 1 chromo data double type (chromo count * 8)
//		- ....
//		- dna N chromo count (4)
//		- dna N chromo data double type (chromo count * 8)
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
		struct sDna
		{
			uint layerCnt;
			uint inputCnt;
			uint outputCnt;
			double fitness;
			vector<double> chromo;
		};
		StrPath m_fileName;
		StrId m_name;
		vector<sDna> m_dnas;
	};

}
