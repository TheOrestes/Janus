#include "Core/Log.h"
#include "Core/FileSystem.h"

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace JN
{
    spdlog::logger* G_pEngineLogger = nullptr;
    spdlog::logger* G_pAppLogger    = nullptr;
    spdlog::logger* G_pD3D12Logger  = nullptr;

    // Custom pattern flag %k: the level as a bracketed 4-letter tag, e.g. [INFO]. Coloured
    // text on the console, plain text in the file, so both read the same.
    class LevelFlag final : public spdlog::custom_flag_formatter
    {
    public:
        explicit LevelFlag(bool bColor) : m_bColor(bColor) {}

        void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override
        {
            static const char* const names[]  = { "TRCE", "DBUG", "INFO", "WARN", "ERRO", "CRIT", "OFF " };
            static const int         colors[] = { 244, 244, 41, 220, 203, 196, 0 }; // 256-colour foreground
            const int i = static_cast<int>(msg.level);
            const std::string tag = std::string("[") + names[i] + "]";
            const std::string text = m_bColor
                ? fmt::format("\x1b[{}38;5;{}m{}\x1b[0m", msg.level == spdlog::level::critical ? "1;" : "", colors[i], tag)
                : tag;
            dest.append(text.data(), text.data() + text.size());
        }

        std::unique_ptr<custom_flag_formatter> clone() const override
        {
            return std::make_unique<LevelFlag>(m_bColor);
        }

    private:
        bool m_bColor;
    };

    // Custom pattern flag %*: the logger name padded to a fixed width, e.g. JANUS / APP.
    // On the console it is a filled badge in the source's own colour.
    class SourceFlag final : public spdlog::custom_flag_formatter
    {
    public:
        explicit SourceFlag(bool bColor) : m_bColor(bColor) {}

        void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override
        {
            static const struct { const char* pName; int color; } sources[] =
            {
                { "JANUS", 45 }, { "APP", 213 }, { "D3D12", 208 } // 256-colour background
            };

            const std::string name(msg.logger_name.data(), msg.logger_name.size());
            int color = 7;
            for (const auto& source : sources)
            {
                if (name == source.pName)
                {
                    color = source.color;
                }
            }

            std::string text = name + std::string(name.size() < 5 ? 5 - name.size() : 0, ' ');
            if (m_bColor)
            {
                text = fmt::format("\x1b[48;5;{}m\x1b[30m {} \x1b[0m", color, text);
            }
            dest.append(text.data(), text.data() + text.size());
        }

        std::unique_ptr<custom_flag_formatter> clone() const override
        {
            return std::make_unique<SourceFlag>(m_bColor);
        }

    private:
        bool m_bColor;
    };

    //-----------------------------------------------------------------------------------------------------------------
    static std::unique_ptr<spdlog::formatter> MakeFormatter(const char* pPattern, bool bColor)
    {
        auto pFormatter = std::make_unique<spdlog::pattern_formatter>();
        pFormatter->add_flag<LevelFlag>('k', bColor).add_flag<SourceFlag>('*', bColor).set_pattern(pPattern);
        return pFormatter;
    }

    //-----------------------------------------------------------------------------------------------------------------
    static void EnableConsoleColors()
    {
        const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode))
        {
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING); // lets the console read ANSI colour codes
        }
    }

    //-----------------------------------------------------------------------------------------------------------------
    static spdlog::logger* MakeLogger(const char* pName, const std::vector<spdlog::sink_ptr>& sinks)
    {
        auto pLogger = std::make_shared<spdlog::logger>(pName, sinks.begin(), sinks.end());
        pLogger->set_level(spdlog::level::trace);
        pLogger->flush_on(spdlog::level::warn);
        spdlog::register_logger(pLogger); // the registry keeps it alive, so the raw pointer stays valid
        return pLogger.get();
    }

    //-----------------------------------------------------------------------------------------------------------------
    void LogInit()
    {
        EnableConsoleColors();

        const auto logPath = (GetExecutableDirectory() / "janus.log").string();
        const auto pConsole = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        const auto pFile    = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath, true); // true: truncate each run
        const std::vector<spdlog::sink_ptr> sinks = { pConsole, pFile };

        G_pEngineLogger = MakeLogger("JANUS", sinks);
        G_pAppLogger    = MakeLogger("APP",   sinks);
        G_pD3D12Logger  = MakeLogger("D3D12", sinks);

        // Set after the loggers exist: registering a logger resets its sinks' formatter.
        // Each sink gets its own formatter (console coloured, file plain) and the loggers
        // share them.
        pConsole->set_formatter(MakeFormatter("\x1b[90m%T\x1b[0m %* %k %v", true));
        pFile->set_formatter(MakeFormatter("%T %* %k %v", false));
    }
}
