// main.cpp

#include "../../Source/CoreLib/LibIO.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;

#include "os.h"


#ifdef _WIN32
#define SPIRE_TEST_SUPPORT_HLSL 1
#include <d3dcompiler.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

struct Options
{
    char const* appName = "SpireTestTool";

    // only run test cases with names that have this prefix
    char const* testPrefix = nullptr;

    // generate extra output (notably: command lines we run)
    bool shouldBeVerbose = false;

    // force generation of baselines for HLSL tests
    bool generateHLSLBaselines = false;
};
Options options;

void parseOptions(int* argc, char** argv)
{
    int argCount = *argc;
    char const* const* argCursor = argv;
    char const* const* argEnd = argCursor + argCount;

    char const** writeCursor = (char const**) argv;

    // first argument is the application name
    if( argCursor != argEnd )
    {
        options.appName = *argCursor++;
    }

    // now iterate over arguments to collect options
    while(argCursor != argEnd)
    {
        char const* arg = *argCursor++;
        if( arg[0] != '-' )
        {
            *writeCursor++ = arg;
            continue;
        }

        if( strcmp(arg, "--") == 0 )
        {
            while(argCursor != argEnd)
            {
                char const* arg = *argCursor++;
                *writeCursor++ = arg;
            }
            break;
        }
        else if( strcmp(arg, "-v") == 0 )
        {
            options.shouldBeVerbose = true;
        }
        else if( strcmp(arg, "-generate-hlsl-baselines") == 0 )
        {
            options.generateHLSLBaselines = true;
        }
        else
        {
            fprintf(stderr, "unknown option '%s'\n", arg);
            exit(1);
        }
    }
    
    // any arguments left over were positional arguments
    argCount = writeCursor - argv;
    argCursor = argv;
    argEnd = argCursor + argCount;

    // first positional argument is a "filter" to apply
    if( argCursor != argEnd )
    {
        options.testPrefix = *argCursor++;
    }

    // any remaining arguments represent an error
    if(argCursor != argEnd)
    {
        fprintf(stderr, "unexpected arguments\n");
        exit(1);
    }

    *argc = 0;
}

// Called for an error in the test-runner (not for an error involving
// a test itself).
void error(char const* message, ...)
{
    fprintf(stderr, "error: ");

    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);

    fprintf(stderr, "\n");
}

enum TestResult
{
    kTestResult_Fail,
    kTestResult_Pass,
};

bool match(char const** ioCursor, char const* expected)
{
    char const*& cursor = *ioCursor;
    while(*expected && *cursor == *expected )
    {
        cursor++;
        expected++;
    }
    return *expected == 0;
}

// Try to read command-line options from the test file itself
void gatherOptionsFromTestFile(
    String				filePath,
    OSProcessSpawner*	ioSpawner)
{
    String fileContents;
    try
    {
        fileContents = CoreLib::IO::File::ReadAllText(filePath);
    }
    catch (CoreLib::IO::IOException)
    {
        return;
    }

    char const* cursor = fileContents.begin();

    // check for our expected prefix
    if(!match(&cursor, "//SPIRE_TEST_OPTS:"))
    {
        return;
    }

    // start consuming options
    for(;;)
    {
        // Skip to start of an option
        switch(*cursor)
        {
        // end of line/file? done with options
        case '\r': case '\n': case 0:
            return;

        // space? keep skipping
        case ' ':
            cursor++;
            continue;

        default:
            break;
        }

        // start of an option!
        char const* optBegin = cursor;

        // now look for the end of the option
        for(;;)
        {
            switch(*cursor)
            {
            default:
                cursor++;
                continue;

            case '\r': case '\n': case 0: case ' ':
                // end of the option
                break;
            }

            char const* optEnd = cursor;
            assert(optBegin != optEnd);

            StringBuilder sb;
            sb.Append(optBegin, optEnd - optBegin);
            ioSpawner->pushArgument(sb.ProduceString());
            break;
        }
    }
}

OSError spawnAndWait(String	testPath, OSProcessSpawner& spawner)
{
    if( options.shouldBeVerbose )
    {
        fprintf(stderr, "%s\n", spawner.commandLine_.Buffer());
    }

    OSError err = spawner.spawnAndWaitForCompletion();
    if (err != kOSError_None)
    {
        error("failed to run test '%S'", testPath.ToWString());
    }
    return err;
}

String getOutput(OSProcessSpawner& spawner)
{
    OSProcessSpawner::ResultCode resultCode = spawner.getResultCode();

    String standardOuptut = spawner.getStandardOutput();
    String standardError = spawner.getStandardError();

    // We construct a single output string that captures the results
    StringBuilder actualOutputBuilder;
    actualOutputBuilder.Append("result code = ");
    actualOutputBuilder.Append(resultCode);
    actualOutputBuilder.Append("\nstandard error = {\n");
    actualOutputBuilder.Append(standardError);
    actualOutputBuilder.Append("}\nstandard output = {\n");
    actualOutputBuilder.Append(standardOuptut);
    actualOutputBuilder.Append("}\n");

    return actualOutputBuilder.ProduceString();
}


TestResult runTestImpl(
    String	filePath)
{
    // need to execute the stand-alone Spire compiler on the file, and compare its output to what we expect

    OSProcessSpawner spawner;

    spawner.pushExecutableName("Source/Debug/SpireCompiler.exe");
    spawner.pushArgument(filePath);

    gatherOptionsFromTestFile(filePath, &spawner);

    if (spawnAndWait(filePath, spawner) != kOSError_None)
    {
        return kTestResult_Fail;
    }

    String actualOutput = getOutput(spawner);

    String expectedOutputPath = filePath + ".expected";
    String expectedOutput;
    try
    {
        expectedOutput = CoreLib::IO::File::ReadAllText(expectedOutputPath);
    }
    catch (CoreLib::IO::IOException)
    {
    }

    // If no expected output file was found, then we
    // expect everything to be empty
    if (expectedOutput.Length() == 0)
    {
        expectedOutput = "result code = 0\nstandard error = {\n}\nstandard output = {\n}\n";
    }

    TestResult result = kTestResult_Pass;

    // Otherwise we compare to the expected output
    if (actualOutput != expectedOutput)
    {
        result = kTestResult_Fail;
    }

    // If the test failed, then we write the actual output to a file
    // so that we can easily diff it from the command line and
    // diagnose the problem.
    if (result == kTestResult_Fail)
    {
        String actualOutputPath = filePath + ".actual";
        CoreLib::IO::File::WriteAllText(actualOutputPath, actualOutput);
    }

    return result;
}

#ifdef SPIRE_TEST_SUPPORT_HLSL
void generateHLSLBaseline(
    String	filePath)
{
    OSProcessSpawner spawner;
    spawner.pushExecutableName("Source/Debug/SpireCompiler.exe");
    spawner.pushArgument(filePath);

    gatherOptionsFromTestFile(filePath, &spawner);

    spawner.pushArgument("-target");
    spawner.pushArgument("dxbc-assembly");
    spawner.pushArgument("-pass-through");
    spawner.pushArgument("fxc");

    if (spawnAndWait(filePath, spawner) != kOSError_None)
    {
        return;
    }

    String expectedOutput = getOutput(spawner);
    String expectedOutputPath = filePath + ".expected";
    try
    {
        CoreLib::IO::File::WriteAllText(expectedOutputPath, expectedOutput);
    }
    catch (CoreLib::IO::IOException)
    {
    }
}

TestResult runHLSLTestImpl(
    String	filePath)
{
    // We will use the Microsoft compiler to generate out expected output here
    String expectedOutputPath = filePath + ".expected";
    if(options.generateHLSLBaselines || !CoreLib::IO::File::Exists(expectedOutputPath))
    {
        generateHLSLBaseline(filePath);
    }

    // need to execute the stand-alone Spire compiler on the file, and compare its output to what we expect

    OSProcessSpawner spawner;

    spawner.pushExecutableName("Source/Debug/SpireCompiler.exe");
    spawner.pushArgument(filePath);

    gatherOptionsFromTestFile(filePath, &spawner);

    spawner.pushArgument("-target");
    spawner.pushArgument("dxbc-assembly");

    if (spawnAndWait(filePath, spawner) != kOSError_None)
    {
        return kTestResult_Fail;
    }

    // We ignore output to stdout, and only worry about what the compiler
    // wrote to stderr.

    OSProcessSpawner::ResultCode resultCode = spawner.getResultCode();

    String standardOuptut = spawner.getStandardOutput();
    String standardError = spawner.getStandardError();

    // We construct a single output string that captures the results
    StringBuilder actualOutputBuilder;
    actualOutputBuilder.Append("result code = ");
    actualOutputBuilder.Append(resultCode);
    actualOutputBuilder.Append("\nstandard error = {\n");
    actualOutputBuilder.Append(standardError);
    actualOutputBuilder.Append("}\nstandard output = {\n");
    actualOutputBuilder.Append(standardOuptut);
    actualOutputBuilder.Append("}\n");

    String actualOutput = actualOutputBuilder.ProduceString();

    String expectedOutput;
    try
    {
        expectedOutput = CoreLib::IO::File::ReadAllText(expectedOutputPath);
    }
    catch (CoreLib::IO::IOException)
    {
    }

    TestResult result = kTestResult_Pass;

    // If no expected output file was found, then we
    // expect everything to be empty
    if (expectedOutput.Length() == 0)
    {
        if (resultCode != 0)				result = kTestResult_Fail;
        if (standardError.Length() != 0)	result = kTestResult_Fail;
        if (standardOuptut.Length() != 0)	result = kTestResult_Fail;
    }
    // Otherwise we compare to the expected output
    else if (actualOutput != expectedOutput)
    {
        result = kTestResult_Fail;
    }

    // If the test failed, then we write the actual output to a file
    // so that we can easily diff it from the command line and
    // diagnose the problem.
    if (result == kTestResult_Fail)
    {
        String actualOutputPath = filePath + ".actual";
        CoreLib::IO::File::WriteAllText(actualOutputPath, actualOutput);
    }

    return result;
}
#endif

struct TestContext
{
    int totalTestCount;
    int passedTestCount;
    int failedTestCount;
};

void runTest(
    TestContext*	context,
    String			filePath,
    TestResult		(*runFunc)(String))
{
    if( options.testPrefix )
    {
        if( strncmp(options.testPrefix, filePath.begin(), strlen(options.testPrefix)) != 0 )
        {
            return;
        }
    }


    context->totalTestCount++;
    TestResult result = runFunc(filePath);
    if (result == kTestResult_Pass)
    {
        printf("passed");
        context->passedTestCount++;
    }
    else
    {
        printf("FAILED");
        context->failedTestCount++;
    }

    printf(" test: '%S'\n", filePath.ToWString());
}

void runTestsInDirectory(
    TestContext*		context,
    String				directoryPath,
    TestResult(*runFunc)(String),
    char const* pattern)
{
    for (auto file : osFindFilesInDirectoryMatchingPattern(directoryPath, pattern))
    {
        runTest(context, file, runFunc);
    }
    for (auto subdir : osFindChildDirectories(directoryPath))
    {
        runTestsInDirectory(context, subdir, runFunc, pattern);
    }
}

void runTestsInDirectory(
    TestContext*		context,
    String				directoryPath)
{
    runTestsInDirectory(context, directoryPath, &runTestImpl, "*.spire");
}

#ifdef SPIRE_TEST_SUPPORT_HLSL
void runHLSLTestsInDirectory(
    TestContext*		context,
    String				directoryPath)
{
    runTestsInDirectory(context, directoryPath, &runHLSLTestImpl, "*.hlsl");
//	runTestsInDirectory(context, directoryPath, &runHLSLTestImpl, "*.fx");
}
#endif

//

int main(
    int		argc,
    char**	argv)
{
    parseOptions(&argc, argv);

    TestContext context = { 0 };

    // Enumerate test files according to policy
    // TODO: add more directories to this list
    // TODO: allow for a command-line argument to select a particular directory
    runTestsInDirectory(&context, "Tests/FrontEnd/");
    runTestsInDirectory(&context, "Tests/Diagnostics/");
    runTestsInDirectory(&context, "Tests/Preprocessor/");

#ifdef SPIRE_TEST_SUPPORT_HLSL
    runHLSLTestsInDirectory(&context, "Tests/HLSL/DXSDK/");
#endif

    if (!context.totalTestCount)
    {
        printf("no tests run\n");
        return 0;
    }

    printf("\n===\n%d%% of tests passed (%d/%d)\n===\n\n", (context.passedTestCount*100) / context.totalTestCount, context.passedTestCount, context.totalTestCount);
    return context.passedTestCount == context.totalTestCount ? 0 : 1;
}
