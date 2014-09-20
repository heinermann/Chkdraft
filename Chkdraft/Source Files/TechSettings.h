#ifndef TECHSETTINGS_H
#define TECHSETTINGS_H
#include "Common Files/CommonFiles.h"
#include "Mapping Core/MappingCore.h"
#include "Windows UI/WindowsUI.h"

class TechSettingsWindow : public ClassWindow
{
	public:
		bool CreateThis(HWND hParent);

	private:
		LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void CreateSubWindows(HWND hWnd);

		CheckBoxControl checkUseDefaultCosts;
		ButtonControl buttonResetTechDefaults;

		GroupBoxControl groupTechCosts;
		TextControl textMineralCosts;
		EditControl editMineralCosts;
		TextControl textGasCosts;
		EditControl editGasCosts;
		TextControl textTimeCosts;
		EditControl editTimeCosts;
		TextControl textEnergyCosts;
		EditControl editEnergyCosts;

		GroupBoxControl groupDefaultPlayerSettings;
		RadioButtonControl radioDisabledByDefault;
		RadioButtonControl radioEnabledByDefault;
		RadioButtonControl radioResearchedByDefault;

		GroupBoxControl groupPlayerSettings;
		CheckBoxControl checkUsePlayerDefaults[12];
		DropdownControl dropPlayerTechSettings[12];
};

#endif