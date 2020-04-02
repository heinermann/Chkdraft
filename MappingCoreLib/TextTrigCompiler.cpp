#include "TextTrigCompiler.h"
#include "EscapeStrings.h"
#include "Math.h"
#include <cstdio>
#include <cstring>
#include <exception>
#include <string>
#include <utility>
#include <vector>
#include <chrono>
#undef PlaySound

TextTrigCompiler::TextTrigCompiler(bool useAddressesForMemory, u32 deathTableOffset) : useAddressesForMemory(useAddressesForMemory), deathTableOffset(deathTableOffset)
{

}

TextTrigCompiler::~TextTrigCompiler()
{

}

bool TextTrigCompiler::CompileTriggers(std::string & text, ScenarioPtr chk, Sc::Data & scData, size_t trigIndexBegin, size_t trigIndexEnd)
{
    logger.info() << "Starting trigger compilation to replace range [" << trigIndexBegin << ", " << trigIndexEnd << ")..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    if ( !LoadCompiler(chk, scData, trigIndexBegin, trigIndexEnd) )
        return false;

    try
    {
        std::vector<RawString> stringContents;
        CleanText(text, stringContents);

        std::deque<std::shared_ptr<Chk::Trigger>> triggers;
        std::stringstream compilerError;
        std::stringstream buildError;

        if ( ParseTriggers(text, stringContents, triggers, compilerError) )
        {
            if ( BuildNewMap(chk, trigIndexBegin, trigIndexEnd, triggers, buildError) )
            {
                chk->triggers.trig->swap(triggers);
                auto finish = std::chrono::high_resolution_clock::now();
                logger.info() << "Trigger compilation completed without error in " << std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count() << "ms" << std::endl;
                return true;
            }
            else
                compilerError << "No text errors, but build of new TRIG/STR section failed." << std::endl << std::endl << buildError.str();
        }

        CHKD_ERR(compilerError.str());
    }
    catch ( std::bad_alloc ) { CHKD_ERR("Compilation aborted due to low memory."); }
    return false;
}

bool TextTrigCompiler::CompileTrigger(std::string & text, Chk::TriggerPtr trigger, ScenarioPtr chk, Sc::Data & scData, size_t trigIndex)
{
    if ( !LoadCompiler(chk, scData, trigIndex, trigIndex+1) )
        return false;

    try
    {
        std::vector<RawString> stringContents;
        CleanText(text, stringContents);
        std::deque<std::shared_ptr<Chk::Trigger>> triggers;
        std::stringstream compilerError;
        std::stringstream buildError;

        if ( ParseTriggers(text, stringContents, triggers, compilerError) )
        {
            if ( triggers.size() == 1 )
            {

                if ( BuildNewMap(chk, trigIndex, trigIndex+1, triggers, buildError) )
                    return true;
                else
                    compilerError << "No text errors, but build of new TRIG/STR section failed." << std::endl << std::endl << buildError.str();
            }
            else
                compilerError << "Expected 1 trigger but found " << triggers.size() << " triggers.";
        }

        CHKD_ERR(compilerError.str());
    }
    catch ( std::bad_alloc ) { CHKD_ERR("Compilation aborted due to low memory."); }
    return false;
}

bool TextTrigCompiler::ParseConditionName(std::string text, Chk::Condition::Type & conditionType)
{
    std::string txcd = text;
    std::vector<RawString> stringContents;
    CleanText(text, stringContents);
    Chk::Condition::VirtualType newConditionType = Chk::Condition::VirtualType::NoCondition;
    if ( ParseConditionName(txcd, newConditionType) && newConditionType != Chk::Condition::VirtualType::Custom )
    {
        if ( ((s32)newConditionType) < 0 )
            conditionType = ExtendedToRegularCID(newConditionType);
        else
            conditionType = (Chk::Condition::Type)newConditionType;

        return true;
    }
    else
    {
        u32 temp = 0;
        if ( ParseLong(txcd.c_str(), temp, 0, txcd.size()) )
        {
            if ( ((s32)temp) < 0 )
                conditionType = ExtendedToRegularCID((Chk::Condition::VirtualType)temp);
            else
                conditionType = (Chk::Condition::Type)temp;

            return true;
        }
    }
    return false;
}

bool TextTrigCompiler::ParseConditionArg(std::string conditionArgText, Chk::Condition::Argument argument, Chk::Condition & condition, ScenarioPtr chk, Sc::Data & scData, size_t trigIndex)
{
    if ( !LoadCompiler(chk, scData, trigIndex, trigIndex+1) )
        return false;

    std::string txcd = conditionArgText;
    std::stringstream argumentError;
    std::vector<RawString> stringContents = { conditionArgText };
    size_t nextString = 0;
    if ( ParseConditionArg(txcd, stringContents, nextString, condition, 0, txcd.size()-1, (Chk::Condition::VirtualType)condition.conditionType, argument, argumentError) )
        return true;
    else
    {
        std::stringstream errorMessage;
        errorMessage << "Unable to parse condition. " << argumentError.str();
        CHKD_ERR(errorMessage.str());
    }
    return false;
}

bool TextTrigCompiler::ParseActionName(std::string text, Chk::Action::Type & actionType)
{
    std::string txac = text;
    std::vector<RawString> stringContents;
    CleanText(text, stringContents);
    Chk::Action::VirtualType newActionType = Chk::Action::VirtualType::NoAction;
    if ( ParseActionName(txac, newActionType) && newActionType != Chk::Action::VirtualType::Custom )
    {
        if ( ((s32)newActionType) < 0 )
            actionType = ExtendedToRegularAID(newActionType);
        else
            actionType = (Chk::Action::Type)newActionType;

        return true;
    }
    else
    {
        u32 temp = 0;
        if ( ParseLong(txac.c_str(), temp, 0, txac.size()) )
        {
            if ( ((s32)temp) < 0 )
                actionType = ExtendedToRegularAID((Chk::Action::VirtualType)temp);
            else
                actionType = (Chk::Action::Type)temp;

            return true;
        }
    }
    return false;
}

bool TextTrigCompiler::ParseActionArg(std::string actionArgText, Chk::Action::Argument argument, Chk::Action & action, ScenarioPtr chk, Sc::Data & scData, size_t trigIndex)
{
    if ( !LoadCompiler(chk, scData, trigIndex, trigIndex+1) )
        return false;
    
    std::string txac = actionArgText;
    std::stringstream argumentError;
    std::vector<RawString> stringContents = { actionArgText };
    size_t nextString = 0;
    if ( ParseActionArg(txac, stringContents, nextString, action, 0, txac.size() - 1, (Chk::Action::VirtualType)action.actionType, argument, argumentError) )
        return true;
    else
    {
        std::stringstream errorMessage;
        errorMessage << "Unable to parse action. " << argumentError.str();
        CHKD_ERR(errorMessage.str());
    }
    return false;
}

// protected

bool TextTrigCompiler::LoadCompiler(ScenarioPtr chk, Sc::Data & scData, size_t trigIndexBegin, size_t trigIndexEnd)
{
    ClearCompiler();

    return
        PrepLocationTable(chk) && PrepUnitTable(chk) && PrepSwitchTable(chk) &&
        PrepGroupTable(chk) && PrepScriptTable(scData) &&
        PrepStringTable(chk, newStringTable, trigIndexBegin, trigIndexEnd, Chk::Scope::Game) &&
        PrepStringTable(chk, newExtendedStringTable, trigIndexBegin, trigIndexEnd, Chk::Scope::Editor);
}

void TextTrigCompiler::ClearCompiler()
{
    locationTable.clear();
    unitTable.clear();
    switchTable.clear();
    groupTable.clear();

    unassignedStrings.clear();
    newStringTable.clear();

    unassignedExtendedStrings.clear();
    newExtendedStringTable.clear();
}

void TextTrigCompiler::CleanText(std::string & text, std::vector<RawString> & stringContents)
{
    logger.info() << "Starting text trig cleaning" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    size_t pos = 0;
    bool inString = false;
    StringBuffer dest;

    while ( pos < text.size() ) 
    {
        char character = text[pos];
        pos++;
        switch ( character )
        {
        case ' ': case '\t':
            break; // Ignore (delete) spaces and tabs

        case '\r': // CR (line ending)
            if ( text[pos] == '\n' ) // Followed by LF
                pos++; // Increment position and continue to LF case
                       // Else continue to the LF case
        case '\n': // LF (line ending)
        case '\v': // VT (line ending)
        case '\f': // FF (line ending)
            dest += '\n';
            break;

        case '/': // Might be a comment
            if ( text[pos] == '/' ) // Found a comment
            {
                size_t newPos = text.find('\n', pos); // Check for nearby LF
                if ( newPos == std::string::npos ) newPos = text.find('\r', pos); // Check for nearby CR (should be checked only if LF not found)
                if ( newPos == std::string::npos ) newPos = text.find('\v', pos); // Check for nearby VT
                if ( newPos == std::string::npos ) newPos = text.find('\f', pos); // Check for nearby FF

                if ( newPos != std::string::npos )
                    pos = newPos; // Skip (delete) contents until line ending
                else // File ended on this line
                    pos = text.size();
            }
            break;

        case '\"': // Found a string
        {
            dest += '\"';
            size_t closeQuotePos = findStringEnd(text, pos);
            EscString escString = text.substr(pos, closeQuotePos-pos);
            RawString rawString;
            ConvertStr(escString, rawString);
            stringContents.push_back(rawString);
            dest += '\"';
            pos = closeQuotePos+1;
        }
        break;

        default:
            if ( character >= 'a' && character <= 'z' ) // If lowercase
                dest += char(character-32); // Captialize
            else
                dest += character; // Add character as normal
            break;
        }
    }

    if ( !dest.empty() )
        text.assign(dest.begin(), dest.end());
    else
        text = "";

    auto finish = std::chrono::high_resolution_clock::now();
    logger.info() << "Finished text trig cleaning in " << std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count() << "ms" << std::endl;
}

bool TextTrigCompiler::ParseTriggers(std::string & text, std::vector<RawString> & stringContents, std::deque<Chk::TriggerPtr> & output, std::stringstream & error)
{
    text.push_back('\0'); // Add a terminating null character

    u8 flags;
    size_t pos = 0,
        lineEnd = 0,
        playerEnd = 0,
        conditionEnd = 0,
        actionEnd = 0,
        flagsEnd = 0,
        argEnd = 0,
        nextString = 0;

    u32 expecting = 0,
        line = 1,
        argIndex = 0,
        numConditions = 0,
        numActions = 0;

    Chk::Condition::VirtualType conditionId;
    Chk::Action::VirtualType actionId;
    std::shared_ptr<Chk::Trigger> currTrig = nullptr;
    Chk::Condition* currCondition = nullptr;
    Chk::Action* currAction = nullptr;

    while ( pos < text.size() )
    {
        if ( text[pos] == '\n' ) // Line End
        {
            pos ++;
            line ++;
        }
        else
        {
            switch ( expecting )
            {
            case 0: //      trigger
                    // or   %NULL
                if ( !ParsePartZero(text, currTrig, currCondition, currAction, error, pos, line, expecting) )
                    return false;
                break;

            case 1: //      %PlayerName,
                    // or   %PlayerName:Value,
                    // or   %PlayerName:Value)
                    // or   %PlayerName)
                    // or   )
                if ( !ParsePartOne(text, stringContents, nextString, *currTrig, error, pos, line, expecting, playerEnd, lineEnd) )
                    return false;
                break;

            case 2: //      {
                if ( !ParsePartTwo(text, *currTrig, error, pos, line, expecting) )
                    return false;
                break;

            case 3: //      conditions:
                    // or   actions:
                    // or   flags:
                    // or   }
                if ( !ParsePartThree(text, *currTrig, error, pos, line, expecting) )
                    return false;
                break;

            case 4: //      %ConditionName(
                    // or   ;%ConditionName(
                    // or   actions:
                    // or   flags:
                    // or   }
                if ( !ParsePartFour(text, *currTrig, error, pos, line, expecting, conditionEnd, lineEnd, conditionId,
                    flags, argIndex, numConditions, currCondition) )
                    return false;
                break;

            case 5: //      );
                    // or   %ConditionArg,
                    // or   %ConditionArg);
                if ( !ParsePartFive(text, stringContents, nextString, *currTrig, error, pos, line, expecting, argIndex, argEnd, currCondition, conditionId) )
                    return false;
                break;

            case 6: //      actions:
                    // or   flags:
                    // or   }
                if ( !ParsePartSix(text, *currTrig, error, pos, line, expecting) )
                    return false;
                break;

            case 7: //      %ActionName(
                    // or   ;%ActionName(
                    // or   flags:
                    // or   }
                if ( !ParsePartSeven(text, *currTrig, error, pos, line, expecting, flags, actionEnd, lineEnd,
                    actionId, argIndex, numActions, currAction) )
                    return false;
                break;

            case 8: //      );
                    // or   %ActionArg,
                    // or   %ActionArg);
                if ( !ParsePartEight(text, stringContents, nextString, *currTrig, error, pos, line, expecting, argIndex, argEnd, currAction, actionId) )
                    return false;
                break;

            case 9: //      }
                    // or   flags:,
                if ( !ParsePartNine(text, *currTrig, error, pos, line, expecting) )
                    return false;
                break;

            case 10: //     ;
                     // or  %32BitFlags;
                if ( !ParsePartTen(text, *currTrig, error, pos, line, expecting, flagsEnd) )
                    return false;
                break;

            case 11: //     }
                if ( !ParsePartEleven(text, *currTrig, error, pos, line, expecting) )
                    return false;
                break;

            case 12: // Trigger end was found, reset
            {
                numConditions = 0;
                numActions = 0;

                output.push_back(currTrig);
                expecting = 0;
                if ( text[pos] == '\0' ) // End of Text
                {
                    error << "Success!";
                    return true;
                }
            }
            break;
            }
        }
    }
    return true;
}

inline bool TextTrigCompiler::ParsePartZero(std::string & text, Chk::TriggerPtr & currTrig, Chk::Condition* & currCondition, Chk::Action* & currAction, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting)
{
    //      trigger
    // or   %NULL
    if ( text.compare(pos, 8, "TRIGGER(") == 0 )
    {
        pos += 8;
        expecting ++;
    }
    else if ( text.compare(pos, 7, "TRIGGER") == 0 )
    {
        pos += 7;
        while ( text[pos] == '\n' )
        {
            pos ++;
            line ++;
        }
        if ( text[pos] == '(' )
        {
            pos ++;
            expecting ++;
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \'(\'";
            return false;
        }
    }
    else if ( text[pos] == '\0' ) // End of text
    {
        pos ++;
    }
    else
    {
        error << "Line: " << line << std::endl << std::endl << "Expected: \"Trigger\" or End of Text";
        return false;
    }
    currTrig = Chk::TriggerPtr(new Chk::Trigger());
    currCondition = &currTrig->conditions[0];
    currAction = &currTrig->actions[0];
    return true;
}

inline bool TextTrigCompiler::ParsePartOne(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, Chk::Trigger & output, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting, size_t & playerEnd, size_t & lineEnd)
{
    //      %PlayerName,
    // or   %PlayerName:Value,
    // or   %PlayerName)
    // or   %PlayerName:Value)
    // or   )
    if ( text[pos] == ')' ) // No players
    {
        pos ++;
        expecting ++;
    }
    else
    {
        size_t nextComma = findNextUnquoted(text, pos, ',', '{');
        playerEnd = nextComma != std::string::npos ? nextComma : findNextUnquoted(text, pos, ')', '{');

        if ( playerEnd != std::string::npos )
        {
            lineEnd = findNextUnquoted(text, pos, '\n');
            if ( lineEnd == std::string::npos )
                lineEnd = text.length(); // Text ends on this line

            playerEnd = std::min(playerEnd, lineEnd);

            if ( ParseExecutingPlayer(text, stringContents, nextString, output, pos, playerEnd) )
            {
                pos = playerEnd;
                while ( text[pos] == '\n' )
                {
                    pos ++;
                    line ++;
                }
                if ( text[pos] == ')' )
                    expecting ++;

                pos ++;
            }
            else
            {
                error << "Line: " << line << std::endl << std::endl << "Expected: Player Identifier";
                return false;
            }
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \',\' or \')\'";
            return false;
        }
    }
    return true;
}

inline bool TextTrigCompiler::ParsePartTwo(std::string & text, Chk::Trigger & output, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting)
{
    //      {
    if ( text[pos] == '{' )
    {
        pos ++;
        expecting ++;
    }
    else
    {
        error << "Line: " << line << std::endl << std::endl << "Expected: '{'";
        return false;
    }
    return true;
}

inline bool TextTrigCompiler::ParsePartThree(std::string & text, Chk::Trigger & output, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting)
{
    //      conditions:
    // or   actions:
    // or   flags:
    // or   }
    if ( text.compare(pos, 11, "CONDITIONS:") == 0 )
    {
        pos += 11;
        expecting ++;
    }
    else if ( text.compare(pos, 8, "ACTIONS:") == 0 )
    {
        pos += 8;
        expecting += 4;
    }
    else if ( text.compare(pos, 6, "FLAGS:") == 0 )
    {
        pos += 6;
        expecting += 7;
    }
    else if ( text[pos] == '}' )
    {
        pos ++;
        expecting = 12;
    }
    else
    {
        bool hasConditions = text.compare(pos, 10, "CONDITIONS") == 0;
        if ( hasConditions || text.compare(pos, 7, "ACTIONS") == 0 )
        {
            pos += hasConditions ? 10 : 7;
            while ( text[pos] == '\n' )
            {
                pos ++;
                line ++;
            }

            if ( text[pos] == ':' ) 
            {
                pos ++;
                expecting += hasConditions ? 1 : 4;
            }
            else
            {
                error << "Line: " << line << std::endl << std::endl << "Expected: \':\'";
                return false;
            }
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \"Conditions\" or \"Actions\" or \"Flags\" or \'}\'";
            return false;
        }
    }
    return true;
}

inline bool TextTrigCompiler::ParsePartFour(std::string & text, Chk::Trigger & output, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting,
    size_t & conditionEnd, size_t & lineEnd, Chk::Condition::VirtualType & conditionId, u8 & flags, u32 & argIndex, u32 & numConditions,
    Chk::Condition* & currCondition)
{
    //      %ConditionName(
    // or   ;%ConditionName(
    // or   actions:
    // or   flags:
    // or   }
    if ( text[pos] == ';' ) // Disabled condition
    {
        pos ++;
        while ( text[pos] == '\n' )
        {
            pos ++;
            line ++;
        }
        conditionEnd = text.find('(', pos);
        if ( conditionEnd != std::string::npos )
        {
            lineEnd = findNextUnquoted(text, pos, '\n');
            if ( lineEnd != std::string::npos )
                lineEnd = text.size();

            conditionEnd = std::min(conditionEnd, lineEnd);

            if ( ParseCondition(text, pos, conditionEnd, true, conditionId, flags) )
            {
                argIndex = 0;
                if ( numConditions > Chk::Trigger::MaxConditions )
                {
                    error << "Line: " << line << std::endl << std::endl << "Condition Max Exceeded!";
                    return false;
                }
                currCondition = &output.condition(numConditions);
                currCondition->flags = flags | Chk::Condition::Flags::Disabled;
                if ( (s32)conditionId < 0 )
                    currCondition->conditionType = ExtendedToRegularCID(conditionId);
                else
                    currCondition->conditionType = (Chk::Condition::Type)conditionId;
                numConditions ++;

                pos = conditionEnd;
                while ( text[pos] == '\n' )
                {
                    pos ++;
                    line ++;
                }

                if ( text[pos] == '(' )
                {
                    pos ++;
                    expecting ++;
                }
                else
                {
                    error << "Line: " << line << std::endl << std::endl << "Expected: \'(\'";
                    return false;
                }
            }
            else
            {
                error << "Line: " << line << std::endl << std::endl << "Expected: Condition Name";
                return false;
            }
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \'(\'";
            return false;
        }
    }
    else if ( text[pos] == '}' ) // End trigger
    {
        pos ++;
        expecting = 12;
    }
    else if ( text.compare(pos, 7, "ACTIONS") == 0 ) // End conditions
    {
        pos += 7;
        while ( text[pos] == '\n' )
        {
            pos ++;
            line ++;
        }
        if ( text[pos] == ':' )
        {
            pos ++;
            expecting = 7;
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \':\'";
            return false;
        }
    }
    else if ( text.compare(pos, 5, "FLAGS") == 0 ) // End conditions, no actions
    {
        pos += 5;
        if ( text[pos] == ':' )
        {
            pos ++;
            expecting = 10;
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \':\'";
            return false;
        }
    }
    else
    {
        conditionEnd = text.find('(', pos);
        if ( conditionEnd != std::string::npos ) // Has a condition or an error
        {
            lineEnd = text.find('\n', pos);
            if ( lineEnd == std::string::npos )
                lineEnd = text.length();

            conditionEnd = std::min(conditionEnd, lineEnd);

            if ( ParseCondition(text, pos, conditionEnd, false, conditionId, flags) )
            {
                argIndex = 0;
                if ( numConditions > Chk::Trigger::MaxConditions )
                {
                    error << "Line: " << line << std::endl << std::endl << "Condition Max Exceeded!";
                    return false;
                }
                currCondition = &output.condition(numConditions);
                currCondition->flags = flags;
                if ( (s32)conditionId < 0 )
                    currCondition->conditionType = ExtendedToRegularCID(conditionId);
                else
                    currCondition->conditionType = (Chk::Condition::Type)conditionId;
                numConditions ++;

                pos = conditionEnd;
                while ( text[pos] == '\n' )
                {
                    pos ++;
                    line ++;
                }

                if ( text[pos] == '(' )
                {
                    pos ++;
                    expecting ++;
                }
                else
                {
                    error << "Line: " << line << std::endl << std::endl << "Expected: \'(\'";
                    return false;
                }
            }
            else
            {
                error << "Line: " << line << std::endl << std::endl << "Expected: Condition Name";
                return false;
            }
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \'(\'";
            return false;
        }
    }
    return true;
}

inline bool TextTrigCompiler::ParsePartFive(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, Chk::Trigger & output, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting, u32 & argIndex, size_t & argEnd,
    Chk::Condition* & currCondition, Chk::Condition::VirtualType & conditionId)
{
    //      );
    // or   %ConditionArg,
    // or   %ConditionArg);
    Chk::Condition::Argument argument = Chk::Condition::getTextArg(conditionId, argIndex);
    Chk::Condition::ArgType argType = argument.type;
    if ( text[pos] == ')' ) // Condition End
    {
        if ( argType != Chk::Condition::ArgType::NoType )
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: Condition Argument";
            return false;
        }

        pos ++;
        while ( text[pos] == '\n' )
        {
            pos ++;
            line ++;
        }

        if ( text[pos] == ';' )
        {
            pos ++;
            expecting --;
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \';\'";
            return false;
        }
    }
    else if ( argType == Chk::Condition::ArgType::NoType )
    {
        error << "Line: " << line << std::endl << std::endl << "Expected: \')\'";
        return false;
    }
    else if ( Chk::Condition::getTextArgType(conditionId, (size_t)argIndex+1) == Chk::Condition::ArgType::NoType )
    {
        argEnd = findNextUnquoted(text, pos, ')');
        if ( argEnd != std::string::npos )
        {
            std::stringstream argumentError;
            if ( ParseConditionArg(text, stringContents, nextString, *currCondition, pos, argEnd, conditionId, argument, argumentError) )
            {
                pos = argEnd;
                argIndex ++;
            }
            else
            {
                error << "Line: " << line << std::endl << std::endl << argumentError.str();
                return false;
            }
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \')\'";
            return false;
        }
    }
    else
    {
        argEnd = findNextUnquoted(text, pos, ',');
        if ( argEnd != std::string::npos ) // Has argument
        {
            std::stringstream argumentError;
            if ( ParseConditionArg(text, stringContents, nextString, *currCondition, pos, argEnd, conditionId, argument, argumentError) )
            {
                pos = argEnd+1;
                argIndex ++;
            }
            else
            {
                error << "Line: " << line << std::endl << std::endl << argumentError.str();
                return false;
            }
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \',\'";
            return false;
        }
    }
    return true;
}

inline bool TextTrigCompiler::ParsePartSix(std::string & text, Chk::Trigger & output, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting)
{
    //      actions:
    // or   flags:
    // or   }
    if ( text[pos] == '}' )
    {
        pos ++;
        expecting = 12;
    }
    else if ( text.compare(pos, 8, "ACTIONS:") == 0 )
    {
        pos += 8;
        expecting ++;
    }
    else if ( text.compare(pos, 6, "FLAGS:") == 0 )
    {
        pos += 6;
        expecting = 10;
    }
    else
    {
        if ( text.compare(pos, 7, "ACTIONS") == 0 || text.compare(pos, 5, "FLAGS") == 0 )
            error << "Line: " << line << std::endl << std::endl << "Expected: \':\'";
        else
            error << "Line: " << line << std::endl << std::endl << "Expected: \"Actions\" or \"Flags\" or '}'";
        return false;
    }
    return true;
}

inline bool TextTrigCompiler::ParsePartSeven(std::string & text, Chk::Trigger & output, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting,
    u8 & flags, size_t & actionEnd, size_t & lineEnd, Chk::Action::VirtualType & actionId, u32 & argIndex, u32 & numActions,
    Chk::Action* & currAction)
{
    //      %ActionName(
    // or   ;%ActionName(
    // or   flags:
    // or   }
    if ( text[pos] == ';' )
    {
        pos ++;
        actionEnd = text.find('(', pos);
        if ( actionEnd != std::string::npos )
        {
            lineEnd = findNextUnquoted(text, pos, '\n');
            if ( lineEnd == std::string::npos )
                lineEnd = text.length();

            actionEnd = std::min(actionEnd, lineEnd);

            if ( ParseAction(text, pos, actionEnd, true, actionId, flags) )
            {
                argIndex = 0;
                if ( numActions > Chk::Trigger::MaxActions )
                {
                    error << "Line: " << line << std::endl << std::endl << "Action Max Exceeded!";
                    return false;
                }
                currAction = &output.action(numActions);
                currAction->flags = flags | Chk::Action::Flags::Disabled;
                if ( actionId < 0 )
                    currAction->actionType = ExtendedToRegularAID(actionId);
                else
                    currAction->actionType = (Chk::Action::Type)actionId;
                numActions ++;

                pos = actionEnd+1;
                expecting ++;
            }
            else
            {
                error << "Line: " << line << std::endl << std::endl << "Expected: Action Name or \'}\'";
                return false;
            }
        }
    }
    else if ( text[pos] == '}' )
    {
        pos ++;
        expecting = 12;
    }
    else if ( text.compare(pos, 5, "FLAGS") == 0 ) // End actions
    {
        pos += 5;
        if ( text[pos] == ':' )
        {
            pos ++;
            expecting = 10;
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \':\'";
            return false;
        }
    }
    else
    {
        actionEnd = text.find('(', pos);
        if ( actionEnd != std::string::npos )
        {
            lineEnd = findNextUnquoted(text, pos, '\n');
            if ( lineEnd == std::string::npos )
                lineEnd = text.length();

            actionEnd = std::min(actionEnd, lineEnd);

            if ( ParseAction(text, pos, actionEnd, false, actionId, flags) )
            {
                argIndex = 0;
                if ( numActions > Chk::Trigger::MaxActions )
                {
                    error << "Line: " << line << std::endl << std::endl << "Action Max Exceeded!";
                    return false;
                }
                currAction = &output.action(numActions);
                currAction->flags = flags;
                if ( (s32)actionId < 0 )
                    currAction->actionType = ExtendedToRegularAID(actionId);
                else
                    currAction->actionType = (Chk::Action::Type)actionId;
                numActions ++;

                pos = actionEnd+1;
                expecting ++;
            }
            else
            {
                error << "Line: " << line << std::endl << std::endl << "Expected: Action Name or \'}\'";
                return false;
            }
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \'(\'";
            return false;
        }
    }
    return true;
}

inline bool TextTrigCompiler::ParsePartEight(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, Chk::Trigger & output, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting,
    u32 & argIndex, size_t & argEnd, Chk::Action* & currAction, Chk::Action::VirtualType & actionId)
{
    //      );
    // or   %ActionArg,
    // or   %ActionArg);
    Chk::Action::Argument argument = Chk::Action::getTextArg(actionId, argIndex);
    Chk::Action::ArgType argType = argument.type;
    if ( text[pos] == ')' ) // Action End
    {
        if ( argType != Chk::Action::ArgType::NoType )
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: Action Argument";
            return false;
        }

        pos ++;
        while ( text[pos] == '\n' )
        {
            pos ++;
            line ++;
        }

        if ( text[pos] == ';' )
        {
            pos ++;
            expecting --;
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \';\'";
            return false;
        }
    }
    else if ( argType == Chk::Action::ArgType::NoType )
    {
        error << "Line: " << line << std::endl << std::endl << "Expected: \')\'";
        return false;
    }
    else if ( Chk::Action::getTextArgType(actionId, (size_t)argIndex+1) == Chk::Action::ArgType::NoType )
    {
        argEnd = findNextUnquoted(text, pos, ')');
        if ( argEnd != std::string::npos )
        {
            std::stringstream argumentError;
            if ( ParseActionArg(text, stringContents, nextString, *currAction, pos, argEnd, actionId, argument, argumentError) )
            {
                pos = argEnd;
                argIndex ++;
            }
            else
            {
                error << "Line: " << line << std::endl << std::endl << "Expected: Action Argument" << std::endl << std::endl << argumentError.str();
                return false;
            }
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \')\'";
            return false;
        }
    }
    else
    {
        argEnd = findNextUnquoted(text, pos, ',');
        if ( argEnd != std::string::npos ) // Has argument
        {
            std::stringstream argumentError;
            if ( ParseActionArg(text, stringContents, nextString, *currAction, pos, argEnd, actionId, argument, argumentError) )
            {
                pos = argEnd+1;
                argIndex ++;
            }
            else
            {
                error << "Line: " << line << std::endl << std::endl << "Expected: Action Argument" << std::endl << std::endl << argumentError.str();
                return false;
            }
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: Additional Arguments";
            return false;
        }
    }

    return true;
}

inline bool TextTrigCompiler::ParsePartNine(std::string & text, Chk::Trigger & output, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting)
{
    //      }
    // or   flags:,
    if ( text[pos] == '}' )
    {
        pos ++;
        expecting = 12;
    }
    else if ( text.compare(pos, 6, "FLAGS:") == 0 )
    {
        pos += 6;
        expecting ++;
    }
    else
    {
        if ( text.compare(pos, 5, "FLAGS") == 0 )
            error << "Line: " << line << std::endl << std::endl << "Expected: \':\'";
        else
            error << "Line: " << line << std::endl << std::endl << "Expected: \"Flags\" or \'}\'";
        return false;
    }
    return true;
}

inline bool TextTrigCompiler::ParsePartTen(std::string & text, Chk::Trigger & output, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting,
    size_t & flagsEnd)
{
    //      ;
    // or  %32BitFlags;
    if ( text[pos] == ';' )
    {
        pos ++;
        expecting ++;
    }
    else
    {
        flagsEnd = text.find(';', pos);
        if ( flagsEnd != std::string::npos )
        {
            if ( ParseExecutionFlags(text, pos, flagsEnd, output.flags) )
            {
                pos = flagsEnd+1;
                expecting ++;
            }
            else
            {
                error << "Line: " << line << std::endl << std::endl << "Expected: Binary Execution Flags (32-bit max).";
                return false;
            }
        }
        else
        {
            error << "Line: " << line << std::endl << std::endl << "Expected: \';\'";
            return false;
        }
    }
    return true;
}

inline bool TextTrigCompiler::ParsePartEleven(std::string & text, Chk::Trigger & output, std::stringstream & error, size_t & pos, u32 & line, u32 & expecting)
{
    //      }
    if ( text[pos] == '}' )
    {
        pos ++;
        expecting ++;
    }
    else
    {
        error << "Line: " << line << std::endl << std::endl << "Expected: \'}\'";
        return false;
    }
    return true;
}

bool TextTrigCompiler::ParseExecutingPlayer(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, Chk::Trigger & currTrig, size_t pos, size_t end)
{
    u32 group;
    size_t separator = findNextUnquoted(text, pos, ':');
    if ( separator != std::string::npos &&
        ParsePlayer(text, stringContents, nextString, group, pos, separator) &&
        group < 27 )
    {
        u8 appendedValue;
        if ( !ParseByte(text.c_str(), appendedValue, separator+1, end) )
            appendedValue = 1;

        currTrig.owners[group] = (Chk::Trigger::Owned)appendedValue;
        return true;
    }
    else if ( ParsePlayer(text, stringContents, nextString, group, pos, end) && group < 27 )
    {
        currTrig.owners[group] = Chk::Trigger::Owned::Yes;
        return true;
    }
    return false;
}

bool TextTrigCompiler::ParseConditionName(std::string & arg, Chk::Condition::VirtualType & conditionType)
{
    char currChar = arg[0];
    switch ( currChar )
    {
    case 'A':
    {
        if ( arg.compare(1, 9, "CCUMULATE") == 0 )
            conditionType = Chk::Condition::VirtualType::Accumulate;
        else if ( arg.compare(1, 5, "LWAYS") == 0 )
            conditionType = Chk::Condition::VirtualType::Always;
    }
    break;

    case 'B':
        if ( arg.compare(1, 4, "RING") == 0 )
            conditionType = Chk::Condition::VirtualType::Bring;
        break;

    case 'C':
        if ( arg.compare(1, 6, "OMMAND") == 0 )
        {
            if ( arg.compare(7, 8, "THELEAST") == 0 )
            {
                if ( arg.compare(15, 2, "AT") == 0 )
                    conditionType = Chk::Condition::VirtualType::CommandTheLeastAt;
                else if ( arg.size() == 15 )
                    conditionType = Chk::Condition::VirtualType::CommandTheLeast;
            }
            else if ( arg.compare(7, 7, "THEMOST") == 0 )
            {
                if ( arg.compare(14, 2, "AT") == 0 )
                    conditionType = Chk::Condition::VirtualType::CommandTheMostAt;
                else if ( arg.size() == 14 )
                    conditionType = Chk::Condition::VirtualType::CommandTheMost;
            }
            else if ( arg.compare(7, 10, "STHEMOSTAT") == 0 ) // command'S', added for backwards compatibility
                conditionType = Chk::Condition::VirtualType::CommandTheMostAt;
            else if ( arg.size() == 7 )
                conditionType = Chk::Condition::VirtualType::Command;
        }
        else if ( arg.compare(1, 13, "OUNTDOWNTIMER") == 0 )
            conditionType = Chk::Condition::VirtualType::CountdownTimer;
        else if ( arg.compare(1, 5, "USTOM") == 0 )
            conditionType = Chk::Condition::VirtualType::Custom;
        break;

    case 'D':
        if ( arg.compare(1, 5, "EATHS") == 0 )
            conditionType = Chk::Condition::VirtualType::Deaths;
        break;

    case 'E':
        if ( arg.compare(1, 10, "LAPSEDTIME") == 0 )
            conditionType = Chk::Condition::VirtualType::ElapsedTime;
        break;

    case 'H':
        if ( arg.compare(1, 11, "IGHESTSCORE") == 0 )
            conditionType = Chk::Condition::VirtualType::HighestScore;
        break;

    case 'K':
        if ( arg.compare(1, 3, "ILL") == 0 )
            conditionType = Chk::Condition::VirtualType::Kill;
        break;

    case 'L':
        if ( arg.compare(1, 4, "EAST") == 0 )
        {
            if ( arg.compare(5, 5, "KILLS") == 0 )
                conditionType = Chk::Condition::VirtualType::LeastKills;
            else if ( arg.compare(5, 9, "RESOURCES") == 0 )
                conditionType = Chk::Condition::VirtualType::LeastResources;
        }
        else if ( arg.compare(1, 10, "OWESTSCORE") == 0 )
            conditionType = Chk::Condition::VirtualType::LowestScore;
        break;

    case 'M':
        if ( arg.compare(1, 5, "EMORY") == 0 )
            conditionType = Chk::Condition::VirtualType::Memory;
        else if ( arg.compare(1, 3, "OST") == 0 )
        {
            if ( arg.compare(4, 5, "KILLS") == 0 )
                conditionType = Chk::Condition::VirtualType::MostKills;
            else if ( arg.compare(4, 9, "RESOURCES") == 0 )
                conditionType = Chk::Condition::VirtualType::MostResources;
        }
        break;

    case 'N':
        if ( arg.compare(1, 4, "EVER") == 0 )
            conditionType = Chk::Condition::VirtualType::Never;
        break;

    case 'O':
        if ( arg.compare(1, 8, "PPONENTS") == 0 )
            conditionType = Chk::Condition::VirtualType::Opponents;
        break;

    case 'S':
        if ( arg.compare(1, 4, "CORE") == 0 )
            conditionType = Chk::Condition::VirtualType::Score;
        else if ( arg.compare(1, 5, "WITCH") == 0 )
            conditionType = Chk::Condition::VirtualType::Switch;
        break;
    }

    return conditionType != Chk::Condition::VirtualType::NoCondition;
}

bool TextTrigCompiler::ParseCondition(std::string & text, size_t pos, size_t end, bool disabled, Chk::Condition::VirtualType & conditionType, u8 & flags)
{
    conditionType = Chk::Condition::VirtualType::NoCondition;
    u16 number = 0;

    size_t size = end - pos;
    std::string arg;

    for ( u32 i=0; i<size; i++ ) // Copy argument to arg buffer
    {
        char character = text[i+pos];
        if ( character > 96 && character < 123 ) // lower-case
            arg.push_back(character-32); // Capitalize
        else if ( character != ' ' && character != '\t' ) // Ignore spaces and tabs
            arg.push_back(character);
    }

    ParseConditionName(arg, conditionType);

    flags = Chk::Condition::getDefaultFlags(conditionType);
    flags = Chk::Condition::defaultFlags[0];

    return conditionType != Chk::Condition::VirtualType::NoCondition;
}

bool TextTrigCompiler::ParseActionName(std::string & arg, Chk::Action::VirtualType & actionType)
{
    char currChar = arg[0];
    switch ( currChar )
    {
    case 'C':
        if ( arg.compare(1, 6, "OMMENT") == 0 )
            actionType = Chk::Action::VirtualType::Comment;
        else if ( arg.compare(1, 9, "REATEUNIT") == 0 )
        {
            if ( arg.compare(10, 14, "WITHPROPERTIES") == 0 )
                actionType = Chk::Action::VirtualType::CreateUnitWithProperties;
            else if ( arg.size() == 10 )
                actionType = Chk::Action::VirtualType::CreateUnit;
        }
        else if ( arg.compare(1, 9, "ENTERVIEW") == 0 )
            actionType = Chk::Action::VirtualType::CenterView;
        else if ( arg.compare(1, 5, "USTOM") == 0 )
            actionType = Chk::Action::VirtualType::Custom;
        break;

    case 'D':
        if ( arg.compare(1, 17, "ISPLAYTEXTMESSAGE") == 0 )
            actionType = Chk::Action::VirtualType::DisplayTextMessage;
        else if ( arg.compare(1, 5, "EFEAT") == 0 )
            actionType = Chk::Action::VirtualType::Defeat;
        else if ( arg.compare(1, 3, "RAW") == 0 )
            actionType = Chk::Action::VirtualType::Draw;
        break;

    case 'G':
        if ( arg.compare(1, 16, "IVEUNITSTOPLAYER") == 0 )
            actionType = Chk::Action::VirtualType::GiveUnitsToPlayer;
        break;

    case 'K':
        if ( arg.compare(1, 7, "ILLUNIT") == 0 )
        {
            if ( arg.compare(8, 10, "ATLOCATION") == 0 )
                actionType = Chk::Action::VirtualType::KillUnitAtLocation;
            else if ( arg.size() == 8 )
                actionType = Chk::Action::VirtualType::KillUnit;
        }
        break;

    case 'L':
        if ( arg.compare(1, 10, "EADERBOARD") == 0 )
        {
            if ( arg.compare(11, 4, "GOAL") == 0 )
            {
                if ( arg.compare(15, 7, "CONTROL") == 0 )
                {
                    if ( arg.compare(22, 10, "ATLOCATION") == 0 )
                        actionType = Chk::Action::VirtualType::LeaderboardGoalCtrlAtLoc;
                    else if ( arg.size() == 22 )
                        actionType = Chk::Action::VirtualType::LeaderboardGoalCtrl;
                }
                else if ( arg.compare(15, 5, "KILLS") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardGoalKills;
                else if ( arg.compare(15, 6, "POINTS") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardGoalPoints;
                else if ( arg.compare(15, 9, "RESOURCES") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardGoalResources;
            }
            else
            {
                if ( arg.compare(11, 7, "CONTROL") == 0 )
                {
                    if ( arg.compare(18, 10, "ATLOCATION") == 0 )
                        actionType = Chk::Action::VirtualType::LeaderboardCtrlAtLoc;
                    else if ( arg.size() == 18 )
                        actionType = Chk::Action::VirtualType::LeaderboardCtrl;
                }
                else if ( arg.compare(11, 5, "GREED") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardGreed;
                else if ( arg.compare(11, 5, "KILLS") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardKills;
                else if ( arg.compare(11, 6, "POINTS") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardPoints;
                else if ( arg.compare(11, 9, "RESOURCES") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardResources;
                else if ( arg.compare(11, 15, "COMPUTERPLAYERS") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardCompPlayers;
            }
        }
        break;

    case 'M':
        if ( arg.compare(1, 5, "EMORY") == 0 )
            actionType = Chk::Action::VirtualType::SetMemory;
        else if ( arg.compare(1, 3, "OVE") == 0 )
        {
            if ( arg.compare(4, 4, "UNIT") == 0 )
                actionType = Chk::Action::VirtualType::MoveUnit;
            else if ( arg.compare(4, 8, "LOCATION") == 0 )
                actionType = Chk::Action::VirtualType::MoveLocation;
        }
        else if ( arg.compare(1, 9, "ODIFYUNIT") == 0 )
        {
            if ( arg.compare(10, 6, "ENERGY") == 0 )
                actionType = Chk::Action::VirtualType::ModifyUnitEnergy;
            else if ( arg.compare(10, 11, "HANGERCOUNT") == 0 )
                actionType = Chk::Action::VirtualType::ModifyUnitHangerCount;
            else if ( arg.compare(10, 9, "HITPOINTS") == 0 )
                actionType = Chk::Action::VirtualType::ModifyUnitHitpoints;
            else if ( arg.compare(10, 14, "RESOURCEAMOUNT") == 0 )
                actionType = Chk::Action::VirtualType::ModifyUnitResourceAmount;
            else if ( arg.compare(10, 12, "SHIELDPOINTS") == 0 )
                actionType = Chk::Action::VirtualType::ModifyUnitShieldPoints;
        }
        else if ( arg.compare(1, 10, "INIMAPPING") == 0 )
            actionType = Chk::Action::VirtualType::MinimapPing;
        else if ( arg.compare(1, 13, "UTEUNITSPEECH") == 0 )
            actionType = Chk::Action::VirtualType::MuteUnitSpeech;
        break;

    case 'O':
        if ( arg.compare(1, 4, "RDER") == 0 )
            actionType = Chk::Action::VirtualType::Order;
        break;

    case 'P':
        if ( arg.compare(1, 14, "RESERVETRIGGER") == 0 )
            actionType = Chk::Action::VirtualType::PreserveTrigger;
        else if ( arg.compare(1, 6, "LAYWAV") == 0 )
            actionType = Chk::Action::VirtualType::PlaySound;
        else if ( arg.compare(1, 4, "AUSE") == 0 )
        {
            if ( arg.compare(5, 4, "GAME") == 0 )
                actionType = Chk::Action::VirtualType::PauseGame;
            else if ( arg.compare(5, 5, "TIMER") == 0 )
                actionType = Chk::Action::VirtualType::PauseTimer;
        }
        break;

    case 'R':
        if ( arg.compare(1, 9, "EMOVEUNIT") == 0 )
        {
            if ( arg.compare(10, 10, "ATLOCATION") == 0 )
                actionType = Chk::Action::VirtualType::RemoveUnitAtLocation;
            else if ( arg.size() == 10 )
                actionType = Chk::Action::VirtualType::RemoveUnit;
        }
        else if ( arg.compare(1, 10, "UNAISCRIPT") == 0 )
        {
            if ( arg.compare(11, 10, "ATLOCATION") == 0 )
                actionType = Chk::Action::VirtualType::RunAiScriptAtLocation;
            else if ( arg.size() == 11 )
                actionType = Chk::Action::VirtualType::RunAiScript;
        }
        break;

    case 'S':
        if ( arg.compare(1, 2, "ET") == 0 )
        {
            if ( arg.compare(3, 6, "DEATHS") == 0 )
                actionType = Chk::Action::VirtualType::SetDeaths;
            else if ( arg.compare(3, 6, "SWITCH") == 0 )
                actionType = Chk::Action::VirtualType::SetSwitch;
            else if ( arg.compare(3, 9, "RESOURCES") == 0 )
                actionType = Chk::Action::VirtualType::SetResources;
            else if ( arg.compare(3, 5, "SCORE") == 0 )
                actionType = Chk::Action::VirtualType::SetScore;
            else if ( arg.compare(3, 14, "ALLIANCESTATUS") == 0 )
                actionType = Chk::Action::VirtualType::SetAllianceStatus;
            else if ( arg.compare(3, 14, "COUNTDOWNTIMER") == 0 )
                actionType = Chk::Action::VirtualType::SetCountdownTimer;
            else if ( arg.compare(3, 11, "DOODADSTATE") == 0 )
                actionType = Chk::Action::VirtualType::SetDoodadState;
            else if ( arg.compare(3, 13, "INVINCIBILITY") == 0 )
                actionType = Chk::Action::VirtualType::SetInvincibility;
            else if ( arg.compare(3, 17, "MISSIONOBJECTIVES") == 0 )
                actionType = Chk::Action::VirtualType::SetMissionObjectives;
            else if ( arg.compare(3, 12, "NEXTSCENARIO") == 0 )
                actionType = Chk::Action::VirtualType::SetNextScenario;
            else if ( arg.compare(3, 6, "MEMORY") == 0 )
                actionType = Chk::Action::VirtualType::SetMemory;
        }
        break;

    case 'T':
        if ( arg.compare(1, 14, "ALKINGPORTRAIT") == 0 )
            actionType = Chk::Action::VirtualType::TalkingPortrait;
        else if ( arg.compare(1, 11, "RANSMISSION") == 0 )
            actionType = Chk::Action::VirtualType::Transmission;
        break;

    case 'U':
        if ( arg.compare(1, 6, "NPAUSE") == 0 )
        {
            if ( arg.compare(7, 5, "TIMER") == 0 )
                actionType = Chk::Action::VirtualType::UnpauseTimer;
            else if ( arg.compare(7, 4, "GAME") == 0 )
                actionType = Chk::Action::VirtualType::UnpauseGame;
        }
        else if ( arg.compare(1, 15, "NMUTEUNITSPEECH") == 0 )
            actionType = Chk::Action::VirtualType::MuteUnitSpeech;
        break;

    case 'V':
        if ( arg.compare(1, 6, "ICTORY") == 0 )
            actionType = Chk::Action::VirtualType::Victory;
        break;

    case 'W':
        if ( arg.compare(1, 3, "AIT") == 0 )
            actionType = Chk::Action::VirtualType::Wait;
        break;
    }

    return actionType != Chk::Action::VirtualType::NoAction;
}

bool TextTrigCompiler::ParseAction(std::string & text, size_t pos, size_t end, bool diabled, Chk::Action::VirtualType & actionType, u8 & flags)
{
    actionType = Chk::Action::VirtualType::NoAction;
    u16 number = 0;

    size_t size = end - pos;
    std::string arg;

    for ( u32 i=0; i<size; i++ ) // Copy argument to arg buffer
    {
        char character = text[i+pos];
        if ( character > 96 && character < 123 ) // lower-case
            arg.push_back(character-32); // Capitalize
        else if ( character != ' ' && character != '\t' ) // Ignore spaces and tabs
            arg.push_back(character);
    }

    char currChar = arg[0];

    switch ( currChar )
    {
    case 'C':
        if ( arg.compare(1, 6, "OMMENT") == 0 )
            actionType = Chk::Action::VirtualType::Comment;
        else if ( arg.compare(1, 9, "REATEUNIT") == 0 )
        {
            if ( arg.compare(10, 14, "WITHPROPERTIES") == 0 )
                actionType = Chk::Action::VirtualType::CreateUnitWithProperties;
            else if ( arg.size() == 10 )
                actionType = Chk::Action::VirtualType::CreateUnit;
        }
        else if ( arg.compare(1, 9, "ENTERVIEW") == 0 )
            actionType = Chk::Action::VirtualType::CenterView;
        else if ( arg.compare(1, 5, "USTOM") == 0 )
            actionType = Chk::Action::VirtualType::Custom;
        break;

    case 'D':
        if ( arg.compare(1, 17, "ISPLAYTEXTMESSAGE") == 0 )
            actionType = Chk::Action::VirtualType::DisplayTextMessage;
        else if ( arg.compare(1, 5, "EFEAT") == 0 )
            actionType = Chk::Action::VirtualType::Defeat;
        else if ( arg.compare(1, 3, "RAW") == 0 )
            actionType = Chk::Action::VirtualType::Draw;
        break;

    case 'G':
        if ( arg.compare(1, 16, "IVEUNITSTOPLAYER") == 0 )
            actionType = Chk::Action::VirtualType::GiveUnitsToPlayer;
        break;

    case 'K':
        if ( arg.compare(1, 7, "ILLUNIT") == 0 )
        {
            if ( arg.compare(8, 10, "ATLOCATION") == 0 )
                actionType = Chk::Action::VirtualType::KillUnitAtLocation;
            else if ( arg.size() == 8 )
                actionType = Chk::Action::VirtualType::KillUnit;
        }
        break;

    case 'L':
        if ( arg.compare(1, 10, "EADERBOARD") == 0 )
        {
            if ( arg.compare(11, 4, "GOAL") == 0 )
            {
                if ( arg.compare(15, 7, "CONTROL") == 0 )
                {
                    if ( arg.compare(22, 10, "ATLOCATION") == 0 )
                        actionType = Chk::Action::VirtualType::LeaderboardGoalCtrlAtLoc;
                    else if ( arg.size() == 22 )
                        actionType = Chk::Action::VirtualType::LeaderboardGoalCtrl;
                }
                else if ( arg.compare(15, 5, "KILLS") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardGoalKills;
                else if ( arg.compare(15, 6, "POINTS") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardGoalPoints;
                else if ( arg.compare(15, 9, "RESOURCES") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardGoalResources;
            }
            else
            {
                if ( arg.compare(11, 7, "CONTROL") == 0 )
                {
                    if ( arg.compare(18, 10, "ATLOCATION") == 0 )
                        actionType = Chk::Action::VirtualType::LeaderboardCtrlAtLoc;
                    else if ( arg.size() == 18 )
                        actionType = Chk::Action::VirtualType::LeaderboardCtrl;
                }
                else if ( arg.compare(11, 5, "GREED") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardGreed;
                else if ( arg.compare(11, 5, "KILLS") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardKills;
                else if ( arg.compare(11, 6, "POINTS") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardPoints;
                else if ( arg.compare(11, 9, "RESOURCES") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardResources;
                else if ( arg.compare(11, 15, "COMPUTERPLAYERS") == 0 )
                    actionType = Chk::Action::VirtualType::LeaderboardCompPlayers;
            }
        }
        break;

    case 'M':
        if ( arg.compare(1, 5, "EMORY") == 0 )
            actionType = Chk::Action::VirtualType::SetMemory;
        else if ( arg.compare(1, 3, "OVE") == 0 )
        {
            if ( arg.compare(4, 4, "UNIT") == 0 )
                actionType = Chk::Action::VirtualType::MoveUnit;
            else if ( arg.compare(4, 8, "LOCATION") == 0 )
                actionType = Chk::Action::VirtualType::MoveLocation;
        }
        else if ( arg.compare(1, 9, "ODIFYUNIT") == 0 )
        {
            if ( arg.compare(10, 6, "ENERGY") == 0 )
                actionType = Chk::Action::VirtualType::ModifyUnitEnergy;
            else if ( arg.compare(10, 11, "HANGERCOUNT") == 0 )
                actionType = Chk::Action::VirtualType::ModifyUnitHangerCount;
            else if ( arg.compare(10, 9, "HITPOINTS") == 0 )
                actionType = Chk::Action::VirtualType::ModifyUnitHitpoints;
            else if ( arg.compare(10, 14, "RESOURCEAMOUNT") == 0 )
                actionType = Chk::Action::VirtualType::ModifyUnitResourceAmount;
            else if ( arg.compare(10, 12, "SHIELDPOINTS") == 0 )
                actionType = Chk::Action::VirtualType::ModifyUnitShieldPoints;
        }
        else if ( arg.compare(1, 10, "INIMAPPING") == 0 )
            actionType = Chk::Action::VirtualType::MinimapPing;
        else if ( arg.compare(1, 13, "UTEUNITSPEECH") == 0 )
            actionType = Chk::Action::VirtualType::MuteUnitSpeech;
        break;

    case 'O':
        if ( arg.compare(1, 4, "RDER") == 0 )
            actionType = Chk::Action::VirtualType::Order;
        break;

    case 'P':
        if ( arg.compare(1, 14, "RESERVETRIGGER") == 0 )
            actionType = Chk::Action::VirtualType::PreserveTrigger;
        else if ( arg.compare(1, 6, "LAYWAV") == 0 )
            actionType = Chk::Action::VirtualType::PlaySound;
        else if ( arg.compare(1, 4, "AUSE") == 0 )
        {
            if ( arg.compare(5, 4, "GAME") == 0 )
                actionType = Chk::Action::VirtualType::PauseGame;
            else if ( arg.compare(5, 5, "TIMER") == 0 )
                actionType = Chk::Action::VirtualType::PauseTimer;
        }
        break;

    case 'R':
        if ( arg.compare(1, 9, "EMOVEUNIT") == 0 )
        {
            if ( arg.compare(10, 10, "ATLOCATION") == 0 )
                actionType = Chk::Action::VirtualType::RemoveUnitAtLocation;
            else if ( arg.size() == 10 )
                actionType = Chk::Action::VirtualType::RemoveUnit;
        }
        else if ( arg.compare(1, 10, "UNAISCRIPT") == 0 )
        {
            if ( arg.compare(11, 10, "ATLOCATION") == 0 )
                actionType = Chk::Action::VirtualType::RunAiScriptAtLocation;
            else if ( arg.size() == 11 )
                actionType = Chk::Action::VirtualType::RunAiScript;
        }
        break;

    case 'S':
        if ( arg.compare(1, 2, "ET") == 0 )
        {
            if ( arg.compare(3, 6, "DEATHS") == 0 )
                actionType = Chk::Action::VirtualType::SetDeaths;
            else if ( arg.compare(3, 6, "SWITCH") == 0 )
                actionType = Chk::Action::VirtualType::SetSwitch;
            else if ( arg.compare(3, 9, "RESOURCES") == 0 )
                actionType = Chk::Action::VirtualType::SetResources;
            else if ( arg.compare(3, 5, "SCORE") == 0 )
                actionType = Chk::Action::VirtualType::SetScore;
            else if ( arg.compare(3, 14, "ALLIANCESTATUS") == 0 )
                actionType = Chk::Action::VirtualType::SetAllianceStatus;
            else if ( arg.compare(3, 14, "COUNTDOWNTIMER") == 0 )
                actionType = Chk::Action::VirtualType::SetCountdownTimer;
            else if ( arg.compare(3, 11, "DOODADSTATE") == 0 )
                actionType = Chk::Action::VirtualType::SetDoodadState;
            else if ( arg.compare(3, 13, "INVINCIBILITY") == 0 )
                actionType = Chk::Action::VirtualType::SetInvincibility;
            else if ( arg.compare(3, 17, "MISSIONOBJECTIVES") == 0 )
                actionType = Chk::Action::VirtualType::SetMissionObjectives;
            else if ( arg.compare(3, 12, "NEXTSCENARIO") == 0 )
                actionType = Chk::Action::VirtualType::SetNextScenario;
            else if ( arg.compare(3, 6, "MEMORY") == 0 )
                actionType = Chk::Action::VirtualType::SetMemory;
        }
        break;

    case 'T':
        if ( arg.compare(1, 14, "ALKINGPORTRAIT") == 0 )
            actionType = Chk::Action::VirtualType::TalkingPortrait;
        else if ( arg.compare(1, 11, "RANSMISSION") == 0 )
            actionType = Chk::Action::VirtualType::Transmission;
        break;

    case 'U':
        if ( arg.compare(1, 6, "NPAUSE") == 0 )
        {
            if ( arg.compare(7, 5, "TIMER") == 0 )
                actionType = Chk::Action::VirtualType::UnpauseTimer;
            else if ( arg.compare(7, 4, "GAME") == 0 )
                actionType = Chk::Action::VirtualType::UnpauseGame;
        }
        else if ( arg.compare(1, 15, "NMUTEUNITSPEECH") == 0 )
            actionType = Chk::Action::VirtualType::UnmuteUnitSpeech;
        break;

    case 'V':
        if ( arg.compare(1, 6, "ICTORY") == 0 )
            actionType = Chk::Action::VirtualType::Victory;
        break;

    case 'W':
        if ( arg.compare(1, 3, "AIT") == 0 )
            actionType = Chk::Action::VirtualType::Wait;
        break;
    }

    flags = Chk::Action::getDefaultFlags(actionType);

    return actionType != Chk::Action::VirtualType::NoAction;
}

bool TextTrigCompiler::ParseConditionArg(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, Chk::Condition & currCondition, size_t pos, size_t end, Chk::Condition::VirtualType conditionType, Chk::Condition::Argument argument, std::stringstream & error)
{
    const char* textPtr = text.c_str();

    // returns whether the condition was true and prints msg to the error message if false
#define returnMsg(condition, msg)                           \
    if ( condition )                                        \
        return true;                                        \
    else {                                                  \
        error << msg;                                       \
        return false;                                       \
    }

    Chk::Condition::ArgType argType = argument.type;
    switch ( argType )
    {
        case Chk::Condition::ArgType::Unit:
            returnMsg( ParseUnitName(text, stringContents, nextString, currCondition.unitType, pos, end) ||
                ParseShort(textPtr, (u16 &)currCondition.unitType, pos, end),
                "Expected: Unit name or 2-byte unitID" );
        case Chk::Condition::ArgType::Location:
            returnMsg( ParseLocationName(text, stringContents, nextString, currCondition.locationId, pos, end) ||
                ParseLong(textPtr, currCondition.locationId, pos, end),
                "Expected: Location name or 4-byte locationNum" );
        case Chk::Condition::ArgType::Player:
            returnMsg( ParsePlayer(text, stringContents, nextString, currCondition.player, pos, end) ||
                ParseLong(textPtr, currCondition.player, pos, end),
                "Expected: Player/group name or 4-byte id" );
        case Chk::Condition::ArgType::Amount:
            returnMsg( ParseLong(textPtr, currCondition.amount, pos, end),
                "Expected: 4-byte amount" );
        case Chk::Condition::ArgType::NumericComparison:
            returnMsg( ParseNumericComparison(textPtr, stringContents, nextString, currCondition.comparison, pos, end) ||
                ParseByte(textPtr, (u8 &)currCondition.comparison, pos, end),
                "Expected: Numeric comparison or 1-byte comparisonID" );
        case Chk::Condition::ArgType::ResourceType:
            returnMsg( ParseResourceType(textPtr, stringContents, nextString, currCondition.typeIndex, pos, end) ||
                ParseByte(textPtr, currCondition.typeIndex, pos, end),
                "Expected: Resource type or 1-byte resourceID" );
        case Chk::Condition::ArgType::ScoreType:
            returnMsg( ParseScoreType(text, stringContents, nextString, currCondition.typeIndex, pos, end) ||
                ParseByte(textPtr, currCondition.typeIndex, pos, end),
                "Expected: Score type or 1-byte scoreID" );
        case Chk::Condition::ArgType::Switch:
            returnMsg( ParseSwitch(text, stringContents, nextString, currCondition.typeIndex, pos, end) ||
                ParseByte(textPtr, currCondition.typeIndex, pos, end),
                "Expected: Switch name or 1-byte switchID" );
        case Chk::Condition::ArgType::SwitchState:
            returnMsg( ParseSwitchState(textPtr, stringContents, nextString, currCondition.comparison, pos, end) ||
                ParseByte(textPtr, (u8 &)currCondition.comparison, pos, end),
                "Expected: Switch state or 1-byte comparisonID" );
        case Chk::Condition::ArgType::Comparison: // NumericComparison, SwitchState
            returnMsg( ParseByte(textPtr, (u8 &)currCondition.comparison, pos, end) ||
                ParseNumericComparison(textPtr, stringContents, nextString, currCondition.comparison, pos, end) ||
                ParseSwitchState(textPtr, stringContents, nextString, currCondition.comparison, pos, end),
                "Expected: 1-byte comparison" );
        case Chk::Condition::ArgType::ConditionType:
            returnMsg( ParseByte(textPtr, (u8 &)currCondition.conditionType, pos, end),
                "Expected: 1-byte conditionID" );
        case Chk::Condition::ArgType::TypeIndex: // ResourceType, ScoreType, Switch
            returnMsg( ParseByte(textPtr, currCondition.typeIndex, pos, end) ||
                ParseResourceType(text, stringContents, nextString, currCondition.typeIndex, pos, end) ||
                ParseScoreType(text, stringContents, nextString, currCondition.typeIndex, pos, end) ||
                ParseSwitch(text, stringContents, nextString, currCondition.typeIndex, pos, end),
                "Expected: 1-byte typeId, resource type, score type, or switch name" );
        case Chk::Condition::ArgType::Flags:
            returnMsg( ParseByte(textPtr, currCondition.flags, pos, end),
                "Expected: 1-byte flags" );
        case Chk::Condition::ArgType::MaskFlag:
            returnMsg( ParseShort(textPtr, (u16 &)currCondition.maskFlag, pos, end),
                "Expected: 2-byte internal data" );
        case Chk::Condition::ArgType::MemoryOffset:
            returnMsg( (useAddressesForMemory && ParseMemoryAddress(textPtr, currCondition.player, pos, end, deathTableOffset) ||
                !useAddressesForMemory && ParseLong(textPtr, currCondition.player, pos, end)),
                (useAddressesForMemory ? "Expected: 4-byte address" : "Expected: 4-byte death table offset") );
    }
    CHKD_ERR("INTERNAL ERROR: Invalid argIndex or argument unhandled, report this");
    return false;
}

bool TextTrigCompiler::ParseActionArg(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, Chk::Action & currAction, size_t pos, size_t end, Chk::Action::VirtualType actionType, Chk::Action::Argument arg, std::stringstream & error)
{
    const char* textPtr = text.c_str();
    
    switch ( arg.type )
    {
        case Chk::Action::ArgType::Location:
            returnMsg( ParseLocationName(text, stringContents, nextString, arg.field == Chk::Action::ArgField::Number ? currAction.number : currAction.locationId, pos, end) ||
                ParseLong(textPtr, arg.field == Chk::Action::ArgField::Number ? currAction.number : currAction.locationId, pos, end),
                "Expected: Location name or 4-byte locationNum" );
        case Chk::Action::ArgType::String:
            returnMsg( ParseString(text, stringContents, nextString, currAction.stringId, pos, end) ||
                ParseLong(textPtr, currAction.stringId, pos, end),
                "Expected: String or stringNum" );
        case Chk::Action::ArgType::Player:
            returnMsg( ParsePlayer(text, stringContents, nextString, arg.field == Chk::Action::ArgField::Number ? currAction.number : currAction.group, pos, end) ||
                ParseLong(textPtr, arg.field == Chk::Action::ArgField::Number ? currAction.number : currAction.group, pos, end),
                "Expected: Group name or 4-byte groupID" );
        case Chk::Action::ArgType::Unit:
            returnMsg( ParseUnitName(text, stringContents, nextString, (Sc::Unit::Type &)currAction.type, pos, end) ||
                ParseShort(textPtr, (u16 &)currAction.type, pos, end),
                "Expected: Unit name or 2-byte unitID" );
        case Chk::Action::ArgType::NumUnits:
            returnMsg( ParseSpecialUnitAmount(textPtr, stringContents, nextString, currAction.type2, pos, end) ||
                ParseByte(textPtr, currAction.type2, pos, end),
                "Expected: 1-byte number" );
        case Chk::Action::ArgType::CUWP:
        case Chk::Action::ArgType::Percent:
        case Chk::Action::ArgType::Amount:
            returnMsg((arg.field == Chk::Action::ArgField::Type2 ? ParseByte(textPtr, currAction.type2, pos, end) : ParseLong(textPtr, currAction.number, pos, end)),
                (arg.field == Chk::Action::ArgField::Type2 ? "Expected: 1-byte number" : "Expected: 4-byte number" ));
        case Chk::Action::ArgType::ScoreType:
            returnMsg( ParseScoreType(text, stringContents, nextString, currAction.type, pos, end) ||
                ParseShort(textPtr, currAction.type, pos, end),
                "Expected: Score type or 1-byte scoreID" );
        case Chk::Action::ArgType::ResourceType:
            returnMsg( ParseResourceType(text, stringContents, nextString, currAction.type, pos, end) ||
                ParseShort(textPtr, currAction.type, pos, end),
                "Expected: Resource type or 2-byte number" );
        case Chk::Action::ArgType::StateMod:
            returnMsg( ParseStateMod(textPtr, stringContents, nextString, currAction.type2, pos, end) ||
                ParseByte(textPtr, currAction.type2, pos, end),
                "Expected: State modifier or 1-byte number" );
        case Chk::Action::ArgType::Order:
            returnMsg ( ParseOrder(textPtr, stringContents, nextString, currAction.type2, pos, end) ||
                ParseByte(textPtr, currAction.type2, pos, end),
                "Expected: Order or 1-byte number" );
        case Chk::Action::ArgType::Sound:
            returnMsg( ParseWavName(text, stringContents, nextString, currAction.soundStringId, pos, end) ||
                ParseLong(textPtr, currAction.soundStringId, pos, end),
                "Expected: Wav name or 4-byte wavID" );
        case Chk::Action::ArgType::Duration:
            returnMsg( ParseLong(textPtr, currAction.time, pos, end),
                "Expected: 4-byte duration" );
        case Chk::Action::ArgType::Script:
            returnMsg ( ParseScript(text, stringContents, nextString, currAction.number, pos, end) ||
                ParseLong(textPtr, currAction.number, pos, end),
                "Expected: Script name or 4-byte script num" );
        case Chk::Action::ArgType::AllyState:
            returnMsg( ParseAllianceStatus(textPtr, stringContents, nextString, currAction.type, pos, end) ||
                ParseShort(textPtr, currAction.type, pos, end),
                "Expected: Alliance status or 2-byte number" );
        case Chk::Action::ArgType::NumericMod:
            returnMsg( ParseNumericModifier(textPtr, stringContents, nextString, currAction.type2, pos, end) ||
                ParseByte(textPtr, currAction.type2, pos, end),
                "Expected: Numeric modifier or 1-byte number" );
        case Chk::Action::ArgType::Switch:
            returnMsg ( ParseSwitch(text, stringContents, nextString, currAction.number, pos, end) ||
                ParseLong(textPtr, currAction.number, pos, end),
                "Expected: Switch name or 4-byte number" );
        case Chk::Action::ArgType::SwitchMod:
            returnMsg ( ParseSwitchMod(textPtr, stringContents, nextString, currAction.type2, pos, end) ||
                ParseByte(textPtr, currAction.type2, pos, end),
                "Expected: Switch modifier or 1-byte number" );
        case Chk::Action::ArgType::ActionType:
            returnMsg( ParseByte(textPtr, (u8 &)currAction.actionType, pos, end),
                "Expected: 1-byte actionID" );
        case Chk::Action::ArgType::TextFlags:
        case Chk::Action::ArgType::Flags:
            returnMsg( ParseTextDisplayFlag(textPtr, stringContents, nextString, currAction.flags, pos, end) ||
                ParseByte(textPtr, currAction.flags, pos, end),
                "Expected: Always display text flags or 1-byte flag data" );
        case Chk::Action::ArgType::Number: // Amount, Group2, LocDest, UnitPropNum, ScriptNum
            returnMsg( ParsePlayer(text, stringContents, nextString, currAction.number, pos, end) ||
                ParseLocationName(text, stringContents, nextString, currAction.number, pos, end) ||
                ParseScript(text, stringContents, nextString, currAction.number, pos, end) ||
                ParseSwitch(text, stringContents, nextString, currAction.number, pos, end) ||
                ParseLong(textPtr, currAction.number, pos, end),
                "Expected: Group, location, script, switch, or 4-byte number" );
        case Chk::Action::ArgType::TypeIndex: // Unit, ScoreType, ResourceType, AllianceStatus
            returnMsg( ParseUnitName(text, stringContents, nextString, (Sc::Unit::Type &)currAction.type, pos, end) ||
                ParseScoreType(text, stringContents, nextString, currAction.type, pos, end) ||
                ParseResourceType(text, stringContents, nextString, currAction.type, pos, end) ||
                ParseAllianceStatus(textPtr, stringContents, nextString, currAction.type, pos, end) ||
                ParseShort(textPtr, currAction.type, pos, end),
                "Expected: Unit, score type, resource type, alliance status, or 2-byte typeID" );
        case Chk::Action::ArgType::SecondaryTypeIndex: // NumUnits (0=all), SwitchAction, UnitOrder, ModifyType
            returnMsg( ParseSwitchMod(textPtr, stringContents, nextString, currAction.type2, pos, end) ||
                ParseOrder(textPtr, stringContents, nextString, currAction.type2, pos, end) ||
                ParseNumericModifier(textPtr, stringContents, nextString, currAction.type2, pos, end) ||
                ParseStateMod(textPtr, stringContents, nextString, currAction.type2, pos, end) ||
                ParseByte(textPtr, currAction.type2, pos, end),
                "Expected: Switch modifier, order, numeric modifier, state modifier, or 1-byte number" );
        case Chk::Action::ArgType::Padding:
            returnMsg( ParseByte(textPtr, currAction.padding, pos, end),
                "Expected: 1-byte padding" );
        case Chk::Action::ArgType::MaskFlag:
            returnMsg( ParseShort(textPtr, (u16 &)currAction.maskFlag, pos, end),
                "Expected: 2-byte mask flag" );
        case Chk::Action::ArgType::MemoryOffset:
            returnMsg( (useAddressesForMemory && ParseMemoryAddress(textPtr, currAction.group, pos, end, deathTableOffset) ||
                !useAddressesForMemory && ParseLong(textPtr, currAction.group, pos, end)),
                (useAddressesForMemory ? "Expected: 4-byte address" : "Expected: 4-byte death table offset") );
    }
    CHKD_ERR("INTERNAL ERROR: Invalid argIndex or argument unhandled, report this");
    return false;
}

bool TextTrigCompiler::ParseExecutionFlags(std::string & text, size_t pos, size_t end, u32 & flags)
{
    flags = 0;

    size_t size = end - pos;
    std::string arg;

    for ( u32 i=0; i<size; i++ ) // Copy argument to arg buffer
    {
        char character = text[i+pos];
        if ( character > 96 && character < 123 ) // lower-case
            arg.push_back(character-32); // Capitalize
        else if ( character != ' ' && character != '\t' ) // Ignore spaces and tabs
            arg.push_back(character);
    }

    const char* argData = arg.c_str();
    return ParseBinaryLong(argData, flags, 0, arg.size());
}

bool TextTrigCompiler::ParseString(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, u32 & dest, size_t pos, size_t end)
{
    if ( compareCaseless(text, pos, 9, "No String") )
    {
        dest = 0;
        return true;
    }
    else if ( pos < end && text[pos] == '\"' )
    {
        const std::string & str = stringContents[nextString];
        nextString++;
        
        size_t hash = strHash(str);
        auto matches = newStringTable.equal_range(hash);
        if ( matches.first != newStringTable.end() && matches.first->first == hash )
        {
            for ( auto it = matches.first; it != matches.second; ++it )
            {
                StringTableNodePtr & node = it->second;
                if ( node->scStr->compare<RawString>(str) == 0 )
                {
                    if ( node->unused )
                        node->unused = false;

                    if ( node->stringId == Chk::StringId::NoString )
                        node->assignees.push_back(&dest);
                    else
                        dest = node->stringId;

                    return true;
                }
            }
        }

        // No matches
        StringTableNodePtr node = StringTableNodePtr(new StringTableNode({}));
        node->unused = false;
        node->scStr = ScStrPtr(new ScStr(str));
        node->stringId = 0;
        node->assignees.push_back(&dest);
        newStringTable.insert(std::pair<size_t, StringTableNodePtr>(strHash(str), node));
        unassignedStrings.push_back(node);
        return true;
    }
    else
        return ParseLong(text.c_str(), dest, pos, end);
}

bool TextTrigCompiler::ParseLocationName(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, u32 & dest, size_t pos, size_t end)
{
    std::string str;
    if ( text.compare(pos, end-pos, "NOLOCATION") == 0 )
    {
        dest = 0;
        return true;
    }
    else if ( pos < end && text[pos] == '\"' )
    {
        str = stringContents[nextString];
        nextString ++;
    }
    else if ( ParseLong(text.c_str(), dest, pos, end) )
        return true;
    else
        str = text.substr(pos, end-pos);

    bool success = false;

    if ( str.compare("anywhere") == 0 ) // Capitalize lower-case anywhere's
        str[0] = 'A';

    // Grab the string hash
    size_t hash = strHash(str);
    size_t numMatching = locationTable.count(hash);
    if ( numMatching == 1 )
    { // Should guarentee that you can find at least one entry
        LocationTableNode & node = locationTable.find(hash)->second;
        if ( node.locationName.compare(str) == 0 )
        {
            dest = node.locationId;
            success = true;
        }
    }
    else if ( numMatching > 1 )
    {
        auto range = locationTable.equal_range(hash);
        foreachin(pair, range)
        {
            LocationTableNode & node = pair->second;
            if ( node.locationName.compare(str) == 0 )
            {
                if ( success == false ) // If no matches have previously been found
                {
                    dest = node.locationId;
                    success = true;
                }
                else // If matches have previously been found
                {
                    if ( u32(node.locationId) < dest )
                        dest = node.locationId; // Replace if locationNum < previous locationNum
                }
            }
        }
    }
    return success;
}

bool TextTrigCompiler::ParseUnitName(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, Sc::Unit::Type & dest, size_t pos, size_t end)
{
    std::string str;
    if ( text[pos] == '\"' ) // If quoted, ignore quotes
    {
        if ( end-pos < 2 )
            return false;

        str = stringContents[nextString];
        nextString ++;
        pos = 0;
        end = str.size();
    }
    else if ( ParseShort(text.c_str(), (u16 &)dest, pos, end) )
        return true;
    else
        str = text.substr(pos, end-pos);

    bool success = false;
    size_t size = str.size();
    char unit[40] = {};
    if ( size < 40 )
    {
        // Take an upper case copy of the name
        for ( int i=0; i<size; i++ )
        {
            char currChar = str[i];
            if ( currChar > 96 && currChar < 123 )
                unit[i] = currChar - 32;
            else
                unit[i] = currChar;
        }
        unit[size] = '\0';

        switch ( unit[0] ) // First search standard unit names
        {
        case 'A':
            if      ( strcmp(&unit[1], "LAN SCHEZAR (GOLIATH)"         ) == 0 ) { dest = Sc::Unit::Type::AlanSchezar_Goliath; success = true; }
            else if ( strcmp(&unit[1], "LAN SCHEZAR TURRET"            ) == 0 ) { dest = Sc::Unit::Type::AlanTurret; success = true; }
            else if ( strcmp(&unit[1], "LDARIS (TEMPLAR)"              ) == 0 ) { dest = Sc::Unit::Type::Aldaris_Templar; success = true; }
            else if ( strcmp(&unit[1], "LEXEI STUKOV (GHOST)"          ) == 0 ) { dest = Sc::Unit::Type::AlexeiStukov_Ghost; success = true; }
            else if ( strcmp(&unit[1], "NY UNIT"                       ) == 0 ) { dest = Sc::Unit::Type::AnyUnit; success = true; }
            else if ( strcmp(&unit[1], "RCTURUS MENGSK (BATTLECRUISER)") == 0 ) { dest = Sc::Unit::Type::ArcturusMengsk_Battlecruiser; success = true; }
            else if ( strcmp(&unit[1], "RTANIS (SCOUT)"                ) == 0 ) { dest = Sc::Unit::Type::Artanis_Scout; success = true; }
            break;
        case 'B':
            if      ( strcmp(&unit[1], "ENGALAAS (JUNGLE CRITTER)") == 0 ) { dest = Sc::Unit::Type::Bengalaas_Jungle; success = true; }
            else if ( strcmp(&unit[1], "UILDINGS"                 ) == 0 ) { dest = Sc::Unit::Type::Buildings; success = true; }
            break;
        case 'C':
            if      ( strcmp(&unit[1], "ARGO SHIP (UNUSED)") == 0 ) { dest = Sc::Unit::Type::CargoShip_Unused; success = true; }
            else if ( strcmp(&unit[1], "ATINA (UNUSED)"    ) == 0 ) { dest = Sc::Unit::Type::Cantina; success = true; }
            else if ( strcmp(&unit[1], "AVE (UNUSED)"      ) == 0 ) { dest = Sc::Unit::Type::Cave; success = true; }
            else if ( strcmp(&unit[1], "AVE-IN (UNUSED)"   ) == 0 ) { dest = Sc::Unit::Type::CaveIn; success = true; }
            break;
        case 'D':
            if      ( strcmp(&unit[1], "ANIMOTH (ARBITER)"      ) == 0 ) { dest = Sc::Unit::Type::Danimoth_Arbiter; success = true; }
            else if ( strcmp(&unit[1], "ARK SWARM"              ) == 0 ) { dest = Sc::Unit::Type::DarkSwarm; success = true; }
            else if ( strcmp(&unit[1], "ATA DISC"               ) == 0 ) { dest = Sc::Unit::Type::DataDisc; success = true; }
            else if ( strcmp(&unit[1], "EVOURING ONE (ZERGLING)") == 0 ) { dest = Sc::Unit::Type::DevouringOne_Zergling; success = true; }
            else if ( strcmp(&unit[1], "ISRUPTION WEB"          ) == 0 ) { dest = Sc::Unit::Type::DisruptionField; success = true; }
            break;
        case 'E':
            if      ( strcmp(&unit[1], "DMUND DUKE (SIEGE MODE)"       ) == 0 ) { dest = Sc::Unit::Type::EdmundDuke_SiegeMode; success = true; }
            else if ( strcmp(&unit[1], "DMUND DUKE (TANK MODE)"        ) == 0 ) { dest = Sc::Unit::Type::EdmundDuke_SiegeTank; success = true; }
            else if ( strcmp(&unit[1], "DMUND DUKE TURRET (SIEGE MODE)") == 0 ) { dest = Sc::Unit::Type::DukeTurretType2; success = true; }
            else if ( strcmp(&unit[1], "DMUND DUKE TURRET (TANK MODE)" ) == 0 ) { dest = Sc::Unit::Type::DukeTurretType1; success = true; }
            break;
        case 'F':
            if      ( strcmp(&unit[1], "ACTORIES"           ) == 0 ) { dest = Sc::Unit::Type::Factories; success = true; }
            else if ( strcmp(&unit[1], "ENIX (DRAGOON)"     ) == 0 ) { dest = Sc::Unit::Type::Fenix_Dragoon; success = true; }
            else if ( strcmp(&unit[1], "ENIX (ZEALOT)"      ) == 0 ) { dest = Sc::Unit::Type::Fenix_Zealot; success = true; }
            else if ( strcmp(&unit[1], "LAG"                ) == 0 ) { dest = Sc::Unit::Type::Flag; success = true; }
            else if ( strcmp(&unit[1], "LOOR GUN TRAP"      ) == 0 ) { dest = Sc::Unit::Type::FloorGunTrap; success = true; }
            else if ( strcmp(&unit[1], "LOOR HATCH (UNUSED)") == 0 ) { dest = Sc::Unit::Type::FloorHatch_Unused; success = true; }
            else if ( strcmp(&unit[1], "LOOR MISSILE TRAP"  ) == 0 ) { dest = Sc::Unit::Type::FloorMissileTrap; success = true; }
            break;
        case 'G':
            if      ( strcmp(&unit[1], "ANTRITHOR (CARRIER)"          ) == 0 ) { dest = Sc::Unit::Type::Gantrithor_Carrier; success = true; }
            else if ( strcmp(&unit[1], "ERARD DUGALLE (BATTLECRUISER)") == 0 ) { dest = Sc::Unit::Type::GerardDuGalle_BattleCruiser; success = true; }
            else if ( strcmp(&unit[1], "OLIATH TURRET"                ) == 0 ) { dest = Sc::Unit::Type::GoliathTurret; success = true; }
            else if ( strcmp(&unit[1], "UI MONTAG (FIREBAT)"          ) == 0 ) { dest = Sc::Unit::Type::GuiMontag_Firebat; success = true; }
            break;
        case 'H':
            if      ( strcmp(&unit[1], "UNTER KILLER (HYDRALISK)") == 0 ) { dest = Sc::Unit::Type::HunterKiller_Hydralisk; success = true; }
            else if ( strcmp(&unit[1], "YPERION (BATTLECRUISER)" ) == 0 ) { dest = Sc::Unit::Type::Hyperion_Battlecruiser; success = true; }
            break;
        case 'I':
            if ( unit[1] == 'D' && unit[2] == ':' )
                success = ParseShort(unit, (u16 &)dest, 3, size);
            else if ( strcmp(&unit[1], "NDEPENDENT COMMAND CENTER (UNUSED)"  ) == 0 ) { dest = Sc::Unit::Type::IndependentCommandCenter_Unused; success = true; }
            else if ( strcmp(&unit[1], "NDEPENDENT JUMP GATE (UNUSED)"       ) == 0 ) { dest = Sc::Unit::Type::IndependentJumpGate_Unused; success = true; }
            else if ( strcmp(&unit[1], "NDEPENDENT STARPORT (UNUSED)"        ) == 0 ) { dest = Sc::Unit::Type::IndependentStarport_Unused; success = true; }
            else if ( strcmp(&unit[1], "NFESTED COMMAND CENTER"              ) == 0 ) { dest = Sc::Unit::Type::InfestedCommandCenter; success = true; }
            else if ( strcmp(&unit[1], "NFESTED DURAN (INFESTED TERRAN)"     ) == 0 ) { dest = Sc::Unit::Type::InfestedDuran; success = true; }
            else if ( strcmp(&unit[1], "NFESTED KERRIGAN (INFESTED TERRAIN)" ) == 0 ) { dest = Sc::Unit::Type::InfestedKerrigan_InfestedTerran; success = true; }
            else if ( strcmp(&unit[1], "NFESTED TERRAN"                      ) == 0 ) { dest = Sc::Unit::Type::InfestedTerran; success = true; }
            else if ( strcmp(&unit[1], "ON CANNON"                           ) == 0 ) { dest = Sc::Unit::Type::IonCannon; success = true; }
            break;
        case 'J':
            if      ( strcmp(&unit[1], "IM RAYNOR (MARINE)" ) == 0 ) { dest = Sc::Unit::Type::JimRaynor_Marine; success = true; }
            else if ( strcmp(&unit[1], "IM RAYNOR (VULTURE)") == 0 ) { dest = Sc::Unit::Type::JimRaynor_Vulture; success = true; }
            break;
        case 'K':
            if      ( strcmp(&unit[1], "AKARU (TWILIGHT CRITTER)"          ) == 0 ) { dest = Sc::Unit::Type::Kakaru_TwilightCritter; success = true; }
            else if ( strcmp(&unit[1], "HADARIN CRYSTAL FORMATION (UNUSED)") == 0 ) { dest = Sc::Unit::Type::KhadarinCrystalFormation_Unused; success = true; }
            else if ( strcmp(&unit[1], "HALIS CRYSTAL"                     ) == 0 ) { dest = Sc::Unit::Type::KhalisCrystal; success = true; }
            else if ( strcmp(&unit[1], "HAYDARIN CRYSTAL"                  ) == 0 ) { dest = Sc::Unit::Type::KhaydarinCrystal; success = true; }
            else if ( strcmp(&unit[1], "HAYDARIN CRYSTAL FORMATION"        ) == 0 ) { dest = Sc::Unit::Type::KhaydarinCrystalFormation; success = true; }
            else if ( strcmp(&unit[1], "UKULZA (GUARDIAN)"                 ) == 0 ) { dest = Sc::Unit::Type::Kukulza_Guardian; success = true; }
            else if ( strcmp(&unit[1], "UKULZA (MUTALISK)"                 ) == 0 ) { dest = Sc::Unit::Type::Kukulza_Mutalisk; success = true; }
            break;
        case 'L':
            if      ( strcmp(&unit[1], "EFT PIT DOOR"         ) == 0 ) { dest = Sc::Unit::Type::LeftPitDoor; success = true; }
            else if ( strcmp(&unit[1], "EFT UPPER LEVEL DOOR" ) == 0 ) { dest = Sc::Unit::Type::LeftUpperLevelDoor; success = true; }
            else if ( strcmp(&unit[1], "EFT WALL FLAME TRAP"  ) == 0 ) { dest = Sc::Unit::Type::LeftWallFlameTrap; success = true; }
            else if ( strcmp(&unit[1], "EFT WALL MISSILE TRAP") == 0 ) { dest = Sc::Unit::Type::LeftWallMissileTrap; success = true; }
            else if ( strcmp(&unit[1], "URKER EGG"            ) == 0 ) { dest = Sc::Unit::Type::LurkerEgg; success = true; }
            break;
        case 'M':
            if      ( strcmp(&unit[1], "AGELLAN (SCIENCE VESSEL)" ) == 0 ) { dest = Sc::Unit::Type::Magellan_ScienceVessel; success = true; }
            else if ( strcmp(&unit[1], "AP REVEALER"              ) == 0 ) { dest = Sc::Unit::Type::MapRevealer; success = true; }
            else if ( strcmp(&unit[1], "ATRIARCH (QUEEN)"         ) == 0 ) { dest = Sc::Unit::Type::Matriarch_Queen; success = true; }
            else if ( strcmp(&unit[1], "ATURE CRYSALIS"           ) == 0 ) { dest = Sc::Unit::Type::MatureCrysalis; success = true; }
            else if ( strcmp(&unit[1], "EN"                       ) == 0 ) { dest = Sc::Unit::Type::Men; success = true; }
            else if ( strcmp(&unit[1], "ERCENARY GUNSHIP (UNUSED)") == 0 ) { dest = Sc::Unit::Type::MercenaryGunship_Unused; success = true; }
            else if ( strcmp(&unit[1], "INERAL CLUSTER TYPE 1"    ) == 0 ) { dest = Sc::Unit::Type::MineralClusterType1; success = true; }
            else if ( strcmp(&unit[1], "INERAL CLUSTER TYPE 2"    ) == 0 ) { dest = Sc::Unit::Type::MineralClusterType2; success = true; }
            else if ( strcmp(&unit[1], "INERAL FIELD (TYPE 1)"    ) == 0 ) { dest = Sc::Unit::Type::MineralFieldType1; success = true; }
            else if ( strcmp(&unit[1], "INERAL FIELD (TYPE 2)"    ) == 0 ) { dest = Sc::Unit::Type::MineralFieldType2; success = true; }
            else if ( strcmp(&unit[1], "INERAL FIELD (TYPE 3)"    ) == 0 ) { dest = Sc::Unit::Type::MineralFieldType3; success = true; }
            else if ( strcmp(&unit[1], "INING PLATFORM (UNUSED)"  ) == 0 ) { dest = Sc::Unit::Type::MiningPlatform_Unused; success = true; }
            else if ( strcmp(&unit[1], "OJO (SCOUT)"              ) == 0 ) { dest = Sc::Unit::Type::Mojo_Scout; success = true; }
            else if ( strcmp(&unit[1], "UTALISK COCOON"           ) == 0 ) { dest = Sc::Unit::Type::Cocoon; success = true; }
            break;
        case 'N':
            if      ( strcmp(&unit[1], "ORAD II (BATTLECRUISER)") == 0 ) { dest = Sc::Unit::Type::NoradII_Battlecruiser; success = true; }
            else if ( strcmp(&unit[1], "ORAD II (CRASHED)"      ) == 0 ) { dest = Sc::Unit::Type::NoradII_Crashed; success = true; }
            else if ( strcmp(&unit[1], "UCLEAR MISSILE"         ) == 0 ) { dest = Sc::Unit::Type::NuclearMissile; success = true; }
            break;
        case 'O':
            if      ( strcmp(&unit[1], "VERMIND COCOON") == 0 ) { dest = Sc::Unit::Type::OvermindCocoon; success = true; }
            break;
        case 'P':
            if ( unit[1] == 'R' )
            {
                switch ( unit[8] )
                {
                case 'A':
                    if      ( strcmp(&unit[2], "OTOSS ARBITER"         ) == 0 ) { dest = Sc::Unit::Type::ProtossArbiter; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS ARBITER TRIBUNAL") == 0 ) { dest = Sc::Unit::Type::ProtossArbiterTribunal; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS ARCHON"          ) == 0 ) { dest = Sc::Unit::Type::ProtossArchon; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS ASSIMILATOR"    ) == 0 ) { dest = Sc::Unit::Type::ProtossAssimilator; success = true; }
                    break;
                case 'B':
                    if ( strcmp(&unit[2], "OTOSS BEACON") == 0 ) { dest = Sc::Unit::Type::ProtossBeacon; success = true; }
                    break;
                case 'C':
                    if      ( strcmp(&unit[2], "OTOSS CARRIER"         ) == 0 ) { dest = Sc::Unit::Type::ProtossCarrier; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS CITADEL OF ADUN" ) == 0 ) { dest = Sc::Unit::Type::ProtossCitadelOfAdum; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS CORSAIR"         ) == 0 ) { dest = Sc::Unit::Type::ProtossCorsair; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS CYBERNETICS CORE") == 0 ) { dest = Sc::Unit::Type::ProtossCyberneticsCore; success = true; }
                    break;
                case 'D':
                    if      ( strcmp(&unit[2], "OTOSS DARK ARCHON"        ) == 0 ) { dest = Sc::Unit::Type::ProtossDarkArchon; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS DARK TEMPLAR (HERO)") == 0 ) { dest = Sc::Unit::Type::DarkTemplar_Hero; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS DARK TEMPLAR (UNIT)") == 0 ) { dest = Sc::Unit::Type::ProtossDarkTemplar; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS DRAGOON"            ) == 0 ) { dest = Sc::Unit::Type::ProtossDragoon; success = true; }
                    break;
                case 'F':
                    if      ( strcmp(&unit[2], "OTOSS FLAG BEACON" ) == 0 ) { dest = Sc::Unit::Type::ProtossFlagBeacon; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS FLEET BEACON") == 0 ) { dest = Sc::Unit::Type::ProtossFleetBeacon; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS FORGE"       ) == 0 ) { dest = Sc::Unit::Type::ProtossForge; success = true; }
                    break;
                case 'G':
                    if ( strcmp(&unit[2], "OTOSS GATEWAY") == 0 ) { dest = Sc::Unit::Type::ProtossGateway; success = true; }
                    break;
                case 'H':
                    if ( strcmp(&unit[2], "OTOSS HIGH TEMPLAR") == 0 ) { dest = Sc::Unit::Type::ProtossHighTemplar; success = true; }
                    break;
                case 'I':
                    if ( strcmp(&unit[2], "OTOSS INTERCEPTOR") == 0 ) { dest = Sc::Unit::Type::ProtossInterceptor; success = true; }
                    break;
                case 'M':
                    if ( strcmp(&unit[2], "OTOSS MARKER") == 0 ) { dest = Sc::Unit::Type::ProtossMarker; success = true; }
                    break;
                case 'N':
                    if ( strcmp(&unit[2], "OTOSS NEXUS") == 0 ) { dest = Sc::Unit::Type::ProtossNexus; success = true; }
                    break;
                case 'O':
                    if      ( strcmp(&unit[2], "OTOSS OBSERVATORY") == 0 ) { dest = Sc::Unit::Type::ProtossObservatory; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS OBSERVER"   ) == 0 ) { dest = Sc::Unit::Type::ProtossObserver; success = true; }
                    break;
                case 'P':
                    if      ( strcmp(&unit[2], "OTOSS PHOTON CANNON") == 0 ) { dest = Sc::Unit::Type::ProtossPhotonCannon; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS PROBE"        ) == 0 ) { dest = Sc::Unit::Type::ProtossProbe; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS PYLON"        ) == 0 ) { dest = Sc::Unit::Type::ProtossPylon; success = true; }
                    break;
                case 'R':
                    if      ( strcmp(&unit[2], "OTOSS REAVER"              ) == 0 ) { dest = Sc::Unit::Type::ProtossReaver; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS ROBOTICS FACILITY"   ) == 0 ) { dest = Sc::Unit::Type::ProtossRoboticsFacility; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS ROBOTICS SUPPORT BAY") == 0 ) { dest = Sc::Unit::Type::ProtossRoboticsSupportBay; success = true; }
                    break;
                case 'S':
                    if      ( strcmp(&unit[2], "OTOSS SCARAB"        ) == 0 ) { dest = Sc::Unit::Type::ProtossScarab; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS SCOUT"         ) == 0 ) { dest = Sc::Unit::Type::ProtossScout; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS SHIELD BATTERY") == 0 ) { dest = Sc::Unit::Type::ProtossShieldBattery; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS SHUTTLE"       ) == 0 ) { dest = Sc::Unit::Type::ProtossShuttle; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS STARGATE"      ) == 0 ) { dest = Sc::Unit::Type::ProtossStargate; success = true; }
                    break;
                case 'T':
                    if      ( strcmp(&unit[2], "OTOSS TEMPLAR ARCHIVES") == 0 ) { dest = Sc::Unit::Type::ProtossTemplarArchives; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS TEMPLE"          ) == 0 ) { dest = Sc::Unit::Type::ProtossTemple; success = true; }
                    break;
                case 'V':
                    if      ( strcmp(&unit[2], "OTOSS VESPENE GAS ORB TYPE 1") == 0 ) { dest = Sc::Unit::Type::ProtossVespeneGasOrbType1; success = true; }
                    else if ( strcmp(&unit[2], "OTOSS VESPENE GAS ORB TYPE 2") == 0 ) { dest = Sc::Unit::Type::ProtossVespeneGasOrbType2; success = true; }
                    break;
                case 'Z':
                    if ( strcmp(&unit[2], "OTOSS ZEALOT") == 0 ) { dest = Sc::Unit::Type::ProtossZealot; success = true; }
                    break;
                }
            }
            else if ( unit[1] == 'S' )
            {
                if      ( strcmp(&unit[2], "I DISRUPTER") == 0 ) { dest = Sc::Unit::Type::PsiDistrupter; success = true; }
                else if ( strcmp(&unit[2], "I EMITTER"  ) == 0 ) { dest = Sc::Unit::Type::PsiEmitter; success = true; }
            }
            else if ( strcmp(&unit[1], "OWER GENERATOR") == 0 ) { dest = Sc::Unit::Type::PowerGenerator; success = true; }
            break;
        case 'R':
            if      ( strcmp(&unit[1], "AGNASAUR (ASHWORLD CRITTER)") == 0 ) { dest = Sc::Unit::Type::Ragnasaur_AshworldCritter; success = true; }
            else if ( strcmp(&unit[1], "ASZAGAL (CORSAIR)"          ) == 0 ) { dest = Sc::Unit::Type::Raszagal_Corsair; success = true; }
            else if ( strcmp(&unit[1], "EPAIR BAY (UNUSED)"         ) == 0 ) { dest = Sc::Unit::Type::RepairBay_Unused; success = true; }
            else if ( strcmp(&unit[1], "HYNADON (BADLANDS CRITTER)" ) == 0 ) { dest = Sc::Unit::Type::Rhynadon_BadlandsCritter; success = true; }
            else if ( strcmp(&unit[1], "IGHT PIT DOOR"              ) == 0 ) { dest = Sc::Unit::Type::RightPitDoor; success = true; }
            else if ( strcmp(&unit[1], "IGHT UPPER LEVEL DOOR"      ) == 0 ) { dest = Sc::Unit::Type::RightUpperLevelDoor; success = true; }
            else if ( strcmp(&unit[1], "IGHT WALL FLAME TRAP"       ) == 0 ) { dest = Sc::Unit::Type::RightWallFlameTrap; success = true; }
            else if ( strcmp(&unit[1], "IGHT WALL MISSILE TRAP"     ) == 0 ) { dest = Sc::Unit::Type::RightWallMissileTrap; success = true; }
            else if ( strcmp(&unit[1], "UINS (UNUSED)"              ) == 0 ) { dest = Sc::Unit::Type::Ruins_Unused; success = true; }
            break;
        case 'S':
            if      ( strcmp(&unit[1], "AMIR DURAN (GHOST)"           ) == 0 ) { dest = Sc::Unit::Type::SamirDuran_Ghost; success = true; }
            else if ( strcmp(&unit[1], "ARAH KERRIGAN (GHOST)"        ) == 0 ) { dest = Sc::Unit::Type::SarahKerrigan_Ghost; success = true; }
            else if ( strcmp(&unit[1], "CANNER SWEEP"                 ) == 0 ) { dest = Sc::Unit::Type::ScannerSweep; success = true; }
            else if ( strcmp(&unit[1], "CANTID (DESERT CRITTER)"      ) == 0 ) { dest = Sc::Unit::Type::Scantid_DesertCritter; success = true; }
            else if ( strcmp(&unit[1], "IEGE TANK TURRET (SIEGE MODE)") == 0 ) { dest = Sc::Unit::Type::SiegeTankTurret_SiegeMode; success = true; }
            else if ( strcmp(&unit[1], "IEGE TANK TURRET (TANK MODE)" ) == 0 ) { dest = Sc::Unit::Type::SiegeTankTurret_TankMode; success = true; }
            else if ( strcmp(&unit[1], "PIDER MINE"                   ) == 0 ) { dest = Sc::Unit::Type::SpiderMine; success = true; }
            else if ( strcmp(&unit[1], "TARBASE (UNUSED)"             ) == 0 ) { dest = Sc::Unit::Type::Starbase_Unused; success = true; }
            else if ( strcmp(&unit[1], "TART LOCATION"                ) == 0 ) { dest = Sc::Unit::Type::StartLocation; success = true; }
            else if ( strcmp(&unit[1], "TASIS CELL/PRISON"            ) == 0 ) { dest = Sc::Unit::Type::StasisCellPrison; success = true; }
            break;
        case 'T':
            if ( unit[1] == 'E' )
            {
                switch ( unit[7] )
                {
                case 'A':
                    if      ( strcmp(&unit[2], "RRAN ACADEMY") == 0 ) { dest = Sc::Unit::Type::TerranAcademy; success = true; }
                    else if ( strcmp(&unit[2], "RRAN ARMORY" ) == 0 ) { dest = Sc::Unit::Type::TerranArmory; success = true; }
                    break;
                case 'B':
                    if      ( strcmp(&unit[2], "RRAN BARRACKS"     ) == 0 ) { dest = Sc::Unit::Type::TerranBarracks; success = true; }
                    else if ( strcmp(&unit[2], "RRAN BATTLECRUISER") == 0 ) { dest = Sc::Unit::Type::TerranBattlecruiser; success = true; }
                    else if ( strcmp(&unit[2], "RRAN BEACON"       ) == 0 ) { dest = Sc::Unit::Type::TerranBeacon; success = true; }
                    else if ( strcmp(&unit[2], "RRAN BUNKER"       ) == 0 ) { dest = Sc::Unit::Type::TerranBunker; success = true; }
                    break;
                case 'C':
                    if      ( strcmp(&unit[2], "RRAN CIVILIAN"      ) == 0 ) { dest = Sc::Unit::Type::TerranCivilian; success = true; }
                    else if ( strcmp(&unit[2], "RRAN COMMAND CENTER") == 0 ) { dest = Sc::Unit::Type::TerranCommandCenter; success = true; }
                    else if ( strcmp(&unit[2], "RRAN COMSAT STATION") == 0 ) { dest = Sc::Unit::Type::TerranComsatStation; success = true; }
                    else if ( strcmp(&unit[2], "RRAN CONTROL TOWER" ) == 0 ) { dest = Sc::Unit::Type::TerranControlTower; success = true; }
                    else if ( strcmp(&unit[2], "RRAN COVERT OPS"    ) == 0 ) { dest = Sc::Unit::Type::TerranCovertOps; success = true; }
                    break;
                case 'D':
                    if ( strcmp(&unit[2], "RRAN DROPSHIP") == 0 ) { dest = Sc::Unit::Type::TerranDropship; success = true; }
                    break;
                case 'E':
                    if ( strcmp(&unit[2], "RRAN ENGINEERING BAY") == 0 ) { dest = Sc::Unit::Type::TerranEngineeringBay; success = true; }
                    break;
                case 'F':
                    if      ( strcmp(&unit[2], "RRAN FACTORY"    ) == 0 ) { dest = Sc::Unit::Type::TerranFactory; success = true; }
                    else if ( strcmp(&unit[2], "RRAN FIREBAT"    ) == 0 ) { dest = Sc::Unit::Type::TerranFirebat; success = true; }
                    else if ( strcmp(&unit[2], "RRAN FLAG BEACON") == 0 ) { dest = Sc::Unit::Type::TerranFlagBeacon; success = true; }
                    break;
                case 'G':
                    if      ( strcmp(&unit[2], "RRAN GHOST"  ) == 0 ) { dest = Sc::Unit::Type::TerranGhost; success = true; }
                    else if ( strcmp(&unit[2], "RRAN GOLIATH") == 0 ) { dest = Sc::Unit::Type::TerranGoliath; success = true; }
                    break;
                case 'M':
                    if      ( strcmp(&unit[2], "RRAN MACHINE SHOP"  ) == 0 ) { dest = Sc::Unit::Type::TerranMachineShop; success = true; }
                    else if ( strcmp(&unit[2], "RRAN MARINE"        ) == 0 ) { dest = Sc::Unit::Type::TerranMarine; success = true; }
                    else if ( strcmp(&unit[2], "RRAN MARKER"        ) == 0 ) { dest = Sc::Unit::Type::TerranMarker; success = true; }
                    else if ( strcmp(&unit[2], "RRAN MEDIC"         ) == 0 ) { dest = Sc::Unit::Type::TerranMedic; success = true; }
                    else if ( strcmp(&unit[2], "RRAN MISSILE TURRET") == 0 ) { dest = Sc::Unit::Type::TerranMissileTurret; success = true; }
                    break;
                case 'N':
                    if ( strcmp(&unit[2], "RRAN NUCLEAR SILO") == 0 ) { dest = Sc::Unit::Type::TerranNuclearSilo; success = true; }
                    break;
                case 'P':
                    if ( strcmp(&unit[2], "RRAN PHYSICS LAB") == 0 ) { dest = Sc::Unit::Type::TerranPhysicsLab; success = true; }
                    break;
                case 'R':
                    if ( strcmp(&unit[2], "RRAN REFINERY") == 0 ) { dest = Sc::Unit::Type::TerranRefinery; success = true; }
                    break;
                case 'S':
                    if      ( strcmp(&unit[2], "RRAN SCIENCE FACILITY"       ) == 0 ) { dest = Sc::Unit::Type::TerranScienceFacility; success = true; }
                    else if ( strcmp(&unit[2], "RRAN SCIENCE VESSEL"         ) == 0 ) { dest = Sc::Unit::Type::TerranScienceVessel; success = true; }
                    else if ( strcmp(&unit[2], "RRAN SCV"                    ) == 0 ) { dest = Sc::Unit::Type::TerranScv; success = true; }
                    else if ( strcmp(&unit[2], "RRAN SIEGE TANK (SIEGE MODE)") == 0 ) { dest = Sc::Unit::Type::TerranSiegeTank_SiegeMode; success = true; }
                    else if ( strcmp(&unit[2], "RRAN SIEGE TANK (TANK MODE)" ) == 0 ) { dest = Sc::Unit::Type::TerranSiegeTank_TankMode; success = true; }
                    else if ( strcmp(&unit[2], "RRAN STARPORT"               ) == 0 ) { dest = Sc::Unit::Type::TerranStarport; success = true; }
                    else if ( strcmp(&unit[2], "RRAN SUPPLY DEPOT"           ) == 0 ) { dest = Sc::Unit::Type::TerranSupplyDepot; success = true; }
                    break;
                case 'V':
                    if      ( strcmp(&unit[2], "RRAN VALKYRIE"               ) == 0 ) { dest = Sc::Unit::Type::TerranValkrie; success = true; }
                    else if ( strcmp(&unit[2], "RRAN VESPENE GAS TANK TYPE 1") == 0 ) { dest = Sc::Unit::Type::TerranVespeneGasTankType1; success = true; }
                    else if ( strcmp(&unit[2], "RRAN VESPENE GAS TANK TYPE 2") == 0 ) { dest = Sc::Unit::Type::TerranVespeneGasTankType2; success = true; }
                    else if ( strcmp(&unit[2], "RRAN VULTURE"                ) == 0 ) { dest = Sc::Unit::Type::TerranVulture; success = true; }
                    break;
                case 'W':
                    if ( strcmp(&unit[2], "RRAN WRAITH") == 0 ) { dest = Sc::Unit::Type::TerranWraith; success = true; }
                    break;
                }
            }
            else if ( unit[1] == 'A' )
            {
                if      ( strcmp(&unit[2], "SSADAR (TEMPLAR)"       ) == 0 ) { dest = Sc::Unit::Type::Tassadar_Templar; success = true; }
                else if ( strcmp(&unit[2], "SSADAR/ZERATUL (ARCHON)") == 0 ) { dest = Sc::Unit::Type::TassadarZeratul_Archon; success = true; }
            }
            else if ( unit[1] == 'O' )
            {
                if      ( strcmp(&unit[2], "M KAZANSKY (WRAITH)") == 0 ) { dest = Sc::Unit::Type::TomKazansky_Wraith; success = true; }
                else if ( strcmp(&unit[2], "RRASQUE (ULTRALISK)") == 0 ) { dest = Sc::Unit::Type::Torrasque_Ultralisk; success = true; }
            }
            break;
        case 'U':
            if      ( strcmp(&unit[1], "NCLEAN ONE (DEFILER)"      ) == 0 ) { dest = Sc::Unit::Type::UncleanOne_Defiler; success = true; }
            else if ( strcmp(&unit[1], "NUSED PROTOSS BUILDING 1"  ) == 0 ) { dest = Sc::Unit::Type::UnusedProtossBuilding1; success = true; }
            else if ( strcmp(&unit[1], "NUSED PROTOSS BUILDING 2"  ) == 0 ) { dest = Sc::Unit::Type::UnusedProtossBuilding2; success = true; }
            else if ( strcmp(&unit[1], "NUSED ZERG BUILDING 1"     ) == 0 ) { dest = Sc::Unit::Type::UnusedZergBuilding1; success = true; }
            else if ( strcmp(&unit[1], "NUSED ZERG BUILDING 2"     ) == 0 ) { dest = Sc::Unit::Type::UnusedZergBuilding1; success = true; }
            else if ( strcmp(&unit[1], "RAJ CRYSTAL"               ) == 0 ) { dest = Sc::Unit::Type::UrajCrystal; success = true; }
            else if ( strcmp(&unit[1], "RSADON (ICE WORLD CRITTER)") == 0 ) { dest = Sc::Unit::Type::Ursadon_IceWorldCritter; success = true; }
            break;
        case 'V':
            if      ( strcmp(&unit[1], "ESPENE GEYSER") == 0 ) { dest = Sc::Unit::Type::VespeneGeyser; success = true; }
            break;
        case 'W':
            if      ( strcmp(&unit[1], "ARBRINGER (REAVER)") == 0 ) { dest = Sc::Unit::Type::Warbringer_Reaver; success = true; }
            else if ( strcmp(&unit[1], "ARP GATE"          ) == 0 ) { dest = Sc::Unit::Type::WarpGate; success = true; }
            break;
        case 'X':
            if      ( strcmp(&unit[1], "EL'NAGA TEMPLE") == 0 ) { dest = Sc::Unit::Type::XelNagaTemple; success = true; }
            break;
        case 'Y':
            if      ( strcmp(&unit[1], "GGDRASILL (OVERLORD)") == 0 ) { dest = Sc::Unit::Type::Yggdrasill_Overlord; success = true; }
            else if ( strcmp(&unit[1], "OUNG CHRYSALIS"      ) == 0 ) { dest = Sc::Unit::Type::YoungChrysalis; success = true; }
            break;
        case 'Z':
            if ( unit[3] == 'G' )
            {
                switch ( unit[5] )
                {
                case 'B':
                    if      ( strcmp(&unit[1], "ERG BEACON"   ) == 0 ) { dest = Sc::Unit::Type::ZergBeacon; success = true; }
                    else if ( strcmp(&unit[1], "ERG BROODLING") == 0 ) { dest = Sc::Unit::Type::ZergBroodling; success = true; }
                    break;
                case 'C':
                    if      ( strcmp(&unit[1], "ERG CEREBRATE"        ) == 0 ) { dest = Sc::Unit::Type::ZergCerebrate; success = true; }
                    else if ( strcmp(&unit[1], "ERG CEREBRATE DAGGOTH") == 0 ) { dest = Sc::Unit::Type::ZergCerebrateDaggoth; success = true; }
                    else if ( strcmp(&unit[1], "ERG CREEP COLONY"     ) == 0 ) { dest = Sc::Unit::Type::ZergCreepColony; success = true; }
                    break;
                case 'D':
                    if      ( strcmp(&unit[1], "ERG DEFILER"      ) == 0 ) { dest = Sc::Unit::Type::ZergDefiler; success = true; }
                    else if ( strcmp(&unit[1], "ERG DEFILER MOUND") == 0 ) { dest = Sc::Unit::Type::ZergDefilerMound; success = true; }
                    else if ( strcmp(&unit[1], "ERG DEVOURER"     ) == 0 ) { dest = Sc::Unit::Type::ZergDevourer; success = true; }
                    else if ( strcmp(&unit[1], "ERG DRONE"        ) == 0 ) { dest = Sc::Unit::Type::ZergDrone; success = true; }
                    break;
                case 'E':
                    if      ( strcmp(&unit[1], "ERG EGG"              ) == 0 ) { dest = Sc::Unit::Type::ZergEgg; success = true; }
                    else if ( strcmp(&unit[1], "ERG EVOLUTION CHAMBER") == 0 ) { dest = Sc::Unit::Type::ZergEvolutionChamber; success = true; }
                    else if ( strcmp(&unit[1], "ERG EXTRACTOR"        ) == 0 ) { dest = Sc::Unit::Type::ZergExtractor; success = true; }
                    break;
                case 'F':
                    if ( strcmp(&unit[1], "ERG FLAG BEACON") == 0 ) { dest = Sc::Unit::Type::ZergFlagBeacon; success = true; }
                    break;
                case 'G':
                    if      ( strcmp(&unit[1], "ERG GREATER SPIRE") == 0 ) { dest = Sc::Unit::Type::ZergGreaterSpire; success = true; }
                    else if ( strcmp(&unit[1], "ERG GUARDIAN"     ) == 0 ) { dest = Sc::Unit::Type::ZergGuardian; success = true; }
                    break;
                case 'H':
                    if      ( strcmp(&unit[1], "ERG HATCHERY"     ) == 0 ) { dest = Sc::Unit::Type::ZergHatchery; success = true; }
                    else if ( strcmp(&unit[1], "ERG HIVE"         ) == 0 ) { dest = Sc::Unit::Type::ZergHive; success = true; }
                    else if ( strcmp(&unit[1], "ERG HYDRALISK"    ) == 0 ) { dest = Sc::Unit::Type::ZergHydralisk; success = true; }
                    else if ( strcmp(&unit[1], "ERG HYDRALISK DEN") == 0 ) { dest = Sc::Unit::Type::ZergHydraliskDen; success = true; }
                    break;
                case 'L':
                    if      ( strcmp(&unit[1], "ERG LAIR"  ) == 0 ) { dest = Sc::Unit::Type::ZergLair; success = true; }
                    else if ( strcmp(&unit[1], "ERG LARVA" ) == 0 ) { dest = Sc::Unit::Type::ZergLarva; success = true; }
                    else if ( strcmp(&unit[1], "ERG LURKER") == 0 ) { dest = Sc::Unit::Type::ZergLurker; success = true; }
                    break;
                case 'M':
                    if      ( strcmp(&unit[1], "ERG MARKER"  ) == 0 ) { dest = Sc::Unit::Type::ZergMarker; success = true; }
                    else if ( strcmp(&unit[1], "ERG MUTALISK") == 0 ) { dest = Sc::Unit::Type::ZergMutalisk; success = true; }
                    break;
                case 'N':
                    if ( strcmp(&unit[1], "ERG NYDUS CANAL") == 0 ) { dest = Sc::Unit::Type::ZergNydusCanal; success = true; }
                    break;
                case 'O':
                    if      ( strcmp(&unit[1], "ERG OVERLORD"             ) == 0 ) { dest = Sc::Unit::Type::ZergOverlord; success = true; }
                    else if ( strcmp(&unit[1], "ERG OVERMIND"             ) == 0 ) { dest = Sc::Unit::Type::ZergOvermind; success = true; }
                    else if ( strcmp(&unit[1], "ERG OVERMIND (WITH SHELL)") == 0 ) { dest = Sc::Unit::Type::ZergOvermind_WithShell; success = true; }
                    break;
                case 'Q':
                    if      ( strcmp(&unit[1], "ERG QUEEN"       ) == 0 ) { dest = Sc::Unit::Type::ZergQueen; success = true; }
                    else if ( strcmp(&unit[1], "ERG QUEEN'S NEST") == 0 ) { dest = Sc::Unit::Type::ZergQueensNest; success = true; }
                    break;
                case 'S':
                    if      ( strcmp(&unit[1], "ERG SCOURGE"      ) == 0 ) { dest = Sc::Unit::Type::ZergScourge; success = true; }
                    else if ( strcmp(&unit[1], "ERG SPAWNING POOL") == 0 ) { dest = Sc::Unit::Type::ZergSpawningPool; success = true; }
                    else if ( strcmp(&unit[1], "ERG SPIRE"        ) == 0 ) { dest = Sc::Unit::Type::ZergSpire; success = true; }
                    else if ( strcmp(&unit[1], "ERG SPORE COLONY" ) == 0 ) { dest = Sc::Unit::Type::ZergSporeColony; success = true; }
                    else if ( strcmp(&unit[1], "ERG SUNKEN COLONY") == 0 ) { dest = Sc::Unit::Type::ZergSunkenColony; success = true; }
                    break;
                case 'U':
                    if      ( strcmp(&unit[1], "ERG ULTRALISK"       ) == 0 ) { dest = Sc::Unit::Type::ZergUltralisk; success = true; }
                    else if ( strcmp(&unit[1], "ERG ULTRALISK CAVERN") == 0 ) { dest = Sc::Unit::Type::ZergUltraliskCavern; success = true; }
                    break;
                case 'V':
                    if      ( strcmp(&unit[1], "ERG VESPENE GAS SAC TYPE 1") == 0 ) { dest = Sc::Unit::Type::ZergVespeneGasSacType1; success = true; }
                    else if ( strcmp(&unit[1], "ERG VESPENE GAS SAC TYPE 2") == 0 ) { dest = Sc::Unit::Type::ZergVespeneGasSacType2; success = true; }
                    break;
                case 'Z':
                    if ( strcmp(&unit[1], "ERG ZERGLING") == 0 ) { dest = Sc::Unit::Type::ZergZergling; success = true; }
                    break;
                }
            }
            else if ( strcmp(&unit[1], "ERATUL (DARK TEMPLAR)") == 0 ) { dest = Sc::Unit::Type::Zeratul_DarkTemplar; success = true; }
            break;
        }
    }

    if ( !success ) // Now search the unit name table
    {
        size_t hash = strHash(str);
        size_t numMatching = unitTable.count(hash);
        if ( numMatching == 1 )
        { // Should guarentee that you can find at least one entry
            UnitTableNode & node = unitTable.find(hash)->second;
            if ( node.unitName.compare(str) == 0 )
            {
                dest = node.unitType;
                success = true;
            }
        }
        else if ( numMatching > 1 )
        {
            auto range = unitTable.equal_range(hash);
            foreachin(pair, range)
            {
                UnitTableNode & node = pair->second;
                if ( node.unitName.compare(str) == 0 )
                {
                    if ( success == false ) // If no matches have previously been found
                    {
                        dest = node.unitType;
                        success = true;
                    }
                    else // If matches have previously been found
                    {
                        if ( node.unitType < dest )
                            dest = node.unitType; // Replace if unitID < previous unitID
                    }
                }
            }
        }
    }

    if ( !success && size < 40 ) // Now search legacy names, akas, and shortcut names
    {
        switch ( unit[0] )
        {
        case '[':
            if      ( strcmp(&unit[1], "ANY UNIT]" ) == 0 ) { dest = Sc::Unit::Type::AnyUnit; success = true; }
            else if ( strcmp(&unit[1], "BUILDINGS]") == 0 ) { dest = Sc::Unit::Type::Buildings; success = true; }
            else if ( strcmp(&unit[1], "FACTORIES]") == 0 ) { dest = Sc::Unit::Type::Factories; success = true; }
            else if ( strcmp(&unit[1], "MEN]"      ) == 0 ) { dest = Sc::Unit::Type::Men; success = true; }
            break;
        case 'A':
            if ( strcmp(&unit[1], "LAN TURRET") == 0 ) { dest = Sc::Unit::Type::AlanTurret; success = true; }
            break;
        case 'B':
            if ( strcmp(&unit[1], "ENGALAAS (JUNGLE)") == 0 ) { dest = Sc::Unit::Type::Bengalaas_Jungle; success = true; }
            break;
        case 'C':
            if      ( strcmp(&unit[1], "ANTINA") == 0 ) { dest = Sc::Unit::Type::Cantina; success = true; }
            else if ( strcmp(&unit[1], "AVE"   ) == 0 ) { dest = Sc::Unit::Type::Cave; success = true; }
            else if ( strcmp(&unit[1], "AVE-IN") == 0 ) { dest = Sc::Unit::Type::CaveIn; success = true; }
            else if ( strcmp(&unit[1], "OCOON" ) == 0 ) { dest = Sc::Unit::Type::Cocoon; success = true; }
            break;
        case 'D':
            if      ( strcmp(&unit[1], "ARK TEMPLAR (HERO)") == 0 ) { dest = Sc::Unit::Type::DarkTemplar_Hero; success = true; }
            else if ( strcmp(&unit[1], "ISRUPTION FIELD"   ) == 0 ) { dest = Sc::Unit::Type::DisruptionField; success = true; }
            else if ( strcmp(&unit[1], "UKE TURRET TYPE 1" ) == 0 ) { dest = Sc::Unit::Type::DukeTurretType1; success = true; }
            else if ( strcmp(&unit[1], "UKE TURRET TYPE 2" ) == 0 ) { dest = Sc::Unit::Type::DukeTurretType2; success = true; }
            break;
        case 'E':
            if ( strcmp(&unit[1], "DMUND DUKE (SIEGE TANK)") == 0 ) { dest = Sc::Unit::Type::EdmundDuke_SiegeTank; success = true; }
            break;
        case 'G':
            if ( strcmp(&unit[1], "ERARD DUGALLE (GHOST)") == 0 ) { dest = Sc::Unit::Type::GerardDuGalle_BattleCruiser; success = true; }
            break;
        case 'I':
            if      ( strcmp(&unit[1], "NDEPENDENT COMMAND CENTER"         ) == 0 ) { dest = Sc::Unit::Type::IndependentCommandCenter_Unused; success = true; }
            else if ( strcmp(&unit[1], "NDEPENDENT STARPORT"               ) == 0 ) { dest = Sc::Unit::Type::IndependentStarport_Unused; success = true; }
            else if ( strcmp(&unit[1], "NFESTED DURAN"                     ) == 0 ) { dest = Sc::Unit::Type::InfestedDuran; success = true; }
            else if ( strcmp(&unit[1], "NFESTED KERRIGAN (INFESTED TERRAN)") == 0 ) { dest = Sc::Unit::Type::InfestedKerrigan_InfestedTerran; success = true; }
            else if ( strcmp(&unit[1], "NVALID UNIT"                       ) == 0 ) { dest = Sc::Unit::Type::Id228; success = true; }
            break;
        case 'J':
            if ( strcmp(&unit[1], "UMP GATE") == 0 ) { dest = Sc::Unit::Type::IndependentJumpGate_Unused; success = true; }
            break;
        case 'K':
            if      ( strcmp(&unit[1], "AKARU (TWILIGHT)"         ) == 0 ) { dest = Sc::Unit::Type::Kakaru_TwilightCritter; success = true; }
            else if ( strcmp(&unit[1], "YADARIN CRYSTAL FORMATION") == 0 ) { dest = Sc::Unit::Type::KhadarinCrystalFormation_Unused; success = true; }
            break;
        case 'M':
            if      ( strcmp(&unit[1], "INING PLATFORM"       ) == 0 ) { dest = Sc::Unit::Type::MiningPlatform_Unused; success = true; }
            else if ( strcmp(&unit[1], "INERAL CHUNK (TYPE 1)") == 0 ) { dest = Sc::Unit::Type::MineralClusterType1; success = true; }
            else if ( strcmp(&unit[1], "INERAL CHUNK (TYPE 2)") == 0 ) { dest = Sc::Unit::Type::MineralClusterType2; success = true; }
            break;
        case 'N':
            if ( strcmp(&unit[1], "ORAD II (CRASHED BATTLECRUISER)") == 0 ) { dest = Sc::Unit::Type::NoradII_Crashed; success = true; }
            break;
        case 'P':
            if      ( strcmp(&unit[1], "ROTOSS DARK TEMPLAR" ) == 0 ) { dest = Sc::Unit::Type::ProtossDarkTemplar; success = true; }
            else if ( strcmp(&unit[1], "ROTOSS UNUSED TYPE 1") == 0 ) { dest = Sc::Unit::Type::UnusedProtossBuilding1; success = true; }
            else if ( strcmp(&unit[1], "ROTOSS UNUSED TYPE 2") == 0 ) { dest = Sc::Unit::Type::UnusedProtossBuilding2; success = true; }
            break;
        case 'R':
            if      ( strcmp(&unit[1], "AGNASAUR (ASH WORLD)"  ) == 0 ) { dest = Sc::Unit::Type::Ragnasaur_AshworldCritter; success = true; }
            else if ( strcmp(&unit[1], "UINS"                  ) == 0 ) { dest = Sc::Unit::Type::Ruins_Unused; success = true; }
            else if ( strcmp(&unit[1], "HYNADON (BADLANDS)"    ) == 0 ) { dest = Sc::Unit::Type::Rhynadon_BadlandsCritter; success = true; }
            else if ( strcmp(&unit[1], "ASZAGAL (DARK TEMPLAR)") == 0 ) { dest = Sc::Unit::Type::Raszagal_Corsair; success = true; }
        case 'S':
            if ( strcmp(&unit[1], "CANTID (DESERT)") == 0 ) { dest = Sc::Unit::Type::Scantid_DesertCritter; success = true; }
            break;
        case 'T':
            if      ( strcmp(&unit[1], "ANK TURRET TYPE 1") == 0 ) { dest = Sc::Unit::Type::SiegeTankTurret_TankMode; success = true; }
            else if ( strcmp(&unit[1], "ANK TURRET TYPE 2") == 0 ) { dest = Sc::Unit::Type::SiegeTankTurret_SiegeMode; success = true; }
            break;
        case 'U':
            if      ( strcmp(&unit[1], "NUSED TERRAN BLDG TYPE 1") == 0 ) { dest = Sc::Unit::Type::Starbase_Unused; success = true; }
            else if ( strcmp(&unit[1], "NUSED TERRAN BLDG TYPE 2") == 0 ) { dest = Sc::Unit::Type::RepairBay_Unused; success = true; }
            else if ( strcmp(&unit[1], "NUSED TYPE 1"            ) == 0 ) { dest = Sc::Unit::Type::CargoShip_Unused; success = true; }
            else if ( strcmp(&unit[1], "NUSED TYPE 2"            ) == 0 ) { dest = Sc::Unit::Type::MercenaryGunship_Unused; success = true; }
            else if ( strcmp(&unit[1], "NUSED ZERG BLDG"         ) == 0 ) { dest = Sc::Unit::Type::UnusedZergBuilding1; success = true; }
            else if ( strcmp(&unit[1], "NUSED ZERG BLDG 5"       ) == 0 ) { dest = Sc::Unit::Type::UnusedZergBuilding2; success = true; }
            else if ( strcmp(&unit[1], "RSADON (ICE WORLD)"      ) == 0 ) { dest = Sc::Unit::Type::Ursadon_IceWorldCritter; success = true; }
            break;
        case 'V':
            if      ( strcmp(&unit[1], "ULTURE SPIDER MINE"         ) == 0 ) { dest = Sc::Unit::Type::SpiderMine; success = true; }
            else if ( strcmp(&unit[1], "ESPENE TANK (TERRAN TYPE 1)") == 0 ) { dest = Sc::Unit::Type::TerranVespeneGasTankType1; success = true; }
            else if ( strcmp(&unit[1], "ESPENE TANK (TERRAN TYPE 2)") == 0 ) { dest = Sc::Unit::Type::TerranVespeneGasTankType2; success = true; }
            else if ( strcmp(&unit[1], "ESPENE ORB (PROTOSS TYPE 1)") == 0 ) { dest = Sc::Unit::Type::ProtossVespeneGasOrbType1; success = true; }
            else if ( strcmp(&unit[1], "ESPENE ORB (PROTOSS TYPE 2)") == 0 ) { dest = Sc::Unit::Type::ProtossVespeneGasOrbType2; success = true; }
            else if ( strcmp(&unit[1], "ESPENE SAC (ZERG TYPE 1)"   ) == 0 ) { dest = Sc::Unit::Type::ZergVespeneGasSacType1; success = true; }
            else if ( strcmp(&unit[1], "ESPENE SAC (ZERG TYPE 2)"   ) == 0 ) { dest = Sc::Unit::Type::ZergVespeneGasSacType2; success = true; }
            break;
        case 'Z':
            if ( strcmp(&unit[1], "ERG LURKER EGG") == 0 ) { dest = Sc::Unit::Type::LurkerEgg; success = true; }
            break;
        }
    }
    return success;
}

bool TextTrigCompiler::ParseWavName(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, u32 & dest, size_t pos, size_t end)
{
    if ( compareCaseless(text, 0, 6, "No WAV") )
    {
        dest = 0;
        return true;
    }
    else
        return ParseString(text, stringContents, nextString, dest, pos, end);
}

bool TextTrigCompiler::ParsePlayer(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, u32 & dest, size_t pos, size_t end)
{
    std::string str;
    u32 number = 0;
    if ( text[pos] == '\"' )
    {
        str = stringContents[nextString];
        nextString ++;
    }
    else if ( ParseLong(text.c_str(), dest, pos, end) )
        return true;
    else
        str = text.substr(pos, end-pos);

    size_t size = str.size();
    std::string upperStr;
    for ( u32 i=0; i<size; i++ ) // Copy argument to arg buffer
    {
        char character = str[i];
        if ( character > 96 && character < 123 ) // lower-case
            upperStr.push_back(character-32); // Capitalize
        else if ( character != ' ' && character != '\t' ) // Ignore spaces and tabs
            upperStr.push_back(character);
    }

    char currChar = upperStr[0];
    if ( currChar == 'P' )
    {
        currChar = upperStr[1];
        if ( currChar == 'L' )
        {
            if ( compareCaseless(upperStr, 2, 4, "AYER") )
            {
                const char* argPtr = &upperStr.c_str()[6];
                if ( number = atoi(argPtr) )
                {
                    // Do something with player number
                    if ( number < 13 && number > 0 )
                    {
                        dest = number-1;
                        return true;
                    }
                }
            }
        }
        else if ( currChar > 47 && currChar < 58 ) // Number
        {
            const char* argPtr = &upperStr.c_str()[2];
            if ( number = atoi(argPtr) )
            {
                // Do something with player number
                if ( number < 13 && number > 0 )
                {
                    dest = number-1;
                    return true;
                }
            }
        }
    }
    else if ( currChar == 'F' )
    {
        currChar = upperStr[1];
        if ( currChar == 'O' )
        {
            if ( compareCaseless(upperStr, 2, 3, "RCE") )
            {
                const char* argPtr = &upperStr.c_str()[5];

                if ( number = atoi(argPtr) )
                {
                    // Do something with force number
                    if ( number < 5 )
                    {
                        dest = number+17;
                        return true;
                    }
                }
            }
            else if ( compareCaseless(upperStr, 2, 2, "ES") )
            {
                // Do something with foes
                dest = 14;
                return true;
            }
        }
        else if ( currChar > 47 && currChar < 58 ) // Number
        {
            const char* argPtr = &upperStr.c_str()[2];

            if ( number = atoi(argPtr) )
            {
                // Do something with force number
                if ( number < 5 )
                {
                    dest = number+17;
                    return true;
                }
            }
        }
    }
    else if ( currChar == 'A' )
    {
        currChar = upperStr[1];
        if ( currChar == 'L' )
        {
            if ( compareCaseless(upperStr, 2, 8, "LPLAYERS") )
            {
                // Do something with all players
                dest = 17;
                return true;
            }
            else if ( compareCaseless(upperStr, 2, 4, "LIES") )
            {
                // Do something with allies
                dest = 15;
                return true;
            }
        }
        else if ( currChar == 'P' )
        {
            // Do something with all players
            dest = 17;
            return true;
        }
    }
    else if ( currChar == 'C' )
    {
        if ( compareCaseless(upperStr, 1, 12, "URRENTPLAYER") )
        {
            // Do something with current player
            dest = 13;
            return true;
        }
        else if ( upperStr[1] == 'P' )
        {
            // Do something with current player
            dest = 13;
            return true;
        }
    }
    else if ( currChar == 'I' )
    {
        if ( upperStr.compare(1, 2, "D:") == 0 )
        {
            const char* argPtr = &upperStr.c_str()[3];
            if ( number = atoi(argPtr) )
            {
                // Do something with player number
                dest = number;
                return true;
            }
        }
    }
    else if ( currChar == 'N' )
    {
        if ( upperStr.compare(1, 13, "EUTRALPLAYERS") == 0 )
        {
            // Do something with Neutral Players
            dest = 16;
            return true;
        }
        else if ( upperStr.compare(1, 22, "ONALLIEDVICTORYPLAYERS") == 0 )
        {
            // Do something with non allied victory players
            dest = 26;
            return true;
        }
        else if ( upperStr.compare(1, 3, "ONE") == 0 )
        {
            // Do something with 'none' players (ID:12)
            dest = 12;
            return true;
        }
        else if ( upperStr.compare(1, 11, "ONAVPLAYERS") == 0 )
        {
            // Do something with non av players
            dest = 26;
            return true;
        }
    }
    else if ( currChar == 'U' )
    {
        if ( upperStr.compare(1, 13, "NKNOWN/UNUSED") == 0 )
        {
            // Do something with Unknown/Unused
            dest = 12;
            return true;
        }
        else if ( upperStr.compare(1, 6, "NUSED1") == 0 )
        {
            dest = 22;
            return true;
        }
        else if ( upperStr.compare(1, 6, "NUSED2") == 0 )
        {
            dest = 23;
            return true;
        }
        else if ( upperStr.compare(1, 6, "NUSED3") == 0 )
        {
            dest = 24;
            return true;
        }
        else if ( upperStr.compare(1, 6, "NUSED4") == 0 )
        {
            dest = 25;
            return true;
        }
    }
    else if ( currChar > 47 && currChar < 58 ) // pure number
    {
        const char* argPtr = upperStr.c_str();
        if ( number = atoi(argPtr) )
        {
            dest = number;
            return true;
        }
    }

    // Might be a defined group name
    bool success = false;
    size_t hash = strHash(str);
    size_t numMatching = groupTable.count(hash);
    if ( numMatching == 1 )
    { // Should guarentee that you can find at least one entry
        GroupTableNode & node = groupTable.find(hash)->second;
        if ( node.groupName.compare(str) == 0 )
        {
            dest = node.groupId;
            success = true;
        }
    }
    else if ( numMatching > 1 )
    {
        auto range = groupTable.equal_range(hash);
        foreachin(pair, range)
        {
            GroupTableNode & node = pair->second;
            if ( node.groupName.compare(str) == 0 )
            {
                if ( success == false ) // If no matches have previously been found
                {
                    dest = node.groupId;
                    success = true;
                }
                else // If matches have previously been found
                {
                    if ( u32(node.groupId) < dest )
                        dest = node.groupId; // Replace if groupID < previous groupID
                }
            }
        }
    }
    return success;
}

bool TextTrigCompiler::ParseSwitch(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, u8 & dest, size_t pos, size_t end)
{
    std::string str;
    if ( text[pos] == '\"' ) // If quoted, ignore quotes
    {
        str = stringContents[nextString];
        nextString ++;
    }
    else if ( ParseByte(text.c_str(), dest, pos, end) )
        return true;
    else
        str = text.substr(pos, end-pos);

    size_t size = str.size();
    bool success = false;

    const char* switchNamePtr = &str.c_str()[0];

    if ( size < 12 )
    {
        // Take an upper case copy of the name
        char sw[12] = { };
        size_t numSkipped = 0;
        for ( size_t i=0; i<size; i++ )
        {
            char currChar = switchNamePtr[i];
            if ( currChar > 96 && currChar < 123 )
                sw[i-numSkipped] = currChar - 32;
            else if ( currChar != ' ' )
                sw[i-numSkipped] = currChar;
            else
                numSkipped ++;
        }
        sw[size] = '\0';

        // Check if it's a standard switch name
        if ( sw[0] == 'S' && sw[1] == 'W' && sw[2] == 'I' &&
            sw[3] == 'T' && sw[4] == 'C' && sw[5] == 'H' &&
            ( dest = atoi(&sw[6]) ) )
        {
            dest --; // 0 based
            success = true;
        }
    }

    if ( !success ) // Otherwise search switch name table
    {
        size_t hash = strHash(str);
        size_t numMatching = switchTable.count(hash);
        if ( numMatching == 1 )
        { // Should guarentee that you can find at least one entry
            SwitchTableNode & node = switchTable.find(hash)->second;
            if ( node.switchName.compare(str) == 0 )
            {
                dest = node.switchId;
                success = true;
            }
        }
        else if ( numMatching > 1 )
        {
            auto range = switchTable.equal_range(hash);
            foreachin(pair, range)
            {
                SwitchTableNode & node = pair->second;
                if ( node.switchName.compare(str) == 0 )
                {
                    if ( success == false ) // If no matches have previously been found
                    {
                        dest = node.switchId;
                        success = true;
                    }
                    else // If matches have previously been found
                    {
                        if ( node.switchId < dest )
                            dest = node.switchId; // Replace if switchID < previous switchId
                    }
                }
            }
        }
    }
    return success;
}

bool TextTrigCompiler::ParseSwitch(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, u32 & dest, size_t pos, size_t end)
{
    u8 temp = 0;
    bool success = ParseSwitch(text, stringContents, nextString, temp, pos, end);
    dest = temp;
    return success;
}

bool TextTrigCompiler::ParseScript(std::string & text, std::vector<RawString> & stringContents, size_t & nextString, u32 & dest, size_t pos, size_t end)
{
    if ( compareCaseless(text, pos, 8, "NOSCRIPT") || compareCaseless(text, pos, 9, "No Script") )
    {
        dest = 0;
        return true;
    }

    std::string str;
    bool isQuoted = text[pos] == '\"';
    if ( isQuoted )
    {
        str = stringContents[nextString];
        nextString ++;
    }
    else
        str = text.substr(pos, end-pos);

    bool success = false;
    size_t size = str.size();

    // Grab the string hash
    size_t hash = strHash(str);
    size_t numMatching = scriptTable.count(hash);
    if ( numMatching == 1 )
    { // Should guarentee that you can find at least one entry
        ScriptTableNode & node = scriptTable.find(hash)->second;
        if ( node.scriptName.compare(str) == 0 )
        {
            dest = node.scriptId;
            success = true;
        }
    }
    else if ( numMatching > 1 )
    {
        auto range = scriptTable.equal_range(hash);
        foreachin(pair, range)
        {
            ScriptTableNode & node = pair->second;
            if ( node.scriptName.compare(str) == 0 && success == false ) // Compare equal and no prev matches
            {
                dest = node.scriptId;
                success = true;
                break;
            }
        }
    }

    if ( !success && size == 4 )
    {
        /** With scripts, the exact ascii characters entered can be the exact bytes out.
        As a consequence, if the script name is not quoted and is comprised entirely
        of numbers, it must be considered a script number, and this method should
        return false so ParseByte can be called. */

        bool hasNonNumericCharacter =
            str[0] < '0' || str[0] > '9' ||
            str[1] < '0' || str[1] > '9' ||
            str[2] < '0' || str[2] > '9' ||
            str[3] < '0' || str[3] > '9';

        if ( isQuoted || hasNonNumericCharacter )
        {
            dest = (u32 &)str.c_str()[0];
            success = true;
        }
    }
    return success;
}

Chk::Condition::Type TextTrigCompiler::ExtendedToRegularCID(Chk::Condition::VirtualType conditionType)
{
    switch ( conditionType )
    {
        // Don't include CID_CUSTOM, that is set while parsing args
    case Chk::Condition::VirtualType::Memory:
        return (Chk::Condition::Type)Chk::Condition::ExtendedBaseType::Memory;
        break;
    }
    return (Chk::Condition::Type)conditionType;
}

Chk::Action::Type TextTrigCompiler::ExtendedToRegularAID(Chk::Action::VirtualType actionType)
{
    switch ( actionType )
    {
        // Don't include AID_CUSTOM, that is set while parsing args
    case Chk::Action::VirtualType::SetMemory:
        return (Chk::Action::Type)Chk::Action::ExtendedBaseType::SetMemory;
        break;
    }
    return (Chk::Action::Type)actionType;
}

bool TextTrigCompiler::ParseNumericComparison(const char* text, std::vector<RawString> & stringContents, size_t & nextString, Chk::Condition::Comparison & dest, size_t pos, size_t end)
{
    size_t size = end-pos;
    if ( text[pos] == '\"' ) // Quoted argument, ignore the quotes
    {
        if ( size < 2 )
            return false;
        else
        {
            RawString & rawString = stringContents[nextString];
            text = rawString.c_str();
            nextString ++;
            pos = 0;
            end = rawString.length();
        }
    }

    text = &text[pos];

    if ( size < 1 || size > 8 )
        return false;

    char comparison[12] = { };

    // Take uppercase copy of argument
    size_t numSkipped = 0;
    for ( size_t i=0; i<size; i++ )
    {
        if ( text[i] >= 'a' && text[i] <= 'z' )
            comparison[i-numSkipped] = text[i] - 32;
        else if ( text[i] != ' ' ) // also ignore spacing
            comparison[i-numSkipped] = text[i];
        else
            numSkipped ++;
    }
    comparison[size] = '\0';
    bool success = false;
    switch ( comparison[0] )
    {
        case 'A':
            if      ( strcmp(&comparison[1], "TLEAST") == 0 ) { dest = Chk::Condition::Comparison::AtLeast; success = true; }
            else if ( strcmp(&comparison[1], "TMOST" ) == 0 ) { dest = Chk::Condition::Comparison::AtMost; success = true; }
            break;
        case 'E':
            if ( strcmp(&comparison[1], "XACTLY") == 0 ) { dest = Chk::Condition::Comparison::Exactly; success = true; }
            break;
    }
    return success;
}

bool TextTrigCompiler::ParseSwitchState(const char* text, std::vector<RawString> & stringContents, size_t & nextString, Chk::Condition::Comparison & dest, size_t pos, size_t end)
{
    size_t size = end-pos;
    if ( text[pos] == '\"' ) // Quoted argument, ignore the quotes
    {
        if ( size < 2 )
            return false;
        else
        {
            RawString & rawString = stringContents[nextString];
            text = rawString.c_str();
            nextString ++;
            pos = 0;
            end = rawString.length();
        }
    }

    if ( size < 1 || size > 7 )
        return false;

    char comparison[8] = { };
    const char* srcStr = &text[pos];

    // Take uppercase copy of argument
    size_t numSkipped = 0;
    for ( size_t i=0; i<size; i++ )
    {
        if ( srcStr[i] >= 'a' && srcStr[i] <= 'z' )
            comparison[i-numSkipped] = srcStr[i] - 32;
        else if ( srcStr[i] != ' ' ) // also ignore spacing
            comparison[i-numSkipped] = srcStr[i];
        else
            numSkipped ++;
    }

    comparison[size] = '\0';
    bool success = false;
    switch ( comparison[0] )
    {
        case 'C':
            if ( strcmp(&comparison[1], "LEARED") == 0 ) { dest = Chk::Condition::Comparison::NotSet; success = true; }
            break;
        case 'N':
            if ( strcmp(&comparison[1], "OTSET") == 0 ) { dest = Chk::Condition::Comparison::NotSet; success = true; }
            break;
        case 'S':
            if ( strcmp(&comparison[1], "ET") == 0 ) { dest = Chk::Condition::Comparison::Set; success = true; }
            break;
    }
    return success;
}

bool TextTrigCompiler::ParseSpecialUnitAmount(const char* text, std::vector<RawString> & stringContents, size_t & nextString, u8 & dest, size_t pos, size_t end)
    // All
{
    size_t size = end-pos;
    if ( text[pos] == '\"' ) // Quoted argument, ignore the quotes
    {
        if ( size < 2 )
            return false;
        else
        {
            RawString & rawString = stringContents[nextString];
            text = rawString.c_str();
            nextString ++;
            pos = 0;
            end = rawString.length();
        }
    }

    if ( size < 1 || size > 3 )
        return false;

    char comparison[4] = { };
    const char* srcStr = &text[pos];

    // Take uppercase copy of argument
    size_t numSkipped = 0;
    for ( size_t i=0; i<size; i++ )
    {
        if ( srcStr[i] >= 'a' && srcStr[i] <= 'z' )
            comparison[i-numSkipped] = srcStr[i] - 32;
        else if ( srcStr[i] != ' ' ) // also ignore spacing
            comparison[i-numSkipped] = srcStr[i];
        else
            numSkipped ++;
    }

    comparison[size] = '\0';
    if ( strcmp(comparison, "ALL") == 0 )
    {
        dest = 0;
        return true;
    }
    else
        return false;
}

bool TextTrigCompiler::ParseAllianceStatus(const char* text, std::vector<RawString> & stringContents, size_t & nextString, u16 & dest, size_t pos, size_t end)
    // Ally, Enemy, Allied Victory
{
    size_t size = end-pos;
    if ( text[pos] == '\"' ) // Quoted argument, ignore the quotes
    {
        if ( size < 2 )
            return false;
        else
        {
            RawString & rawString = stringContents[nextString];
            text = rawString.c_str();
            nextString ++;
            pos = 0;
            end = rawString.length();
        }
    }

    if ( size < 1 || size > 14 )
        return false;

    char comparison[16] = { };
    const char* srcStr = &text[pos];

    // Take uppercase copy of argument
    size_t numSkipped = 0;
    for ( size_t i=0; i<size; i++ )
    {
        if ( srcStr[i] >= 'a' && srcStr[i] <= 'z' )
            comparison[i-numSkipped] = srcStr[i] - 32;
        else if ( srcStr[i] != ' ' ) // also ignore spacing
            comparison[i-numSkipped] = srcStr[i];
        else
            numSkipped ++;
    }

    comparison[size] = '\0';
    bool success = false;
    switch ( comparison[0] )
    {
        case 'A':
            if      ( strcmp(&comparison[1], "LLIEDVICTORY") == 0 ) { dest = 2; success = true; }
            else if ( strcmp(&comparison[1], "LLY"         ) == 0 ) { dest = 1; success = true; }
            break;
        case 'E':
            if ( strcmp(&comparison[1], "NEMY") == 0 ) { dest = 0; success = true; }
            break;
    }
    return success;
}

bool TextTrigCompiler::ParseResourceType(const std::string & text, std::vector<RawString> & stringContents, size_t & nextString, u8 & dest, size_t pos, size_t end)
{
    size_t size = end-pos;
    const char* srcStr = &text.c_str()[pos];
    if ( text[pos] == '\"' ) // Quoted argument, ignore the quotes
    {
        if ( size < 2 )
            return false;
        else
        {
            RawString & rawString = stringContents[nextString];
            srcStr = rawString.c_str();
            nextString ++;
            pos = 0;
            end = rawString.length();
        }
    }
    
    if ( size < 1 || size > 11 )
        return false;

    char resource[12] = { };

    // Take uppercase copy of argument
    size_t numSkipped = 0;
    for ( size_t i=0; i<size; i++ )
    {
        if ( srcStr[i] >= 'a' && srcStr[i] <= 'z' )
            resource[i-numSkipped] = srcStr[i] - 32;
        else if ( srcStr[i] != ' ' ) // also ignore spacing
            resource[i-numSkipped] = srcStr[i];
        else
            numSkipped ++;
    }

    resource[size] = '\0';
    bool success = false;
    switch ( resource[0] )
    {
        case 'O':
            if      ( strcmp(&resource[1], "RE"      ) == 0 ) { dest = 0; success = true; }
            else if ( strcmp(&resource[1], "REANDGAS") == 0 ) { dest = 2; success = true; }
            break;
        case 'G':
            if ( strcmp(&resource[1], "AS") == 0 ) { dest = 1; success = true; }
            break;
    }
    return success;
}

bool TextTrigCompiler::ParseScoreType(const std::string & text, std::vector<RawString> & stringContents, size_t & nextString, u8 & dest, size_t pos, size_t end)
{
    size_t size = end-pos;
    const char* srcStr = &text.c_str()[pos];
    if ( text[pos] == '\"' ) // Quoted argument, ignore the quotes
    {
        if ( size < 2 )
            return false;
        else
        {
            RawString & rawString = stringContents[nextString];
            srcStr = rawString.c_str();
            nextString ++;
            pos = 0;
            end = rawString.length();
        }
    }
    
    if ( size < 1 || size > 19 )
        return false;

    char score[20] = { };

    // Take uppercase copy of argument
    size_t numSkipped = 0;
    for ( size_t i=0; i<20; i++ )
    {
        if ( srcStr[i] >= 'a' && srcStr[i] <= 'z' )
            score[i-numSkipped] = srcStr[i] - 32;
        else if ( srcStr[i] != ' ' ) // also ignore spacing
            score[i-numSkipped] = srcStr[i];
        else
            numSkipped ++;
    }

    score[size] = '\0';
    bool success = false;
    switch ( score[0] )
    {
        case 'B':
            if ( strcmp(&score[1], "UILDINGS") == 0 ) { dest = 2; success = true; }
            break;
        case 'C':
            if ( strcmp(&score[1], "USTOM") == 0 ) { dest = 7; success = true; }
            break;
        case 'K':
            if      ( strcmp(&score[1], "ILLS"          ) == 0 ) { dest = 4; success = true; }
            else if ( strcmp(&score[1], "ILLSANDRAZINGS") == 0 ) { dest = 6; success = true; }
            break;
        case 'R':
            if ( strcmp(&score[1], "AZINGS") == 0 ) { dest = 5; success = true; }
            break;
        case 'T':
            if ( strcmp(&score[1], "OTAL") == 0 ) { dest = 0; success = true; }
            break;
        case 'U':
            if      ( strcmp(&score[1], "NITS"            ) == 0 ) { dest = 1; success = true; }
            else if ( strcmp(&score[1], "NITSANDBUILDINGS") == 0 ) { dest = 3; success = true; }
            break;
    }
    return success;
}

bool TextTrigCompiler::ParseTextDisplayFlag(const char* text, std::vector<RawString> & stringContents, size_t & nextString, u8 & dest, size_t pos, size_t end)
    // Always Display, Don't Always Display
{
    size_t size = end-pos;
    if ( text[pos] == '\"' ) // Quoted argument, ignore the quotes
    {
        if ( size < 2 )
            return false;
        else
        {
            RawString & rawString = stringContents[nextString];
            text = rawString.c_str();
            nextString ++;
            pos = 0;
            end = rawString.length();
        }
    }

    if ( size < 1 || size > 20 )
        return false;

    char comparison[24] = { };
    const char* srcStr = &text[pos];

    // Take uppercase copy of argument
    size_t numSkipped = 0;
    for ( size_t i=0; i<size; i++ )
    {
        if ( srcStr[i] >= 'a' && srcStr[i] <= 'z' )
            comparison[i-numSkipped] = srcStr[i] - 32;
        else if ( srcStr[i] != ' ' ) // also ignore spacing
            comparison[i-numSkipped] = srcStr[i];
        else
            numSkipped ++;
    }

    comparison[size] = '\0';
    bool success = false;
    switch ( comparison[0] )
    {
        case 'A':
            if ( strcmp(&comparison[1], "LWAYSDISPLAY") == 0 ) { dest |= Chk::Action::Flags::AlwaysDisplay; success = true; }
            break;
        case 'D':
            if ( strcmp(&comparison[1], "ON'TALWAYSDISPLAY") == 0 ) { dest &= Chk::Action::Flags::xAlwaysDisplay; success = true; }
            break;
    }
    return success;
}

bool TextTrigCompiler::ParseNumericModifier(const char* text, std::vector<RawString> & stringContents, size_t & nextString, u8 & dest, size_t pos, size_t end)
    // Add, subtract, set to
{
    size_t size = end-pos;
    if ( text[pos] == '\"' ) // Quoted argument, ignore the quotes
    {
        if ( size < 2 )
            return false;
        else
        {
            RawString & rawString = stringContents[nextString];
            text = rawString.c_str();
            nextString ++;
            pos = 0;
            end = rawString.length();
        }
    }

    if ( size < 1 || size > 8 )
    {
        return false;
    }

    char comparison[12] = { };
    const char* srcStr = &text[pos];

    // Take uppercase copy of argument
    size_t numSkipped = 0;
    for ( size_t i=0; i<size; i++ )
    {
        if ( srcStr[i] >= 'a' && srcStr[i] <= 'z' )
            comparison[i-numSkipped] = srcStr[i] - 32;
        else if ( srcStr[i] != ' ' ) // also ignore spacing
            comparison[i-numSkipped] = srcStr[i];
        else
            numSkipped ++;
    }

    comparison[size] = '\0';
    bool success = false;
    switch ( comparison[0] )
    {
        case 'A':
            if ( strcmp(&comparison[1], "DD") == 0 ) { dest = 8; success = true; }
            break;
        case 'S':
            if      ( strcmp(&comparison[1], "ETTO"   ) == 0 ) { dest = 7; success = true; }
            else if ( strcmp(&comparison[1], "UBTRACT") == 0 ) { dest = 9; success = true; }
            break;
    }
    return success;
}

bool TextTrigCompiler::ParseSwitchMod(const char* text, std::vector<RawString> & stringContents, size_t & nextString, u8 & dest, size_t pos, size_t end)
    // Set, clear, toggle, randomize
{
    size_t size = end-pos;
    if ( text[pos] == '\"' ) // Quoted argument, ignore the quotes
    {
        if ( size < 2 )
            return false;
        else
        {
            RawString & rawString = stringContents[nextString];
            text = rawString.c_str();
            nextString ++;
            pos = 0;
            end = rawString.length();
        }
    }

    if ( size < 1 || size > 9 )
        return false;

    char comparison[10] = { };
    const char* srcStr = &text[pos];

    // Take uppercase copy of argument
    size_t numSkipped = 0;
    for ( size_t i=0; i<size; i++ )
    {
        if ( srcStr[i] >= 'a' && srcStr[i] <= 'z' )
            comparison[i-numSkipped] = srcStr[i] - 32;
        else if ( srcStr[i] != ' ' ) // also ignore spacing
            comparison[i-numSkipped] = srcStr[i];
        else
            numSkipped ++;
    }

    comparison[size] = '\0';
    bool success = false;
    switch ( comparison[0] )
    {
        case 'C':
            if ( strcmp(&comparison[1], "LEAR") == 0 ) { dest = 5; success = true; }
            break;
        case 'R':
            if ( strcmp(&comparison[1], "ANDOMIZE") == 0 ) { dest = 11; success = true; }
            break;
        case 'S':
            if ( strcmp(&comparison[1], "ET") == 0 ) { dest = 4; success = true; }
            break;
        case 'T':
            if ( strcmp(&comparison[1], "OGGLE") == 0 ) { dest = 6; success = true; }
            break;
    }
    return success;
}

bool TextTrigCompiler::ParseStateMod(const char* text, std::vector<RawString> & stringContents, size_t & nextString, u8 & dest, size_t pos, size_t end)
    // Disable, Disabled, Enable, Enabled, Toggle
{
    size_t size = end-pos;
    if ( text[pos] == '\"' ) // Quoted argument, ignore the quotes
    {
        if ( size < 2 )
            return false;
        else
        {
            RawString & rawString = stringContents[nextString];
            text = rawString.c_str();
            nextString ++;
            pos = 0;
            end = rawString.length();
        }
    }

    if ( size < 1 || size > 8 )
        return false;

    char comparison[12] = { };
    const char* srcStr = &text[pos];

    // Take uppercase copy of argument
    size_t numSkipped = 0;
    for ( size_t i=0; i<size; i++ )
    {
        if ( srcStr[i] >= 'a' && srcStr[i] <= 'z' )
            comparison[i-numSkipped] = srcStr[i] - 32;
        else if ( srcStr[i] != ' ' ) // also ignore spacing
            comparison[i-numSkipped] = srcStr[i];
        else
            numSkipped ++;
    }

    comparison[size] = '\0';
    bool success = false;
    switch ( comparison[0] )
    {
        case 'D':
            if      ( strcmp(&comparison[1], "ISABLE" ) == 0 ) { dest = 5; success = true; }
            else if ( strcmp(&comparison[1], "ISABLED") == 0 ) { dest = 5; success = true; }
            break;
        case 'E':
            if      ( strcmp(&comparison[1], "NABLE" ) == 0 ) { dest = 4; success = true; }
            else if ( strcmp(&comparison[1], "NABLED") == 0 ) { dest = 4; success = true; }
            break;
        case 'T':
            if ( strcmp(&comparison[1], "OGGLE") == 0 ) { dest = 6; success = true; }
            break;
    }
    return success;
}

bool TextTrigCompiler::ParseOrder(const char* text, std::vector<RawString> & stringContents, size_t & nextString, u8 & dest, size_t pos, size_t end)
    // Attack, move, patrol
{
    size_t size = end-pos;
    if ( text[pos] == '\"' ) // Quoted argument, ignore the quotes
    {
        if ( size < 2 )
            return false;
        else
        {
            RawString & rawString = stringContents[nextString];
            text = rawString.c_str();
            nextString ++;
            pos = 0;
            end = rawString.length();
        }
    }

    if ( size < 1 || size > 6 )
        return false;

    char comparison[8] = { };
    const char* srcStr = &text[pos];

    // Take uppercase copy of argument
    size_t numSkipped = 0;
    for ( size_t i=0; i<size; i++ )
    {
        if ( srcStr[i] >= 'a' && srcStr[i] <= 'z' )
            comparison[i-numSkipped] = srcStr[i] - 32;
        else if ( srcStr[i] != ' ' ) // also ignore spacing
            comparison[i-numSkipped] = srcStr[i];
        else
            numSkipped ++;
    }

    comparison[size] = '\0';
    bool success = false;
    switch ( comparison[0] )
    {
        case 'A':
            if ( strcmp(&comparison[1], "TTACK") == 0 ) { dest = 2; success = true; }
            break;
        case 'M':
            if ( strcmp(&comparison[1], "OVE") == 0 ) { dest = 0; success = true; }
            break;
        case 'P':
            if ( strcmp(&comparison[1], "ATROL") == 0 ) { dest = 1; success = true; }
            break;
    }
    return success;
}

bool TextTrigCompiler::ParseMemoryAddress(const char* text, u32 & dest, size_t pos, size_t end, u32 deathTableOffset)
{
    u32 temp = 0;
    if ( ParseLong(text, temp, pos, end) )
    {
        dest = (temp/4*4-deathTableOffset)/4;
        return true;
    }
    return false;
}

bool TextTrigCompiler::ParseResourceType(const std::string & text, std::vector<RawString> & stringContents, size_t & nextString, u16 & dest, size_t pos, size_t end)
{
    u8 temp = 0;
    bool success = ParseResourceType(text, stringContents, nextString, temp, pos, end);
    dest = temp;
    return success;
}

bool TextTrigCompiler::ParseScoreType(const std::string & text, std::vector<RawString> & stringContents, size_t & nextString, u16 & dest, size_t pos, size_t end)
{
    u8 temp = 0;
    bool success = ParseScoreType(text, stringContents, nextString, temp, pos, end);
    dest = temp;
    return success;
}

bool TextTrigCompiler::ParseBinaryLong(const char* text, u32 & dest, size_t pos, size_t end)
{
    size_t size = end - pos;
    if ( size < 33 )
    {
        if ( size == 0 )
        {
            dest = 0;
            return true;
        }
        else
        {
            bool allZero = true;
            for ( size_t i=0; i<size; i++ )
            {
                if ( text[pos+i] != '0' )
                    allZero = false;
            }

            if ( allZero )
            {
                dest = 0;
                return true;
            }
            else
            {
                char potentialLong[36] = { };
                std::memcpy(potentialLong, &text[pos], (size_t)size);
                potentialLong[size] = '\0';
                dest = (u32)strtoll(potentialLong, nullptr, 2);
                return dest > 0;
            }
        }
    }
    return false;
}

bool TextTrigCompiler::ParseLong(const char* text, u32 & dest, size_t pos, size_t end)
{
    size_t size = end - pos;
    if ( size < 12 )
    {
        if ( size == 1 && text[pos] == '0' )
        {
            dest = 0;
            return true;
        }
        else if ( size > 2 && text[pos] == '0' && (text[pos + 1] == 'x' || text[pos + 1] == 'X') )
        {
            char potentialLong[12] = {};
            std::memcpy(potentialLong, &text[pos + 2], size_t(size - 2));
            potentialLong[size - 2] = '\0';
            try {
                dest = (u32)std::stoll(potentialLong, nullptr, 16);
                return true;
            }
            catch (std::exception e) {}
        }
        else
        {
            char potentialLong[12] = { };
            std::memcpy(potentialLong, &text[pos], (size_t)size);
            potentialLong[size] = '\0';
            return ( (dest = (u32)std::atoll(potentialLong)) > 0 );
        }
    }
    return false;
}

bool TextTrigCompiler::ParseTriplet(const char* text, u8* dest, size_t pos, size_t end)
{
    u32 val;
    bool isNegative = false;

    if ( text[pos] == '-' ) // isNegative
    {
        pos ++;
        isNegative = true;
    }

    if ( ParseLong(text, val, pos, end) )
    {
        if ( isNegative )
            val = 0x1000000 - val;
        
        dest[2] = u8(val/0x10000);
        val /= 0x10000;
        dest[1] = u8(val/0x100);
        val /= 0x100;
        dest[0] = u8(val);
        return true;
    }
    return false;
}

bool TextTrigCompiler::ParseShort(const char* text, u16 & dest, size_t pos, size_t end)
{
    size_t size = end - pos;
    if ( size < 7 )
    {
        if ( size == 1 && text[pos] == '0' )
        {
            dest = 0;
            return true;
        }
        else if ( size > 2 && text[pos] == '0' && (text[pos + 1] == 'x' || text[pos + 1] == 'X') )
        {
            char potentialShort[7] = {};
            std::memcpy(potentialShort, &text[pos + 2], size_t(size - 2));
            potentialShort[size - 2] = '\0';
            try {
                dest = (u32)std::stol(potentialShort, nullptr, 16);
                return true;
            }
            catch ( std::exception e ) { return false; }
        }
        else
        {
            char potentialShort[7] = {};
            std::memcpy(potentialShort, &text[pos], (size_t)size);
            potentialShort[size] = '\0';
            return ((dest = std::atoi(potentialShort)) > 0);
        }
    }
    return false;
}

bool TextTrigCompiler::ParseByte(const char* text, u8 & dest, size_t pos, size_t end)
{
    size_t size = end - pos;
    if ( size < 5 )
    {
        if ( size == 1 && text[pos] == '0' )
        {
            dest = 0;
            return true;
        }
        else if ( size > 2 && text[pos] == '0' && (text[pos + 1] == 'x' || text[pos + 1] == 'X') )
        {
            char potentialByte[5] = {};
            std::memcpy(potentialByte, &text[pos + 2], size_t(size - 2));
            potentialByte[size - 2] = '\0';
            try {
                dest = (u32)std::stol(potentialByte, nullptr, 16);
                return true;
            }
            catch ( std::exception e ) { return false; }
        }
        else
        {
            char potentialByte[5] = {};
            std::memcpy(potentialByte, &text[pos], (size_t)size);
            potentialByte[size] = '\0';
            return ((dest = std::atoi(potentialByte)) > 0);
        }
    }
    return false;
}

// private

/*bool TextTrigCompiler::useNextString(u32 & index)
{
    for ( size_t i=1; i>0 && i<Chk::MaxStrings; i++ )
    {
        if ( !stringUsed[i] )
        {
            index = (u32)i;
            return true;
        }
    }
    return false;
}

bool TextTrigCompiler::useNextExtendedString(u32 & index)
{
    for ( size_t i=1; i>0 && i<Chk::MaxStrings; i++ )
    {
        if ( !extendedStringUsed[i] )
        {
            index = (u32)i;
            return true;
        }
    }
    return false;
}*/

bool TextTrigCompiler::PrepLocationTable(ScenarioPtr map)
{
    LocationTableNode locNode = {};
    locationTable.reserve(map->layers.numLocations()+1);
    locNode.locationId = 0;
    locNode.locationName = "No Location";
    locationTable.insert(std::pair<size_t, LocationTableNode>(strHash(locNode.locationName), locNode));
    for ( u32 i=1; i<=map->layers.numLocations(); i++ )
    {
        Chk::LocationPtr loc = map->layers.getLocation(i);
        locNode.locationName = "";

        if ( i == Chk::LocationId::Anywhere )
        {
            locNode.locationId = Chk::LocationId::Anywhere;
            locNode.locationName = "Anywhere";
            locationTable.insert( std::pair<size_t, LocationTableNode>(strHash(locNode.locationName), locNode) );
        }
        else if ( loc->stringId > 0 )
        {
            RawStringPtr locationName = map->strings.getString<RawString>(loc->stringId);
            if ( locationName != nullptr )
            {
                locNode.locationId = u8(i);
                locNode.locationName = *locationName;
                locationTable.insert( std::pair<size_t, LocationTableNode>(strHash(locNode.locationName), locNode) );
            }
        }
    }
    locationTable.reserve(locationTable.size());
    return true;
}

bool TextTrigCompiler::PrepUnitTable(ScenarioPtr map)
{
    UnitTableNode unitNode = {};
    u16 stringId = 0;
    for ( u16 unitId=0; unitId<Sc::Unit::TotalTypes; unitId++ )
    {
        unitNode.unitType = (Sc::Unit::Type)unitId;
        RawStringPtr unitName = map->strings.getUnitName<RawString>((Sc::Unit::Type)unitId, true);
        if ( unitName != nullptr )
            unitNode.unitName = *unitName;
        else
            unitNode.unitName = Sc::Unit::defaultDisplayNames[unitId];
        
        unitTable.insert( std::pair<size_t, UnitTableNode>(strHash(unitNode.unitName), unitNode) );
    }
    return true;
}

bool TextTrigCompiler::PrepSwitchTable(ScenarioPtr map)
{
    SwitchTableNode switchNode = {};
    size_t stringId = 0;
    for ( size_t switchIndex=0; switchIndex<256; switchIndex++ )
    {
        size_t switchNameStringId = map->triggers.getSwitchNameStringId(switchIndex);
        if ( switchNameStringId != Chk::StringId::NoString )
        {
            RawStringPtr switchName = map->strings.getString<RawString>(switchNameStringId);
            if ( switchName != nullptr )
            {
                switchNode.switchId = u8(switchIndex);
                switchNode.switchName = *switchName;
                switchTable.insert( std::pair<size_t, SwitchTableNode>(strHash(switchNode.switchName), switchNode) );
            }
        }
    }
    return true;
}

bool TextTrigCompiler::PrepGroupTable(ScenarioPtr map)
{
    GroupTableNode groupNode = {};
    for ( u32 i=0; i<4; i++ )
    {
        RawStringPtr forceName = map->strings.getForceName<RawString>((Chk::Force)i);
        if ( forceName != nullptr )
        {
            groupNode.groupId = i + 18;
            groupNode.groupName = *forceName;
            groupTable.insert(std::pair<size_t, GroupTableNode>(strHash(groupNode.groupName), groupNode));
        }
    }
    return true;
}

bool TextTrigCompiler::PrepStringTable(ScenarioPtr map, std::unordered_multimap<size_t, StringTableNodePtr> & stringHashTable, size_t trigIndexBegin, size_t trigIndexEnd, const Chk::Scope & scope)
{
    std::bitset<Chk::MaxStrings> stringUsed; // Table of strings currently used in the map
    u32 userMask = scope == Chk::Scope::Game ? Chk::StringUserFlag::xTrigger : Chk::StringUserFlag::All;
    map->strings.markValidUsedStrings(stringUsed, Chk::Scope::Either, scope, userMask);
    size_t stringCapacity = map->strings.getCapacity(scope);
    for ( size_t stringId=1; stringId<=stringCapacity; stringId++ )
    {
        if ( stringUsed[stringId] )
        {
            RawStringPtr rawString = map->strings.getString<RawString>(stringId, scope);
            if ( rawString != nullptr )
            {
                StringTableNodePtr node = StringTableNodePtr(new StringTableNode({}));
                node->unused = false;
                node->scStr = ScStrPtr(new ScStr(*rawString));
                node->stringId = (u32)stringId;
                stringHashTable.insert(std::pair<size_t, StringTableNodePtr>(strHash(*rawString), node));
            }
        }
    }

    if ( scope == Chk::Scope::Game )
    {
        auto & triggers = map->triggers;
        size_t numTriggers = triggers.numTriggers();
        for ( size_t trigIndex = 0; trigIndex < numTriggers; trigIndex++ )
        {
            bool inReplacedRange = trigIndex >= trigIndexBegin && trigIndex < trigIndexEnd;
            const Chk::TriggerPtr trigger = triggers.getTrigger(trigIndex);
            for ( size_t actionIndex = 0; actionIndex < Chk::Trigger::MaxActions; actionIndex++ )
            {
                const Chk::Action & action = trigger->actions[actionIndex];
                const Chk::Action::Type & actionType = action.actionType;
                if ( actionType < Chk::Action::NumActionTypes )
                {
                    if ( Chk::Action::actionUsesStringArg[actionType] && action.stringId > 0 )
                        PrepTriggerString(*map, stringHashTable, action.stringId, inReplacedRange, Chk::Scope::Game);

                    if ( Chk::Action::actionUsesSoundArg[actionType] && action.soundStringId > 0 )
                        PrepTriggerString(*map, stringHashTable, action.soundStringId, inReplacedRange, Chk::Scope::Game);
                }
            }
        }
    }

    return true;
}

void TextTrigCompiler::PrepTriggerString(Scenario & scenario, std::unordered_multimap<size_t, StringTableNodePtr> & stringHashTable, const u32 & stringId, const bool & inReplacedRange, const Chk::Scope & scope)
{
    RawStringPtr rawString = scenario.strings.getString<RawString>(stringId, scope);
    if ( rawString != nullptr )
    {
        size_t hash = strHash(*rawString);
        bool exists = false;
        auto matches = stringHashTable.equal_range(hash);
        if ( matches.first != stringHashTable.end() && matches.first->first == hash )
        {
            for ( auto it = matches.first; it != matches.second; ++it )
            {
                if ( it->second->stringId == stringId && it->second->scStr->compare(*rawString) == 0 )
                {
                    exists = true;
                    if ( it->second->unused && !inReplacedRange )
                        it->second->unused = false;

                    break;
                }
            }
        }

        if ( !exists )
        {
            StringTableNodePtr node = StringTableNodePtr(new StringTableNode({}));
            node->unused = inReplacedRange;
            node->scStr = ScStrPtr(new ScStr(*rawString));
            node->stringId = stringId;
            stringHashTable.insert(std::pair<size_t, StringTableNodePtr>(strHash(*rawString), node));
        }
    }
}

bool TextTrigCompiler::PrepScriptTable(Sc::Data & scData)
{
    std::string aiName;
    size_t numScripts = scData.ai.numEntries();
    for ( size_t i = 0; i < numScripts; i++ )
    {
        ScriptTableNode scriptNode = {};
        const Sc::Ai::Entry & entry = scData.ai.getEntry(i);
        if ( scData.ai.getName(i, aiName) )
            scriptTable.insert(std::pair<size_t, ScriptTableNode>(strHash(scriptNode.scriptName), scriptNode));
    }
    return true;
}

bool TextTrigCompiler::BuildNewMap(ScenarioPtr scenario, size_t trigIndexBegin, size_t trigIndexEnd, std::deque<Chk::TriggerPtr> triggers, std::stringstream & error)
{
    auto strBackup = scenario->strings.backup();
    std::deque<Chk::TriggerPtr> replacedTriggers = scenario->triggers.replaceRange(trigIndexBegin, trigIndexEnd, triggers);
    bool success = true;
    try {
        scenario->strings.deleteUnusedStrings(Chk::Scope::Both);
        for ( auto str : unassignedStrings )
        {
            str->stringId = (u32)scenario->strings.addString<RawString>(str->scStr->str, Chk::Scope::Game);
            if ( str->stringId != Chk::StringId::NoString )
            {
                for ( auto assignee : str->assignees )
                    *assignee = str->stringId;
            }
            else
            {
                success = false;
                break;
            }
        }
    } catch ( std::exception & e ) {
        error << e.what() << std::endl;
        success = false;
    }

    if ( !success )
    {
        size_t unreplaceEndIndex = trigIndexBegin + replacedTriggers.size();
        auto unused = scenario->triggers.replaceRange(trigIndexBegin, unreplaceEndIndex, replacedTriggers);
        scenario->strings.restore(strBackup);
    }
    return success;
}

size_t findStringEnd(const std::string & str, size_t pos)
{
    size_t strSize = str.size();
    while ( pos < strSize )
    {
        size_t nextQuote = str.find('\"', pos);
        if ( nextQuote == std::string::npos )
            return std::string::npos;
        else if ( nextQuote > 0 && str[nextQuote-1] == '\\' ) // Escaped quote
            pos = nextQuote+1;
        else  // Terminating quote
            return nextQuote;
    }
    return std::string::npos;
}

size_t findNextUnquoted(const std::string & str, size_t pos, char character)
{
    const char * cStr = str.c_str();
    size_t strSize = str.size();
    for ( ; pos<strSize; pos++ )
    {
        char curr = cStr[pos];
        if ( curr == '\"' )
        {
            pos = findStringEnd(str, pos+1);
            if ( pos == std::string::npos )
                return std::string::npos;
        }
        else if ( curr == character )
            return pos;
    }
    return std::string::npos;
}

size_t findNextUnquoted(const std::string & str, size_t pos, char character, char terminator)
{
    const char * cStr = str.c_str();
    size_t strSize = str.size();
    for ( ; pos<strSize; pos++ )
    {
        char curr = cStr[pos];
        if ( curr == '\"' )
        {
            pos = findStringEnd(str, pos+1);
            if ( pos == std::string::npos )
                return std::string::npos;
        }
        else if ( curr == terminator && (pos == 0 || cStr[pos-1] != '\\') )
            return std::string::npos;
        else if ( curr == character )
            return pos;
    }
    return std::string::npos;
}

bool compareCaseless(const std::string & str, size_t pos, size_t count, const char* other)
{
    size_t limit = pos+count;
    if ( limit <= str.size() )
    {
        const char* cStr = &str.c_str()[pos];
        for ( size_t i=0; i<count; i++ )
        {
            if ( cStr[i] != other[i] )
                return false;
        }
        return true;
    }
    return false;
}
