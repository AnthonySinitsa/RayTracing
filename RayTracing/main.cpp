#include "first_app.hpp"

// std
#include <cstdlib>
#include <iostream>
#include <stdexcept>

/**
 * @brief Entry point for the application.
 *
 * This function initializes an instance of `lve::FirstApp` and attempts to run it. If an exception is thrown
 * during the execution of the application, it catches the exception, prints the error message to the standard
 * error stream, and returns a failure exit code. If the application completes successfully, it returns a success
 * exit code.
 *
 * The main function performs the following steps:
 * 1. Creates an instance of `lve::FirstApp`.
 * 2. Calls the `run` method on the `FirstApp` instance.
 * 3. Catches and handles any exceptions thrown during the `run` method execution.
 * 4. Returns an appropriate exit code based on the success or failure of the application.
 *
 * @return An exit code indicating the success or failure of the application.
 *         - `EXIT_SUCCESS` (0) if the application completes successfully.
 *         - `EXIT_FAILURE` (1) if an exception is thrown and caught.
 */

int main() {
	lve::FirstApp app{};

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}