#pragma once

// Writes a crash report (exception, faulting module + offset, stack trace) to jpog_crash.log in
// the working directory when the process hits an unhandled exception.
void InstallCrashHandler();
