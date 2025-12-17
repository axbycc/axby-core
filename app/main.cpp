#include "absl/flags/parse.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/debugging/symbolize.h"
#include "absl/debugging/failure_signal_handler.h"
#include "app/files.h"
#include "app/stop_all.h"
#include <csignal>
#include <exception>

void sigint_handler(int signal) {
    if (axby::should_stop_all()) {
	// stop_all() already called, but we got another signal to
	// kill, so forcefully terminate.
	LOG(FATAL) << "Forcefully killing the program.";
    }
		
    LOG(INFO) << "Gracefully stopping the program.";
    axby::stop_all();
}

void app_main_init(int argc, char* argv[]) {
    axby::init_runfiles(argv[0]);
    absl::SetStderrThreshold(absl::LogSeverity(0));
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();

    signal(SIGINT, sigint_handler); // For Ctrl+C
    signal(SIGTERM, sigint_handler);

    absl::InitializeSymbolizer(argv[0]);
    absl::FailureSignalHandlerOptions options;
    absl::InstallFailureSignalHandler(options);
}
