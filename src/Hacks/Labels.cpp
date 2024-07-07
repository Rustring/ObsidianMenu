#include "Labels.h"
#include "../ConstData.h"
#include "../GUI/GUI.h"
#include "../JsonPatches/JsonPatches.h"
#include "../Settings.hpp"
#include "Hacks/SafeMode.h"
#include "Hacks/ShowTrajectory.h"
#include "Macrobot/Macrobot.h"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace Labels;

Labels::Label Labels::setupLabel(const std::string& labelSettingName,
								 const std::function<void(cocos2d::CCLabelBMFont*)>& function,
								 cocos2d::CCLayer* playLayer)
{
	int opacity = Settings::get<int>("labels/" + labelSettingName + "/opacity", 150);

	auto label = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
	label->setZOrder(1000);
	label->setScale(0.4f);
	label->setOpacity(opacity);

	Label l;
	l.pointer = label;
	l.settingName = labelSettingName;
	l.function = function;

	labels.push_back(l);
	playLayer->addChild(l.pointer);

	return l;
}

class $modify(PlayLayer)
{

	bool init(GJGameLevel* p0, bool p1, bool p2)
	{
		levelCompleted = false;
		labels.clear();
		bool res = PlayLayer::init(p0, p1, p2);

		setupLabel(
			"Cheat Indicator",
			[&](cocos2d::CCLabelBMFont* pointer) {
				pointer->setString(".");
				pointer->setScale(0.85f);

				if (!Common::isCheating)
					pointer->setColor({ 0, 255, 0 });
				else
					pointer->setColor({ 255, 0, 0 });
			},
			this
		);

		setupLabel(
			"Framerate",
			[&](cocos2d::CCLabelBMFont* pointer) {
			pointer->setString(std::to_string((int)std::roundf(ImGui::GetIO().Framerate)).c_str());
			},
			this
		);

		setupLabel(
			"CPS",
			[&](cocos2d::CCLabelBMFont* pointer) {
				float currentTime = GetTickCount();
				for (int i = 0; i < clicks.size(); i++)
				{
					if ((currentTime - clicks[i]) / 1000.0f >= 1)
						clicks.erase(clicks.begin() + i);
				}
				pointer->setString(
					cocos2d::CCString::createWithFormat("%i/%i CPS", clicks.size(), totalClicks)->getCString());

				if (click)
				{
					float clickColor[3];
					clickColor[0] = Settings::get<float>("labels/CPS/color/r", 1) * 255.f;
					clickColor[1] = Settings::get<float>("labels/CPS/color/g", 0.2f) * 255.f;
					clickColor[2] = Settings::get<float>("labels/CPS/color/b", 0.2f) * 255.f;

					pointer->setColor({(GLubyte)clickColor[0], (GLubyte)clickColor[1], (GLubyte)clickColor[2]});
				}
				else
					pointer->setColor({ 255, 255, 255 });
			},
			this
		);

		setupLabel(
			"Time",
			[&](cocos2d::CCLabelBMFont* pointer) {
				auto t = std::time(nullptr);
				auto tm = *std::localtime(&t);
				std::ostringstream s;

				bool h12 = Settings::get<bool>("labels/Time/12h", false);

				if (!h12)
					s << std::put_time(&tm, "%H:%M:%S");
				else
					s << std::put_time(&tm, "%I:%M:%S %p");

				pointer->setString(s.str().c_str());
			},
			this
		);

		setupLabel(
			"Noclip Accuracy",
			[&](cocos2d::CCLabelBMFont* pointer) {
				float p = (float)(frames - deaths) / (float)frames;
				float acc = p * 100.f;
				float limit = Settings::get<float>("labels/Noclip Accuracy/limit", 0.f);

				pointer->setColor(dead ? ccColor3B(0, 255, 0) : ccColor3B(255, 255, 255));

				if (!noclipDead)
				{
					if (acc <= limit && limit > 0)
						noclipDead = true;
					pointer->setString(
						frames == 0
							? "Accuracy: 100.00%"
							: cocos2d::CCString::createWithFormat("Accuracy: %.2f%%", acc)->getCString()
					);
				}
			},
			this
		);

		setupLabel(
			"Noclip Deaths",
			[&](cocos2d::CCLabelBMFont* pointer) {
				pointer->setColor(dead ? ccColor3B(0, 255, 0) : ccColor3B(255, 255, 255));
				pointer->setString(cocos2d::CCString::createWithFormat("Deaths: %i", realDeaths)->getCString());
			},
			this
		);

		setupLabel(
			"Custom Message",
			[&](cocos2d::CCLabelBMFont* pointer) {
				std::string message = Settings::get<std::string>("labels/Custom Message/message", "Hello");
				pointer->setString(message.c_str());
			},
			this
		);

		setupLabel(
			"Best Run",
			[&](cocos2d::CCLabelBMFont* pointer) {
			if (bestRun.first == 0)
			{
				if (bestRun.second == 0)
					pointer->setString("Best Run: None");
				else
					pointer->setString(fmt::format("Best Run: {}%", (int)bestRun.second).c_str());
			}
			else
				pointer->setString(fmt::format("Best Run: {}-{}", (int)bestRun.first, (int)bestRun.second).c_str());
			},
			this
		);

		setupLabel(
			"Attempts",
			[&](cocos2d::CCLabelBMFont* pointer) {
				pointer->setString(fmt::format("{} Attempts", this->m_level->m_attempts.value()).c_str());
			},
			this
		);

		//keep this at the bottom :)
		setupLabel(
			"Macro Info",
			[&](cocos2d::CCLabelBMFont* pointer) {
			if (Macrobot::playerMode == Macrobot::PLAYBACK)
				pointer->setString(fmt::format("Playing {}/{}", Macrobot::actionIndex, Macrobot::macro.inputs.size()).c_str());
			else if (Macrobot::playerMode == Macrobot::RECORDING)
				pointer->setString(fmt::format("Recording {}", Macrobot::macro.inputs.size()).c_str());
			else
				pointer->setString("");
			},
			this
		);

		calculatePositions();

		labelsCreated = true;

		GJBaseGameLayerProcessCommands(this);

		return res;
	}

	void destroyPlayer(PlayerObject* player, GameObject* object)
	{
		if (!anticheatSpike && frames < 5)
			anticheatSpike = object;

		if (object == anticheatSpike)
			return PlayLayer::destroyPlayer(player, object);

		if (noclipDead)
			JsonPatches::togglePatch(JsonPatches::player, "NoClip", false);

		PlayLayer::destroyPlayer(player, object);

		if (noclipDead)
			JsonPatches::togglePatch(JsonPatches::player, "NoClip", true);

		if (player && player->m_isDead)
		{
			currentRun.second = this->getCurrentPercent();

			float currentRunTotal = currentRun.second - currentRun.first;
			float bestRunTotal = bestRun.second - bestRun.first;

			if (currentRunTotal > bestRunTotal)
				bestRun = currentRun;
		}

		dead = true;
	}

	void levelComplete()
	{
		PlayLayer::levelComplete();
		currentRun.second = PlayLayer::getCurrentPercent();
		levelCompleted = true;

		float currentRunTotal = currentRun.second - currentRun.first;
		float bestRunTotal = bestRun.second - bestRun.first;
		if (currentRunTotal > bestRunTotal)
			bestRun = currentRun;
	}

	void resetLevel()
	{
		Common::updateCheating();
		SafeMode::updateAuto();
		noclipDead = false;
		levelCompleted = false;
		dead = false;
		totalClicks = 0;
		frames = 0;
		deaths = 0;
		realDeaths = 0;
		clicks.clear();
		currentRun.second = 0;
		PlayLayer::resetLevel();

		anticheatSpike = nullptr;

		currentRun.first = PlayLayer::getCurrentPercent();
	}

	void onQuit()
	{
		bestRun.first = 0;
		bestRun.second = 0;
		currentRun.first = 0;
		currentRun.second = 0;
		labels.clear();
		PlayLayer::onQuit();
	}
};

class $modify(LevelEditorLayer)
{
	bool init(GJGameLevel* level, bool unk)
	{
		bestRun.first = 0;
		bestRun.second = 0;
		currentRun.first = 0;
		currentRun.second = 0;
		labels.clear();

		return LevelEditorLayer::init(level, unk);
	}
};

void Labels::GJBaseGameLayerProcessCommands(GJBaseGameLayer *self)
{
	if (!GameManager::sharedState()->getPlayLayer()) 
	{
		reinterpret_cast<void(__thiscall *)(GJBaseGameLayer *)>(base::get() + 0x1BD240)(self);
		return;
	}

	if (labelsCreated)
	{
		bool hide = Settings::get<bool>("labels/hideAll", false);
		for (Label& l : labels)
		{
			int opacity = Settings::get<int>("labels/" + l.settingName + "/opacity", 150);
			l.pointer->setOpacity(hide ? 0 : opacity);
			
			if(!hide)
				l.process();
		}
	}

	clickRegistered = false;

	if(!levelCompleted)
		frames++;

	if (dead)
	{
		deaths++;

		if (!lastFrameDead)
			realDeaths++;
	}

	lastFrameDead = dead;
	dead = false;

	reinterpret_cast<void(__thiscall *)(GJBaseGameLayer *)>(base::get() + 0x1BD240)(self);
}

$execute
{
	Mod::get()->hook(reinterpret_cast<void *>(base::get() + 0x1BD240), &GJBaseGameLayerProcessCommands, "GJBaseGameLayer::processCommands", tulip::hook::TulipConvention::Thiscall);
}

class $modify(PlayerObject)
{
	void pushButton(PlayerButton btn)
	{
		if(ShowTrajectory::isSimulation)
			return PlayerObject::pushButton(btn);

		if (!clickRegistered)
		{
			clicks.push_back(GetTickCount());
			clickRegistered = true;
		}
		if(!click)
			totalClicks++;
		
		click = true;
		PlayerObject::pushButton(btn);
	}

	void releaseButton(PlayerButton btn)
	{
		if(!ShowTrajectory::isSimulation)
			click = false;
		
		PlayerObject::releaseButton(btn);
	}
};

void Labels::calculatePositions()
{
	if (!GameManager::sharedState()->getPlayLayer()) return;

	auto size = cocos2d::CCDirector::sharedDirector()->getWinSize();

	tl.clear();
	tr.clear();
	bl.clear();
	br.clear();

	for (Label& l : labels)
	{
		int position = Settings::get<int>("labels/" + l.settingName + "/position", 0);

		switch (position)
		{
		case 0:
			l.pointer->setAnchorPoint({ 0.f, 0.5f });
			if (l.settingName == "Cheat Indicator")
				l.pointer->setAnchorPoint({ 0.f, 0.2f });
			tl.push_back(l);
			break;
		case 1:
			l.pointer->setAnchorPoint({ 1.f, 0.5f });
			if (l.settingName == "Cheat Indicator")
				l.pointer->setAnchorPoint({ 1.f, 0.2f });
			tr.push_back(l);
			break;
		case 2:
			l.pointer->setAnchorPoint({ 0.f, 0.5f });
			if (l.settingName == "Cheat Indicator")
				l.pointer->setAnchorPoint({ 0.f, 0.2f });
			bl.push_back(l);
			break;
		case 3:
			l.pointer->setAnchorPoint({ 1.f, 0.5f });
			if (l.settingName == "Cheat Indicator")
				l.pointer->setAnchorPoint({ 1.f, 0.2f });
			br.push_back(l);
			break;
		}
	}

	size_t counter = 0;
	for (auto label : tl)
	{
		if (Settings::get<bool>("labels/" + label.settingName + "/enabled", false))
		{
			label.pointer->setPositionX(5.f);
			label.pointer->setPositionY(size.height - 10 - (15 * counter));
			counter++;
		}
		else
			label.pointer->setPositionX(-500.f);
	}

	counter = 0;
	for (auto label : tr)
	{
		if (Settings::get<bool>("labels/" + label.settingName + "/enabled", false))
		{
			label.pointer->setPositionX(size.width - 5.f);
			label.pointer->setPositionY(size.height - 10 - (15 * counter));
			counter++;
		}
		else
			label.pointer->setPositionX(-500.f);
	}

	counter = 0;
	for (auto label : bl)
	{
		if (Settings::get<bool>("labels/" + label.settingName + "/enabled", false))
		{
			label.pointer->setPositionX(5.f);
			label.pointer->setPositionY(10 + (15 * counter));
			counter++;
		}
		else
			label.pointer->setPositionX(-500.f);
	}

	counter = 0;
	for (auto label : br)
	{
		if (Settings::get<bool>("labels/" + label.settingName + "/enabled", false))
		{
			label.pointer->setPositionX(size.width - 5.f);
			label.pointer->setPositionY(10 + (15 * counter));
			counter++;
		}
		else
			label.pointer->setPositionX(-500.f);
	}
}

void Labels::settingsForLabel(const std::string& labelSettingName, std::function<void()> extraSettings)
{
	if (GUI::checkbox(labelSettingName.c_str(), "labels/" + labelSettingName + "/enabled"))
		calculatePositions();

	GUI::arrowButton("Settings##" + labelSettingName);

	GUI::modalPopup("Settings##" + labelSettingName, [&] {
		int position = Settings::get<int>("labels/" + labelSettingName + "/position", 0); // 0 tl 1 tr 2 bl 3 br
		if (GUI::combo("Position##" + labelSettingName, &position, positions, 4))
		{
			Mod::get()->setSavedValue<int>("labels/" + labelSettingName + "/position", position);
			calculatePositions();
		}

		int opacity = Settings::get<int>("labels/" + labelSettingName + "/opacity", 150);

		if (GUI::inputInt("Opacity##" + labelSettingName, &opacity, 10, 255))
			Mod::get()->setSavedValue<int>("labels/" + labelSettingName + "/opacity", opacity);

		extraSettings();
	});
}

void Labels::drawWindow()
{
	GUI::checkbox("Hide All", "labels/hideAll");
	settingsForLabel("Cheat Indicator", [] {});
	settingsForLabel("Framerate", [] {});
	settingsForLabel("CPS", [] {
		float clickColor[3];
		clickColor[0] = Settings::get<float>("labels/CPS/color/r", 1.f);
		clickColor[1] = Settings::get<float>("labels/CPS/color/g", 0.f);
		clickColor[2] = Settings::get<float>("labels/CPS/color/b", 0.f);

		if (GUI::colorEdit("Click color", clickColor))
		{
			Mod::get()->setSavedValue<float>("labels/CPS/color/r", clickColor[0]);
			Mod::get()->setSavedValue<float>("labels/CPS/color/g", clickColor[1]);
			Mod::get()->setSavedValue<float>("labels/CPS/color/b", clickColor[2]);
		}
	});
	settingsForLabel("Time", [] { GUI::checkbox("Use 12h format", "labels/Time/12h"); });
	settingsForLabel("Noclip Accuracy", [] {
		float limit = Settings::get<float>("labels/Noclip Accuracy/limit", 0.f);

		if (GUI::inputFloat("Limit", &limit, 0, 100))
			Mod::get()->setSavedValue<float>("labels/Noclip Accuracy/limit", limit);
	});
	settingsForLabel("Noclip Deaths", [] { });

	settingsForLabel("Custom Message", [] {
		std::string message = Settings::get<std::string>("labels/Custom Message/message", "Hello");

		if (GUI::inputText("Message", &message))
			Mod::get()->setSavedValue<std::string>("labels/Custom Message/message", message);
	});

	settingsForLabel("Best Run", [] { });

	settingsForLabel("Attempts", [] { });

	settingsForLabel("Macro Info", [] { });
}
