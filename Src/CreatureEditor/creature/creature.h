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

		void Update(const float deltaSeconds);
		bool SetKinematic(const bool isKinematic);
		void SetTransform(const Transform &tfm);
		bool GetSyncIds(OUT vector<int> &out);
		cPNode* FindPNode(phys::cRigidActor *actor);
		ai::cNeuralNet* GetNeuralNetwork(const uint idx=0) const;
		ai::sGenome GetGenome() const;
		bool SetGenome(const ai::sGenome &genome);
		void Clear();


	protected:
		bool ReadGenoTypeFile(graphic::cRenderer &renderer, const StrPath &fileName);
		void LoadFromGenoType(graphic::cRenderer &renderer, const uint generation
			, const uint neuralNetLayerCnt );
		bool GenerationGenoType(const uint generation, const uint maxGeneration
			, const bool isRecursive =true);

		enum class eClone {Generation, Copy};
		enum class eHierarchy {Parent, Child};
		sGenotypeNode* CloneGenoTypeNode(const eClone cloneType, const uint generation
			, sGenotypeNode *src, sGenotypeNode *dst);
		sGenotypeNode* CloneGenoTypeNode(const eClone cloneType, const eHierarchy target
			, const uint generation, sGenotypeLink *srcLink, sGenotypeNode *dst);

		sGenotypeNode* CloneGenoTypeNode(
			const eClone copy
			, const eHierarchy cloneType
			, const uint generation
			, sGenotypeLink *srcLink
			, sGenotypeNode *srcParent, sGenotypeNode *srcChild, sGenotypeNode *dst
			, INOUT set<sGenotypeNode*> &ignoreNodes
			, INOUT set<std::pair<sGenotypeNode*, sGenotypeNode*>> &ignoreLinks);

		sGenotypeLink* FindParentLink(sGenotypeNode *child);
		sGenotypeLink* FindLink(sGenotypeNode *parent, sGenotypeNode *child);
		bool RemoveGenoTypeNode(sGenotypeNode *gnode
			, const bool removeFinalNode = false);

		void MoveAllFinalNode(const uint generation);
		void CollectLinkedNode(sGenotypeNode *gnode, const set<sGenotypeNode*> &ignores
			, OUT set<sGenotypeNode*> &out);
		bool IsAlreadyGenerated(sGenotypeNode *gnode);


	public:
		int m_id;
		StrId m_name;
		uint m_generation;
		bool m_isNN; // neural network simulation?

		// phenotype node
		vector<cPNode*> m_nodes;

		// DNA, genotype node
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
