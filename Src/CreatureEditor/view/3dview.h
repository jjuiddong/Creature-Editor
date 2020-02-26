//
// 2020-01-19, jjuiddong
// 3D View
//
#pragma once


class c3DView : public framework::cDockWindow
{
public:
	c3DView(const string &name);
	virtual ~c3DView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnPreRender(const float deltaSeconds) override;
	virtual void OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect) override;
	virtual void OnEventProc(const sf::Event &evt) override;
	virtual void OnResetDevice() override;


protected:
	void RenderScene(graphic::cRenderer &renderer, const StrId &techiniqName
		, const bool isBuildShadowMap, const XMMATRIX &parentTm = graphic::XMIdentity);
	void RenderEtc(graphic::cRenderer &renderer);
	void RenderSelectModel(graphic::cRenderer &renderer, const bool buildOutline
		, const XMMATRIX &tm);
	void RenderPopupMenu();
	void RenderTooltip();
	void RenderSaveDialog();
	void RenderEvolutionGraph();
	void RenderReflectionMap(graphic::cRenderer &renderer);
	void UpdateSelectModelTransform(const bool isGizmoEdit);
	void UpdateSelectModelTransform_RigidActor();
	void UpdateSelectModelTransform_MultiObject();
	void UpdateSelectModelTransform_Joint();

	bool PickingProcess(const POINT &mousePos);
	int PickingRigidActor(const int pickType, const POINT &mousePos
		, OUT float *outDistance = nullptr);

	void DeleteConnectJointAll();
	void DeleteSelectCreature();

	void UpdateLookAt();
	void OnWheelMove(const float delta, const POINT mousePt);
	void OnMouseMove(const POINT mousePt);
	void OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt);
	void OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt);


public:
	graphic::cRenderTarget m_renderTarget;
	graphic::cRenderTarget m_reflectMap;
	graphic::cCascadedShadowMap m_ccsm;
	graphic::cDepthBuffer m_depthBuff;
	graphic::cGridLine m_gridLine;
	graphic::cGrid *m_groundPlane; // reference
	graphic::cSkyBoxCube m_skybox;
	graphic::cShader11 m_reflectShader;

	bool m_showGrid;
	bool m_showReflection;
	bool m_showShadow;
	bool m_showJoint;
	bool m_showSaveDialog;
	bool m_isSaveOnlySelectionActor;
	int m_popupMenuState; // 0:no show, 1:open, 2:show, 3:close
	int m_popupMenuType; // 0:actor menu, 1:spawn selection menu
	int m_saveFileSyncId; // save file id
	Vector3 m_tempSpawnPos;

	Vector3 m_pivotPos;

	// MouseMove Variable
	POINT m_viewPos;
	sRectf m_viewRect; // detect mouse event area
	POINT m_mousePos; // window 2d mouse pos
	POINT m_mouseClickPos; // window 2d mouse pos
	Vector3 m_mousePickPos; // mouse cursor pos in ground picking
	bool m_mouseDown[3]; // Left, Right, Middle
	float m_rotateLen;
};
