
#include "stdafx.h"
#include "3dview.h"

using namespace graphic;
using namespace framework;


c3DView::c3DView(const string &name)
	: framework::cDockWindow(name)
	, m_showGrid(true)
	, m_showReflection(true)
	, m_showShadow(true)
	, m_groundPlane(nullptr)
	, m_showSaveDialog(false)
	, m_popupMenuState(0)
	, m_popupMenuType(0)
{
}

c3DView::~c3DView()
{
}


bool c3DView::Init(cRenderer &renderer)
{
	const Vector3 eyePos(30, 20, -30);
	const Vector3 lookAt(0,0,0);
	m_camera.SetCamera(eyePos, lookAt, Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, m_rect.Width() / m_rect.Height(), 0.1f, 100000.f);
	m_camera.SetViewPort(m_rect.Width(), m_rect.Height());

	GetMainLight().Init(graphic::cLight::LIGHT_DIRECTIONAL);
	GetMainLight().SetDirection(Vector3(-1, -2, 1.3f).Normal());

	sf::Vector2u size((u_int)m_rect.Width() - 15, (u_int)m_rect.Height() - 50);
	cViewport vp = renderer.m_viewPort;
	vp.m_vp.Width = (float)size.x;
	vp.m_vp.Height = (float)size.y;
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true
		, DXGI_FORMAT_D24_UNORM_S8_UINT);

	{
		cViewport rflvp;
		rflvp.Create(0, 0, 1024, 1024, 0.f, 1.f);
		m_reflectMap.Create(renderer, rflvp, DXGI_FORMAT_R8G8B8A8_UNORM, false);
	}

	m_ccsm.Create(renderer);
	cViewport dvp;
	dvp.Create(0, 0, 1024, 1024, 0, 1);
	m_depthBuff.Create(renderer, dvp, false);
	m_gridLine.Create(renderer, 200, 200, 1.f, 1.f
		, (eVertexType::POSITION | eVertexType::COLOR)
		, cColor(0.6f, 0.6f, 0.6f, 0.5f)
		, cColor(0.f, 0.f, 0.f, 0.5f)
	);
	m_gridLine.m_offsetY = 0.01f;

	m_skybox.Create(renderer, "./media/skybox/sky.dds");

	m_reflectShader.Create(renderer, "media/shader11/pos-norm-tex-reflect.fxo", "Unlit"
		, eVertexType::POSITION | eVertexType::NORMAL | eVertexType::TEXTURE0);
	renderer.m_cbClipPlane.m_v->reflectAlpha[0] = 0.2f;

	// setting reflection map
	phys::sSyncInfo *sync = g_global->m_physSync->FindSyncInfo(g_global->m_groundGridPlaneId);
	if (sync)
	{
		m_groundPlane = dynamic_cast<graphic::cGrid*>(sync->node);
		if (m_groundPlane)
		{
			m_groundPlane->SetShader(&m_reflectShader);
			m_groundPlane->m_reflectionMap = &m_reflectMap;
		}
	}

	return true;
}


void c3DView::OnUpdate(const float deltaSeconds)
{
	g_global->m_physics.PreUpdate(deltaSeconds);

	for (auto &p : g_global->m_physSync->m_syncs)
		if (p->joint)
			p->joint->Update(deltaSeconds);
}


void c3DView::OnPreRender(const float deltaSeconds)
{
	cRenderer &renderer = GetRenderer();
	phys::cPhysicsSync *physSync = g_global->m_physSync;

	cAutoCam cam(&m_camera);

	renderer.UnbindShaderAll();
	renderer.UnbindTextureAll();
	GetMainCamera().Bind(renderer);
	GetMainLight().Bind(renderer);

	// build shadowmap
	if (m_showShadow)
	{
		m_ccsm.UpdateParameter(renderer, GetMainCamera());
		for (int i = 0; i < cCascadedShadowMap::SHADOWMAP_COUNT; ++i)
		{
			m_ccsm.Begin(renderer, i);
			RenderScene(renderer, "BuildShadowMap", true);
			m_ccsm.End(renderer, i);
		}
	}

	// build reflectionmap
	if (m_showReflection)
	{
		GetMainCamera().Bind(renderer);
		RenderReflectionMap(renderer);
	}

	// Render Outline select object
	if (!g_global->m_selects.empty() || !g_global->m_highLights.empty())
	{
		m_depthBuff.Begin(renderer);
		RenderSelectModel(renderer, true, XMIdentity);
		m_depthBuff.End(renderer);
	}

	const bool isShowGizmo = (g_global->m_selects.size() >= 1)
		&& !::GetAsyncKeyState(VK_SHIFT)
		&& !::GetAsyncKeyState(VK_CONTROL)
		;

	bool isGizmoEdit = false;
	if (m_renderTarget.Begin(renderer))
	{
		cAutoCam cam(&m_camera);
		renderer.UnbindShaderAll();
		renderer.UnbindTextureAll();
		GetMainCamera().Bind(renderer);
		GetMainLight().Bind(renderer);

		m_skybox.Render(renderer);

		CommonStates states(renderer.GetDevice());
		renderer.GetDevContext()->RSSetState(states.CullCounterClockwise());

		if (m_showShadow)
			m_ccsm.Bind(renderer);
		if (m_showReflection)
			m_reflectMap.Bind(renderer, 8);

		RenderScene(renderer, "ShadowMap", false);
		RenderEtc(renderer);

		if (isShowGizmo)
			isGizmoEdit = g_global->m_gizmo.Render(renderer, deltaSeconds, m_mousePos, m_mouseDown[0]);

		// render spawn position
		{
			Vector3 spawnPos = g_global->m_spawnTransform.pos;
			spawnPos.y = 0.f;

			renderer.m_dbgCube.SetColor(cColor::GREEN);
			renderer.m_dbgCube.SetCube(Transform(spawnPos, Vector3::Ones*0.2f));
			renderer.m_dbgCube.Render(renderer);
		}

		// render reserve spawn position (to show popupmenu selection)
		if ((m_popupMenuState == 2) && (m_popupMenuType == 1))
		{
			renderer.m_dbgCube.SetColor(cColor::BLUE);
			renderer.m_dbgCube.SetCube(Transform(m_tempSpawnPos, Vector3(0.05f,5.f,0.05f)));
			renderer.m_dbgCube.Render(renderer);
		}

		renderer.RenderAxis2();
	}
	m_renderTarget.End(renderer);

	// Modify rigid actor transform by Gizmo
	// process before g_global->m_physics.PostUpdate()
	UpdateSelectModelTransform(isGizmoEdit);

	g_global->m_physics.PostUpdate(deltaSeconds);
}


// scene render, shadowmap, scene
void c3DView::RenderScene(graphic::cRenderer &renderer
	, const StrId &techiniqName
	, const bool isBuildShadowMap
	, const XMMATRIX &parentTm //= graphic::XMIdentity
)
{
	phys::cPhysicsSync *physSync = g_global->m_physSync;
	RET(!physSync);

	for (auto &p : physSync->m_syncs)
	{
		if (isBuildShadowMap && (p->name == "plane"))
			continue;
		p->node->SetTechnique(techiniqName.c_str());
		p->node->Render(renderer, parentTm);
	}

	if (!isBuildShadowMap)
	{
		m_gridLine.Render(renderer);
		RenderSelectModel(renderer, false, parentTm);
	}
}


void c3DView::RenderEtc(graphic::cRenderer &renderer)
{
	phys::cPhysicsSync *physSync = g_global->m_physSync;
	RET(!physSync);

	// render pivot position
	if ((eEditState::Pivot0 == g_global->m_state)
		|| (eEditState::Pivot1 == g_global->m_state))
	{
		Transform tfm(m_pivotPos, Vector3::Ones*0.05f);
		renderer.m_dbgBox.SetColor(cColor::RED);
		renderer.m_dbgBox.SetBox(tfm);
		renderer.m_dbgBox.Render(renderer);
	}

	// render joint
	//renderer.m_dbgLine.m_isSolid = true;
	//renderer.m_dbgLine.SetColor(cColor::GREEN);
	//renderer.m_dbgBox.SetColor(cColor::GREEN);
	//for (auto &jointRenderer : g_global->m_jointRenderers)
	//	jointRenderer->Render(renderer);
	renderer.m_dbgLine.m_isSolid = false;
	renderer.m_dbgLine.SetColor(cColor::BLACK);
	renderer.m_dbgBox.SetColor(cColor::BLACK);

	// render ui joint
	//if (g_global->m_showUIJoint)
	//	g_global->m_uiJointRenderer.Render(renderer);
}


// Render Outline Selected model
// buildOutline : true, render depth buffer
//				  false, general render
void c3DView::RenderSelectModel(graphic::cRenderer &renderer, const bool buildOutline
	, const XMMATRIX &tm)
{
	if (g_global->m_selects.empty() && g_global->m_highLights.empty())
		return;

	// render function object
	auto render = [&](phys::sSyncInfo *sync) {
		if (buildOutline)
		{
			sync->node->SetTechnique("DepthTech");
			Matrix44 parentTm = sync->node->GetParentWorldMatrix();
			sync->node->Render(renderer, parentTm.GetMatrixXM());
		}
		else
		{
			renderer.BindTexture(m_depthBuff, 7);
			sync->node->SetTechnique("Outline");
			Matrix44 parentTm = sync->node->GetParentWorldMatrix();
			sync->node->Render(renderer, parentTm.GetMatrixXM());
		}
	};

	phys::cPhysicsSync *physSync = g_global->m_physSync;

	CommonStates state(renderer.GetDevice());
	if (!buildOutline)
		renderer.GetDevContext()->OMSetDepthStencilState(state.DepthNone(), 0);
	renderer.m_cbPerFrame.m_v->outlineColor = Vector4(0.8f, 0, 0, 1).GetVectorXM();
	for (auto syncId : g_global->m_selects)
	{
		if (phys::sSyncInfo *sync = physSync->FindSyncInfo(syncId))
			render(sync);
	}
	renderer.m_cbPerFrame.m_v->outlineColor = Vector4(0.f, 1.f, 0.f, 1).GetVectorXM();
	for (auto syncId : g_global->m_highLights)
	{
		if (phys::sSyncInfo *sync = physSync->FindSyncInfo(syncId))
			render(sync);
	}
	if (!buildOutline)
		renderer.GetDevContext()->OMSetDepthStencilState(state.DepthDefault(), 0);
}


// Modify rigid actor transform by Gizmo
// process before g_global->m_physics.PostUpdate()
void c3DView::UpdateSelectModelTransform(const bool isGizmoEdit)
{
	phys::cPhysicsSync *physSync = g_global->m_physSync;

	const bool isShowGizmo = (g_global->m_selects.size() != 0);
	if (!isShowGizmo || !isGizmoEdit)
		return;
	if (!g_global->m_gizmo.m_controlNode)
		return;

	if (g_global->m_selects.size() == 1)
	{
		const int selectSyncId = *g_global->m_selects.begin();
		phys::sSyncInfo *sync = physSync->FindSyncInfo(selectSyncId);
		if (!sync)
			return;

		if (sync->actor)
			UpdateSelectModelTransform_RigidActor();
		else if (sync->joint)
			UpdateSelectModelTransform_Joint();
	}
	else if (g_global->m_selects.size() > 1)
	{
		UpdateSelectModelTransform_MultiObject();
	}
}


// Modify rigid actor transform by Gizmo, rigid actor
void c3DView::UpdateSelectModelTransform_RigidActor()
{
	using namespace physx;
	phys::cPhysicsSync *physSync = g_global->m_physSync;

	const int selectSyncId = (g_global->m_selects.size() == 1) ? *g_global->m_selects.begin() : -1;
	phys::sSyncInfo *sync = physSync->FindSyncInfo(selectSyncId);
	if (sync->actor->m_type != phys::eRigidType::Dynamic) // dynamic actor?
		return;

	Transform temp = g_global->m_gizmo.m_targetTransform;
	temp.pos.y = max(0.f, temp.pos.y);
	const Transform tfm = temp;

	PxTransform tm(*(PxVec3*)&tfm.pos, *(PxQuat*)&tfm.rot);
	sync->actor->SetGlobalPose(tm);

	// change dimension? apply wakeup
	if (eGizmoEditType::SCALE == g_global->m_gizmo.m_type)
	{
		sync->actor->SetKinematic(true); // stop simulation

		// rigidactor dimension change
		g_global->m_gizmo.m_controlNode->m_transform.scale = tfm.scale;

		// RigidActor Scale change was delayed, change when wakeup time
		// store change value
		// change 3d model dimension
		switch (sync->actor->m_shape)
		{
		case phys::eShapeType::Box:
		{
			g_global->ModifyRigidActorTransform(selectSyncId, tfm.scale);
		}
		break;
		case phys::eShapeType::Sphere:
		{
			float scale = 1.f;
			switch (g_global->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: scale = sync->node->m_transform.scale.x; break;
			case eGizmoEditAxis::Y: scale = sync->node->m_transform.scale.y; break;
			case eGizmoEditAxis::Z: scale = sync->node->m_transform.scale.z; break;
			}

			((cSphere*)sync->node)->SetRadius(scale); // update radius transform
			g_global->m_gizmo.UpdateTargetTransform(sync->node->m_transform); // update gizmo
			g_global->ModifyRigidActorTransform(selectSyncId, Vector3(scale, 1, 1));
		}
		break;
		case phys::eShapeType::Capsule:
		{
			const Vector3 scale = sync->node->m_transform.scale;
			float radius = 1.f;
			switch (g_global->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: radius = scale.y; break;
			case eGizmoEditAxis::Y: radius = scale.y; break;
			case eGizmoEditAxis::Z: radius = scale.z; break;
			}

			const float halfHeight = scale.x - radius;

			((cCapsule*)sync->node)->SetDimension(radius, halfHeight); // update capsule transform
			g_global->m_gizmo.UpdateTargetTransform(sync->node->m_transform); // update gizmo
			g_global->ModifyRigidActorTransform(selectSyncId, Vector3(halfHeight, radius, radius));
		}
		break;
		}
	}
}


// update gizmo transform to multi selection object
void c3DView::UpdateSelectModelTransform_MultiObject()
{
	using namespace physx;
	phys::cPhysicsSync *physSync = g_global->m_physSync;

	const Vector3 realCenter = g_global->m_multiSelPos - Vector3(0, 0.5f, 0);
	const Vector3 center = g_global->m_multiSelPos;
	const Vector3 offset = g_global->m_multiSel.m_transform.pos - center;
	g_global->m_multiSelPos = g_global->m_multiSel.m_transform.pos; // update current position

	const Quaternion rot = g_global->m_multiSel.m_transform.rot * g_global->m_multiSelRot.Inverse();
	g_global->m_multiSelRot = g_global->m_multiSel.m_transform.rot; // update current rotation

	// update
	for (auto id : g_global->m_selects)
	{
		phys::sSyncInfo *sync = physSync->FindSyncInfo(id);
		if (!sync || !sync->actor)
			continue;
		if (sync->actor->m_type != phys::eRigidType::Dynamic) // dynamic actor?
			continue;

		const Vector3 pos = (sync->node->m_transform.pos + offset) - realCenter;
		const Vector3 p = pos * rot;
		sync->node->m_transform.rot *= rot;
		sync->node->m_transform.pos = p + realCenter;
		sync->node->m_transform.pos.y = max(0.f, sync->node->m_transform.pos.y);

		const Transform tfm = sync->node->m_transform;
		PxTransform tm(*(PxVec3*)&tfm.pos, *(PxQuat*)&tfm.rot);
		sync->actor->SetGlobalPose(tm); // update physics transform
	}
}


// Modify rigid actor transform by Gizmo, joint
void c3DView::UpdateSelectModelTransform_Joint()
{
	using namespace physx;
	phys::cPhysicsSync *physSync = g_global->m_physSync;

	const int selectSyncId = (g_global->m_selects.size() == 1) ? *g_global->m_selects.begin() : -1;
	phys::sSyncInfo *sync = physSync->FindSyncInfo(selectSyncId);

	cJointRenderer *jointRenderer = dynamic_cast<cJointRenderer*>(sync->node);
	if (!jointRenderer)
		return; // error occurred

	//g_global->m_gizmo;
	switch (g_global->m_gizmo.m_type)
	{
	case eGizmoEditType::TRANSLATE:
		jointRenderer->SetRevoluteAxisPos(jointRenderer->m_transform.pos);
		break;
	case eGizmoEditType::SCALE:
		break;
	case eGizmoEditType::ROTATE:
		break;
	}
}


void c3DView::OnRender(const float deltaSeconds)
{
	ImVec2 pos = ImGui::GetCursorScreenPos();
	m_viewPos = { (int)(pos.x), (int)(pos.y) };
	m_viewRect = { pos.x + 5, pos.y, pos.x + m_rect.Width() - 30, pos.y + m_rect.Height() - 42 };

	// HUD
	bool isOpen = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoBackground
		;

	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	ImGui::Image(m_renderTarget.m_resolvedSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 42));

	// Render Information
	ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y));
	ImGui::SetNextWindowBgAlpha(0.f);
	ImGui::SetNextWindowSize(ImVec2(min(m_viewRect.Width(), 350.f), m_viewRect.Height()));
	if (ImGui::Begin("Map Information", &isOpen, flags))
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Checkbox("grid", &m_showGrid);
		ImGui::SameLine();
		ImGui::Checkbox("reflection", &m_showReflection);
		ImGui::SameLine();
		ImGui::Checkbox("shadow", &m_showShadow);
		if (m_isOrbitMove)
		{
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
			ImGui::TextUnformatted("Oribit");
			ImGui::PopStyleColor();
		}

		ImGui::End();
	}
	ImGui::PopStyleColor();

	RenderPopupMenu();
	RenderSaveDialog();
}


void c3DView::RenderPopupMenu()
{
	if (m_popupMenuState == 1)
	{
		switch (m_popupMenuType)
		{
		case 0: ImGui::OpenPopup("Actor PopupMenu"); break;
		case 1: ImGui::OpenPopup("Spawn PopupMenu"); break;
		default: assert(0); break;
		}
		m_popupMenuState = 2;
	}

	if (ImGui::BeginPopup("Actor PopupMenu"))
	{
		if (g_global->m_selects.empty())
		{
			ImGui::EndPopup();
			return;
		}

		bool isLockMenu = false;
		bool isUnlockMenu = false;

		// check menu enable
		int cntKinematic[2] = { 0, 0 }; // kinematic, unkinematic
		for (auto id : g_global->m_selects)
			if (phys::sSyncInfo *sync = g_global->FindSyncInfo(id))
				if (sync && sync->actor)
					cntKinematic[sync->actor->IsKinematic()]++;

		if (cntKinematic[0] > 0)
			isLockMenu = true;
		if (cntKinematic[1] > 0)
			isUnlockMenu = true;

		phys::sSyncInfo *sync = g_global->FindSyncInfo(*g_global->m_selects.begin());
		phys::cRigidActor *actor = (sync && sync->actor) ? sync->actor : nullptr;
		if (!actor)
		{
			ImGui::EndPopup();
			return;
		}

		if (ImGui::MenuItem("Save Creature"))
		{
			m_showSaveDialog = true;
			m_popupMenuState = 0;
		}
		if (ImGui::MenuItem("Select All"))
		{
			g_global->SetAllConnectionActorSelect(actor);
		}
		if (ImGui::MenuItem("Lock", nullptr, false, isLockMenu))
		{
			for (auto id : g_global->m_selects)
				if (phys::sSyncInfo *sync = g_global->FindSyncInfo(id))
					if (sync && sync->actor)
						sync->actor->SetKinematic(true);
		}
		if (ImGui::MenuItem("Lock All", nullptr, false, isLockMenu))
		{
			// all connect actor lock
			g_global->SetAllConnectionActorKinematic(actor, true);
		}
		if (ImGui::MenuItem("Unlock", nullptr, false, isUnlockMenu))
		{
			for (auto id : g_global->m_selects)
				if (phys::sSyncInfo *sync = g_global->FindSyncInfo(id))
					if (sync && sync->actor)
						sync->actor->SetKinematic(false);
		}
		if (ImGui::MenuItem("Unlock All", nullptr, false, isUnlockMenu))
		{
			// all connect actor unlock
			g_global->UpdateAllConnectionActorDimension(actor, false);
		}
		ImGui::EndPopup();
	}
	else if (ImGui::BeginPopup("Spawn PopupMenu"))
	{
		if (ImGui::MenuItem("Locate SpawnPos"))
		{
			g_global->m_spawnTransform.pos.x = m_tempSpawnPos.x;
			g_global->m_spawnTransform.pos.z = m_tempSpawnPos.z;
		}
		ImGui::EndPopup();
	}
	else
	{
		m_popupMenuState = 0;
	}
}


void c3DView::RenderSaveDialog()
{
	if (!m_showSaveDialog)
		return;

	bool isOpen = true;
	const sf::Vector2u psize((uint)m_rect.Width(), (uint)m_rect.Height());
	const ImVec2 size(300, 110);
	const ImVec2 pos(psize.x / 2.f - size.x / 2.f
		, psize.y / 2.f - size.y / 2.f);
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowBgAlpha(0.9f);

	if (ImGui::Begin("Save Creature", &isOpen, 0))
	{
		ImGui::Spacing();
		ImGui::Text("FileName : ");
		ImGui::SameLine();

		static StrPath fileName("filename.pnt");
		ImGui::InputText("##fileName", fileName.m_str, fileName.SIZE);

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		if (ImGui::Button("Save"))
		{
			const StrPath filePath = StrPath("./media/creature/") + fileName;
			phys::sSyncInfo *sync = g_global->FindSyncInfo(m_saveFileSyncId);
			if (sync)
			{
				g_global->UpdateAllConnectionActorDimension(sync->actor, true);

				evc::WritePhenoTypeFileFrom_RigidActor(filePath, sync->actor);
			}
			m_showSaveDialog = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			m_showSaveDialog = false;
		}
	}
	ImGui::End();

	if (!isOpen)
		m_showSaveDialog = false;
}


void c3DView::RenderReflectionMap(graphic::cRenderer &renderer)
{
	m_reflectMap.SetRenderTarget(renderer);
	// clear gray.dds color
	renderer.ClearScene(false, Vector4(128.f / 255.f, 128.f / 255.f, 128.f / 255.f, 1));
	renderer.BeginScene();

	// lighting direction mirror
	GetMainLight().m_direction.y *= -1.f;
	GetMainLight().m_pos.y *= -1.f;
	GetMainLight().Bind(renderer);

	// Reflection plane in local space.
	Plane groundPlaneL(0, 1, 0, 0);

	// Reflection plane in world space.
	Matrix44 groundWorld;
	groundWorld.SetTranslate(Vector3(0, 0.0f, 0)); // ground height
	Matrix44 WInvTrans;
	WInvTrans = groundWorld.Inverse();
	WInvTrans.Transpose();
	Plane groundPlaneW = groundPlaneL * WInvTrans;
	float f[4] = { groundPlaneW.N.x, groundPlaneW.N.y, groundPlaneW.N.z, groundPlaneW.D };

	CommonStates states(renderer.GetDevice());
	memcpy(renderer.m_cbClipPlane.m_v->clipPlane, f, sizeof(f));
	renderer.GetDevContext()->RSSetState(states.CullClockwise());
	renderer.GetDevContext()->OMSetDepthStencilState(states.DepthDefault(), 0);

	m_groundPlane->SetRenderFlag(eRenderFlag::VISIBLE, false);
	RenderScene(renderer, "ShadowMap", false, groundPlaneW.GetReflectMatrix().GetMatrixXM());
	m_groundPlane->SetRenderFlag(eRenderFlag::VISIBLE, true);

	renderer.EndScene();
	m_reflectMap.RecoveryRenderTarget(renderer);

	renderer.GetDevContext()->RSSetState(states.CullCounterClockwise());
	//renderer.GetDevContext()->OMSetDepthStencilState(states.DepthDefault(), 0);

	float f2[4] = { 1,1,1,100000 }; // default clipplane always positive return
	memcpy(renderer.m_cbClipPlane.m_v->clipPlane, f2, sizeof(f2));

	// lighting direction mirror recovery
	GetMainLight().m_direction.y *= -1.f;
	GetMainLight().m_pos.y *= -1.f;
	GetMainLight().Bind(renderer);
}


void c3DView::OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect)
{
	if (type == eDockResize::DOCK_WINDOW)
	{
		m_owner->RequestResetDeviceNextFrame();
	}
}


// mouse picking process
bool c3DView::PickingProcess(const POINT &mousePos)
{
	// if pivot mode? ignore selection
	if ((eEditState::Pivot0 == g_global->m_state)
		|| (eEditState::Pivot1 == g_global->m_state)
		|| (eEditState::Revolute == g_global->m_state)
		|| (eEditState::SpawnLocation == g_global->m_state)
		|| g_global->m_gizmo.IsKeepEditMode()
		)
		return false;

	// picking rigidactor, joint
	const int syncId = PickingRigidActor(2, mousePos);

	phys::cPhysicsSync *physSync = g_global->m_physSync;
	phys::sSyncInfo *sync = physSync->FindSyncInfo(syncId);
	if (sync && sync->actor) // rigidactor picking?
	{
		if (::GetAsyncKeyState(VK_SHIFT)) // add picking
		{
			g_global->SelectObject(syncId);
		}
		else if (::GetAsyncKeyState(VK_CONTROL)) // toggle picking
		{
			g_global->SelectObject(syncId, true);
		}
		else
		{
			g_global->ClearSelection();
			g_global->SelectObject(syncId);
		}

		if ((g_global->m_selects.size() == 1)
			&& (g_global->m_gizmo.m_controlNode != sync->node))
		{
			g_global->m_gizmo.SetControlNode(sync->node);
		}
	}

	// joint picking
	if (sync && sync->joint)
	{
		g_global->ClearSelection();
		g_global->SelectObject(syncId);
		
		g_global->m_highLights.clear();
		if (phys::sSyncInfo *p = physSync->FindSyncInfo(sync->joint->m_actor0))
			g_global->m_highLights.insert(p->id);
		if (phys::sSyncInfo *p = physSync->FindSyncInfo(sync->joint->m_actor1))
			g_global->m_highLights.insert(p->id);

		g_global->m_state = eEditState::Revolute;
		g_global->m_gizmo.SetControlNode(sync->node);
		g_global->m_gizmo.LockEditType(graphic::eGizmoEditType::SCALE, true);
	}

	if (!sync) // no selection?
	{
		if (!g_global->m_gizmo.IsKeepEditMode()
			&& !::GetAsyncKeyState(VK_SHIFT) 
			&& !::GetAsyncKeyState(VK_CONTROL)
			)
		{
			g_global->m_gizmo.SetControlNode(nullptr);
			g_global->ClearSelection();
		}
		return false;
	}

	return true;
}


// picking rigidactor, joint
// return syncId
// if not found, return -1
// return distance
// pickType = 0: only actor
//			  1: only joint
//            2: actor + joint
int c3DView::PickingRigidActor(const int pickType, const POINT &mousePos
	, OUT float *outDistance //= nullptr
)
{
	const Ray ray = GetMainCamera().GetRay(mousePos.x, mousePos.y);

	int minId = -1;
	float minDist = FLT_MAX;
	phys::cPhysicsSync *sync = g_global->m_physSync;
	for (auto &p : sync->m_syncs)
	{
		if ((p->name == "plane") || (p->name == "wall"))
			continue; // ground or wall?

		const bool isSpherePicking = (p->actor && (p->actor->m_shape == phys::eShapeType::Sphere));

		graphic::cNode *node = p->node;
		float distance = FLT_MAX;
		if (node->Picking(ray, eNodeType::MODEL, isSpherePicking, &distance))
		{
			if (distance < minDist)
			{
				minId = p->id;
				minDist = distance;
				if (outDistance)
					*outDistance = distance;
			}
		}
	}

	return minId;
}


void c3DView::UpdateLookAt()
{
	GetMainCamera().MoveCancel();

	const float centerX = GetMainCamera().m_width / 2;
	const float centerY = GetMainCamera().m_height / 2;
	const Ray ray = GetMainCamera().GetRay((int)centerX, (int)centerY);
	const Plane groundPlane(Vector3(0, 1, 0), 0);
	const float distance = groundPlane.Collision(ray.dir);
	if (distance < -0.2f)
	{
		GetMainCamera().m_lookAt = groundPlane.Pick(ray.orig, ray.dir);
	}
	else
	{ // horizontal viewing
		const Vector3 lookAt = GetMainCamera().m_eyePos + GetMainCamera().GetDirection() * 5.f;
		GetMainCamera().m_lookAt = lookAt;
	}

	GetMainCamera().UpdateViewMatrix();
}


// 휠을 움직였을 때,
// 카메라 앞에 박스가 있다면, 박스 정면에서 멈춘다.
void c3DView::OnWheelMove(const float delta, const POINT mousePt)
{
	if (m_showSaveDialog || (m_popupMenuState > 0))
		return;

	UpdateLookAt();

	float len = 0;
	const Plane groundPlane(Vector3(0, 1, 0), 0);
	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
	Vector3 lookAt = groundPlane.Pick(ray.orig, ray.dir);
	len = min(50.f, (ray.orig - lookAt).Length());

	const int lv = 10;
	const float zoomLen = min(len * 0.1f, (float)(2 << (16 - lv)));

	GetMainCamera().Zoom(ray.dir, (delta < 0) ? -zoomLen : zoomLen);
}


// Handling Mouse Move Event
void c3DView::OnMouseMove(const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	if (g_global->m_gizmo.IsKeepEditMode())
		return;
	if (m_showSaveDialog || (m_popupMenuState > 0))
		return;

	// joint pivot setting mode
	if ((eEditState::Pivot0 == g_global->m_state)
		|| (eEditState::Pivot1 == g_global->m_state)
		&& !g_global->m_selects.empty()
		&& g_global->m_selJoint)
	{
		for (int selId : g_global->m_selects)
		{
			// picking all rigidbody
			float distance = 0.f;
			const int syncId = PickingRigidActor(0, mousePt, &distance);
			phys::cPhysicsSync *phySync = g_global->m_physSync;
			phys::sSyncInfo *sync = phySync->FindSyncInfo(syncId);
			if (sync && (syncId == selId))
			{
				const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
				m_pivotPos = ray.dir * distance + ray.orig;
			}
		}
	}

	if (m_mouseDown[0])
	{
		Vector3 dir = GetMainCamera().GetDirection();
		Vector3 right = GetMainCamera().GetRight();
		dir.y = 0;
		dir.Normalize();
		right.y = 0;
		right.Normalize();

		GetMainCamera().MoveRight(-delta.x * m_rotateLen * 0.001f);
		GetMainCamera().MoveFrontHorizontal(delta.y * m_rotateLen * 0.001f);
	}
	else if (m_mouseDown[1])
	{
		const float scale = 0.003f;
		if (m_orbitTarget.Distance(GetMainCamera().GetEyePos()) > 50.f)
		{
			// cancel orbit moving
			m_isOrbitMove = false;
		}

		if (m_isOrbitMove)
		{
			m_camera.Yaw3(delta.x * scale, m_orbitTarget);
			m_camera.Pitch3(delta.y * scale, m_orbitTarget);
		}
		else
		{
			m_camera.Yaw2(delta.x * scale, Vector3(0, 1, 0));
			m_camera.Pitch2(delta.y * scale, Vector3(0, 1, 0));
		}
	}
	else if (m_mouseDown[2])
	{
		const float len = GetMainCamera().GetDistance();
		GetMainCamera().MoveRight(-delta.x * len * 0.001f);
		GetMainCamera().MoveUp(delta.y * len * 0.001f);
	}
}


// Handling Mouse Button Down Event
void c3DView::OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt)
{
	m_mousePos = mousePt;
	m_mouseClickPos = mousePt;
	UpdateLookAt();
	SetCapture();

	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
	const Plane groundPlane(Vector3(0, 1, 0), 0);
	const Vector3 target = groundPlane.Pick(ray.orig, ray.dir);
	m_rotateLen = ray.orig.y * 0.9f;// (target - ray.orig).Length();

	if (m_showSaveDialog || (m_popupMenuState > 0))
		return;

	switch (button)
	{
	case sf::Mouse::Left:
	{
		m_mouseDown[0] = true;

		// joint pivot setting mode
		if (((eEditState::Pivot0 == g_global->m_state)
			|| (eEditState::Pivot1 == g_global->m_state))
			&& !g_global->m_selects.empty()
			&& g_global->m_selJoint)
		{
			phys::cJoint *selJoint = g_global->m_selJoint;
			for (int selId : g_global->m_selects)
			{
				// picking all rigidbody
				float distance = 0.f;
				const int syncId = PickingRigidActor(0, mousePt, &distance);
				phys::cPhysicsSync *phySync = g_global->m_physSync;
				phys::sSyncInfo *sync = phySync->FindSyncInfo(syncId);
				if (sync && (selId == syncId))
				{
					const Vector3 pivotPos = ray.dir * distance + ray.orig;
					m_pivotPos = pivotPos;

					// update pivot position
					cJointRenderer *jointRenderer = g_global->FindJointRenderer(selJoint);
					if (jointRenderer)
					{
						// find sync0 or sync1 picking
						if (sync == jointRenderer->m_sync0) // sync0?
						{
							jointRenderer->SetPivotPos(0, pivotPos);
						}
						else if (sync == jointRenderer->m_sync1)// sync1?
						{
							jointRenderer->SetPivotPos(1, pivotPos);
						}
						else
						{
							assert(0);
						}
					}
				}//~sync
			}//~selects
		} //~joint pivot setting mode

		if (eEditState::SpawnLocation == g_global->m_state)
		{
			g_global->m_spawnTransform.pos = target;
			g_global->m_state = eEditState::Normal;
		}

	}//~case
	break;

	case sf::Mouse::Right:
	{
		m_mouseDown[1] = true;

		// orbit moving
		// select target object center orbit moving
		const int syncId = PickingRigidActor(0, mousePt);
		phys::sSyncInfo *sync = g_global->FindSyncInfo(syncId);
		if (sync && sync->node)
		{
			m_isOrbitMove = true;
			m_orbitTarget = sync->node->m_transform.pos;
			g_global->m_highLights.clear();
			g_global->m_highLights.insert(syncId);
		}
	}
	break;

	case sf::Mouse::Middle:
		m_mouseDown[2] = true;
		break;
	}
}


void c3DView::OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	ReleaseCapture();

	if (m_showSaveDialog || (m_popupMenuState > 0))
		return;

	switch (button)
	{
	case sf::Mouse::Left:
	{
		if (m_mouseDown[0]) // mouse down -> up event?
			PickingProcess(mousePt);

		m_mouseDown[0] = false;
	}
	break;


	case sf::Mouse::Right:
	{
		m_mouseDown[1] = false;

		const int dx = m_mouseClickPos.x - mousePt.x;
		const int dy = m_mouseClickPos.y - mousePt.y;
		if (sqrt(dx*dx + dy * dy) > 10)
			break; // move long distance, do not show popup menu

		// check show menu to joint connection
		if ((eEditState::Pivot0 == g_global->m_state)
			|| (eEditState::Pivot1 == g_global->m_state)
			|| (eEditState::Revolute == g_global->m_state)
			|| (eEditState::SpawnLocation == g_global->m_state))
			break;

		const int syncId = PickingRigidActor(0, mousePt);
		if (syncId >= 0)
		{
			m_popupMenuState = 1; // open popup menu
			m_popupMenuType = 0; // actor menu
			m_saveFileSyncId = syncId;
			//g_global->ClearSelection();
			g_global->SelectObject(syncId);
			if (phys::sSyncInfo *sync = g_global->FindSyncInfo(syncId))
			{
				//g_global->m_gizmo.SetControlNode(sync->node);
				g_global->m_gizmo.m_type = eGizmoEditType::None;
			}
		}
		else
		{
			// spawn pos popupmenu
			// picking ground?
			const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
			const Plane groundPlane(Vector3(0, 1, 0), 0);
			const Vector3 target = groundPlane.Pick(ray.orig, ray.dir);
			//m_rotateLen = (target - ray.orig).Length();

			m_popupMenuState = 1; // open popup menu
			m_popupMenuType = 1; // spawn selection menu
			m_tempSpawnPos = target;
		}
	}
	break;

	case sf::Mouse::Middle:
		m_mouseDown[2] = false;
		break;
	}
}


void c3DView::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		if ((m_owner->GetFocus() != this)
			&& (m_owner->GetFocus() != (framework::cDockWindow*)g_global->m_resourceView))
			break;

		switch (evt.key.cmd)
		{
		case sf::Keyboard::Return:
			break;

		case sf::Keyboard::Space:
			break;

		case sf::Keyboard::R: g_global->m_gizmo.m_type = graphic::eGizmoEditType::ROTATE; break;
		case sf::Keyboard::T: g_global->m_gizmo.m_type = graphic::eGizmoEditType::TRANSLATE; break;
		case sf::Keyboard::S: g_global->m_gizmo.m_type = graphic::eGizmoEditType::SCALE; break;
		case sf::Keyboard::H: g_global->m_gizmo.m_type = graphic::eGizmoEditType::None; break;
		case sf::Keyboard::F5: g_global->RefreshResourceView(); break;

		case sf::Keyboard::Escape:
			m_isOrbitMove = false;

			if (m_popupMenuState > 0)
			{
				m_popupMenuState = 0;
			}
			else if (m_showSaveDialog)
			{
				m_showSaveDialog = false;
			}
			else
			{
				// clear selection
				g_global->m_state = eEditState::Normal;
				g_global->m_gizmo.SetControlNode(nullptr);
				g_global->m_gizmo.LockEditType(graphic::eGizmoEditType::SCALE, false);
				g_global->m_selJoint = nullptr;
				g_global->ClearSelection();
			}
			break;

		case sf::Keyboard::C:
		{
			// copy
			if (::GetAsyncKeyState(VK_CONTROL))
			{
				if (!g_global->m_selects.empty())
				{
					phys::sSyncInfo *sync = g_global->FindSyncInfo(*g_global->m_selects.begin());
					if (sync)
					{
						g_global->UpdateAllConnectionActorDimension(sync->actor, true);

						vector<phys::cRigidActor*> actors;
						for (auto &id : g_global->m_selects)
							if (phys::sSyncInfo *sync = g_global->FindSyncInfo(id))
								if (sync->actor)
									actors.push_back(sync->actor);
						evc::WritePhenoTypeFileFrom_RigidActor("tmp.pnt", actors);
					}
				}
			}
		}
		break;

		case sf::Keyboard::V:
		{
			// paste
			if (::GetAsyncKeyState(VK_CONTROL))
			{
				vector<int> syncIds;
				evc::ReadPhenoTypeFile(g_global->GetRenderer(), "tmp.pnt", &syncIds);
				if (syncIds.empty())
					break;

				// moving actor position to camera center
				if (phys::sSyncInfo *sync = g_global->FindSyncInfo(syncIds[0]))
				{
					const Ray ray = m_camera.GetRay();
					const Vector3 spawnPos = ray.orig + ray.dir * 8.f - sync->node->m_transform.pos;

					for (auto &id : g_global->m_selects)
					{
						phys::sSyncInfo *sync = g_global->FindSyncInfo(id);
						if (!sync && !sync->node)
							continue;

						sync->node->m_transform.pos += spawnPos;

						if (sync->actor)
						{
							sync->actor->SetGlobalPose( physx::PxTransform(
									*(physx::PxVec3*)&sync->node->m_transform.pos
									, *(physx::PxQuat*)&sync->node->m_transform.rot));
						}
					}
				}//~if FindSyncInfo()
			}//~VK_CONTROL
		}
		break;

		}
		break;

	case sf::Event::MouseMoved:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		OnMouseMove(pos);
	}
	break;

	case sf::Event::MouseButtonPressed:
	case sf::Event::MouseButtonReleased:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		const sRectf viewRect = GetWindowSizeAvailible(true);

		if (sf::Event::MouseButtonPressed == evt.type)
		{
			if (viewRect.IsIn((float)curPos.x, (float)curPos.y))
				OnMouseDown(evt.mouseButton.button, pos);
		}
		else
		{
			// 화면밖에 마우스가 있더라도 Capture 상태일 경우 Up 이벤트는 받게한다.
			if (viewRect.IsIn((float)curPos.x, (float)curPos.y)
				|| (this == GetCapture()))
				OnMouseUp(evt.mouseButton.button, pos);
		}
	}
	break;

	case sf::Event::MouseWheelScrolled:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		OnWheelMove(evt.mouseWheelScroll.delta, pos);
	}
	break;

	case sf::Event::Gestured:
	{
		POINT curPos = { evt.gesture.x, evt.gesture.y };
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
	}
	break;

	}
}


void c3DView::OnResetDevice()
{
	cRenderer &renderer = GetRenderer();

	// update viewport
	sRectf viewRect = { 0, 0, m_rect.Width() - 15, m_rect.Height() - 50 };
	m_camera.SetViewPort(viewRect.Width(), viewRect.Height());

	cViewport vp = GetRenderer().m_viewPort;
	vp.m_vp.Width = viewRect.Width();
	vp.m_vp.Height = viewRect.Height();
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, DXGI_FORMAT_D24_UNORM_S8_UINT);
}
