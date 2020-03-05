
#include "stdafx.h"
#include "resourceview.h"
#include "3dview.h"
#include "genoview.h"
#include "../creature/creature.h"

using namespace graphic;

cResourceView::cResourceView(const StrId &name)
	: framework::cDockWindow(name)
	, m_selectFileIdx(-1)
	, m_dirPath(g_creatureResourcePath)
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

	const bool isOpenPopup = RenderFileList(filter);
	if (isOpenPopup)
		ImGui::OpenPopup("PopupMenu");

	RenderPopupMenu();
}


// return popup menu window?
bool cResourceView::RenderFileList(ImGuiTextFilter &filter)
{
	bool isOpenPopup = false;

	if (ImGui::BeginChild("Resource FileList", ImVec2(0,0), true))
	{
		ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode((void*)0, "Creature File List"))
		{
			ImGui::Columns(4, "creaturecolumns4", false);
			int i = 0;
			for (auto &str : m_creatureFileList)
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
							const StrPath fileName = m_dirPath + str;
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

		ImGui::Columns(1, "genomecolumns1", false);
		ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode((void*)1, "Genome File List"))
		{
			ImGui::Columns(4, "genomecolumns4", false);
			int i = 0;
			for (auto &str : m_genomeFileList)
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
							const StrPath fileName = m_dirPath + str;
							LoadGenomeFile(fileName);

						}//~IsDoubleClicked
					}//~IsItemClicked
					ImGui::NextColumn();
				}//~PassFilter
				++i;
			}
			ImGui::TreePop();
		}
	}
	ImGui::EndChild();

	return isOpenPopup;
}


void cResourceView::RenderPopupMenu()
{
	if (m_selectFileIdx < 0)
		return;

	if (ImGui::BeginPopup("PopupMenu"))
	{
		if (ImGui::MenuItem("Spawn PhenoType View"))
		{
			if (m_creatureFileList.size() > (uint)m_selectFileIdx)
			{
				const StrPath fileName = m_dirPath + m_creatureFileList[m_selectFileIdx];
				LoadPhenotypeView(fileName);
			}
		}
		if (ImGui::MenuItem("Spawn GenoType View"))
		{
			if (m_creatureFileList.size() > (uint)m_selectFileIdx)
			{
				const StrPath fileName = m_dirPath + m_creatureFileList[m_selectFileIdx];
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
	g_pheno->m_saveFileName = fileName.GetFileNameExceptExt() + ".pnt"; // update save file dialog filename
	g_pheno->m_saveGenomeFileName = fileName.GetFileNameExceptExt() + ".gen"; // update save file dialog filename
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
	g_geno->m_saveFileName = fileName.GetFileName(); // update save file dialog filename
}


// load genome file and put selection creature
void cResourceView::LoadGenomeFile(const StrPath &fileName)
{
	const uint cnt = g_genome->SetGenomeSelectCreature(fileName);

	Str128 text;
	text.Format("Update Creature Genome [ %d ]", cnt);
	::MessageBoxA(m_owner->getSystemHandle()
		, text.c_str(), "Confirm", MB_OK | MB_ICONINFORMATION);
}


void cResourceView::UpdateResourceFiles()
{
	m_creatureFileList.clear();
	m_genomeFileList.clear();
	m_evolutionFileList.clear();

	// load creature file list
	{
		list<string> fileList;
		list<string> exts;
		exts.push_back(".pnt");
		exts.push_back(".gnt");
		const StrPath dir = m_dirPath.GetFullFileName();
		common::CollectFiles2(exts, dir.c_str(), dir.c_str(), fileList);

		m_creatureFileList.reserve(fileList.size());
		std::copy(fileList.begin(), fileList.end(), std::back_inserter(m_creatureFileList));
	}

	// load genome file list
	{
		list<string> fileList;
		list<string> exts;
		exts.push_back(".gen");
		const StrPath dir = m_dirPath.GetFullFileName();
		common::CollectFiles2(exts, dir.c_str(), dir.c_str(), fileList);

		m_genomeFileList.reserve(fileList.size());
		std::copy(fileList.begin(), fileList.end(), std::back_inserter(m_genomeFileList));
	}

	// load evolution genome file list
	{
		list<string> fileList;
		list<string> exts;
		exts.push_back(".gen");
		const StrPath dir = g_evolutionResourcePath.GetFullFileName();
		common::CollectFiles2(exts, dir.c_str(), dir.c_str(), fileList);

		m_evolutionFileList.reserve(fileList.size());
		std::copy(fileList.begin(), fileList.end(), std::back_inserter(m_evolutionFileList));
	}
}


// return select file list, file name
StrPath cResourceView::GetSelectFileName()
{
	if (m_selectFileIdx < 0)
		return "";
	if (m_selectFileIdx >= (int)m_creatureFileList.size())
		return "";

	StrPath fileName = m_dirPath + m_creatureFileList[m_selectFileIdx];
	return fileName;
}
