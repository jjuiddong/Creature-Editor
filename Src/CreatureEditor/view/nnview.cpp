
#include "stdafx.h"
#include "nnview.h"
#include "3dview.h"
#include "../creature/gnode.h"
#include "genoeditorview.h"
#include "phenoeditorview.h"
#include "resourceview.h"

using namespace graphic;
using namespace framework;


cNNView::cNNView(const string &name)
	: framework::cDockWindow(name)
	, m_showGrid(true)
	, m_showName(false)
	, m_showId(false)
	, m_showJoint(true)
	, m_popupMenuType(0)
	, m_popupMenuState(0)
	, m_showSaveDialog(false)
	, m_curCreature(nullptr)
	, m_showSensor(true)
	, m_showEffector(true)
	, m_showNN(true)
	, m_showPhenotype(true)
	, m_line2DList(1024)
	, m_showGenomeFileList(false)
	, m_selectFileType(0)
	, m_selectFileIdx(-1)
{
}

cNNView::~cNNView()
{
}


bool cNNView::Init(cRenderer &renderer)
{
	const Vector3 eyePos(30, 20, -30);
	const Vector3 lookAt(0, 0, 0);
	m_camera.SetCamera(eyePos, lookAt, Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, m_rect.Width() / m_rect.Height(), 0.1f, 100000.f);
	m_camera.SetViewPort(m_rect.Width(), m_rect.Height());

	GetMainLight().Init(graphic::cLight::LIGHT_DIRECTIONAL);
	GetMainLight().SetDirection(Vector3(-1, -2, -1.3f).Normal());

	sf::Vector2u size((uint)m_rect.Width() - 15, (uint)m_rect.Height() - 50);
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
	m_bezier.Create(renderer, 20, cColor::GREEN);
	m_billboard.Create(renderer, BILLBOARD_TYPE::ALL_AXIS, 0.02f, 0.02f, Vector3::Zeroes
		, "white.dds");
	m_circle.Create(renderer, Vector3(0, 0, 0), 1.f, 20, cColor::WHITE);
	m_line2D.Create(renderer);
	m_line2DList.Create(renderer);

	return true;
}


void cNNView::OnUpdate(const float deltaSeconds)
{
}


void cNNView::OnPreRender(const float deltaSeconds)
{
	cRenderer &renderer = GetRenderer();
	cAutoCam cam(&m_camera);
	renderer.UnbindShaderAll();
	renderer.UnbindTextureAll();
	GetMainCamera().Bind(renderer);
	GetMainLight().Bind(renderer);

	// Render Outline select object
	//if (!g_geno->m_selects.empty() || !g_geno->m_highLights.empty() || (g_geno->m_orbitId >= 0))
	//{
	//	m_depthBuff.Begin(renderer);
	//	RenderSelectModel(renderer, true, XMIdentity);
	//	m_depthBuff.End(renderer);
	//}

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

		//if (isShowGizmo)
		//	isGizmoEdit = g_geno->m_gizmo.Render(renderer, deltaSeconds, m_mousePos, m_mouseDown[0]);

		// render spawn position
		//{
		//	Vector3 spawnPos = g_geno->m_spawnTransform.pos;
		//	spawnPos.y = 0.f;

		//	renderer.m_dbgCube.SetColor(cColor::GREEN);
		//	renderer.m_dbgCube.SetCube(Transform(spawnPos, Vector3::Ones*0.2f));
		//	renderer.m_dbgCube.Render(renderer);
		//}

		// render reserve spawn position (to show popupmenu selection)
		//if ((m_popupMenuState == 2) && (m_popupMenuType == 1))
		//{
		//	renderer.m_dbgCube.SetColor(cColor(1.f, 1.f, 0.f, 0.9f));
		//	renderer.m_dbgCube.SetCube(Transform(m_tempSpawnPos + Vector3(0, 0.5f, 0)
		//		, Vector3(0.05f, 0.5f, 0.05f)));
		//	renderer.m_dbgCube.Render(renderer);
		//}

		//m_gridLine.Render(renderer);
		//renderer.RenderAxis2();
	}
	m_renderTarget.End(renderer);
	renderer.UnbindTextureAll();

	UpdateSelectModelTransform(isGizmoEdit);
}


// render scene
void cNNView::RenderScene(graphic::cRenderer &renderer
	, const StrId &techiniqName
	, const bool isBuildShadowMap
	, const XMMATRIX &parentTm //= graphic::XMIdentity
)
{
	evc::cCreature *creature = g_nn->m_creature;
	RET(!creature); // check creature valid?
	RET(creature->m_nodes.empty());
	RET(!creature->m_nodes[0]->m_gnode);

	// creature move to 0,0,0 position
	if (m_curCreature != creature)
	{
		m_curCreature = creature;
		const Transform &tfm = creature->m_nodes[0]->m_gnode->transform;
		m_creatureOffsetPos = -tfm.pos;
		m_creatureOffsetPos.y = 0.f;
		g_nn->m_orbitId = creature->m_nodes[0]->m_id;
		g_nn->m_orbitTarget = tfm.pos;
	}

	// render phenotype node
	if (m_showPhenotype)
	{
		for (auto &p : creature->m_nodes)
		{
			graphic::cNode *node = p->m_node;
			const Transform tmp = node->m_transform;
			node->m_transform = p->m_gnode->transform;
			node->m_transform.pos += m_creatureOffsetPos;

			node->SetTechnique(techiniqName.c_str());
			node->Render(renderer, parentTm, (m_showName || m_showId) ? eRenderFlag::TEXT : 0);
			node->m_transform = tmp;
		}
	}

	renderer.GetDevContext()->OMSetDepthStencilState(renderer.m_renderState.DepthNone(), 0);
	if (m_showPhenotype && m_showSensor)
		RenderSensor();
	if (m_showPhenotype && m_showEffector)
		RenderEffector();
	if (m_showNN)
		RenderNeuralNetwork();
	renderer.GetDevContext()->OMSetDepthStencilState(renderer.m_renderState.DepthDefault(), 0);

	if (!isBuildShadowMap)
	{
		//m_gridLine.Render(renderer);
		RenderSelectModel(renderer, false, parentTm);
	}
}


// render sensor
void cNNView::RenderSensor()
{
	cRenderer &renderer = GetRenderer();
	evc::cCreature *creature = g_nn->m_creature;

	m_bezier.SetColor(cColor::GREEN);
	m_billboard.SetColor(cColor::GREEN);
	const Vector3 right = m_camera.GetRight();

	for (auto &p : creature->m_nodes)
	{
		Vector3 p0 = p->m_gnode->transform.pos;
		p0 += m_creatureOffsetPos;

		map<phys::cJoint*, int> cntMap;
		for (auto &sensor : p->m_sensors)
		{
			switch (sensor->m_type)
			{
			case evc::eSensorType::Angular:
			case evc::eSensorType::Limit:
				if (phys::cJoint *joint = sensor->GetJoint())
				{
					const int cnt = cntMap[joint]++;

					Vector3 p1 = joint->m_origPos;
					p1 += m_creatureOffsetPos;

					const Vector3 offset = right * 0.05f * cnt;
					Vector3 p2 = p0 + Vector3(0, 1, 0) + offset;
					Vector3 p3 = p1 + Vector3(0, 1, 0) + offset;
					m_bezier.SetLine(p0, p2, p3, p1 + offset, 0.005f);
					m_bezier.Render(renderer);

					m_billboard.m_transform.pos = p1 + offset;
					m_billboard.Render(renderer);
				}
				break;

			default:
				assert(0);
				break;
			}
		}
	}
}


// render effector
void cNNView::RenderEffector()
{
	cRenderer &renderer = GetRenderer();
	evc::cCreature *creature = g_nn->m_creature;

	m_bezier.SetColor(cColor::VIOLET);
	m_billboard.SetColor(cColor::VIOLET);
	const Vector3 right = m_camera.GetRight();

	for (auto &p : creature->m_nodes)
	{
		Vector3 p0 = p->m_gnode->transform.pos;
		p0 += m_creatureOffsetPos;

		for (auto &effector : p->m_effectors)
		{
			switch (effector->m_type)
			{
			case evc::eEffectorType::Muscle:
				if (evc::cMuscleEffector *eff = dynamic_cast<evc::cMuscleEffector*>(effector))
				{
					Vector3 p1 = eff->m_joint->m_origPos;
					p1 += m_creatureOffsetPos;
					const Vector3 offset = right * 0.05f * -1.f;
					Vector3 p2 = p0 + Vector3(0.1f, 1, 0);
					Vector3 p3 = p1 + Vector3(0.1f, 1, 0);
					m_bezier.SetLine(p0, p2, p3, p1 + offset, 0.005f);
					m_bezier.Render(renderer);

					m_billboard.m_transform.pos = p1 + offset;
					m_billboard.Render(renderer);
				}
				break;

			default:
				assert(0);
				break;
			}
		}
	}
}


// render fully connected neural network
void cNNView::RenderNeuralNetwork()
{
	cRenderer &renderer = GetRenderer();
	evc::cCreature *creature = g_nn->m_creature;

	ai::cNeuralNet *nn = creature->m_nodes[0]->m_nn;
	RET(!nn);
	RET(nn->m_layers.empty());
	RET(nn->m_layers[0].neurons.empty());
	RET(nn->m_layers[0].neurons[0].weight.empty());

	const Vector2 offset(50, 45);
	const float w = m_viewRect.Width() - (offset.x * 1.3f);
	const float h = m_viewRect.Height() - (offset.y * 1.3f);
	const float hr = h / nn->m_layers[0].neurons[0].numInputs;
	const float cr = min(hr / 4.f, 10.f); // circle radius
	const float ngap = min((hr - (cr*2.f)) * 2.f, cr * 4.f); // neuron gap
	const float wr = w / (float)nn->m_layers.size();
	const float lgap = min(400.f, wr); // layer gap

	// render weight line
	m_line2DList.ClearLine();
	for (uint i = 0; i < nn->m_layers.size(); ++i)
	{
		if ((i + 1) == (nn->m_layers.size()))
			break;
		
		// color scale
		//const bool isOutput = false;// (i + 2) == (nn->m_layers.size());

		ai::sNeuronLayer &layer = nn->m_layers[i];
		ai::sNeuronLayer &nlayer = nn->m_layers[i+1]; // next layer
		// center align offset
		const Vector2 offset1(0, (h / 2.f) - ((layer.neurons[0].numInputs * ngap) / 2.f));
		const Vector2 offset2(0, (h / 2.f) - ((nlayer.neurons[0].numInputs * ngap) / 2.f));
		for (uint k = 0; k < layer.numNeurons; ++k)
		{
			ai::sNeuron &neuron = layer.neurons[k];

			const Vector2 p1 = Vector2((i + 1) * lgap, k * ngap) + offset + offset2;
			for (uint m = 0; m < neuron.weight.size(); ++m)
			{
				const Vector2 p0 = Vector2(i * lgap, m * ngap) + offset + offset1;

				//-1.f ~ +1.f scaling
				float c = 0.f;
				//if (isOutput)
				//{
				//	c = (neuron.result[m] < 0.f)? 0.f : 1.f;
				//}
				//else
				//{
					c = max(0.f, (float)((neuron.result[m] + 1.f) / 2.f));
				//}

				Vector4 color(c, c, c, 1.f);
				m_line2DList.AddLine(p0, p1, 0.5f, cColor(color));
			}
		}
	}
	m_line2DList.Render(renderer);

	// render neuron (optimize)
	{
		cShader11 *shader = renderer.m_shaderMgr.FindShader(graphic::eVertexType::POSITION_RHW);
		assert(shader);
		shader->SetTechnique("Unlit");
		shader->Begin();
		shader->BeginPass(renderer, 0);

		renderer.m_cbMaterial.m_v->diffuse = Vector4(1,1,1,1).GetVectorXM();
		renderer.m_cbMaterial.Update(renderer, 2);

		for (uint i=0; i < nn->m_layers.size(); ++i)
		{
			ai::sNeuronLayer &layer = nn->m_layers[i];
			// center align offset
			const Vector2 offset2(0, (h / 2.f) - ((layer.neurons[0].numInputs * ngap) / 2.f));
			for (uint k = 0; k < layer.neurons[0].numInputs; ++k)
			{
				const Vector2 pos = Vector2(i * lgap, k * ngap) + offset + offset2;
				m_circle.SetPos(pos);
				m_circle.m_transform.scale = Vector3::Ones * cr;
				m_circle.Render2(renderer);
			}
		}
	}
}


void cNNView::RenderSelectModel(graphic::cRenderer &renderer, const bool buildOutline
	, const XMMATRIX &tm)
{
	//if (g_geno->m_selects.empty() && g_geno->m_highLights.empty() && (g_geno->m_orbitId < 0))
	//	return;

	//// render function object
	//auto render = [&](graphic::cNode *node) {
	//	if (buildOutline)
	//	{
	//		node->SetTechnique("DepthTech");
	//		Matrix44 parentTm = node->GetParentWorldMatrix();
	//		node->Render(renderer, parentTm.GetMatrixXM(), eRenderFlag::OUTLINE);
	//	}
	//	else
	//	{
	//		renderer.BindTexture(m_depthBuff, 7);
	//		node->SetTechnique("Outline");
	//		Matrix44 parentTm = node->GetParentWorldMatrix();
	//		node->Render(renderer, parentTm.GetMatrixXM(), eRenderFlag::OUTLINE);
	//	}
	//};

	//if (!buildOutline) // render outline? -> depthnone
	//	renderer.GetDevContext()->OMSetDepthStencilState(renderer.m_renderState.DepthNone(), 0);
	//renderer.m_cbPerFrame.m_v->outlineColor = Vector4(1.f, 0.2f, 0, 1).GetVectorXM();
	//for (auto id : g_geno->m_selects)
	//{
	//	if (evc::cGNode *node = g_geno->FindGNode(id))
	//		render(node);
	//}
	//renderer.m_cbPerFrame.m_v->outlineColor = Vector4(0.f, 1.f, 0.f, 1).GetVectorXM();
	//for (auto id : g_geno->m_highLights)
	//{
	//	if (evc::cGNode *node = g_geno->FindGNode(id))
	//		render(node);
	//}
	//renderer.m_cbPerFrame.m_v->outlineColor = Vector4(1.f, 1.f, 0.f, 1).GetVectorXM();
	//if (g_geno->m_orbitId >= 0)
	//{
	//	if (evc::cGNode *node = g_geno->FindGNode(g_geno->m_orbitId))
	//		render(node);
	//}
	//if (!buildOutline)
	//	renderer.GetDevContext()->OMSetDepthStencilState(renderer.m_renderState.DepthDefault(), 0);
}


void cNNView::RenderGenomeFileList()
{
	bool isOpen = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoBackground
		;

	const float width = m_viewRect.Width() + 15.f;
	const float height = m_viewRect.Height() - 70.f;

	ImGui::SetNextWindowPos(ImVec2((float)m_viewPos.x, (float)m_viewPos.y + 50.f));
	ImGui::SetNextWindowSize(ImVec2(width, height));
	if (ImGui::Begin("NNView GenomeFileList", &isOpen, flags))
	{
		static ImGuiTextFilter filter;
		ImGui::Text("Search");
		ImGui::SameLine();
		filter.Draw("##Genome Search", 145);

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.9f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0, 1));
		if (ImGui::Button("Refresh (F5)"))
		{
			g_global->m_resView->UpdateResourceFiles();
		}
		ImGui::PopStyleColor(3);

		// genome file list
		ImGui::SetNextWindowBgAlpha(0.95f);
		if (ImGui::BeginChild("NN GenomeFileList", ImVec2(0, height/2.f - 25.f), true))
		{
			ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode((void*)(this + 0x01), "Genome File List"))
			{
				ImGui::Columns(1, "nn gen genomecolumns1", false);
				int i = 0;
				for (auto &str : g_global->m_resView->m_genomeFileList)
				{
					const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow
						| ImGuiTreeNodeFlags_OpenOnDoubleClick
						| ImGuiTreeNodeFlags_Leaf
						| ImGuiTreeNodeFlags_NoTreePushOnOpen
						| (((m_selectFileType == 0) && (i == m_selectFileIdx)) ? 
							ImGuiTreeNodeFlags_Selected : 0);

					if (filter.PassFilter(str.c_str()))
					{
						ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags, str.c_str());

						if (ImGui::IsItemClicked())
						{
							if ((m_selectFileType != 0) || (m_selectFileIdx != i))
							{
								const StrPath fileName = g_creatureResourcePath + str;
								m_genome.Read(fileName);
							}
							m_selectFileType = 0;
							m_selectFileIdx = i;
						}//~IsItemClicked
						ImGui::NextColumn();
					}//~PassFilter
					++i;
				}
				ImGui::TreePop();
			}

			ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Evolution File List"))
			{
				ImGui::Columns(1, "nn evo genomecolumns1", false);
				int i = 0;
				for (auto &str : g_global->m_resView->m_evolutionFileList)
				{
					const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow
						| ImGuiTreeNodeFlags_OpenOnDoubleClick
						| ImGuiTreeNodeFlags_Leaf
						| ImGuiTreeNodeFlags_NoTreePushOnOpen
						| (((m_selectFileType == 1) && (i == m_selectFileIdx)) ?
							ImGuiTreeNodeFlags_Selected : 0);

					if (filter.PassFilter(str.c_str()))
					{
						ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags, str.c_str());

						if (ImGui::IsItemClicked())
						{
							if ((m_selectFileType != 1) || (m_selectFileIdx != i))
							{
								const StrPath fileName = g_evolutionResourcePath + str;
								m_genome.Read(fileName);
							}
							m_selectFileType = 1;
							m_selectFileIdx = i;
						}//~IsItemClicked
						ImGui::NextColumn();
					}//~PassFilter
					++i;
				}
				ImGui::TreePop();
			}
		}
		ImGui::EndChild();

		// select genome information
		ImGui::SetNextWindowBgAlpha(0.95f);
		if (ImGui::BeginChild("NN GenomeFile Info", ImVec2(0, height / 2.f - 20.f), true))
		{
			ImGui::Columns(1, "nn genomecolumns1", false);
			ImGui::TextUnformatted("Genome Information");
			ImGui::Text("Name : %s", m_genome.m_name.c_str());
			ImGui::Spacing();

			ImGui::Columns(6, "nn genomecolumns1", true);
			ImGui::Separator();
			ImGui::TextUnformatted("Action");
			ImGui::NextColumn();
			ImGui::TextUnformatted("Order");
			ImGui::NextColumn();
			ImGui::TextUnformatted("Input");
			ImGui::NextColumn();
			ImGui::TextUnformatted("Output");
			ImGui::NextColumn();
			ImGui::TextUnformatted("Layer");
			ImGui::NextColumn();
			ImGui::TextUnformatted("DNA");
			ImGui::NextColumn();
			ImGui::Separator();

			if (!m_genome.m_dnas.empty())
			{
				for (uint i=0; i < m_genome.m_dnas.size(); ++i)
				{
					auto &dna = m_genome.m_dnas[i];
					ImGui::PushID(i+(int)this);
					if (ImGui::SmallButton("Load"))
					{
						const uint cnt = g_genome->SetGenomeSelectCreature(m_genome, i);
						Str128 text;
						text.Format("Update Creature Genome [ %d ]", cnt);
						::MessageBoxA(m_owner->getSystemHandle()
							, text.c_str(), "Confirm", MB_OK | MB_ICONINFORMATION);
					}
					ImGui::PopID();

					ImGui::NextColumn();
					ImGui::Text("%d", i + 1);
					ImGui::NextColumn();
					ImGui::Text("%d", dna.inputCnt);
					ImGui::NextColumn();
					ImGui::Text("%d", dna.outputCnt);
					ImGui::NextColumn();
					ImGui::Text("%d", dna.layerCnt);
					ImGui::NextColumn();
					ImGui::Text("%d", dna.chromo.size());
					ImGui::NextColumn();

					ImGui::Separator();
				}
			}
			ImGui::Separator();
		}
		ImGui::EndChild();
	}
	ImGui::End();
}


void cNNView::OnRender(const float deltaSeconds)
{
	ImVec2 pos = ImGui::GetCursorScreenPos();
	m_viewPos = { (int)(pos.x), (int)(pos.y) };
	m_viewRect = { pos.x + 5, pos.y, pos.x + m_rect.Width() - 30, pos.y + m_rect.Height() - 42 };

	// HUD
	bool isOpen = true;
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
		//| ImGuiWindowFlags_NoBackground
		;

	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	ImGui::Image(m_renderTarget.m_resolvedSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 42));

	// Render Information
	ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y));
	ImGui::SetNextWindowBgAlpha(0.6f);
	ImGui::SetNextWindowSize(ImVec2(min(400.f,m_viewRect.Width()), 55.f));
	if (ImGui::Begin("Neural Network Information", &isOpen, flags))
	{
		ImGui::Checkbox("phenotype", &m_showPhenotype);
		ImGui::SameLine();
		ImGui::Checkbox("sensor", &m_showSensor);
		ImGui::SameLine();
		ImGui::Checkbox("effector", &m_showEffector);
		ImGui::SameLine();
		ImGui::Checkbox("nn", &m_showNN);

		if (g_nn->m_orbitId >= 0)
		{
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
			ImGui::TextUnformatted("Orbit");
			ImGui::PopStyleColor();
		}

		evc::cCreature *creature = g_nn->m_creature;
		if (creature && !creature->m_nodes.empty())
		{
			ai::cNeuralNet *nn = creature->m_nodes[0]->m_nn;
			if (nn)
			{
				ImGui::Text("input=%d, output=%d, layer=%d"
					, nn->m_numInputs, nn->m_numOutputs, nn->m_layers.size());
			}
		}
	}
	ImGui::End();

	// Render GenomeList Toggle Button
	const ImGuiWindowFlags flags2 = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoBackground
		;
	const float toggleBtnX = pos.x + m_viewRect.Width() - 90.f;
	const float toggleBtnY = pos.y + m_viewRect.Height() - 30.f;
	ImGui::SetNextWindowPos(ImVec2(toggleBtnX, toggleBtnY));
	ImGui::SetNextWindowSize(ImVec2(100, 30.f));
	if (ImGui::Begin("Genome List Btn", &isOpen, flags2))
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.9f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0, 1));
		if (ImGui::Button("Genome List"))
		{
			m_showGenomeFileList = !m_showGenomeFileList;
		}
		ImGui::PopStyleColor(3);
	}
	ImGui::End();
	ImGui::PopStyleColor();

	if (m_showGenomeFileList)
		RenderGenomeFileList();

	//RenderPopupMenu();
	//RenderSaveDialog();
}


//void cNNView::RenderPopupMenu()
//{
//	if (m_popupMenuState == 1)
//	{
//		switch (m_popupMenuType)
//		{
//		case 0: ImGui::OpenPopup("Genotype PopupMenu"); break;
//		case 1: ImGui::OpenPopup("New PopupMenu"); break;
//		default: assert(0); break;
//		}
//		m_popupMenuState = 2;
//	}
//
//	if (ImGui::BeginPopup("Genotype PopupMenu"))
//	{
//		if ((m_popupMenuState == 3) || g_geno->m_selects.empty())
//		{
//			ImGui::CloseCurrentPopup();
//			ImGui::EndPopup();
//			return;
//		}
//
//		evc::cGNode *gnode = g_geno->FindGNode(*g_geno->m_selects.begin());
//		if (!gnode)
//		{
//			ImGui::EndPopup();
//			return;
//		}
//
//		if (ImGui::MenuItem("Save GenoType"))
//		{
//			m_showSaveDialog = true;
//			m_popupMenuState = 0;
//		}
//		if (ImGui::MenuItem("Spawn", "W"))
//		{
//			SpawnSelectNodeToPhenoTypeView();
//		}
//		if (ImGui::MenuItem("Iterator", "I"))
//		{
//			SpawnSelectIterator();
//		}
//		if (ImGui::MenuItem("Select All", "A"))
//		{
//			g_geno->SetAllLinkedNodeSelect(gnode);
//		}
//
//		ImGui::Separator();
//
//		if (ImGui::MenuItem("Delete All Link", "J", false, true))
//		{
//			DeleteSelectLink();
//		}
//		if (ImGui::MenuItem("Delete", "D"))
//		{
//			DeleteSelectNode();
//		}
//
//		ImGui::EndPopup();
//	}
//	else if (ImGui::BeginPopup("New PopupMenu"))
//	{
//		if (ImGui::BeginMenu("New"))
//		{
//			if (ImGui::MenuItem("Box"))
//			{
//				g_geno->SpawnBox(m_tempSpawnPos);
//			}
//			if (ImGui::MenuItem("Sphere"))
//			{
//				g_geno->SpawnSphere(m_tempSpawnPos);
//			}
//			if (ImGui::MenuItem("Capsule"))
//			{
//				g_geno->SpawnCapsule(m_tempSpawnPos);
//			}
//			if (ImGui::MenuItem("Cylinder"))
//			{
//				g_geno->SpawnCylinder(m_tempSpawnPos);
//			}
//			ImGui::EndMenu();
//		}
//
//		ImGui::EndPopup();
//	}
//	else
//	{
//		m_popupMenuState = 0;
//	}
//}
//
//
//void cNNView::RenderSaveDialog()
//{
//	if (!m_showSaveDialog)
//		return;
//
//	bool isOpen = true;
//	const sf::Vector2u psize((uint)m_rect.Width(), (uint)m_rect.Height());
//	const ImVec2 size(300, 115);
//	const ImVec2 pos(psize.x / 2.f - size.x / 2.f + m_rect.left
//		, psize.y / 2.f - size.y / 2.f + m_rect.top);
//	ImGui::SetNextWindowPos(pos);
//	ImGui::SetNextWindowSize(size);
//	ImGui::SetNextWindowBgAlpha(0.9f);
//
//	if (ImGui::Begin("Save GenoType", &isOpen, 0))
//	{
//		ImGui::Spacing();
//		ImGui::Text("FileName : ");
//		ImGui::SameLine();
//
//		StrPath &fileName = g_geno->m_saveFileName;
//
//		bool isSave = false;
//		const int flags = ImGuiInputTextFlags_AutoSelectAll
//			| ImGuiInputTextFlags_EnterReturnsTrue;
//		if (ImGui::InputText("##fileName", fileName.m_str, fileName.SIZE, flags))
//		{
//			isSave = true;
//		}
//
//		ImGui::Spacing();
//		ImGui::Spacing();
//		ImGui::Spacing();
//		ImGui::Spacing();
//		ImGui::Spacing();
//		const bool isSaveBtnClick = ImGui::Button("Save");
//		if (isSaveBtnClick || isSave)
//		{
//			const StrPath filePath = g_creatureResourcePath + fileName;
//			if (!g_geno->m_selects.empty())
//			{
//				evc::cGNode *gnode = g_geno->FindGNode(*g_geno->m_selects.begin());
//				bool isSave = true;
//				if (filePath.IsFileExist()) // file already exist?
//				{
//					isSave = false;
//
//					Str128 text;
//					text.Format("[ %s ] File Already Exist\nOverWrite?"
//						, filePath.c_str());
//					if (IDYES == ::MessageBoxA(m_owner->getSystemHandle(), text.c_str()
//						, "Confirm", MB_YESNO | MB_ICONWARNING))
//					{
//						isSave = true;
//					}
//				}
//
//				if (isSave)
//				{
//					evc::WriteGenoTypeFileFrom_Node(filePath, gnode);
//				}
//			}
//
//			m_showSaveDialog = false;
//		} //~save operation
//
//		ImGui::SameLine();
//		if (ImGui::Button("Cancel"))
//		{
//			m_showSaveDialog = false;
//		}
//	}
//	ImGui::End();
//
//	if (!isOpen)
//		m_showSaveDialog = false;
//}


void cNNView::OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect)
{
	if (type == eDockResize::DOCK_WINDOW)
	{
		m_owner->RequestResetDeviceNextFrame();
	}
}


// Modify gnode, glink transform by Gizmo
void cNNView::UpdateSelectModelTransform(const bool isGizmoEdit)
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
void cNNView::UpdateSelectModelTransform_GNode()
{
	using namespace physx;

	const int id = (g_geno->m_selects.size() == 1) ? *g_geno->m_selects.begin() : -1;
	evc::cGNode *gnode = g_geno->FindGNode(id);
	evc::cGLink *glink = g_geno->FindGLink(id);
	if (!gnode && !glink)
		return;

	Transform tmp = g_geno->m_gizmo.m_targetTransform;
	tmp.pos.y = max(0.f, tmp.pos.y);

	if (eGizmoEditType::SCALE == g_geno->m_gizmo.m_type)
	{
		const Vector3 delta = g_geno->m_gizmo.m_deltaTransform.scale / 5.f;

		// change 3d model dimension
		switch (gnode->m_prop.shape)
		{
		case phys::eShapeType::Box:
		{
			Vector3 scale = gnode->m_transform.scale;
			switch (g_geno->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: scale.x += delta.x; break;
			case eGizmoEditAxis::Y: scale.y += delta.y; break;
			case eGizmoEditAxis::Z: scale.z += delta.z; break;
			}
			gnode->m_transform.scale = scale.Maximum(Vector3(0, 0, 0));
			g_geno->m_gizmo.UpdateTargetTransform(gnode->m_transform); // update gizmo
		}
		break;

		case phys::eShapeType::Sphere:
		{
			float radius = gnode->GetSphereRadius();
			switch (g_geno->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: radius += delta.x; break;
			case eGizmoEditAxis::Y: radius += delta.y; break;
			case eGizmoEditAxis::Z: radius += delta.z; break;
			}
			radius = max(0.f, radius);
			gnode->SetSphereRadius(radius); // update radius transform
			g_geno->m_gizmo.UpdateTargetTransform(gnode->m_transform); // update gizmo
		}
		break;

		case phys::eShapeType::Capsule:
		{
			const Vector3 delta = g_geno->m_gizmo.m_deltaTransform.scale / 5.f;
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
			const Vector3 delta = g_geno->m_gizmo.m_deltaTransform.scale / 5.f;
			float radius = 1.f;
			switch (g_geno->m_gizmo.m_axisType)
			{
			case eGizmoEditAxis::X: radius = delta.y; break;
			case eGizmoEditAxis::Y: radius = delta.y; break;
			case eGizmoEditAxis::Z: radius = delta.z; break;
			}
			Vector2 dim = gnode->GetCylinderDimension();
			dim.x = max(0.f, dim.x + radius);
			dim.y = max(0.f, dim.y + delta.x);
			gnode->SetCylinderDimension(dim.x, dim.y); // update cylinder transform
			g_geno->m_gizmo.UpdateTargetTransform(gnode->m_transform); // update gizmo
		}
		break;
		}
	}
	else
	{
		gnode->m_transform = tmp;
		g_geno->m_gizmo.UpdateTargetTransform(gnode->m_transform); // update gizmo
	}

	// update link info, if connect link
	for (auto &glink : gnode->m_links)
		glink->UpdateLinkInfo();
}


// update gizmo transform to multi selection object
void cNNView::UpdateSelectModelTransform_MultiObject()
{
	const Transform tfm = g_geno->m_gizmo.m_targetTransform;
	g_geno->m_multiSel.m_transform = tfm;

	const Vector3 realCenter = g_geno->m_multiSelPos - Vector3(0, 0.5f, 0);
	const Vector3 center = g_geno->m_multiSelPos;
	const Vector3 offset = tfm.pos - center;
	g_geno->m_multiSelPos = tfm.pos; // update current position

	const Quaternion rot = tfm.rot * g_geno->m_multiSelRot.Inverse();
	g_geno->m_multiSelRot = tfm.rot; // update current rotation

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
void cNNView::UpdateSelectModelTransform_Link()
{
	const int selectId = (g_geno->m_selects.size() == 1) ? *g_geno->m_selects.begin() : -1;
	evc::cGLink *link = g_geno->FindGLink(selectId);
	if (!link)
		return; // error occurred

	switch (g_geno->m_gizmo.m_type)
	{
	case eGizmoEditType::TRANSLATE:
		link->m_transform.pos += g_geno->m_gizmo.m_deltaTransform.pos;
		link->SetPivotPosByRevolutePos(link->m_transform.pos);
		break;

	case eGizmoEditType::ROTATE:
	{
		const Quaternion delta = g_geno->m_gizmo.m_deltaTransform.rot;
		link->m_transform.rot *= delta;
		link->m_prop.revoluteAxis = link->m_prop.revoluteAxis * delta;
		link->m_prop.rotRevolute = Quaternion(Vector3(1, 0, 0), link->m_prop.revoluteAxis);
		link->SetPivotPosByRevoluteAxis(link->m_prop.revoluteAxis, link->m_transform.pos);
	}
	break;

	case eGizmoEditType::SCALE:
		break;
	}
}


// mouse picking process
bool cNNView::PickingProcess(const POINT &mousePos)
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
		g_geno->SetSelection(glink->m_id);

		g_geno->m_highLights.clear();
		if (glink->m_gnode0)
			g_geno->m_highLights.insert(glink->m_gnode0->m_id);
		if (glink->m_gnode1)
			g_geno->m_highLights.insert(glink->m_gnode1->m_id);

		g_geno->ChangeEditMode(eGenoEditMode::Revolute);
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
int cNNView::PickingNode(const int pickType, const POINT &mousePos
	, OUT float *outDistance //= nullptr
)
{
	const Ray ray = GetMainCamera().GetRay(mousePos.x, mousePos.y);

	int minId = -1;
	float minDist = FLT_MAX;
	for (auto &gnode : g_geno->m_gnodes)
	{
		const bool isSpherePicking = (gnode->m_prop.shape == phys::eShapeType::Sphere);

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


// spawn select node to phenotype view
void cNNView::SpawnSelectNodeToPhenoTypeView()
{
	if (g_geno->m_selects.empty())
		return;

	evc::cGNode *gnode = g_geno->FindGNode(*g_geno->m_selects.begin());
	if (!gnode)
		return;

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


// spawn select iterator node
void cNNView::SpawnSelectIterator()
{
	evc::cGNode *gnode = g_geno->FindGNode(*g_geno->m_selects.begin());
	if (!gnode)
		return;

	g_geno->AutoSave();

	evc::cGNode *clone = nullptr;
	if (gnode->m_prop.iteration >= 0)
	{
		// find original genotype node
		if (evc::cGNode *p = g_geno->FindGNode(gnode->m_prop.iteration))
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


// delete select node
void cNNView::DeleteSelectNode()
{
	g_geno->AutoSave();

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
}


// delete select node link
void cNNView::DeleteSelectLink()
{
	g_geno->AutoSave();

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


void cNNView::UpdateLookAt()
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
		GetMainCamera().UpdateViewMatrix();
	}
}


// 휠을 움직였을 때,
// 카메라 앞에 박스가 있다면, 박스 정면에서 멈춘다.
void cNNView::OnWheelMove(const float delta, const POINT mousePt)
{
	if (m_showGenomeFileList
		//|| m_showSaveDialog || (m_popupMenuState > 0)
		)
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

	// active genotype editor view
	//m_owner->SetActiveWindow(g_global->m_geditorView);
}


// Handling Mouse Move Event
void cNNView::OnMouseMove(const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	if (g_geno->m_gizmo.IsKeepEditMode())
		return;
	if (m_showGenomeFileList
		//|| m_showSaveDialog || (m_popupMenuState > 0)
		)
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

		GetMainCamera().MoveRight(-delta.x * m_rotateLen * 0.0005f);
		GetMainCamera().MoveFrontHorizontal(delta.y * m_rotateLen * 0.0005f);
	}
	else if (m_mouseDown[1])
	{
		const float scale = 0.001f;
		if (g_geno->m_orbitTarget.Distance(GetMainCamera().GetEyePos()) > 25.f)
		{
			// cancel orbit moving
			g_geno->m_orbitId = -1;
		}

		if (g_geno->m_orbitId >= 0)
		{
			m_camera.Yaw3(delta.x * scale, g_geno->m_orbitTarget);
			m_camera.Pitch3(delta.y * scale, g_geno->m_orbitTarget);
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
void cNNView::OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt)
{
	m_mousePos = mousePt;
	m_mouseClickPos = mousePt;
	UpdateLookAt();
	SetCapture();

	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
	const Plane groundPlane(Vector3(0, 1, 0), 0);
	const Vector3 target = groundPlane.Pick(ray.orig, ray.dir);
	m_rotateLen = ray.orig.y * 0.9f;// (target - ray.orig).Length();

	if (m_showGenomeFileList 
		//m_showSaveDialog || (m_popupMenuState > 0)
		)
		return;

	// active genotype editor view
	//m_owner->SetActiveWindow(g_global->m_geditorView);

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

		const int id = PickingNode(0, mousePt);
		evc::cGNode *gnode = g_geno->FindGNode(id);
		if (gnode)
		{
			g_geno->m_orbitId = id;
			g_geno->m_orbitTarget = gnode->m_transform.pos;
		}
	}
	break;

	case sf::Mouse::Middle:
		m_mouseDown[2] = true;
		break;
	}
}


void cNNView::OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	ReleaseCapture();

	if (m_showGenomeFileList
		//|| m_showSaveDialog || (m_popupMenuState > 0)
		)
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


void cNNView::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		if ((m_owner->GetFocus() != this)
			&& (m_owner->GetFocus() != (framework::cDockWindow*)g_global->m_resView))
			break;

		switch (evt.key.cmd)
		{
		case sf::Keyboard::Return: break;
		case sf::Keyboard::Space: break;
		case sf::Keyboard::Home: break;
		case sf::Keyboard::Tilde:
			m_camera.SetCamera(Vector3(30, 20, -30), Vector3(0, 0, 0), Vector3(0, 1, 0));
			break;

		case sf::Keyboard::R: if (!m_showSaveDialog && (m_popupMenuState != 2)) g_geno->m_gizmo.m_type = graphic::eGizmoEditType::ROTATE; break;
		case sf::Keyboard::T: if (!m_showSaveDialog && (m_popupMenuState != 2)) g_geno->m_gizmo.m_type = graphic::eGizmoEditType::TRANSLATE; break;
		case sf::Keyboard::S: if (!m_showSaveDialog && (m_popupMenuState != 2)) g_geno->m_gizmo.m_type = graphic::eGizmoEditType::SCALE; break;
		case sf::Keyboard::H: if (!m_showSaveDialog && (m_popupMenuState != 2)) g_geno->m_gizmo.m_type = graphic::eGizmoEditType::None; break;
		case sf::Keyboard::F5: g_pheno->RefreshResourceView(); break;

		case sf::Keyboard::Tab:
			if (::GetAsyncKeyState(VK_CONTROL))
			{
				framework::cDockWindow *wnd = m_owner->SetActiveNextTabWindow(this);
				if (wnd)
				{
					if (wnd->m_name == "3D View")
						m_owner->SetActiveWindow(g_global->m_peditorView);
					else if (wnd->m_name == "GenoType View")
						m_owner->SetActiveWindow(g_global->m_geditorView);
					m_owner->SetFocus(wnd);
				}
			}
			break;

		case sf::Keyboard::Escape:
			g_geno->m_orbitId = -1;

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
				g_geno->AutoSave();
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
			if (m_popupMenuState == 2)
			{
				SpawnSelectNodeToPhenoTypeView();
				m_popupMenuState = 3;
			}
			break;

		case sf::Keyboard::J: // popup menu shortcut, joint remove
			if (m_popupMenuState == 2)
			{
				DeleteSelectLink();
				m_popupMenuState = 3; // close popup
			}
			break;

		case sf::Keyboard::D:
			if (m_popupMenuState == 2)
			{
				DeleteSelectNode();
				m_popupMenuState = 3; // close popup
			}
			break;

		case sf::Keyboard::I: // spawn iterator
			if (m_popupMenuState == 2)
			{
				SpawnSelectIterator();
				m_popupMenuState = 3; // close popup
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


void cNNView::OnResetDevice()
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
