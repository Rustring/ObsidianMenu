#pragma once

#include <cocos2d.h>
#include <gdr.hpp>
#include <vector>
#include <unordered_map>

#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

namespace Macrobot
{
	enum PlayerMode
	{
		DISABLED = -1,
		PLAYBACK = 0,
		RECORDING = 1
	};

	enum CorrectionType
	{
		NONE = 0,
		ACTION = 1
	};

	class MPlayerCheckpoint
	{
	public:
		double yVel;
		double xVel;
		float xPos;
		float yPos;
		float rotation;
		float rotationRate;
		GameObject* lastSnappedTo = nullptr;
		GameObject* lastSnappedTo2 = nullptr;

		bool isOnSlope;
		bool wasOnSlope;
		bool isOnGround;

		std::vector<float> randomProperties;

		void apply(PlayerObject* player, bool fullRestore);
		void fromPlayer(PlayerObject* player, bool fullCapture);
	};

	struct CheckpointData
	{
		uint32_t frame;
		int randomSeed;
		MPlayerCheckpoint p1;
		MPlayerCheckpoint p2;
	};

	struct Correction
	{
		uint32_t frame;
		bool player2;
		MPlayerCheckpoint checkpoint;

		Correction() {}
		Correction(uint32_t frame, bool player2) : frame(frame), player2(player2) {}
	};

	struct Action : gdr::Input
	{
		std::optional<Correction> correction;

		Action() = default;

		Action(uint32_t frame, int button, bool player2, bool down) : gdr::Input(frame, button, player2, down)
		{
		}

		void parseExtension(gdr::json::object_t obj) override
		{
			if (obj.contains("correction"))
			{
				Correction c;
				if (obj["correction"].contains("frame"))
					c.frame = obj["correction"]["frame"];
				c.player2 = obj["correction"]["player2"];
				c.checkpoint.xVel = obj["correction"]["xVel"];
				c.checkpoint.yVel = obj["correction"]["yVel"];
				c.checkpoint.xPos = obj["correction"]["xPos"];
				c.checkpoint.yPos = obj["correction"]["yPos"];
				c.checkpoint.rotation = obj["correction"]["rotation"];
				c.checkpoint.rotationRate = obj["correction"]["rotationRate"];
				correction = c;
			}
		}

		gdr::json::object_t saveExtension() const override
		{
			gdr::json::object_t obj = gdr::json::object();

			if (correction.has_value())
			{
				Correction c = correction.value();
				obj["correction"]["frame"] = c.frame;
				obj["correction"]["player2"] = c.player2;
				obj["correction"]["xVel"] = c.checkpoint.xVel;
				obj["correction"]["yVel"] = c.checkpoint.yVel;
				obj["correction"]["xPos"] = c.checkpoint.xPos;
				obj["correction"]["yPos"] = c.checkpoint.yPos;
				obj["correction"]["rotation"] = c.checkpoint.rotation;
				obj["correction"]["rotationRate"] = c.checkpoint.rotationRate;
			}

			return obj;
		}
	};

	struct Macro : gdr::Replay<Macro, Action>
	{
		Macro() : gdr::Replay<Macro, Action>("Macrobot", "1.1") {}
	};

	inline bool botInput = false;
	inline bool resetFrame = false;
	inline bool resetFromStart = true;
	inline bool holdingAdvance = false;

	inline PlayerMode playerMode = DISABLED;

	inline unsigned int actionIndex = 0;
	inline unsigned int targetSteps = 0;

	inline float advanceHoldTime = 0;

	inline int8_t direction = 0;

	inline Macro macro;

	inline std::unordered_map<void*, Macrobot::CheckpointData> checkpoints;
	inline std::unordered_map<std::string, Macro> macroList;

	inline std::string macroName;
	inline std::string macroDescription;

	inline std::unordered_map<int, bool> downForKey1;
	inline std::unordered_map<int, bool> downForKey2;
	inline std::unordered_map<int, float> timeForKey1;
	inline std::unordered_map<int, float> timeForKey2;

	inline FMOD::Channel* clickChannel = nullptr;

	void GJBaseGameLayerProcessCommands(GJBaseGameLayer* self);
	void handleAction(Action& action);

	Action* recordAction(PlayerButton key, uint32_t frame, bool press, bool player1);

	void save(const std::string& file);
	bool load(const std::string& file);
	void remove(const std::string& file);

	std::optional<Macro> loadMacro(const std::string& file, bool inputs = true);

	void getMacros();

	void drawWindow();
	void drawMacroTable();

	bool clickBetweenFramesCheck();
};
