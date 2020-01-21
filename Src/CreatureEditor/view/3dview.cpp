
#include "stdafx.h"
#include "3dview.h"

using namespace graphic;
using namespace framework;


c3DView::c3DView(const string &name)
	: framework::cDockWindow(name)
	, m_showGrid(true)
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
	m_grid.Create(renderer, 200, 200, 1.f, 1.f
		, (eVertexType::POSITION | eVertexType::COLOR)
		, cColor(0.6f, 0.6f, 0.6f, 0.5f)
		, cColor(0.f, 0.f, 0.f, 0.5f)
	);
	m_grid.m_offsetY = 0.01f;

	m_skybox.Create(renderer, "./media/skybox/sky.dds");

	return true;
}


void c3DView::OnUpdate(const float deltaSeconds)
{
	g_global->m_physics.PreUpdate(deltaSeconds);
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
	{
		m_ccsm.UpdateParameter(renderer, GetMainCamera());
		for (int i = 0; i < cCascadedShadowMap::SHADOWMAP_COUNT; ++i)
		{
			m_ccsm.Begin(renderer, i);
			RenderScene(renderer, "BuildShadowMap", true);
			m_ccsm.End(renderer, i);
		}
	}

	// Render Outline select object
	if (!g_global->m_selects.empty())
	{
		if (m_depthBuff.Begin(renderer))
		{
			for (auto id : g_global->m_selects)
			{
				if (phys::sActorInfo *info = physSync->FindActorInfo(id))
				{
					GetMainCamera().Bind(renderer);
						const Matrix44 parentTm = info->node->GetParentWorldMatrix();
						info->node->SetTechnique("DepthTech");
						info->node->Render(renderer, parentTm.GetMatrixXM());
				}
			}
		}
	}

	const bool isShowGizmo = (g_global->m_selects.size() == 1);

	bool isGizmoEdit = false;
	if (m_renderTarget.Begin(renderer))
	{
		cAutoCam cam(&m_camera);
		renderer.UnbindShaderAll();
		renderer.UnbindTextureAll();
		GetMainCamera().Bind(renderer);
		GetMainLight().Bind(renderer);

		m_skybox.Render(renderer);

		m_ccsm.Bind(renderer);
		RenderScene(renderer, "ShadowMap", false);
		RenderEtc(renderer);
		
		if (isShowGizmo)
			isGizmoEdit = g_global->m_gizmo.Render(renderer, deltaSeconds, m_mousePos, m_mouseDown[0]);

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
	, const bool isBuildShadowMap)
{
	phys::cPhysicsSync *physSync = g_global->m_physSync;
	RET(!physSync);

	for (auto &p : physSync->m_actors)
		p->node->SetTechnique(techiniqName.c_str());

	CommonStates states(renderer.GetDevice());
	renderer.GetDevContext()->RSSetState(states.CullCounterClockwise());

	for (auto &p : physSync->m_actors)
	{
		if (isBuildShadowMap && (p->name == "plane"))
			continue;
		p->node->Render(renderer);
	}

	if (!isBuildShadowMap)
	{
		m_grid.Render(renderer);
		RenderSelectModel(renderer, XMIdentity);
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
	renderer.m_dbgLine.m_isSolid = true;
	renderer.m_dbgLine.SetColor(cColor::GREEN);
	renderer.m_dbgBox.SetColor(cColor::GREEN);
	for (auto &jointRenderer : g_global->m_jointRenderers)
		jointRenderer->Render(renderer);
	renderer.m_dbgLine.m_isSolid = false;
	renderer.m_dbgLine.SetColor(cColor::WHITE);
	renderer.m_dbgBox.SetColor(cColor::WHITE);

	// render ui joint
	if (g_global->m_showUIJoint)
		g_global->m_uiJointRenderer.Render(renderer);
}


// Render Outline Selected model
void c3DView::RenderSelectModel(graphic::cRenderer &renderer, const XMMATRIX &tm)
{
	if (g_global->m_selects.empty())
		return;

	phys::cPhysicsSync *physSync = g_global->m_physSync;

	for (auto id : g_global->m_selects)
	{
		phys::sActorInfo *info = physSync->FindActorInfo(id);
		if (!info)
			return;

		CommonStates state(renderer.GetDevice());
		renderer.GetDevContext()->OMSetDepthStencilState(state.DepthNone(), 0);
		renderer.GetDevContext()->OMSetBlendState(state.NonPremultiplied(), NULL, 0xffffffff);

		renderer.BindTexture(m_depthBuff, 7);
		info->node->SetTechnique("Outline");
		Matrix44 parentTm = info->node->GetParentWorldMatrix();
		info->node->Render(renderer, parentTm.GetMatrixXM());

		renderer.GetDevContext()->OMSetDepthStencilState(state.DepthDefault(), 0);
		renderer.GetDevContext()->OMSetBlendState(state.Opaque(), NULL, 0xffffffff);
	}
}


// Modify rigid actor transform by Gizmo
// process before g_global->m_physics.PostUpdate()
void c3DView::UpdateSelectModelTransform(const bool isGizmoEdit)
{
	using namespace physx;
	phys::cPhysicsSync *physSync = g_global->m_physSync;

	const bool isShowGizmo = (g_global->m_selects.size() == 1);
	if (!isShowGizmo || !isGizmoEdit)
		return;
	if (!g_global->m_gizmo.m_controlNode)
		return;
	if (!physSync)
		return;

	const int selectId = (g_global->m_selects.size() == 1) ? *g_global->m_selects.begin() : -1;
	phys::sActorInfo *info = physSync->FindActorInfo(selectId);
	if (!info)
		return;
	if (!info->actor->m_dynamic) // dynamic actor?
		return;

	cNode *node = g_global->m_gizmo.m_controlNode;
	const Transform tfm = node->m_transform;

	PxTransform tm(*(PxVec3*)&tfm.pos, *(PxQuat*)&tfm.rot);
	info->actor->m_dynamic->setGlobalPose(tm);

	// change dimension? apply wakeup
	if (eGizmoEditType::SCALE == g_global->m_gizmo.m_type)
	{
		info->actor->SetKinematic(true); // stop simulation

		// RigidActor Scale change was delayed, change when wakeup time
		// store change value
		// change 3d model dimension
		switch (info->actor->m_shape)
		{
		case phys::cRigidActor::eShape::Box:
		{
			g_global->ModifyRigidActorTransform(selectId, tfm.scale);
		}
		break;
		case phys::cRigidActor::eShape::Sphere:
		{
			float scale = 1.f;
			switch (g_global->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: scale = info->node->m_transform.scale.x; break;
			case eGizmoEditAxis::Y: scale = info->node->m_transform.scale.y; break;
			case eGizmoEditAxis::Z: scale = info->node->m_transform.scale.z; break;
			}

			((cSphere*)info->node)->SetRadius(scale);
			g_global->ModifyRigidActorTransform(selectId, Vector3(scale,1,1));
		}
		break;
		case phys::cRigidActor::eShape::Capsule:
		{
			const Vector3 scale = info->node->m_transform.scale;
			float radius = 1.f;
			switch (g_global->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: radius = scale.y; break;
			case eGizmoEditAxis::Y: radius = scale.y; break;
			case eGizmoEditAxis::Z: radius = scale.z; break;
			}

			const float halfHeight = scale.x - radius;
			
			((cCapsule*)info->node)->SetDimension(radius, halfHeight);
			g_global->ModifyRigidActorTransform(selectId, Vector3(halfHeight, radius, radius));
		}
		break;
		}
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
		ImGui::End();
	}
	ImGui::PopStyleColor();

	RenderPopupMenu();
}


void c3DView::RenderPopupMenu()
{
	if (m_showMenu)
	{
		//ImGui::OpenPopup("PopupMenu");
		m_showMenu = false;
	}

	if (ImGui::BeginPopup("PopupMenu"))
	{
		if (ImGui::MenuItem("Joint Connection"))
		{

		}
		ImGui::EndPopup();
	}
}


void c3DView::OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect)
{
	if (type == eDockResize::DOCK_WINDOW)
	{
		m_owner->RequestResetDeviceNextFrame();
	}
}


// picing rigidactor
// return actor id
// if not found actor, return -1
// return distance
int c3DView::PickingRigidActor(const POINT &mousePos
	, OUT float *outDistance //= nullptr
)
{
	const Ray ray = GetMainCamera().GetRay(mousePos.x, mousePos.y);

	int minId = -1;
	float minDist = FLT_MAX;
	phys::cPhysicsSync *sync = g_global->m_physSync;
	for (auto &p : sync->m_actors)
	{
		if (p->name == "plane")
			continue;

		graphic::cNode *node = p->node;
		float distance = FLT_MAX;
		if (node->Picking(ray, eNodeType::MODEL, false, &distance))
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
		const Vector3 lookAt = GetMainCamera().m_eyePos + GetMainCamera().GetDirection() * 50.f;
		GetMainCamera().m_lookAt = lookAt;
	}

	GetMainCamera().UpdateViewMatrix();
}


// 휠을 움직였을 때,
// 카메라 앞에 박스가 있다면, 박스 정면에서 멈춘다.
void c3DView::OnWheelMove(const float delta, const POINT mousePt)
{
	UpdateLookAt();

	float len = 0;
	const Plane groundPlane(Vector3(0, 1, 0), 0);
	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
	Vector3 lookAt = groundPlane.Pick(ray.orig, ray.dir);
	len = (ray.orig - lookAt).Length();

	const int lv = 10;
	const float zoomLen = min(len * 0.1f, (float)(2 << (16 - lv)));

	GetMainCamera().Zoom(ray.dir, (delta < 0) ? -zoomLen : zoomLen);
}


// Handling Mouse Move Event
void c3DView::OnMouseMove(const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	if (ImGui::IsMouseHoveringRect(ImVec2(-1000, -1000), ImVec2(1000, 200), false))
		return;

	if (g_global->m_gizmo.IsKeepEditMode())
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
			const int actorId = PickingRigidActor(mousePt, &distance);
			phys::cPhysicsSync *sync = g_global->m_physSync;
			phys::sActorInfo *info = sync->FindActorInfo(actorId);
			if (info && (actorId == selId))
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
		const float scale = 0.005f;
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
void c3DView::OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt)
{
	m_mousePos = mousePt;
	UpdateLookAt();
	SetCapture();

	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
	const Plane groundPlane(Vector3(0, 1, 0), 0);
	const Vector3 target = groundPlane.Pick(ray.orig, ray.dir);
	m_rotateLen = (target - ray.orig).Length();


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
			const int actorId = PickingRigidActor(mousePt, &distance);
			phys::cPhysicsSync *sync = g_global->m_physSync;
			phys::sActorInfo *info = sync->FindActorInfo(actorId);
			if (info && (selId == actorId))
			{
				const Vector3 pivotPos = ray.dir * distance + ray.orig;
				m_pivotPos = pivotPos;

				// update pivot position
				cJointRenderer *jointRenderer = g_global->FindJointRenderer(selJoint);
				if (jointRenderer)
				{
					// find actor0 or actor1 picking
					if (info == jointRenderer->m_info0) // actor0?
					{
						jointRenderer->SetPivotPos(0, pivotPos);
					}
					else if (info == jointRenderer->m_info1)// actor1?
					{
						jointRenderer->SetPivotPos(1, pivotPos);
					}
					else
					{
						assert(0);
					}
				}
			}
		}//~selects
	} //~// joint pivot setting mode


	switch (button)
	{
	case sf::Mouse::Left:
		m_mouseDown[0] = true;
		break;
	case sf::Mouse::Right:
		m_mouseDown[1] = true;
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

	switch (button)
	{
	case sf::Mouse::Left:
	{
		m_mouseDown[0] = false;

		// if pivot mode? ignore rigid actor selection
		if ((eEditState::Pivot0 == g_global->m_state)
			|| (eEditState::Pivot0 == g_global->m_state))
			break;

		const int actorId = PickingRigidActor(mousePt);
		phys::cPhysicsSync *sync = g_global->m_physSync;
		phys::sActorInfo *info = sync->FindActorInfo(actorId);
		if (info)
		{
			if (::GetAsyncKeyState(VK_SHIFT))
			{
				g_global->SelectRigidActor(actorId);
			}
			else if (::GetAsyncKeyState(VK_CONTROL))
			{
				g_global->SelectRigidActor(actorId, true);
			}
			else
			{
				g_global->ClearSelection();
				g_global->SelectRigidActor(actorId);
			}

			if ((g_global->m_selects.size() == 1)
				&& (g_global->m_gizmo.m_controlNode != info->node))
			{
				g_global->m_gizmo.SetControlNode(info->node);
			}
		}
		else
		{
			if (!g_global->m_gizmo.IsKeepEditMode())
			{
				g_global->m_gizmo.SetControlNode(nullptr);
				g_global->m_selects.clear();
			}
		}
	}
	break;


	case sf::Mouse::Right:
	{
		m_mouseDown[1] = false;

		// check show menu to joint connection
		const int actorId = PickingRigidActor(mousePt);
		if (actorId >= 0)
		{
			m_showMenu = true;
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

		case sf::Keyboard::Escape:
			g_global->m_state = eEditState::Normal;
			g_global->m_gizmo.SetControlNode(nullptr);
			g_global->m_selJoint = nullptr;
			g_global->ClearSelection();
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
