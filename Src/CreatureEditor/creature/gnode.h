//
// 2020-02-03, jjuiddong
// GenoType Node
//		- genotype node rendering
//		- genotype node connection, editing
//
#pragma once


namespace evc
{

	class cGLink;

	class cGNode : public graphic::cNode
	{
	public:
		cGNode();
		virtual ~cGNode();

		bool Create(graphic::cRenderer &renderer, const sGenotypeNode &gnode);
		bool CreateBox(graphic::cRenderer &renderer, const Transform &tfm);
		bool CreateSphere(graphic::cRenderer &renderer, const Transform &tfm
			, const float radius);
		bool CreateCapsule(graphic::cRenderer &renderer, const Transform &tfm
			, const float radius, const float halfHeight);
		bool CreateCylinder(graphic::cRenderer &renderer, const Transform &tfm
			, const float radius, const float height);

		virtual bool Render(graphic::cRenderer &renderer
			, const XMMATRIX &parentTm = graphic::XMIdentity
			, const int flags = 1) override;

		virtual graphic::cNode* Picking(const Ray &ray, const graphic::eNodeType::Enum type
			, const bool isSpherePicking = true
			, OUT float *dist = NULL) override;

		bool AddLink(cGLink *glink);
		bool RemoveLink(cGLink *glink);
		bool RemoveLink(const int linkId);
		void SetSphereRadius(const float radius);
		float GetSphereRadius();
		void SetCapsuleDimension(const float radius, const float halfHeight);
		Vector2 GetCapsuleDimension();
		void SetCylinderDimension(const float radius, const float height);
		Vector2 GetCylinderDimension();
		void SetColor(const graphic::cColor &color);

		cGNode* Clone(graphic::cRenderer &renderer);
		void Clear();


	public:
		WStrId m_wname;
		WStrId m_wnameId;
		sGenotypeNode m_prop; // property
		graphic::cColor m_txtColor;
		vector<cGLink*> m_links; // reference
		int m_gid; // genotype id
	};

}
