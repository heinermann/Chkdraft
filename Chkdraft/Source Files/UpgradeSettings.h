#ifndef UPGRADESETTINGS_H
#define UPGRADESETTINGS_H
#include "Common Files/CommonFiles.h"
#include "Mapping Core/MappingCore.h"
#include "Windows UI/WindowsUI.h"

class UpgradeSettingsWindow : public ClassWindow
{
	public:
		bool CreateThis(HWND hParent);

	private:
		LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void CreateSubWindows(HWND hWnd);

		CheckBoxControl checkUseDefaultCosts;
		ButtonControl buttonResetUpgradeDefaults;

		GroupBoxControl groupMineralCosts;
		TextControl textMineralBaseCosts;
		EditControl editMineralBaseCosts;
		TextControl textMineralUpgradeFactor;
		EditControl editMineralUpgradeFactor;

		GroupBoxControl groupGasCosts;
		TextControl textGasBaseCosts;
		EditControl editGasBaseCosts;
		TextControl textGasUpgradeFactor;
		EditControl editGasUpgradeFactor;

		GroupBoxControl groupTimeCosts;
		TextControl textTimeBaseCosts;
		EditControl editTimeBaseCosts;
		TextControl textTimeUpgradeFactor;
		EditControl editTimeUpgradeFactor;

		GroupBoxControl groupPlayerSettings;
		GroupBoxControl groupDefaultSettings;
		TextControl textDefaultStartLevel;
		EditControl editDefaultStartLevel;
		TextControl textDefaultMaxLevel;
		EditControl editDefaultMaxLevel;

		TextControl textPlayer[12];
		CheckBoxControl checkPlayerDefault[12];
		TextControl textPlayerStartLevel[12];
		EditControl editPlayerStartLevel[12];
		TextControl textPlayerMaxLevel[12];
		EditControl editPlayerMaxLevel[12];
};

#endif