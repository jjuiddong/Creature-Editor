//
// 2020-01-24, jjuiddong
// evc : evolved virtual creature
//		- creature
//
#pragma once


namespace evc
{

	class cPNode;
	class cGNode;
	class cGLink;

	class cCreature
	{
	public:
		cCreature(const StrId &name="creature");
		virtual ~cCreature();

		bool Read(graphic::cRenderer &renderer, const StrPath &fileName
			, const Transform &tfm=Transform());
		
		bool Write(const StrPath &fileName);

		bool SetKinematic(const bool isKinematic);
		void SetTransform(const Transform &tfm);
		bool GetSyncIds(OUT vector<int> &out);
		void Clear();


	protected:
		bool ReadGenoTypeFile(graphic::cRenderer &renderer, const StrPath &fileName);
		void LoadFromGenoType(graphic::cRenderer &renderer, const uint generation);
		bool GenerationGenoType(const uint generation);
		void GenerationGenotypeLink(sGenotypeNode *src, sGenotypeNode *gen);


	public:
		int m_id;
		StrId m_name;

		// phenotype
		vector<cPNode*> m_nodes;

		// DNA, genotype
		vector<sGenotypeNode*> m_gnodes;
		vector<sGenotypeLink*> m_glinks;
		map<int, sGenotypeNode*> m_gmap; // key: sGenotypeNode id
	};

}
