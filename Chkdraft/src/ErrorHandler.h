#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H
#include "GenericCommand.h"
#include "Logger.h"
#include <exception>
#include <atomic>
#include <map>

enum class ErrorAction {
    DiscardCommand, // Rolls-back and discards the command
    RetryCommand, // Rolls-back and restarts the entire command 
    RetrySubCommand, // Attempts to execute the sub-command again and continue
    ContinueCommand // Ignores the failed sub-command and continues with the remaining sub-commands
};

class ErrorHandlerResult
{
    public:
        ErrorAction primaryAction;
        LogLevel logLevel;
        std::string logString;

        ErrorHandlerResult(ErrorAction primaryAction, LogLevel logLevel, std::string logString)
            : primaryAction(primaryAction), logLevel(logLevel), logString(logString) {};
};

/**
    Known errors are errors that commands are aware could happen (e.g. file already in use)
*/
class KnownError : public std::exception
{
    public:
        KnownError(u32 errorId);
        KnownError(const KnownError& other);
        u32 getErrorId();
        static std::atomic<u32> GetNextErrorId();

    private:
        u32 errorId;
        static std::atomic<u32> nextErrorId;
        KnownError(); // Disallow ctor
};

/**
    Determines the appropriate action after an error has occured,
    error handlers may seek input from the user as necessary
*/
class ErrorHandler
{
    public:
        virtual ErrorHandlerResult HandleException(GenericCommand* command, KnownError& e) = 0;

        static std::map<u32, std::shared_ptr<ErrorHandler>> knownErrorHandlers;
        static ErrorHandlerResult HandleError(GenericCommand* command, KnownError& e);
};

#endif