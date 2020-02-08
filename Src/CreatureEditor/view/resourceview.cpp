
#include "stdafx.h"
#include "resourceview.h"
#include "3dview.h"
#include "genoview.h"
#include "../creature/creature.h"

using namespace graphic;

cResourceView::cResourceView(const StrId &name)
	: framework::cDockWindow(name)
	, m_selectFileIdx(-1)
{
	//UpdateResourceFiles();
}

cResourceView::~cResourceView()
{
}


void cResourceView::OnUpdate(const float deltaSeconds)
{
}


void cResourceView::OnRender(const float deltaSeconds)
{
	static ImGuiTextFilter filter;
	ImGui::Text("Search");
	ImGui::SameLine();
	filter.Draw("##Search", 200);

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.9f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0, 1));
	if (ImGui::Button("Refresh (F5)"))
		UpdateResourceFiles();
	ImGui::PopStyleColor(3);

	int i = 0;
	bool isOpenPopup = false;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Always);
	if (ImGui::TreeNode((void*)0, "Creature Files"))
	{
		ImGui::Columns(4, "texturecolumns5", false);
		for (auto &str : m_fileList)
		{
			const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow 
				| ImGuiTreeNodeFlags_OpenOnDoubleClick
				| ImGuiTreeNodeFlags_Leaf 
				| ImGuiTreeNodeFlags_NoTreePushOnOpen
				| ((i == m_selectFileIdx) ? ImGuiTreeNodeFlags_Selected : 0);

			if (filter.PassFilter(str.c_str()))
			{
				ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags, str.c_str());

				if (ImGui::IsItemClicked(1))
				{
					m_selectFileIdx = i;
					isOpenPopup = true;
				}

				if (ImGui::IsItemClicked())
				{
					m_selectFileIdx = i;
					if (ImGui::IsMouseDoubleClicked(0))
					{
						// create creature
						const StrPath fileName = StrPath("./media/creature/") + str;
						LoadPhenotypeView(fileName);
						//LoadGenotypeView(fileName);

					}//~IsDoubleClicked
				}//~IsItemClicked
				ImGui::NextColumn();
			}//~PassFilter
			++i;
		}
		ImGui::TreePop();
	}

	if (isOpenPopup)
		ImGui::OpenPopup("PopupMenu");

	RenderPopupMenu();
}


void cResourceView::RenderPopupMenu()
{
	if (m_selectFileIdx < 0)
		return;

	if (ImGui::BeginPopup("PopupMenu"))
	{
		if (ImGui::MenuItem("Spawn PhenoType View"))
		{
			if (m_fileList.size() > (uint)m_selectFileIdx)
			{
				const StrPath fileName = StrPath("./media/creature/") 
					+ m_fileList[m_selectFileIdx];
				LoadPhenotypeView(fileName);
			}
		}
		if (ImGui::MenuItem("Spawn GenoType View"))
		{
			if (m_fileList.size() > (uint)m_selectFileIdx)
			{
				const StrPath fileName = StrPath("./media/creature/")
					+ m_fileList[m_selectFileIdx];
				LoadGenotypeView(fileName);
			}
		}

		ImGui::EndPopup();
	}
}


// phenotype view load
void cResourceView::LoadPhenotypeView(const StrPath &fileName)
{
	const graphic::cCamera3D &camera = g_global->m_3dView->m_camera;
	const Vector2 size(camera.m_width, camera.m_height);
	const Ray ray = camera.GetRay((int)size.x / 2, (int)size.y / 2 + (int)size.y / 5);
	const Plane ground(Vector3(0, 1, 0), 0);
	const Vector3 targetPos = ground.Pick(ray.orig, ray.dir);
	g_pheno->ReadCreatureFile(fileName, targetPos);
}


// genotype view load
void cResourceView::LoadGenotypeView(const StrPath &fileName)
{
	const graphic::cCamera3D &camera = g_global->m_genoView->m_camera;
	const Vector2 size(camera.m_width, camera.m_height);
	const Ray ray = camera.GetRay((int)size.x / 2, (int)size.y / 2 + (int)size.y / 5);
	const Plane ground(Vector3(0, 1, 0), 0);
	const Vector3 targetPos = ground.Pick(ray.orig, ray.dir);
	g_geno->ReadCreatureFile(fileName, targetPos);
}


void cResourceView::UpdateResourceFiles()
{
	m_fileList.clear();

	list<string> fileList;
	list<string> exts;
	exts.push_back(".pnt");
	exts.push_back(".gnt");
	const StrPath dir = StrPath("./media/creature/").GetFullFileName();
	common::CollectFiles2(exts, dir.c_str(), dir.c_str(), fileList);

	m_fileList.reserve(fileList.size());
	std::copy(fileList.begin(), fileList.end(), std::back_inserter(m_fileList));
}
