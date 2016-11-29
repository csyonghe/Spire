#ifndef RASTER_RENDERER_COMPILE_ERROR_H
#define RASTER_RENDERER_COMPILE_ERROR_H

#include "../CoreLib/Basic.h"
#include "../CoreLib/Tokenizer.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;
		using namespace CoreLib::Text;

        enum class Severity
        {
            Note,
            Warning,
            Error,
            Fatal,
            Internal,
        };

        // TODO(tfoley): move this into a source file...
        inline const char* getSeverityName(Severity severity)
        {
            switch (severity)
            {
            case Severity::Note:        return "note";
            case Severity::Warning:     return "warning";
            case Severity::Error:       return "error";
            case Severity::Fatal:       return "fatal error";
            case Severity::Internal:    return "internal error";
            default:                    return "unknown error";
            }
        }

		class Diagnostic
		{
		public:
			String Message;
			CodePosition Position;
			int ErrorID;
            Severity severity;

			Diagnostic()
			{
				ErrorID = -1;
			}
			Diagnostic(
                const String & msg,
                int id,
				const CodePosition & pos,
                Severity severity)
                : severity(severity)
			{
				Message = msg;
				ErrorID = id;
				Position = pos;
			}
		};

        class DiagnosticSink
        {
        public:
            List<Diagnostic> diagnostics;
            int errorCount = 0;

			void Error(int id, const String & msg, const CodePosition & pos)
			{
				diagnostics.Add(Diagnostic(msg, id, pos, Severity::Error));
                errorCount++;
			}

			void Warning(int id, const String & msg, const CodePosition & pos)
			{
				diagnostics.Add(Diagnostic(msg, id, pos, Severity::Warning));
			}

            int GetErrorCount() { return errorCount; }

            // A stored state for the sink, which can be used to restore it later.
            struct State
            {
                int diagnosticCount;
                int errorCount;
            };

            // Save the state of the sink so that it can be restored later.
            State saveState()
            {
                State state;
                state.diagnosticCount = diagnostics.Count();
                state.errorCount = errorCount;
                return state;
            }

            // Restore the state of the sink to a saved state.
            void restoreState(State state)
            {
                errorCount = state.errorCount;
                diagnostics.SetSize(state.diagnosticCount);
            }
        };
	}
}

#endif