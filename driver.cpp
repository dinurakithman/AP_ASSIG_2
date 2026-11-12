/*
 * File        : driver.cpp
 * Project     : Event Registration and Ticket Management System
 * Course      : XBCS2014N Advanced Programming
 * Description : Program entry point (driver). Creates the RegistrationSystem,
 *               loads the three data files and starts the console menu.
 *
 * Compile     : g++ -std=c++17 -Wall -Wextra -Wpedantic driver.cpp header.cpp -o event_system
 * Usage       : ./event_system [data-directory]
 *
 *   Without an argument the program reads Event.txt, Participant.txt and
 *   Registration.txt from the current folder (the folder it is run from).
 */
#include <cstdlib>
#include <exception>
#include <iostream>

#include "header.hpp"

namespace {
// Chooses where the data files are read from (uses both overloaded
// FileManager constructors).
FileManager makeFileManager(int argc, char* argv[]) {
    if (argc > 1) {
        return FileManager{argv[1]};  // directory given on the command line
    }
    return FileManager{};             // default: the current folder
}
}  // namespace

int main(int argc, char* argv[]) {
    try {
        RegistrationSystem registrationSystem{makeFileManager(argc, argv)};
        registrationSystem.loadAll();  // throws FileProcessingException if a required file is missing

        ConsoleMenu menu{registrationSystem};
        menu.run();
    } catch (const FileProcessingException& error) {
        std::cerr << "Fatal error: " << error.what() << '\n'
                  << "Usage: event_system [data-directory]\n"
                  << "The directory must contain " << config::kEventFileName << ", "
                  << config::kParticipantFileName << " and " << config::kRegistrationFileName
                  << ".\n";
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        // Last line of defence: report any unexpected error instead of crashing.
        std::cerr << "Unexpected error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
