
#include "stdafx.h"
#include "3dview.h"
#include "phenoeditorview.h"

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

	// reflection map
	{
		cViewport rflvp;
		rflvp.Create(0, 0, 1024, 1024, 0.f, 1.f);
		m_reflectMap.Create(renderer, rflvp, DXGI_FORMAT_R8G8B8A8_UNORM, false);

		m_reflectShader.Create(renderer, "media/shader11/pos-norm-tex-reflect.fxo", "Unlit"
			, eVertexType::POSITION | eVertexType::NORMAL | eVertexType::TEXTURE0);
		renderer.m_cbClipPlane.m_v->reflectAlpha[0] = 0.2f;

		// setting reflection map
		phys::sSyncInfo *sync = g_pheno->FindSyncInfo(g_pheno->m_groundGridPlaneId);
		if (sync)
		{
			m_groundPlane = dynamic_cast<graphic::cGrid*>(sync->node);
			if (m_groundPlane)
			{
				m_groundPlane->SetShader(&m_reflectShader);
				m_groundPlane->m_reflectionMap = &m_reflectMap;
			}
		}
	}

	return true;
}


void c3DView::OnUpdate(const float deltaSeconds)
{
	g_global->m_physics.PreUpdate(deltaSeconds);

	for (auto &p : g_pheno->m_physSync->m_syncs)
		if (p->joint)
			p->joint->Update(deltaSeconds);
}


void c3DView::OnPreRender(const float deltaSeconds)
{
	cRenderer &renderer = GetRenderer();
	phys::cPhysicsSync *physSync = g_pheno->m_physSync;

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
	if (!g_pheno->m_selects.empty() || !g_pheno->m_highLights.empty())
	{
		m_depthBuff.Begin(renderer);
		RenderSelectModel(renderer, true, XMIdentity);
		m_depthBuff.End(renderer);
	}

	const bool isShowGizmo = (g_pheno->m_selects.size() >= 1)
		&& !((::GetFocus() == m_owner->getSystemHandle()) && ::GetAsyncKeyState(VK_SHIFT))
		&& !((::GetFocus() == m_owner->getSystemHandle()) && ::GetAsyncKeyState(VK_CONTROL))
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

		renderer.GetDevContext()->RSSetState(renderer.m_renderState.CullCounterClockwise());

		if (m_showShadow)
			m_ccsm.Bind(renderer);
		if (m_showReflection)
			m_reflectMap.Bind(renderer, 8);

		RenderScene(renderer, "ShadowMap", false);
		RenderEtc(renderer);

		if (isShowGizmo)
			isGizmoEdit = g_pheno->m_gizmo.Render(renderer, deltaSeconds, m_mousePos, m_mouseDown[0]);

		// render gizmo position
		if (isShowGizmo)
		{
			const Transform &tfm = g_pheno->m_gizmo.m_targetTransform;
			renderer.m_dbgLine.m_isSolid = true;
			renderer.m_dbgLine.SetColor(cColor::BLACK);
			renderer.m_dbgLine.SetLine(tfm.pos, Vector3(tfm.pos.x, 0, tfm.pos.z), 0.01f);
			renderer.m_dbgLine.Render(renderer);
		}

		// render spawn position
		{
			Vector3 spawnPos = g_pheno->m_spawnTransform.pos;
			spawnPos.y = 0.f;

			renderer.m_dbgCube.SetColor(cColor::GREEN);
			renderer.m_dbgCube.SetCube(Transform(spawnPos, Vector3::Ones*0.2f));
			renderer.m_dbgCube.Render(renderer);
		}

		// render reserve spawn position (to show popupmenu selection)
		if ((m_popupMenuState == 2) && (m_popupMenuType == 1))
		{
			renderer.m_dbgCube.SetColor(cColor(0.f, 0.f, 1.f, 0.7f));
			renderer.m_dbgCube.SetCube(Transform(m_tempSpawnPos, Vector3(0.05f,0.5f,0.05f)));
			renderer.m_dbgCube.Render(renderer);
		}

		renderer.RenderAxis2();
	}
	m_renderTarget.End(renderer);
	renderer.UnbindTextureAll();

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
	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
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

	// joint edit mode?
	// hight revolute joint when mouse hovering
	if ((g_pheno->GetEditMode() == ePhenoEditMode::Revolute)
		|| (g_pheno->GetEditMode() == ePhenoEditMode::JointEdit))
	{
		const Ray ray = GetMainCamera().GetRay((int)m_mousePos.x, (int)m_mousePos.y);
		g_pheno->m_uiJointRenderer.m_highlightRevoluteJoint =
			g_pheno->m_uiJointRenderer.Picking(ray, eNodeType::MODEL);
	}
}


// render etc
void c3DView::RenderEtc(graphic::cRenderer &renderer)
{
	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	RET(!physSync);

	// render pivot position
	if ((ePhenoEditMode::Pivot0 == g_pheno->GetEditMode())
		|| (ePhenoEditMode::Pivot1 == g_pheno->GetEditMode()))
	{
		Transform tfm(m_pivotPos, Vector3::Ones*0.05f);
		renderer.m_dbgBox.SetColor(cColor::RED);
		renderer.m_dbgBox.SetBox(tfm);
		renderer.m_dbgBox.Render(renderer);
	}
}


// Render Outline Selected model
// buildOutline : true, render depth buffer
//				  false, general render
void c3DView::RenderSelectModel(graphic::cRenderer &renderer, const bool buildOutline
	, const XMMATRIX &tm)
{
	if (g_pheno->m_selects.empty() && g_pheno->m_highLights.empty())
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

	phys::cPhysicsSync *physSync = g_pheno->m_physSync;

	if (!buildOutline)
		renderer.GetDevContext()->OMSetDepthStencilState(renderer.m_renderState.DepthNone(), 0);
	renderer.m_cbPerFrame.m_v->outlineColor = Vector4(0.8f, 0, 0, 1).GetVectorXM();
	for (auto syncId : g_pheno->m_selects)
	{
		if (phys::sSyncInfo *sync = physSync->FindSyncInfo(syncId))
			render(sync);
	}
	renderer.m_cbPerFrame.m_v->outlineColor = Vector4(0.f, 1.f, 0.f, 1).GetVectorXM();
	for (auto syncId : g_pheno->m_highLights)
	{
		if (phys::sSyncInfo *sync = physSync->FindSyncInfo(syncId))
			render(sync);
	}
	if (!buildOutline)
		renderer.GetDevContext()->OMSetDepthStencilState(renderer.m_renderState.DepthDefault(), 0);
}


// Modify rigid actor transform by Gizmo
// process before g_pheno->m_physics.PostUpdate()
void c3DView::UpdateSelectModelTransform(const bool isGizmoEdit)
{
	phys::cPhysicsSync *physSync = g_pheno->m_physSync;

	const bool isShowGizmo = (g_pheno->m_selects.size() != 0);
	if (!isShowGizmo || !isGizmoEdit)
		return;
	if (!g_pheno->m_gizmo.m_controlNode)
		return;

	if (g_pheno->m_selects.size() == 1)
	{
		const int selectSyncId = *g_pheno->m_selects.begin();
		phys::sSyncInfo *sync = physSync->FindSyncInfo(selectSyncId);
		if (!sync)
			return;

		if (sync->actor)
			UpdateSelectModelTransform_RigidActor();
		else if (sync->joint)
			UpdateSelectModelTransform_Joint();
	}
	else if (g_pheno->m_selects.size() > 1)
	{
		UpdateSelectModelTransform_MultiObject();
	}
}


// Modify rigid actor transform by Gizmo, rigid actor
void c3DView::UpdateSelectModelTransform_RigidActor()
{
	using namespace physx;
	phys::cPhysicsSync *physSync = g_pheno->m_physSync;

	const int selectSyncId = (g_pheno->m_selects.size() == 1) ? *g_pheno->m_selects.begin() : -1;
	phys::sSyncInfo *sync = physSync->FindSyncInfo(selectSyncId);
	if (sync->actor->m_type != phys::eRigidType::Dynamic) // dynamic actor?
		return;

	Transform temp = g_pheno->m_gizmo.m_targetTransform;
	temp.pos.y = max(0.f, temp.pos.y);
	const Transform tfm = temp;

	sync->actor->SetGlobalPose(tfm);

	// change dimension? apply wakeup
	if (eGizmoEditType::SCALE == g_pheno->m_gizmo.m_type)
	{
		sync->actor->SetKinematic(true); // stop simulation

		// rigidactor dimension change
		g_pheno->m_gizmo.m_controlNode->m_transform.scale = tfm.scale;

		// RigidActor Scale change was delayed, change when wakeup time
		// store change value
		// change 3d model dimension
		switch (sync->actor->m_shape)
		{
		case phys::eShapeType::Box:
		{
			g_pheno->ModifyRigidActorTransform(selectSyncId, tfm.scale);
		}
		break;
		case phys::eShapeType::Sphere:
		{
			float scale = 1.f;
			switch (g_pheno->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: scale = sync->node->m_transform.scale.x; break;
			case eGizmoEditAxis::Y: scale = sync->node->m_transform.scale.y; break;
			case eGizmoEditAxis::Z: scale = sync->node->m_transform.scale.z; break;
			}

			((cSphere*)sync->node)->SetRadius(scale); // update radius transform
			g_pheno->m_gizmo.UpdateTargetTransform(sync->node->m_transform); // update gizmo
			g_pheno->ModifyRigidActorTransform(selectSyncId, Vector3(scale, 1, 1));
		}
		break;
		case phys::eShapeType::Capsule:
		{
			const Vector3 scale = sync->node->m_transform.scale;
			float radius = 1.f;
			switch (g_pheno->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: radius = scale.y; break;
			case eGizmoEditAxis::Y: radius = scale.y; break;
			case eGizmoEditAxis::Z: radius = scale.z; break;
			}

			const float halfHeight = scale.x - radius;

			((cCapsule*)sync->node)->SetDimension(radius, halfHeight); // update capsule transform
			g_pheno->m_gizmo.UpdateTargetTransform(sync->node->m_transform); // update gizmo
			g_pheno->ModifyRigidActorTransform(selectSyncId, Vector3(halfHeight, radius, radius));
		}
		break;
		case phys::eShapeType::Cylinder:
		{
			const Vector3 scale = sync->node->m_transform.scale;
			float radius = 1.f;
			switch (g_pheno->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: radius = scale.y; break;
			case eGizmoEditAxis::Y: radius = scale.y; break;
			case eGizmoEditAxis::Z: radius = scale.z; break;
			}

			const float height = scale.x * 2.f;

			((cCylinder*)sync->node)->SetDimension(radius, height); // update capsule transform
			g_pheno->m_gizmo.UpdateTargetTransform(sync->node->m_transform); // update gizmo
			g_pheno->ModifyRigidActorTransform(selectSyncId, Vector3(height, radius, radius));
		}
		break;
		}
	}
}


// update gizmo transform to multi selection object
void c3DView::UpdateSelectModelTransform_MultiObject()
{
	using namespace physx;
	phys::cPhysicsSync *physSync = g_pheno->m_physSync;

	const Vector3 realCenter = g_pheno->m_multiSelPos - Vector3(0, 0.5f, 0);
	const Vector3 center = g_pheno->m_multiSelPos;
	const Vector3 offset = g_pheno->m_multiSel.m_transform.pos - center;
	g_pheno->m_multiSelPos = g_pheno->m_multiSel.m_transform.pos; // update current position

	const Quaternion rot = g_pheno->m_multiSel.m_transform.rot * g_pheno->m_multiSelRot.Inverse();
	g_pheno->m_multiSelRot = g_pheno->m_multiSel.m_transform.rot; // update current rotation

	// update
	for (auto id : g_pheno->m_selects)
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
		sync->actor->SetGlobalPose(tfm); // update physics transform
	}
}


// Modify rigid actor transform by Gizmo, joint
void c3DView::UpdateSelectModelTransform_Joint()
{
	using namespace physx;
	phys::cPhysicsSync *physSync = g_pheno->m_physSync;

	const int selectSyncId = (g_pheno->m_selects.size() == 1) ? *g_pheno->m_selects.begin() : -1;
	phys::sSyncInfo *sync = physSync->FindSyncInfo(selectSyncId);

	cJointRenderer *jointRenderer = dynamic_cast<cJointRenderer*>(sync->node);
	if (!jointRenderer)
		return; // error occurred

	//g_pheno->m_gizmo;
	switch (g_pheno->m_gizmo.m_type)
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
			ImGui::TextUnformatted("Orbit");
			ImGui::PopStyleColor();
		}

		ImGui::End();
	}
	ImGui::PopStyleColor();

	RenderPopupMenu();
	RenderTooltip();
	RenderSaveDialog();
}


void c3DView::RenderPopupMenu()
{
	if (m_popupMenuState == 1)
	{
		switch (m_popupMenuType)
		{
		case 0: ImGui::OpenPopup("Actor PopupMenu"); break;
		case 1: ImGui::OpenPopup("New PopupMenu"); break;
		default: assert(0); break;
		}
		m_popupMenuState = 2;
	}

	if (ImGui::BeginPopup("Actor PopupMenu"))
	{
		if ((m_popupMenuState == 3) || g_pheno->m_selects.empty())
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		bool isLockMenu = false;
		bool isUnlockMenu = false;

		// check menu enable
		int cntKinematic[2] = { 0, 0 }; // kinematic, unkinematic
		for (auto id : g_pheno->m_selects)
			if (phys::cRigidActor *actor = g_pheno->FindRigidActorFromSyncId(id))
				cntKinematic[actor->IsKinematic()]++;

		if (cntKinematic[0] > 0)
			isLockMenu = true;
		if (cntKinematic[1] > 0)
			isUnlockMenu = true;

		phys::cRigidActor *actor = g_pheno->FindRigidActorFromSyncId(*g_pheno->m_selects.begin());
		if (!actor)
		{
			ImGui::EndPopup();
			return;
		}

		if (ImGui::MenuItem("Save Creature"))
		{
			m_showSaveDialog = true;
			m_isSaveOnlySelectionActor = false;
			m_popupMenuState = 0;
		}
		if (ImGui::MenuItem("Select All", "A"))
		{
			g_pheno->SetAllConnectionActorSelect(actor);
		}
		if (ImGui::MenuItem("Lock", "L", false, isLockMenu))
		{
			for (auto id : g_pheno->m_selects)
				if (phys::cRigidActor *actor = g_pheno->FindRigidActorFromSyncId(id))
					actor->SetKinematic(true);
		}
		if (ImGui::MenuItem("Lock All", nullptr, false, isLockMenu))
		{
			// all connect actor lock
			g_pheno->SetAllConnectionActorKinematic(actor, true);
		}
		if (ImGui::MenuItem("Unlock", "U", false, isUnlockMenu))
		{
			for (auto id : g_pheno->m_selects)
				if (phys::cRigidActor *actor = g_pheno->FindRigidActorFromSyncId(id))
					g_pheno->UpdateActorDimension(actor, false);
		}
		if (ImGui::MenuItem("Unlock All", nullptr, false, isUnlockMenu))
		{
			// all connect actor unlock
			g_pheno->UpdateAllConnectionActorDimension(actor, false);
		}
		
		ImGui::Separator();

		if (ImGui::MenuItem("Delete All Joint", "J", false, true))
		{
			// remove connection joint
			set<phys::cJoint*> rms;
			for (auto id : g_pheno->m_selects)
				if (phys::cRigidActor *actor = g_pheno->FindRigidActorFromSyncId(id))
					for (auto &j : actor->m_joints)
						rms.insert(j);

			vector<phys::cRigidActor*> wakeups;
			for (auto *j : rms)
			{
				wakeups.push_back(j->m_actor0);
				wakeups.push_back(j->m_actor1);
				g_pheno->m_physSync->RemoveSyncInfo(j);
			}

			// wakeup
			for (auto *actor : wakeups)
				if (actor)
					actor->WakeUp();
		}
		ImGui::EndPopup();
	}
	else if (ImGui::BeginPopup("New PopupMenu"))
	{
		if (ImGui::BeginMenu("New"))
		{
			if (ImGui::MenuItem("Box"))
			{
				g_pheno->SpawnBox(m_tempSpawnPos);
			}
			if (ImGui::MenuItem("Sphere"))
			{
				g_pheno->SpawnSphere(m_tempSpawnPos);
			}
			if (ImGui::MenuItem("Capsule"))
			{
				g_pheno->SpawnCapsule(m_tempSpawnPos);
			}
			if (ImGui::MenuItem("Cylinder"))
			{
				g_pheno->SpawnCylinder(m_tempSpawnPos);
			}
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Locate SpawnPos"))
		{
			g_pheno->m_spawnTransform.pos.x = m_tempSpawnPos.x;
			g_pheno->m_spawnTransform.pos.z = m_tempSpawnPos.z;
		}
		ImGui::EndPopup();
	}
	else
	{
		m_popupMenuState = 0;
	}
}


void c3DView::RenderTooltip()
{
	if (g_pheno->m_gizmo.IsKeepEditMode())
	{
		ImGui::SetNextWindowBgAlpha(0.7f);
		ImGui::BeginTooltip();
		const Transform &tm = g_pheno->m_gizmo.m_targetTransform;
		switch (g_pheno->m_gizmo.m_type)
		{
		case eGizmoEditType::TRANSLATE:
			ImGui::Text("x=%0.2f y=%0.2f z=%0.2f", tm.pos.x, tm.pos.y, tm.pos.z);
			break;
		case eGizmoEditType::SCALE:
			ImGui::Text("x=%0.2f y=%0.2f z=%0.2f", tm.scale.x, tm.scale.y, tm.scale.z);
			break;
		case eGizmoEditType::ROTATE:
		{
			Vector3 rpy = tm.rot.Euler();
			rpy = Vector3(RAD2ANGLE(rpy.x), RAD2ANGLE(rpy.y), RAD2ANGLE(rpy.z));
			ImGui::Text("x=%0.2f y=%0.2f z=%0.2f", rpy.x, rpy.y, rpy.z);
		}
		break;
		}
		ImGui::EndTooltip();
	}
}


void c3DView::RenderSaveDialog()
{
	if (!m_showSaveDialog)
		return;

	bool isOpen = true;
	const sf::Vector2u psize((uint)m_rect.Width(), (uint)m_rect.Height());
	const ImVec2 size(300, 135);
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

		bool isSave = false;
		const int flags = ImGuiInputTextFlags_AutoSelectAll 
			| ImGuiInputTextFlags_EnterReturnsTrue;
		if (ImGui::InputText("##fileName", fileName.m_str, fileName.SIZE, flags))
		{
			isSave = true;
		}

		ImGui::SetCursorPosX(100);
		ImGui::Checkbox("Save Only Selection Actor", &m_isSaveOnlySelectionActor);

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		const bool isSaveBtnClick = ImGui::Button("Save");
		if (isSaveBtnClick || isSave)
		{
			const StrPath filePath = StrPath("./media/creature/") + fileName;
			if (phys::sSyncInfo *sync = g_pheno->FindSyncInfo(m_saveFileSyncId))				
			{
				bool isSave = true;
				if (filePath.IsFileExist()) // file already exist?
				{
					isSave = false;

					Str128 text;
					text.Format("[ %s ] File Already Exist\nOverWrite?"
						, filePath.c_str());
					if (IDYES == ::MessageBoxA(m_owner->getSystemHandle(), text.c_str()
						, "Confirm", MB_YESNO | MB_ICONWARNING))
					{
						isSave = true;
					}
				}

				if (isSave)
				{
					g_pheno->UpdateAllConnectionActorDimension(sync->actor, true);

					if (m_isSaveOnlySelectionActor)
					{
						vector<phys::cRigidActor*> actors;
						for (auto &id : g_pheno->m_selects)
							if (phys::cRigidActor *actor = g_pheno->FindRigidActorFromSyncId(id))
								actors.push_back(actor);
						for (auto *a : actors)
							g_pheno->UpdateActorDimension(a, true);

						evc::WritePhenoTypeFileFrom_RigidActor(filePath, actors);
					}
					else
					{
						evc::WritePhenoTypeFileFrom_RigidActor(filePath, sync->actor);
					}
				}
			}
			m_showSaveDialog = false;
		} //~save operation

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
	// clear gray.dds color
	m_reflectMap.Begin(renderer
		, Vector4(128.f / 255.f, 128.f / 255.f, 128.f / 255.f, 1));

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

	m_reflectMap.End(renderer);

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
	if ((ePhenoEditMode::Pivot0 == g_pheno->GetEditMode())
		|| (ePhenoEditMode::Pivot1 == g_pheno->GetEditMode())
		|| (ePhenoEditMode::Revolute == g_pheno->GetEditMode())
		|| (ePhenoEditMode::SpawnLocation == g_pheno->GetEditMode())
		|| g_pheno->m_gizmo.IsKeepEditMode()
		)
		return false;

	// picking rigidactor, joint
	const int syncId = PickingRigidActor(2, mousePos);

	phys::cPhysicsSync *physSync = g_pheno->m_physSync;
	phys::sSyncInfo *sync = physSync->FindSyncInfo(syncId);
	if (sync && sync->actor) // rigidactor picking?
	{
		if (g_pheno->m_fixJointSelection)
			return false; // not possible

		if (::GetAsyncKeyState(VK_SHIFT)) // add picking
		{
			g_pheno->SelectObject(syncId);
		}
		else if (::GetAsyncKeyState(VK_CONTROL)) // toggle picking
		{
			g_pheno->SelectObject(syncId, true);
		}
		else
		{
			g_pheno->ClearSelection();
			g_pheno->SelectObject(syncId);
		}

		if ((g_pheno->m_selects.size() == 1)
			&& (g_pheno->m_gizmo.m_controlNode != sync->node))
		{
			g_pheno->m_gizmo.SetControlNode(sync->node);
		}
	}

	// joint picking
	if (sync && sync->joint)
	{
		g_pheno->ClearSelection();
		g_pheno->SelectObject(syncId);
		
		g_pheno->m_highLights.clear();
		if (phys::sSyncInfo *p = physSync->FindSyncInfo(sync->joint->m_actor0))
			g_pheno->m_highLights.insert(p->id);
		if (phys::sSyncInfo *p = physSync->FindSyncInfo(sync->joint->m_actor1))
			g_pheno->m_highLights.insert(p->id);

		g_pheno->ChangeEditMode(ePhenoEditMode::Revolute);
		g_pheno->m_gizmo.SetControlNode(sync->node);
		g_pheno->m_gizmo.LockEditType(graphic::eGizmoEditType::SCALE, true);
	}

	if (!sync) // no selection?
	{
		if (g_pheno->m_fixJointSelection)
			return false; // no selection ignore

		if (!g_pheno->m_gizmo.IsKeepEditMode()
			&& !::GetAsyncKeyState(VK_SHIFT) 
			&& !::GetAsyncKeyState(VK_CONTROL)
			)
		{
			g_pheno->m_gizmo.SetControlNode(nullptr);
			g_pheno->ClearSelection();
			g_pheno->ChangeEditMode(ePhenoEditMode::Normal);
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
//			  1: only joint (only ui joint)
//            2: actor + joint (only ui joint)
int c3DView::PickingRigidActor(const int pickType, const POINT &mousePos
	, OUT float *outDistance //= nullptr
)
{
	const Ray ray = GetMainCamera().GetRay(mousePos.x, mousePos.y);

	int minId = -1;
	float minDist = FLT_MAX;
	phys::cPhysicsSync *sync = g_pheno->m_physSync;
	for (auto &p : sync->m_syncs)
	{
		if ((p->name == "plane") || (p->name == "wall"))
			continue; // ground or wall?

		if ((pickType == 0) && p->joint)
			continue; // ignore joint
		if ((pickType == 1) && p->actor)
			continue; // ignore actor
		if (((pickType == 1) || (pickType == 2)) && p->joint)
			if (p->joint != &g_pheno->m_uiJoint)
				continue; // picking only ui joint

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
	if (g_pheno->m_gizmo.IsKeepEditMode())
		return;
	if (m_showSaveDialog || (m_popupMenuState > 0))
		return;

	// joint pivot setting mode
	if ((ePhenoEditMode::Pivot0 == g_pheno->GetEditMode())
		|| (ePhenoEditMode::Pivot1 == g_pheno->GetEditMode())
		&& !g_pheno->m_selects.empty()
		&& g_pheno->m_selJoint)
	{
		for (int selId : g_pheno->m_selects)
		{
			// picking all rigidbody
			float distance = 0.f;
			const int syncId = PickingRigidActor(0, mousePt, &distance);
			phys::cPhysicsSync *phySync = g_pheno->m_physSync;
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
		if (m_orbitTarget.Distance(GetMainCamera().GetEyePos()) > 25.f)
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

	// active genotype editor view
	m_owner->SetActiveWindow((framework::cDockWindow*)g_global->m_peditorView);

	switch (button)
	{
	case sf::Mouse::Left:
	{
		m_mouseDown[0] = true;

		// joint pivot setting mode
		if (((ePhenoEditMode::Pivot0 == g_pheno->GetEditMode())
			|| (ePhenoEditMode::Pivot1 == g_pheno->GetEditMode()))
			&& !g_pheno->m_selects.empty()
			&& g_pheno->m_selJoint)
		{
			phys::cJoint *selJoint = g_pheno->m_selJoint;
			for (int selId : g_pheno->m_selects)
			{
				// picking all rigidbody
				float distance = 0.f;
				const int syncId = PickingRigidActor(0, mousePt, &distance);
				phys::cPhysicsSync *phySync = g_pheno->m_physSync;
				phys::sSyncInfo *sync = phySync->FindSyncInfo(syncId);
				if (sync && (selId == syncId))
				{
					const Vector3 pivotPos = ray.dir * distance + ray.orig;
					m_pivotPos = pivotPos;

					// update pivot position
					cJointRenderer *jointRenderer = g_pheno->FindJointRenderer(selJoint);
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

		if (ePhenoEditMode::SpawnLocation == g_pheno->GetEditMode())
		{
			g_pheno->m_spawnTransform.pos = target;
			g_pheno->ChangeEditMode(ePhenoEditMode::Normal);
		}

	}//~case
	break;

	case sf::Mouse::Right:
	{
		m_mouseDown[1] = true;

		// orbit moving
		// select target object center orbit moving
		const int syncId = PickingRigidActor(0, mousePt);
		phys::sSyncInfo *sync = g_pheno->FindSyncInfo(syncId);
		if (sync && sync->node)
		{
			m_isOrbitMove = true;
			m_orbitTarget = sync->node->m_transform.pos;
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
		if ((ePhenoEditMode::Pivot0 == g_pheno->GetEditMode())
			|| (ePhenoEditMode::Pivot1 == g_pheno->GetEditMode())
			|| (ePhenoEditMode::Revolute == g_pheno->GetEditMode())
			|| (ePhenoEditMode::SpawnLocation == g_pheno->GetEditMode()))
			break;

		const int syncId = PickingRigidActor(0, mousePt);
		if (syncId >= 0)
		{
			m_popupMenuState = 1; // open popup menu
			m_popupMenuType = 0; // actor menu
			m_saveFileSyncId = syncId;
			g_pheno->SelectObject(syncId);
			if (phys::sSyncInfo *sync = g_pheno->FindSyncInfo(syncId))
			{
				g_pheno->m_gizmo.m_type = eGizmoEditType::None;
			}
		}
		else
		{
			// spawn pos popupmenu
			// picking ground?
			const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
			const Plane groundPlane(Vector3(0, 1, 0), 0);
			const Vector3 target = groundPlane.Pick(ray.orig, ray.dir);

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
		case sf::Keyboard::Return: break;
		case sf::Keyboard::Space: break;
		case sf::Keyboard::Home: break;
		case sf::Keyboard::Tilde: 
			m_camera.SetCamera(Vector3(30,20,-30), Vector3(0,0,0), Vector3(0, 1, 0));
			break;

		case sf::Keyboard::R: if (!m_showSaveDialog) g_pheno->m_gizmo.m_type = graphic::eGizmoEditType::ROTATE; break;
		case sf::Keyboard::T: if (!m_showSaveDialog) g_pheno->m_gizmo.m_type = graphic::eGizmoEditType::TRANSLATE; break;
		case sf::Keyboard::S: if (!m_showSaveDialog) g_pheno->m_gizmo.m_type = graphic::eGizmoEditType::SCALE; break;
		case sf::Keyboard::H: if (!m_showSaveDialog) g_pheno->m_gizmo.m_type = graphic::eGizmoEditType::None; break;
		case sf::Keyboard::F5: g_pheno->RefreshResourceView(); break;

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
			else if (g_pheno->GetEditMode() == ePhenoEditMode::Revolute)
			{
				// recovery actor selection
				g_pheno->ChangeEditMode(ePhenoEditMode::JointEdit);
				g_pheno->ClearSelection();
				g_pheno->SelectObject(g_pheno->m_pairSyncId0);
				g_pheno->SelectObject(g_pheno->m_pairSyncId1);
			}
			else
			{
				// clear selection
				g_pheno->ChangeEditMode(ePhenoEditMode::Normal);
				g_pheno->m_gizmo.SetControlNode(nullptr);
				g_pheno->m_gizmo.LockEditType(graphic::eGizmoEditType::SCALE, false);
				g_pheno->m_selJoint = nullptr;
				g_pheno->m_fixJointSelection = false;
				g_pheno->ClearSelection();
			}
			break;

		case sf::Keyboard::C: // copy
		{			
			if (::GetAsyncKeyState(VK_CONTROL))
			{
				if (!g_pheno->m_selects.empty())
				{
					phys::cRigidActor *actor = g_pheno->FindRigidActorFromSyncId(*g_pheno->m_selects.begin());
					if (actor)
					{
						g_pheno->UpdateAllConnectionActorDimension(actor, true);

						vector<phys::cRigidActor*> actors;
						for (auto &id : g_pheno->m_selects)
							if (phys::cRigidActor *actor = g_pheno->FindRigidActorFromSyncId(id))
								actors.push_back(actor);
						evc::WritePhenoTypeFileFrom_RigidActor("tmp.pnt", actors);
					}
				}
			}
		}
		break;

		case sf::Keyboard::V: // paste
		{			
			if (::GetAsyncKeyState(VK_CONTROL))
			{
				vector<int> syncIds;
				evc::ReadPhenoTypeFile(g_pheno->GetRenderer(), "tmp.pnt", &syncIds);
				if (syncIds.empty())
					break;

				// moving actor position to camera center
				if (phys::sSyncInfo *sync = g_pheno->FindSyncInfo(syncIds[0]))
				{
					const Ray ray = m_camera.GetRay((int)m_camera.m_width/2
						, (int)m_camera.m_height/2 + (int)m_camera.m_height/5);
					const Plane ground(Vector3(0, 1, 0), 0);
					const Vector3 targetPos = ground.Pick(ray.orig, ray.dir);
					Vector3 spawnPos = targetPos - sync->node->m_transform.pos;
					spawnPos.y = 0;

					g_pheno->ClearSelection();

					for (auto &id : syncIds)
					{
						phys::sSyncInfo *p = g_pheno->FindSyncInfo(id);
						if (!p && !p->node)
							continue;
						p->node->m_transform.pos += spawnPos;
						if (p->actor)
							p->actor->SetGlobalPose(p->node->m_transform);

						g_pheno->SelectObject(id);
					}
				}//~if FindSyncInfo()
			}//~VK_CONTROL
		}
		break;

		case sf::Keyboard::A: // popup menu shortcut, select
		{
			if ((m_popupMenuState == 2) && (m_popupMenuType == 0))
			{
				for (auto id : g_pheno->m_selects)
				{
					if (phys::sSyncInfo *sync = g_pheno->FindSyncInfo(id))
					{
						if (sync && sync->actor)
						{
							g_pheno->SetAllConnectionActorSelect(sync->actor);
							break;
						}
					}
				}

				m_popupMenuState = 3; // close popup
			}
		}
		break;

		case sf::Keyboard::U: // popup menu shortcut, unlock
		{
			if ((m_popupMenuState == 2) && (m_popupMenuType == 0))
			{
				for (auto id : g_pheno->m_selects)
					if (phys::sSyncInfo *sync = g_pheno->FindSyncInfo(id))
						if (sync && sync->actor)
							g_pheno->UpdateActorDimension(sync->actor, false);

				m_popupMenuState = 3; // close popup
			}
		}
		break;

		case sf::Keyboard::L: // popup menu shortcut, lock
		{
			if ((m_popupMenuState == 2) && (m_popupMenuType == 0))
			{
				for (auto id : g_pheno->m_selects)
					if (phys::sSyncInfo *sync = g_pheno->FindSyncInfo(id))
						if (sync && sync->actor)
							sync->actor->SetKinematic(true);

				m_popupMenuState = 3; // close popup
			}
		}
		break;

		case sf::Keyboard::J: // popup menu shortcut, joint remove
		{
			if (m_popupMenuState == 2)
			{
				// remove connection joint
				set<phys::cJoint*> rms;
				for (auto id : g_pheno->m_selects)
					if (phys::sSyncInfo *sync = g_pheno->FindSyncInfo(id))
						if (sync && sync->actor)
							for (auto &j : sync->actor->m_joints)
								rms.insert(j);

				vector<phys::cRigidActor*> wakeups;
				for (auto *j : rms)
				{
					wakeups.push_back(j->m_actor0);
					wakeups.push_back(j->m_actor1);
					g_pheno->m_physSync->RemoveSyncInfo(j);
				}

				// wake up
				for (auto *actor : wakeups)
					if (actor)
						actor->WakeUp();

				m_popupMenuState = 3; // close popup
			}
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
			if (viewRect.IsIn((float)pos.x, (float)pos.y))
				OnMouseDown(evt.mouseButton.button, pos);
		}
		else
		{
			// 화면밖에 마우스가 있더라도 Capture 상태일 경우 Up 이벤트는 받게한다.
			if (viewRect.IsIn((float)pos.x, (float)pos.y)
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
