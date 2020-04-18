#include "pch.h"
#pragma warning(push)
#pragma warning(disable : 5040) // dynamic exception specifications are valid only in C++14 and earlier...
#include <crashrpt/CrashRpt.h>
#pragma  warning(pop)
#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <crtdbg.h>

namespace
{
    /// Tests that CrashRpt catches emulated crashes and exits the process with exit code 1.
    class CrashRptDeathTest : public ::testing::TestWithParam<int>
    {
    protected:

        const std::filesystem::path ErrorReportSaveDir = std::filesystem::temp_directory_path() / L"CrashRptDeathTest";

        // Prevent CRT asserts from causing pop-up windows
        const int PrevCrtAssertReportMode = _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

        CrashRptDeathTest()
        {
            std::filesystem::create_directories(ErrorReportSaveDir);
        }

        ~CrashRptDeathTest()
        {
            std::filesystem::remove_all(ErrorReportSaveDir);
            _CrtSetReportMode(_CRT_ASSERT, PrevCrtAssertReportMode);
        }

        void installAndEmulate(unsigned exceptionType)
        {
            installCrashRpt();
            crEmulateCrash(exceptionType);
        }

        void installAndEmulateOnOtherThread(unsigned exceptionType)
        {
            installCrashRpt();

            auto threadFunc = [exceptionType]
            {
                // Install exception handlers for this thread
                CrThreadAutoInstallHelper threadAutoInstall(0);
                EXPECT_EQ(0, threadAutoInstall.m_nInstallStatus);

                crEmulateCrash(exceptionType);
            };

            std::thread t(threadFunc);
            t.join();
        }

        void installCrashRpt()
        {
            CR_INSTALL_INFO info{};
            info.cb = sizeof(info);
            info.pszAppName = L"CrashRptTest";
            info.pszAppVersion = L"1.0.0.0";
            info.dwFlags |= CR_INST_DONT_SEND_REPORT
                | CR_INST_ALL_POSSIBLE_HANDLERS
                | CR_INST_NO_GUI;
            info.pszErrorReportSaveDir = ErrorReportSaveDir.c_str();
            info.uMiniDumpType = MiniDumpNormal;
            EXPECT_EQ(0, crInstall(&info));
        }
    };

    std::string getExceptionName(int i)
    {
        switch (i)
        {
            case CR_SEH_EXCEPTION: return "CR_SEH_EXCEPTION";
            case CR_CPP_TERMINATE_CALL: return "CR_CPP_TERMINATE_CALL";
            case CR_CPP_UNEXPECTED_CALL: return "CR_CPP_UNEXPECTED_CALL";
            case CR_CPP_PURE_CALL: return "CR_CPP_PURE_CALL";
            case CR_CPP_NEW_OPERATOR_ERROR: return "CR_CPP_NEW_OPERATOR_ERROR";
            case CR_CPP_INVALID_PARAMETER: return "CR_CPP_INVALID_PARAMETER";
            case CR_CPP_SIGABRT: return "CR_CPP_SIGABRT";
            case CR_CPP_SIGFPE: return "CR_CPP_SIGFPE";
            case CR_CPP_SIGILL: return "CR_CPP_SIGILL";
            case CR_CPP_SIGINT: return "CR_CPP_SIGINT";
            case CR_CPP_SIGSEGV: return "CR_CPP_SIGSEGV";
            case CR_CPP_SIGTERM: return "CR_CPP_SIGTERM";
            case CR_NONCONTINUABLE_EXCEPTION: return "CR_NONCONTINUABLE_EXCEPTION";
            case CR_STACK_OVERFLOW: return "CR_STACK_OVERFLOW";
            default: return CR_SEH_EXCEPTION;
        }
    }

    // Test that CrashRpt catches the crash on both the main thread and a thread created by std::thread by checking
    // the process exit code, since CrashRpt always terminates the process with exit code 1.
    TEST_P(CrashRptDeathTest, EmulatedCrash_IsCaught)
    {
        const auto exceptionType = GetParam();
        EXPECT_EXIT(installAndEmulate(exceptionType), ::testing::ExitedWithCode(1), "");

        EXPECT_EXIT(installAndEmulateOnOtherThread(exceptionType), ::testing::ExitedWithCode(1), "");
    }

    // Create a test case for each type of crash that CrashRpt can emulate, except CR_CPP_SECURITY_ERROR since
    // it is supported in VS .NET 2003 only and CR_THROW since gtest catches the exception.
    INSTANTIATE_TEST_SUITE_P(
        /*prefix*/,
        CrashRptDeathTest,
        ::testing::Values(
            CR_SEH_EXCEPTION,
            CR_CPP_TERMINATE_CALL,
            CR_CPP_UNEXPECTED_CALL,
            CR_CPP_PURE_CALL,
            CR_CPP_NEW_OPERATOR_ERROR,
            CR_CPP_INVALID_PARAMETER,
            CR_CPP_SIGABRT,
            CR_CPP_SIGFPE,
            CR_CPP_SIGILL,
            CR_CPP_SIGINT,
            CR_CPP_SIGSEGV,
            CR_CPP_SIGTERM,
            CR_NONCONTINUABLE_EXCEPTION,
            CR_STACK_OVERFLOW),
        [](const auto& info) { return getExceptionName(info.param); });
}
