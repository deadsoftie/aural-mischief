#include "MainComponent.h"

#include "Project1.h"
#include "Project2.h"

MainComponent::MainComponent()
{
	projects.emplace_back(std::make_unique<Project1>());
	projects.emplace_back(std::make_unique<Project2>());

	addAndMakeVisible(menuBar);

	// Default to Project 2
	switchToProject(1);

	setSize(1280, 720);
}

MainComponent::~MainComponent()
{
	menuBar.setModel(nullptr);
}

void MainComponent::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::darkgrey);
}

void MainComponent::resized()
{
	auto area = getLocalBounds();

	auto menuArea = area.removeFromTop(24);
	menuBar.setBounds(menuArea);

	if (activeView)
		activeView->setBounds(area);
}

juce::StringArray MainComponent::getMenuBarNames()
{
	return { "Projects" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
	juce::PopupMenu menu;

	if (topLevelMenuIndex == 0)
	{
		for (int i = 0; i < static_cast<int>(projects.size()); ++i)
		{
			const bool isCurrent = (i == currentProjectIndex);
			menu.addItem(ProjectBaseId + i, projects[i]->getName(), true, isCurrent);
		}
	}

	return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int)
{
	if (menuItemID >= ProjectBaseId)
	{
		const int idx = menuItemID - ProjectBaseId;
		switchToProject(idx);
	}
}

void MainComponent::switchToProject(int index)
{
	if (index < 0 || index >= static_cast<int>(projects.size()))
		return;

	currentProjectIndex = index;

	if (activeView)
	{
		removeChildComponent(activeView.get());
		activeView.reset();
	}

	activeView = projects[index]->createView();
	addAndMakeVisible(*activeView);

	resized();
	repaint();
}
