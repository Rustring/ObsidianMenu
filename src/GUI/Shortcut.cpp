#include "Shortcut.h"
#include "GUI.h"

#include <Geode/platform/windows.hpp>

#include "../Common.h"
#include "../ConstData.h"
#include "Widgets.h"

int shortcutIndexKey = 0;

bool GUI::Shortcut::handleShortcut(const std::string& name)
{
	if (GUI::shouldRender())
	{
		bool openPopup = false;

		if (
			ImGui::IsItemHovered() &&
			ImGui::IsKeyDown(ImGuiKey_ModShift) &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Left)
			)
			ImGui::OpenPopup(name.c_str());

		if (ImGui::BeginPopupContextItem(name.c_str()))
		{
			if (ImGui::MenuItem(("Add shortcut##" + name).c_str()))
				openPopup = true;

			ImGui::EndPopup();
		}

		if (openPopup == true)
		{
			ImGui::OpenPopup(("Create shortcut for " + name).c_str());
			openPopup = false;
		}

		if (ImGui::BeginPopupModal(("Create shortcut for " + name).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			GUI::hotkey("Shortcut Key", &shortcutIndexKey);

			if (ImGui::Button("Add"))
			{
				GUI::Shortcut s(shortcutIndexKey, name);
				GUI::shortcuts.push_back(s);
				ImGui::CloseCurrentPopup();
			}
			GUI::sameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
	}

	if (GUI::currentShortcut == name)
	{
		GUI::currentShortcut = "";
		return true;
	}

	return false;
}

void GUI::Shortcut::drawWindow()
{
	if (GUI::shouldRender())
	{
		if (GUI::shortcuts.size() <= 0)
		{
			ImGui::Text("Shift + Left Click an option\nto make a shortcut");
			ImGui::Spacing();
		}
		else
		{
			int uid = 0;
			for (GUI::Shortcut& s : GUI::shortcuts)
			{
				ImGui::AlignTextToFramePadding();
				int displayKeyIndex = s.key - 511;
				if (displayKeyIndex < 0)
					displayKeyIndex = 0;
				ImGui::Text(KeyNames[displayKeyIndex]);
				GUI::sameLine();
				ImGui::Text(s.name.c_str());
				GUI::sameLine(ImGui::GetWindowSize().x - 30);
				if (GUI::button(std::format("x##{}", std::to_string(uid))))
				{
					GUI::shortcuts.erase(GUI::shortcuts.begin() + uid);
					GUI::save();
				}

				ImGui::Separator();
				uid++;
			}
		}
	}

	bool vsync = GameManager::get()->getGameVariable("0030");

	if (GUI::checkbox("Vsync", &vsync))
	{
		GameManager::get()->setGameVariable("0030", vsync);
		Common::calculateFramerate();
	}

	if (GUI::button("Uncomplete Level"))
		Common::uncompleteLevel();

	if (GUI::button("Toggle Practice") && PlayLayer::get())
		PlayLayer::get()->togglePracticeMode(!PlayLayer::get()->m_isPracticeMode);

	if (GUI::button("Reset Level") && PlayLayer::get())
		PlayLayer::get()->resetLevel();
}