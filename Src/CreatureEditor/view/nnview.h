//
// 2020-02-22, jjuiddong
// Neural Network 3D View
//
#pragma once


class cNNView : public framework::cDockWindow
{
public:
	cNNView(const string &name);
	virtual ~cNNView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnPreRender(const float deltaSeconds) override;
	virtual void OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect) override;
	virtual void OnEventProc(const sf::Event &evt) override;
	virtual void OnResetDevice() override;


protected:
	void RenderScene(graphic::cRenderer &renderer
		, const StrId &techiniqName
		, const bool isBuildShadowMap
		, const XMMATRIX &parentTm = graphic::XMIdentity
	);
	void RenderSelectModel(graphic::cRenderer &renderer, const bool buildOutline
		, const XMMATRIX &tm);
	//void RenderPopupMenu();
	//void RenderSaveDialog();

	void UpdateSelectModelTransform(const bool isGizmoEdit);
	void UpdateSelectModelTransform_GNode();
	void UpdateSelectModelTransform_MultiObject();
	void UpdateSelectModelTransform_Link();
	bool PickingProcess(const POINT &mousePos);
	int PickingNode(const int pickType, const POINT &mousePos
		, OUT float *outDistance = nullptr);

	void SpawnSelectNodeToPhenoTypeView();
	void SpawnSelectIterator();
	void DeleteSelectNode();
	void DeleteSelectLink();

	void UpdateLookAt();
	void OnWheelMove(const float delta, const POINT mousePt);
	void OnMouseMove(const POINT mousePt);
	void OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt);
	void OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt);


public:
	graphic::cRenderTarget m_renderTarget;
	graphic::cDepthBuffer m_depthBuff;
	graphic::cGrid m_grid;
	graphic::cGridLine m_gridLine;
	graphic::cSkyBoxCube m_skybox;
	graphic::cBezierLine m_bezier;
	graphic::cBillboard m_billboard;
	evc::cCreature *m_curCreature;
	Vector3 m_creatureOffsetPos;

	bool m_showGrid;
	bool m_showName;
	bool m_showId;
	bool m_showJoint;
	bool m_showSensor;
	bool m_showEffector;

	bool m_showSaveDialog;
	int m_popupMenuState; // 0:no show, 1:open, 2:show, 3:close
	int m_popupMenuType; // 0: genotype menu

	Vector3 m_tempSpawnPos;
	Vector3 m_pivotPos;
	int m_clickedId;

	// MouseMove Variable
	POINT m_viewPos;
	sRectf m_viewRect; // detect mouse event area
	POINT m_mousePos; // window 2d mouse pos
	POINT m_mouseClickPos; // window 2d mouse pos
	Vector3 m_mousePickPos; // mouse cursor pos in ground picking
	bool m_mouseDown[3]; // Left, Right, Middle
	float m_rotateLen;
};
