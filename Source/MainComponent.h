#pragma once
#include <JuceHeader.h>

#include "IProject.h"

class MainComponent : public juce::Component, private juce::MenuBarModel
{
public:
	MainComponent();
	~MainComponent() override;

	void paint(juce::Graphics& g) override;
	void resized() override;

private:
	// Menu
	juce::MenuBarComponent menuBar{ this };

	// Project system
	std::vector <std::unique_ptr<IProject>>projects;
	std::unique_ptr<juce::Component> activeView;
	int currentProjectIndex = -1;

	// MenuBarModal
	juce::StringArray getMenuBarNames() override;
	juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
	void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

	void switchToProject(int index);

	enum MenuIds
	{
		ProjectBaseId = 1000
	};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
