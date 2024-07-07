#pragma once

#include <iostream>

namespace GUI
{
	class Shortcut
	{
	public:
			int key = 0;
			std::string name;
	public:

		Shortcut() {}
		Shortcut(int key, const std::string& name)
		{
			this->key = key;
			this->name = name;
		}

		static bool handleShortcut(const std::string&);
		static void drawWindow();
	};
};
