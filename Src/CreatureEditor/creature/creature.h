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
		bool GenerationGenoType2(const uint generation);

		bool CopyGenoType(sGenotypeNode *src, sGenotypeLink *srcLink, sGenotypeNode *dst
			, set<sGenotypeNode*> &ignore);

		enum class eHierachy {Parent, Child};
		sGenotypeNode* CloneGenoTypeNode(sGenotypeNode *src, sGenotypeNode *dst);
		sGenotypeNode* CloneGenoTypeNode(const eHierachy cloneType
			, sGenotypeLink *srcLink, sGenotypeNode *dst);

		sGenotypeNode* CloneGenoTypeNode(const eHierachy cloneType
			, sGenotypeLink *srcLink
			, sGenotypeNode *srcParent, sGenotypeNode *srcChild, sGenotypeNode *dst
			, INOUT set<sGenotypeNode*> &ignoreNodes
			, INOUT set<std::pair<sGenotypeNode*, sGenotypeNode*>> &ignoreLinks);

		sGenotypeNode* CloneGenoType(sGenotypeLink *srcLink, sGenotypeNode *dstParent);
		sGenotypeLink* FindParentLink(sGenotypeNode *child);
		sGenotypeLink* FindLink(sGenotypeNode *parent, sGenotypeNode *child);
		bool RemoveGenoTypeNode(sGenotypeNode *gnode);
		void UpdateIterationId();

		void GenerationGenotypeLink(sGenotypeNode *src, sGenotypeNode *gen);
		void MoveFinalNodeWithCalcTm(sGenotypeNode *parent, sGenotypeNode *iter
			, sGenotypeNode *addIter, sGenotypeNode *finalNode
			, INOUT set<sGenotypeNode*> &visit);
		void MoveNode(sGenotypeNode *linkNode, const Matrix44 &localTm
			, INOUT set<sGenotypeNode*> &visit);
		void MoveFinalNode(sGenotypeNode *curLinkNode, sGenotypeNode *movLinkNode
			, sGenotypeNode *finalNode);
		void MoveAllFinalNode();


	public:
		int m_id;
		StrId m_name;

		// phenotype
		vector<cPNode*> m_nodes;

		// DNA, genotype
		vector<sGenotypeNode*> m_gnodes;
		vector<sGenotypeLink*> m_glinks;
		map<int, sGenotypeNode*> m_gmap; // key: sGenotypeNode id (reference)
		map<int, sGenotypeNode*> m_clonemap; // key: source sGenotypeNode id
											 // data: generation genotype node (reference)
		map<sGenotypeNode*, set<sGenotypeLink*>> m_linkMap;  // gnode link set (reference)
															 // key: parent gnode
															 // value: gnode link child set
	};

}
