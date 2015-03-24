#ifndef TRIGGER_H
#define TRIGGER_H
#include "Basics.h"
#include "Condition.h"
#include "Action.h"

#define NUM_TRIG_CONDITIONS 16
#define NUM_TRIG_ACTIONS 64
#define NUM_TRIG_PLAYERS 28
class Trigger // A trigger as found in a scenario.chk's TRIG and MBRF sections
{
	public:
		Condition conditions[NUM_TRIG_CONDITIONS];
		Action actions[NUM_TRIG_ACTIONS];
		u32 internalData; // ie: check trigger
		u8 players[NUM_TRIG_PLAYERS];
			#define EXECUTED_BY_PLAYER	   1
			#define NOT_EXECUTED_BY_PLAYER 0

		u8 numUsedConditions();
		u8 numUsedActions();
		bool preserveTriggerFlagged();
		bool disabled();
		bool ignoreConditionsOnce();
		bool ignoreWaitSkipOnce();
		bool ignoreMiscActionsOnce();
		bool ignoreDefeatDraw();
		bool flagPaused();

		void setPreserveTriggerFlagged(bool preserved);
		void setDisabled(bool disabled);
		void setIgnoreConditionsOnce(bool ignoreConditionsOnce);
		void setIgnoreWaitSkipOnce(bool ignoreWaitSkipOnce);
		void setIgnoreMiscActionsOnce(bool ignoreMiscActionsOnce);
		void setIgnoreDefeatDraw(bool ignoreDefeatDraw);
		void setFlagPaused(bool flagPaused);
};
#define TRIG_STRUCT_SIZE 2400

#endif