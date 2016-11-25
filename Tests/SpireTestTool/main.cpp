// main.cpp

#include "../../Source/CoreLib/LibIO.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;

#include "os.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

TestResult runTestImpl(
	String	filePath)
{
	// need to execute the stand-alone Spire compiler on the file, and compare its output to what we expect

	OSProcessSpawner spawner;

	spawner.pushExecutableName(L"Source/Debug/SpireCompiler.exe");
	spawner.pushArgument(filePath);

	if (spawner.spawnAndWaitForCompletion() != kOSError_None)
	{
		error("failed to run test '%ls'", filePath.begin());
		return kTestResult_Fail;
	}

	// We ignore output to stdout, and only worry about what the compiler
	// wrote to stderr.

	OSProcessSpawner::ResultCode resultCode = spawner.getResultCode();

	String standardOuptut = spawner.getStandardOutput();
	String standardError = spawner.getStandardError();

	// We construct a single output string that captures the results
	StringBuilder actualOutputBuilder;
	actualOutputBuilder.Append(L"result code = ");
	actualOutputBuilder.Append(resultCode);
	actualOutputBuilder.Append(L"\nstandard error = {\n");
	actualOutputBuilder.Append(standardError);
	actualOutputBuilder.Append(L"}\nstandard output = {\n");
	actualOutputBuilder.Append(standardOuptut);
	actualOutputBuilder.Append(L"}\n");

	String actualOutput = actualOutputBuilder.ProduceString();

	String expectedOutputPath = filePath + L".expected";
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
		String actualOutputPath = filePath + L".actual";
		CoreLib::IO::File::WriteAllText(actualOutputPath, actualOutput);
	}

	return result;
}

struct TestContext
{
	int totalTestCount;
	int passedTestCount;
	int failedTestCount;
};

void runTest(
	TestContext*	context,
	String			filePath)
{

	context->totalTestCount++;
	TestResult result = runTestImpl(filePath);
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

	printf(" test: '%ls'\n", filePath.begin());
}

void runTestsInDirectory(
	TestContext*		context,
	String				directoryPath)
{
	for (auto file : osFindFilesInDirectoryMatchingPattern(directoryPath, L"*.spire"))
	{
		runTest(context, file);
	}
}

//

int main(
	int		argc,
	char**	argv)
{
	TestContext context = { 0 };

	// Enumerate test files according to policy
	// TODO: add more directories to this list
	// TODO: allow for a command-line argument to select a particular directory
	runTestsInDirectory(&context, L"Tests/FrontEnd/");

	if (!context.totalTestCount)
	{
		printf("no tests run");
		return 0;
	}

	printf("\n===\n%d%% of tests passed (%d/%d)\n===\n\n", (context.passedTestCount*100) / context.totalTestCount, context.passedTestCount, context.totalTestCount);
	return context.passedTestCount == context.totalTestCount ? 0 : 1;
}
