
#include "stdafx.h"
#include "evolutionview.h"
#include "resourceview.h"

using namespace graphic;


cEvolutionView::cEvolutionView(const StrId &name)
	: framework::cDockWindow(name)
	, m_spawnSize(100)
	, m_epochSize(1000)
	, m_epochTime(100.f)
	, m_generation(3)
{
}

cEvolutionView::~cEvolutionView()
{
}


void cEvolutionView::OnUpdate(const float deltaSeconds)
{
}


void cEvolutionView::OnRender(const float deltaSeconds)
{
	cResourceView *resView = g_global->m_resView;

	if (cEvolutionManager::eState::Run == g_evo->m_state)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.1f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.1f, 0, 1));
		ImGui::SetCursorPosX(20);
		if (ImGui::Button("Evolution - Stop", ImVec2(0, 0)))
		{
			if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
				, "Stop Evolution?", "Confirm", MB_YESNO | MB_ICONWARNING))
			{
				g_evo->StopEvolution();
			}
			else
			{
				// nothing~
			}
		}
		ImGui::PopStyleColor(3);
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.9f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0, 1));
		ImGui::SetCursorPosX(20);
		if (ImGui::Button("Evolution - Run", ImVec2(0, 0)))
		{
			const StrPath fileName = resView->GetSelectFileName();
			if (fileName.empty())
			{
				::MessageBoxA(m_owner->getSystemHandle()
					, "Please Select Spawn Creature File from Resource View"
					, "Error", MB_OK | MB_ICONERROR);
			}
			else
			{
				if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
					, "Start Evolution?", "Confirm", MB_YESNO | MB_ICONQUESTION))
				{
					sEvolutionParam param;
					param.creatureFileName = fileName;
					param.spawnSize = (uint)m_spawnSize;
					param.epochSize = (uint)m_epochSize;
					param.epochTime = m_epochTime;
					param.generation = (uint)m_generation;
					g_evo->RunEvolution(param);
				}
				else
				{
					// nothing~
				}
			}
		}
		ImGui::PopStyleColor(3);
	}

	ImGui::Separator();

	if (cEvolutionManager::eState::Run == g_evo->m_state)
	{
		ImGui::Text("Time : %.3f", g_evo->m_incT);
		ImGui::Text("Epoch : %d", g_evo->m_curEpochSize);
		ImGui::Text("Epoch Time : %f", g_evo->m_param.epochTime);
		ImGui::Separator();
	}

	ImGui::TextUnformatted("Spawn Creature : ");
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 0, 1));
	if (resView->m_selectFileIdx >= 0)
	{
		if (resView->m_selectFileIdx < (int)resView->m_fileList.size())
		{
			ImGui::SameLine();
			ImGui::Text(resView->m_fileList[resView->m_selectFileIdx].c_str());
		}
	}
	ImGui::PopStyleColor();

	ImGui::PushItemWidth(180);
	ImGui::TextUnformatted("Spawn Size :         ");
	ImGui::SameLine();
	ImGui::InputInt("##Spawn Size", &m_spawnSize, 1);

	ImGui::TextUnformatted("Epoch Size :          ");
	ImGui::SameLine();
	ImGui::InputInt("##Epoch Size", &m_epochSize, 1);

	ImGui::PushItemWidth(150);
	ImGui::TextUnformatted("Epoch Time (Seconds): ");
	ImGui::SameLine();
	ImGui::DragFloat("##Epoch Time", &m_epochTime, 0.01f, 0.f, 1000.f);
	ImGui::PopItemWidth();

	ImGui::TextUnformatted("Generation :         ");
	ImGui::SameLine();
	ImGui::InputInt("##Generation", &m_generation, 1);

	ImGui::PopItemWidth();

}


void cEvolutionView::OnEventProc(const sf::Event &evt)
{
	if ((GetFocus() == this)
		&& (evt.type == sf::Event::KeyPressed)
		&& (evt.key.cmd == sf::Keyboard::Tab))
	{
		if (::GetAsyncKeyState(VK_CONTROL))
		{
			framework::cDockWindow *wnd = m_owner->SetActiveNextTabWindow(this);
			if (wnd)
				m_owner->SetFocus(wnd);
		}
	}
}
