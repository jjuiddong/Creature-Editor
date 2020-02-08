
#include "stdafx.h"
#include "genoview.h"
#include "3dview.h"
#include "../creature/gnode.h"

using namespace graphic;
using namespace framework;


cGenoView::cGenoView(const string &name)
	: framework::cDockWindow(name)
	, m_showGrid(true)
	, m_showName(false)
	, m_showJoint(true)
	, m_popupMenuType(0)
	, m_popupMenuState(0)
	, m_isOrbitMove(false)
	, m_showSaveDialog(false)
{
}

cGenoView::~cGenoView()
{
}


bool cGenoView::Init(cRenderer &renderer)
{
	const Vector3 eyePos(30, 20, -30);
	const Vector3 lookAt(0, 0, 0);
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

	cViewport dvp;
	dvp.Create(0, 0, 1024, 1024, 0, 1);
	m_depthBuff.Create(renderer, dvp, false);

	m_grid.Create(renderer, 100, 100, 1.f, 1.f
		, (eVertexType::POSITION | eVertexType::NORMAL | eVertexType::TEXTURE0));
	m_grid.m_mtrl.InitGray4();
	m_grid.SetTechnique("Light");

	m_gridLine.Create(renderer, 100, 100, 1.f, 1.f
		, (eVertexType::POSITION | eVertexType::COLOR)
	);
	m_gridLine.m_offsetY = 0.01f;

	m_skybox.Create(renderer, "./media/skybox/sky.dds");

	return true;
}


void cGenoView::OnUpdate(const float deltaSeconds)
{
}


void cGenoView::OnPreRender(const float deltaSeconds)
{
	cRenderer &renderer = GetRenderer();
	cAutoCam cam(&m_camera);
	renderer.UnbindShaderAll();
	renderer.UnbindTextureAll();
	GetMainCamera().Bind(renderer);
	GetMainLight().Bind(renderer);

	// Render Outline select object
	if (!g_geno->m_selects.empty() || !g_geno->m_highLights.empty())
	{
		m_depthBuff.Begin(renderer);
		RenderSelectModel(renderer, true, XMIdentity);
		m_depthBuff.End(renderer);
	}

	const bool isShowGizmo = (g_geno->m_selects.size() >= 1)
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

		renderer.GetDevContext()->OMSetDepthStencilState(renderer.m_renderState.DepthDefault(), 0);
		renderer.GetDevContext()->RSSetState(renderer.m_renderState.CullCounterClockwise());

		RenderScene(renderer, "Light", false);

		if (isShowGizmo)
			isGizmoEdit = g_geno->m_gizmo.Render(renderer, deltaSeconds, m_mousePos, m_mouseDown[0]);

		// render spawn position
		{
			Vector3 spawnPos = g_geno->m_spawnTransform.pos;
			spawnPos.y = 0.f;

			renderer.m_dbgCube.SetColor(cColor::GREEN);
			renderer.m_dbgCube.SetCube(Transform(spawnPos, Vector3::Ones*0.1f));
			renderer.m_dbgCube.Render(renderer);
		}

		// render reserve spawn position (to show popupmenu selection)
		if ((m_popupMenuState == 2) && (m_popupMenuType == 1))
		{
			renderer.m_dbgCube.SetColor(cColor(1.f, 1.f, 0.f, 0.9f));
			renderer.m_dbgCube.SetCube(Transform(m_tempSpawnPos + Vector3(0,0.5f,0)
				, Vector3(0.05f, 0.5f, 0.05f)));
			renderer.m_dbgCube.Render(renderer);
		}

		m_gridLine.Render(renderer);
		renderer.RenderAxis2();
	}
	m_renderTarget.End(renderer);
	renderer.UnbindTextureAll();

	UpdateSelectModelTransform(isGizmoEdit);
}


// render scene
void cGenoView::RenderScene(graphic::cRenderer &renderer
	, const StrId &techiniqName
	, const bool isBuildShadowMap
	, const XMMATRIX &parentTm //= graphic::XMIdentity
)
{
	for (auto &p : g_geno->m_gnodes)
	{
		p->SetTechnique(techiniqName.c_str());
		p->Render(renderer, parentTm, m_showName ? eRenderFlag::TEXT : 0);
	}

	if (m_showJoint)
	{
		for (auto &p : g_geno->m_glinks)
		{
			p->SetTechnique(techiniqName.c_str());
			p->Render(renderer, parentTm, m_showName ? eRenderFlag::TEXT : 0);
		}
	}

	if (!isBuildShadowMap)
	{
		m_gridLine.Render(renderer);
		RenderSelectModel(renderer, false, parentTm);
	}

	// joint edit mode?
	// hight revolute axis when mouse hover
	if (g_geno->GetEditMode() == eGenoEditMode::JointEdit)
	{
		const Ray ray = GetMainCamera().GetRay((int)m_mousePos.x, (int)m_mousePos.y);
		g_geno->m_uiLink.m_highlightRevoluteAxis = g_geno->m_uiLink.Picking(ray, eNodeType::MODEL);
	}
}


void cGenoView::RenderSelectModel(graphic::cRenderer &renderer, const bool buildOutline
	, const XMMATRIX &tm)
{
	if (g_geno->m_selects.empty() && g_geno->m_highLights.empty())
		return;

	// render function object
	auto render = [&](graphic::cNode *node) {
		if (buildOutline)
		{
			node->SetTechnique("DepthTech");
			Matrix44 parentTm = node->GetParentWorldMatrix();
			node->Render(renderer, parentTm.GetMatrixXM(), eRenderFlag::OUTLINE);
		}
		else
		{
			renderer.BindTexture(m_depthBuff, 7);
			node->SetTechnique("Outline");
			Matrix44 parentTm = node->GetParentWorldMatrix();
			node->Render(renderer, parentTm.GetMatrixXM(), eRenderFlag::OUTLINE);
		}
	};

	if (!buildOutline) // render outline? -> depthnone
		renderer.GetDevContext()->OMSetDepthStencilState(renderer.m_renderState.DepthNone(), 0);
	renderer.m_cbPerFrame.m_v->outlineColor = Vector4(1.f, 0.2f, 0, 1).GetVectorXM();
	for (auto id : g_geno->m_selects)
	{
		if (evc::cGNode *node = g_geno->FindGNode(id))
			render(node);
	}
	renderer.m_cbPerFrame.m_v->outlineColor = Vector4(0.f, 1.f, 0.f, 1).GetVectorXM();
	for (auto id : g_geno->m_highLights)
	{
		if (evc::cGNode *node = g_geno->FindGNode(id))
			render(node);
	}
	if (!buildOutline)
		renderer.GetDevContext()->OMSetDepthStencilState(renderer.m_renderState.DepthDefault(), 0);
}


void cGenoView::OnRender(const float deltaSeconds)
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
		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		//ImGui::Checkbox("grid", &m_showGrid);
		ImGui::Checkbox("name", &m_showName);
		ImGui::SameLine();
		ImGui::Checkbox("joint", &m_showJoint);
		ImGui::End();
	}
	ImGui::PopStyleColor();

	RenderPopupMenu();
	RenderSaveDialog();
}


void cGenoView::RenderPopupMenu()
{
	if (m_popupMenuState == 1)
	{
		switch (m_popupMenuType)
		{
		case 0: ImGui::OpenPopup("Genotype PopupMenu"); break;
		case 1: ImGui::OpenPopup("New PopupMenu"); break;
		default: assert(0); break;
		}
		m_popupMenuState = 2;
	}

	if (ImGui::BeginPopup("Genotype PopupMenu"))
	{
		if ((m_popupMenuState == 3) || g_geno->m_selects.empty())
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		evc::cGNode *gnode = g_geno->FindGNode(*g_geno->m_selects.begin());
		if (!gnode)
		{
			ImGui::EndPopup();
			return;
		}

		if (ImGui::MenuItem("Save GenoType"))
		{
			m_showSaveDialog = true;
			m_popupMenuState = 0;
		}
		if (ImGui::MenuItem("Spawn", "W"))
		{
			evc::WriteGenoTypeFileFrom_Node("tmp_spawn.gnt", gnode);

			// phenotype view load
			{
				const graphic::cCamera3D &camera = g_global->m_3dView->m_camera;
				const Vector2 size(camera.m_width, camera.m_height);
				const Ray ray = camera.GetRay((int)size.x / 2, (int)size.y / 2 + (int)size.y / 5);
				const Plane ground(Vector3(0, 1, 0), 0);
				const Vector3 targetPos = ground.Pick(ray.orig, ray.dir);
				g_pheno->ReadCreatureFile("tmp_spawn.gnt", targetPos);
			}
		}
		if (ImGui::MenuItem("Iterator", "I"))
		{
			evc::cGNode *clone = nullptr;
			if (gnode->m_cloneId >= 0)
			{
				// find original genotype node
				if (evc::cGNode *p = g_geno->FindGNode(gnode->m_cloneId))
					clone = p->Clone(GetRenderer());
			}
			else
			{
				clone = gnode->Clone(GetRenderer());
			}

			if (clone)
			{
				g_geno->AddGNode(clone);
				g_geno->ClearSelection();
				g_geno->SelectObject(clone->m_id);
				g_geno->m_gizmo.SetControlNode(clone);
			}
		}
		if (ImGui::MenuItem("Select All", "A"))
		{
			g_geno->SetAllLinkedNodeSelect(gnode);
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Delete All Link", "J", false, true))
		{
			// remove ui joint
			g_geno->RemoveGLink(&g_geno->m_uiLink);

			// remove all link
			set<evc::cGLink*> rms;
			for (auto id : g_geno->m_selects)
				if (evc::cGNode *gnode = g_geno->FindGNode(id))
					for (auto &p : gnode->m_links)
						rms.insert(p);

			for (auto *p : rms)
				g_geno->RemoveGLink(p);
		}
		if (ImGui::MenuItem("Delete", "D"))
		{
			// remove all node
			for (auto id : g_geno->m_selects)
				if (evc::cGNode *gnode = g_geno->FindGNode(id))
					g_geno->RemoveGNode(gnode);

			g_geno->ClearSelection();
			g_geno->m_gizmo.SetControlNode(nullptr);
			g_geno->m_showUILink = false;
			g_geno->m_selLink = nullptr;
		}

		ImGui::EndPopup();
	}
	else if (ImGui::BeginPopup("New PopupMenu"))
	{
		if (ImGui::BeginMenu("New"))
		{
			if (ImGui::MenuItem("Box"))
			{
				g_geno->SpawnBox(m_tempSpawnPos);
			}
			if (ImGui::MenuItem("Sphere"))
			{
				g_geno->SpawnSphere(m_tempSpawnPos);
			}
			if (ImGui::MenuItem("Capsule"))
			{
				g_geno->SpawnCapsule(m_tempSpawnPos);
			}
			if (ImGui::MenuItem("Cylinder"))
			{
				g_geno->SpawnCylinder(m_tempSpawnPos);
			}
			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}
	else
	{
		m_popupMenuState = 0;
	}
}


void cGenoView::RenderSaveDialog()
{
	if (!m_showSaveDialog)
		return;

	bool isOpen = true;
	const sf::Vector2u psize((uint)m_rect.Width(), (uint)m_rect.Height());
	const ImVec2 size(300, 115);
	const ImVec2 pos(psize.x / 2.f - size.x / 2.f + m_rect.left
		, psize.y / 2.f - size.y / 2.f + m_rect.top);
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowBgAlpha(0.9f);

	if (ImGui::Begin("Save GenoType", &isOpen, 0))
	{
		ImGui::Spacing();
		ImGui::Text("FileName : ");
		ImGui::SameLine();

		static StrPath fileName("filename.gnt");

		bool isSave = false;
		const int flags = ImGuiInputTextFlags_AutoSelectAll
			| ImGuiInputTextFlags_EnterReturnsTrue;
		if (ImGui::InputText("##fileName", fileName.m_str, fileName.SIZE, flags))
		{
			isSave = true;
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		const bool isSaveBtnClick = ImGui::Button("Save");
		if (isSaveBtnClick || isSave)
		{
			const StrPath filePath = StrPath("./media/creature/") + fileName;
			if (!g_geno->m_selects.empty())
			{
				evc::cGNode *gnode = g_geno->FindGNode(*g_geno->m_selects.begin());
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
					evc::WriteGenoTypeFileFrom_Node(filePath, gnode);
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


void cGenoView::OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect)
{
	if (type == eDockResize::DOCK_WINDOW)
	{
		m_owner->RequestResetDeviceNextFrame();
	}
}


// Modify gnode, glink transform by Gizmo
void cGenoView::UpdateSelectModelTransform(const bool isGizmoEdit)
{
	const bool isShowGizmo = (g_geno->m_selects.size() != 0);
	if (!isShowGizmo || !isGizmoEdit)
		return;
	if (!g_geno->m_gizmo.m_controlNode)
		return;

	if (g_geno->m_selects.size() == 1)
	{
		const int id = *g_geno->m_selects.begin();
		evc::cGNode *gnode = g_geno->FindGNode(id);
		evc::cGLink *glink = g_geno->FindGLink(id);

		if (gnode)
			UpdateSelectModelTransform_GNode();
		else if (glink)
			UpdateSelectModelTransform_Link();
	}
	else if (g_geno->m_selects.size() > 1)
	{
		UpdateSelectModelTransform_MultiObject();
	}
}


// Modify rigid actor transform by Gizmo, rigid actor
void cGenoView::UpdateSelectModelTransform_GNode()
{
	using namespace physx;

	const int id = (g_geno->m_selects.size() == 1) ? *g_geno->m_selects.begin() : -1;
	evc::cGNode *gnode = g_geno->FindGNode(id);
	evc::cGLink *glink = g_geno->FindGLink(id);
	if (!gnode && !glink)
		return;

	Transform temp = g_geno->m_gizmo.m_targetTransform;
	temp.pos.y = max(0.f, temp.pos.y);
	const Transform tfm = temp;

	// change dimension?
	if ((eGizmoEditType::SCALE == g_geno->m_gizmo.m_type) && gnode)
	{
		// rigidactor dimension change
		g_geno->m_gizmo.m_controlNode->m_transform.scale = tfm.scale;

		// change 3d model dimension
		switch (gnode->m_shape)
		{
		case phys::eShapeType::Box: break;
		case phys::eShapeType::Sphere:
		{
			float scale = 1.f;
			switch (g_geno->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: scale = gnode->m_transform.scale.x; break;
			case eGizmoEditAxis::Y: scale = gnode->m_transform.scale.y; break;
			case eGizmoEditAxis::Z: scale = gnode->m_transform.scale.z; break;
			}
			gnode->SetSphereRadius(scale); // update radius transform
			g_geno->m_gizmo.UpdateTargetTransform(gnode->m_transform); // update gizmo
		}
		break;
		case phys::eShapeType::Capsule:
		{
			const Vector3 delta = g_geno->m_gizmo.m_deltaTransform.scale;
			float radius = 1.f;
			switch (g_geno->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: radius = delta.y; break;
			case eGizmoEditAxis::Y: radius = delta.y; break;
			case eGizmoEditAxis::Z: radius = delta.z; break;
			}
			Vector2 dim = gnode->GetCapsuleDimension();
			dim.x = max(0.f, dim.x + radius);
			dim.y = max(0.f, dim.y + delta.x);
			gnode->SetCapsuleDimension(dim.x, dim.y); // update capsule transform
			g_geno->m_gizmo.UpdateTargetTransform(gnode->m_transform); // update gizmo
		}
		break;
		case phys::eShapeType::Cylinder:
		{
			const Vector3 scale = gnode->m_transform.scale;
			float radius = 1.f;
			switch (g_geno->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: radius = scale.y; break;
			case eGizmoEditAxis::Y: radius = scale.y; break;
			case eGizmoEditAxis::Z: radius = scale.z; break;
			}
			const float height = scale.x * 2.f;
			gnode->SetCylinderDimension(radius, height); // update cylinder transform
			g_geno->m_gizmo.UpdateTargetTransform(gnode->m_transform); // update gizmo
		}
		break;
		}
	}
}


// update gizmo transform to multi selection object
void cGenoView::UpdateSelectModelTransform_MultiObject()
{
	const Vector3 realCenter = g_geno->m_multiSelPos - Vector3(0, 0.5f, 0);
	const Vector3 center = g_geno->m_multiSelPos;
	const Vector3 offset = g_geno->m_multiSel.m_transform.pos - center;
	g_geno->m_multiSelPos = g_geno->m_multiSel.m_transform.pos; // update current position

	const Quaternion rot = g_geno->m_multiSel.m_transform.rot * g_geno->m_multiSelRot.Inverse();
	g_geno->m_multiSelRot = g_geno->m_multiSel.m_transform.rot; // update current rotation

	// update
	for (auto id : g_geno->m_selects)
	{
		evc::cGNode *gnode = g_geno->FindGNode(id);
		evc::cGLink *glink = g_geno->FindGLink(id);
		if (!gnode)
			continue;

		const Vector3 pos = (gnode->m_transform.pos + offset) - realCenter;
		const Vector3 p = pos * rot;
		gnode->m_transform.rot *= rot;
		gnode->m_transform.pos = p + realCenter;
		gnode->m_transform.pos.y = max(0.f, gnode->m_transform.pos.y);
	}
}


// Modify rigid actor transform by Gizmo, joint
void cGenoView::UpdateSelectModelTransform_Link()
{
	const int selectId = (g_geno->m_selects.size() == 1) ? *g_geno->m_selects.begin() : -1;
	evc::cGLink *link = g_geno->FindGLink(selectId);
	if (!link)
		return; // error occurred

	switch (g_geno->m_gizmo.m_type)
	{
	case eGizmoEditType::TRANSLATE:
		link->SetRevoluteAxisPos(link->m_transform.pos);
		break;
	case eGizmoEditType::SCALE:
		break;
	case eGizmoEditType::ROTATE:
		break;
	}
}


// mouse picking process
bool cGenoView::PickingProcess(const POINT &mousePos)
{
	// if pivot mode? ignore selection
	if ((eGenoEditMode::Pivot0 == g_geno->GetEditMode())
		|| (eGenoEditMode::Pivot1 == g_geno->GetEditMode())
		|| (eGenoEditMode::Revolute == g_geno->GetEditMode())
		|| (eGenoEditMode::SpawnLocation == g_geno->GetEditMode())
		|| g_geno->m_gizmo.IsKeepEditMode()
		)
		return false;

	// picking gnode, glink
	const int id = PickingNode(2, mousePos);
	evc::cGNode *gnode = g_geno->FindGNode(id);
	evc::cGLink *glink = g_geno->FindGLink(id);

	if (gnode) // rigidactor picking?
	{
		if (g_geno->m_fixJointSelection)
			return false; // not possible

		if (::GetAsyncKeyState(VK_SHIFT)) // add picking
		{
			g_geno->SelectObject(id);
		}
		else if (::GetAsyncKeyState(VK_CONTROL)) // toggle picking
		{
			g_geno->SelectObject(id, true);
		}
		else
		{
			g_geno->ClearSelection();
			g_geno->SelectObject(id);
		}

		if ((g_geno->m_selects.size() == 1)
			&& (g_geno->m_gizmo.m_controlNode != gnode))
		{
			g_geno->m_gizmo.SetControlNode(gnode);
		}
	}

	// link picking
	if (glink)
	{
		g_geno->ClearSelection();
		g_geno->SelectObject(glink->m_id);

		g_geno->m_highLights.clear();
		if (glink->m_gnode0)
			g_geno->m_highLights.insert(glink->m_gnode0->m_id);
		if (glink->m_gnode1)
			g_geno->m_highLights.insert(glink->m_gnode1->m_id);

		g_geno->ChangeEditMode(eGenoEditMode::Revolute);
		g_geno->m_gizmo.SetControlNode(glink);
		g_geno->m_gizmo.LockEditType(graphic::eGizmoEditType::SCALE, true);
	}


	if (!gnode && !glink) // no selection?
	{
		if (g_geno->m_fixJointSelection)
			return false; // no selection ignore

		if (!g_geno->m_gizmo.IsKeepEditMode()
			&& !::GetAsyncKeyState(VK_SHIFT)
			&& !::GetAsyncKeyState(VK_CONTROL)
			)
		{
			g_geno->m_gizmo.SetControlNode(nullptr);
			g_geno->ClearSelection();
			g_geno->ChangeEditMode(eGenoEditMode::Normal);
		}
		return false;
	}

	return true;
}


// picking gnode, glink
// return id
// if not found, return -1
// return distance
// pickType = 0: only gnode
//			  1: only glink (only ui joint)
//            2: gnode + glink (only ui joint)
int cGenoView::PickingNode(const int pickType, const POINT &mousePos
	, OUT float *outDistance //= nullptr
)
{
	const Ray ray = GetMainCamera().GetRay(mousePos.x, mousePos.y);

	int minId = -1;
	float minDist = FLT_MAX;
	for (auto &gnode : g_geno->m_gnodes)
	{
		const bool isSpherePicking = (gnode->m_shape == phys::eShapeType::Sphere);

		float distance = FLT_MAX;
		if (gnode->Picking(ray, eNodeType::MODEL, isSpherePicking, &distance))
		{
			if (distance < minDist)
			{
				minId = gnode->m_id;
				minDist = distance;
				if (outDistance)
					*outDistance = distance;
			}
		}
	}

	if (g_geno->m_showUILink)
	{
		float distance = FLT_MAX;
		if (g_geno->m_uiLink.Picking(ray, eNodeType::MODEL, false, &distance))
		{
			if (distance < minDist)
			{
				minId = g_geno->m_uiLink.m_id;
				minDist = distance;
				if (outDistance)
					*outDistance = distance;
			}
		}
	}

	return minId;
}


void cGenoView::UpdateLookAt()
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
void cGenoView::OnWheelMove(const float delta, const POINT mousePt)
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
void cGenoView::OnMouseMove(const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	if (g_geno->m_gizmo.IsKeepEditMode())
		return;
	if (m_showSaveDialog || (m_popupMenuState > 0))
		return;

	// joint pivot setting mode
	if ((eGenoEditMode::Pivot0 == g_geno->GetEditMode())
		|| (eGenoEditMode::Pivot1 == g_geno->GetEditMode())
		&& !g_geno->m_selects.empty()
		&& g_geno->m_selLink)
	{
		for (int selId : g_geno->m_selects)
		{
			// picking all genotype node
			float distance = 0.f;
			const int id = PickingNode(0, mousePt, &distance);
			evc::cGNode *gnode = g_geno->FindGNode(id);
			if (gnode && (id == selId))
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
		m_camera.Yaw2(delta.x * scale, Vector3(0, 1, 0));
		m_camera.Pitch2(delta.y * scale, Vector3(0, 1, 0));
	}
	else if (m_mouseDown[2])
	{
		const float len = GetMainCamera().GetDistance();
		GetMainCamera().MoveRight(-delta.x * len * 0.001f);
		GetMainCamera().MoveUp(delta.y * len * 0.001f);
	}
}


// Handling Mouse Button Down Event
void cGenoView::OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt)
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
	m_owner->SetActiveWindow((framework::cDockWindow*)g_global->m_geditorView);

	switch (button)
	{
	case sf::Mouse::Left:
	{
		m_mouseDown[0] = true;

		// joint pivot setting mode
		if (((eGenoEditMode::Pivot0 == g_geno->GetEditMode())
			|| (eGenoEditMode::Pivot1 == g_geno->GetEditMode()))
			&& !g_geno->m_selects.empty()
			&& g_geno->m_selLink)
		{
			evc::cGLink *selLink = g_geno->m_selLink;
			for (int selId : g_geno->m_selects)
			{
				// picking all rigidbody
				float distance = 0.f;
				const int id = PickingNode(0, mousePt, &distance);
				evc::cGNode *gnode = g_geno->FindGNode(id);
				if (gnode && (selId == id))
				{
					const Vector3 pivotPos = ray.dir * distance + ray.orig;
					m_pivotPos = pivotPos;

					// update pivot position
					if (selLink)
					{
						// find sync0 or sync1 picking
						if (gnode == selLink->m_gnode0)
						{
							selLink->SetPivotPos(0, pivotPos);
						}
						else if (gnode == selLink->m_gnode1)
						{
							selLink->SetPivotPos(1, pivotPos);
						}
						else
						{
							assert(0);
						}
					}
				}//~sync
			}//~selects
		} //~joint pivot setting mode

	}//~case
	break;

	case sf::Mouse::Right:
	{
		m_mouseDown[1] = true;
		m_tempSpawnPos = target;
		//g_geno->m_spawnTransform.pos = target;
	}
	break;

	case sf::Mouse::Middle:
		m_mouseDown[2] = true;
		break;
	}
}


void cGenoView::OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	ReleaseCapture();

	if (m_showSaveDialog || (m_popupMenuState > 0))
		return;

	switch (button)
	{
	case sf::Mouse::Left:
		if (m_mouseDown[0]) // mouse down -> up event?
			PickingProcess(mousePt);
		m_mouseDown[0] = false;
		break;
	
	case sf::Mouse::Right:
	{
		m_mouseDown[1] = false;

		const int dx = m_mouseClickPos.x - mousePt.x;
		const int dy = m_mouseClickPos.y - mousePt.y;
		if (sqrt(dx*dx + dy * dy) > 10)
			break; // move long distance, do not show popup menu

		// check show menu to joint connection
		if ((eGenoEditMode::Pivot0 == g_geno->GetEditMode())
			|| (eGenoEditMode::Pivot1 == g_geno->GetEditMode())
			|| (eGenoEditMode::Revolute == g_geno->GetEditMode())
			|| (eGenoEditMode::SpawnLocation == g_geno->GetEditMode()))
			break;

		const int id = PickingNode(0, mousePt);
		if (id >= 0)
		{
			m_popupMenuState = 1; // open popup menu
			m_popupMenuType = 0; // node menu
			m_clickedId = id;
			//m_saveFileSyncId = syncId;
			g_geno->SelectObject(id);
			if (evc::cGNode *node = g_geno->FindGNode(id))
				g_geno->m_gizmo.m_type = eGizmoEditType::None;
		}
		else
		{
			// spawn pos popupmenu
			// picking ground?
			const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
			const Plane groundPlane(Vector3(0, 1, 0), 0);
			const Vector3 target = groundPlane.Pick(ray.orig, ray.dir);

			m_popupMenuState = 1; // open popup menu
			m_popupMenuType = 1; // new node menu
			m_tempSpawnPos = target;
		}
	}
	break;

	case sf::Mouse::Middle:
		m_mouseDown[2] = false;
		break;
	}
}


void cGenoView::OnEventProc(const sf::Event &evt)
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
			//m_camera.SetCamera(Vector3(30, 20, -30), Vector3(0, 0, 0), Vector3(0, 1, 0));
			break;

		case sf::Keyboard::R: if (!m_showSaveDialog && (m_popupMenuState != 2)) g_geno->m_gizmo.m_type = graphic::eGizmoEditType::ROTATE; break;
		case sf::Keyboard::T: if (!m_showSaveDialog && (m_popupMenuState != 2)) g_geno->m_gizmo.m_type = graphic::eGizmoEditType::TRANSLATE; break;
		case sf::Keyboard::S: if (!m_showSaveDialog && (m_popupMenuState != 2)) g_geno->m_gizmo.m_type = graphic::eGizmoEditType::SCALE; break;
		case sf::Keyboard::H: if (!m_showSaveDialog && (m_popupMenuState != 2)) g_geno->m_gizmo.m_type = graphic::eGizmoEditType::None; break;
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
			else if (g_geno->GetEditMode() == eGenoEditMode::Revolute)
			{
				// recovery actor selection
				g_geno->ChangeEditMode(eGenoEditMode::JointEdit);
				g_geno->ClearSelection();
				g_geno->SelectObject(g_geno->m_pairId0);
				g_geno->SelectObject(g_geno->m_pairId1);
			}
			else
			{
				// clear selection
				g_geno->ChangeEditMode(eGenoEditMode::Normal);
				g_geno->m_gizmo.SetControlNode(nullptr);
				g_geno->m_gizmo.LockEditType(graphic::eGizmoEditType::SCALE, false);
				g_geno->m_selLink = nullptr;
				g_geno->m_fixJointSelection = false;
				g_geno->ClearSelection();
			}
			break;

		case sf::Keyboard::C: // copy
		{
			if (::GetAsyncKeyState(VK_CONTROL))
			{
				if (!g_geno->m_selects.empty())
				{
					vector<evc::cGNode*> gnodes;
					for (auto &id : g_geno->m_selects)
						if (evc::cGNode *node = g_geno->FindGNode(id))
							gnodes.push_back(node);
					evc::WriteGenoTypeFileFrom_Node("tmp.gnt", gnodes);
				}
			}
		}
		break;

		case sf::Keyboard::V: // paste
		{
			if (::GetAsyncKeyState(VK_CONTROL))
			{
				const Ray ray = m_camera.GetRay((int)m_camera.m_width / 2
					, (int)m_camera.m_height / 2 + (int)m_camera.m_height / 5);
				const Plane ground(Vector3(0, 1, 0), 0);
				const Vector3 targetPos = ground.Pick(ray.orig, ray.dir);

				g_geno->ReadGenoTypeNodeFile("tmp.gnt", targetPos);
			}//~VK_CONTROL
		}
		break;
		
		case sf::Keyboard::A: // popup menu shortcut, select
		{
			if ((m_popupMenuState == 2) && (m_popupMenuType == 0))
			{
				for (auto id : g_geno->m_selects)
				{
					if (evc::cGNode *gnode = g_geno->FindGNode(id))
					{
						g_geno->SetAllLinkedNodeSelect(gnode);
						break;
					}
				}

				m_popupMenuState = 3; // close popup
			}
		}
		break;

		case sf::Keyboard::U: break;
		case sf::Keyboard::L: break;

		case sf::Keyboard::W:
		{
			if ((m_popupMenuState == 2) && !g_geno->m_selects.empty())
			{
				if (evc::cGNode *gnode = g_geno->FindGNode(g_geno->m_selects[0]))
				{
					evc::WriteGenoTypeFileFrom_Node("tmp_spawn.gnt", gnode);

					// phenotype view load
					{
						const graphic::cCamera3D &camera = g_global->m_3dView->m_camera;
						const Vector2 size(camera.m_width, camera.m_height);
						const Ray ray = camera.GetRay((int)size.x / 2, (int)size.y / 2 + (int)size.y / 5);
						const Plane ground(Vector3(0, 1, 0), 0);
						const Vector3 targetPos = ground.Pick(ray.orig, ray.dir);
						g_pheno->ReadCreatureFile("tmp_spawn.gnt", targetPos);
					}
				}
				m_popupMenuState = 3;
			}
		}
		break;

		case sf::Keyboard::J: // popup menu shortcut, joint remove
		{
			if (m_popupMenuState == 2)
			{
				// remove all link
				set<evc::cGLink*> rms;
				for (auto id : g_geno->m_selects)
					if (evc::cGNode *gnode = g_geno->FindGNode(id))
						for (auto &p : gnode->m_links)
							rms.insert(p);

				for (auto *p : rms)
					g_geno->RemoveGLink(p);

				m_popupMenuState = 3; // close popup
			}
		}
		break;

		case sf::Keyboard::D:
		{
			if (m_popupMenuState == 2)
			{
				// remove ui joint
				g_geno->RemoveGLink(&g_geno->m_uiLink);

				// remove all node
				for (auto id : g_geno->m_selects)
					if (evc::cGNode *gnode = g_geno->FindGNode(id))
						g_geno->RemoveGNode(gnode);

				g_geno->ClearSelection();
				g_geno->m_gizmo.SetControlNode(nullptr);
				g_geno->m_showUILink = false;
				g_geno->m_selLink = nullptr;

				m_popupMenuState = 3; // close popup
			}
		}
		break;

		case sf::Keyboard::I:
		{
			if ((m_popupMenuState == 2) && !g_geno->m_selects.empty())
			{
				evc::cGNode *gnode = g_geno->FindGNode(*g_geno->m_selects.begin());
				if (!gnode)
					break;

				evc::cGNode *clone = nullptr;
				if (gnode->m_cloneId >= 0)
				{
					// find original genotype node
					if (evc::cGNode *p = g_geno->FindGNode(gnode->m_cloneId))
						clone = p->Clone(GetRenderer());
				}
				else
				{
					clone = gnode->Clone(GetRenderer());
				}

				if (clone)
				{
					g_geno->AddGNode(clone);
					g_geno->ClearSelection();
					g_geno->SelectObject(clone->m_id);
					g_geno->m_gizmo.SetControlNode(clone);
				}

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


void cGenoView::OnResetDevice()
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
