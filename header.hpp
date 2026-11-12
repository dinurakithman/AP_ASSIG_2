/*
 * File        : header.hpp
 * Project     : Event Registration and Ticket Management System
 * Course      : XBCS2014N Advanced Programming
 * Description : Declarations of every constant, helper function and class
 *               used by the application. The implementations are in
 *               header.cpp and the program entry point is in driver.cpp.
 *
 * Contents (in dependency order):
 *    1. Configuration constants               (namespace config)
 *    2. Text helper functions                 (namespace textutils)
 *    3. EventType enumeration
 *    4. Exception classes                     (EventSystemException hierarchy)
 *    5. Event (abstract base class) and its derived classes
 *       Workshop, Seminar and Competition
 *    6. EventFactory
 *    7. Participant
 *    8. Ticket
 *    9. Registration
 *   10. RegistrationStats
 *   11. FileRecord and FileManager
 *   12. RegistrationSystem
 *   13. ReportGenerator
 *   14. ConsoleMenu
 *
 * External references are cited as [Ref n]; the full list, with an
 * explanation of how each one was used, is in README.md.
 */
#ifndef HEADER_HPP
#define HEADER_HPP

#include <cstddef>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

// =============================================================================
// 1. Configuration constants
//
// Typed constexpr constants are used instead of #define macros because they
// are type-checked and obey scope rules [Ref 1: "Getting Organized VI" - const
// vs Macro]. "inline constexpr" (C++17) lets them be defined in a header
// without breaking the One Definition Rule [Ref 2: "inline specifier"].
// =============================================================================
namespace config {

// ----- Data files (their structure must NOT be modified) -----
inline constexpr const char* kDefaultDataDirectory{"."};  // the folder the program is run from
inline constexpr const char* kEventFileName{"Event.txt"};
inline constexpr const char* kParticipantFileName{"Participant.txt"};
inline constexpr const char* kRegistrationFileName{"Registration.txt"};

// Field separator used by all three data files.
inline constexpr char kFieldDelimiter{';'};

// Expected number of fields in each type of record.
inline constexpr std::size_t kEventFieldCount{4};         // eventId;eventName;eventType;registrationFee
inline constexpr std::size_t kParticipantFieldCount{2};   // participantId;participantName
inline constexpr std::size_t kRegistrationFieldCount{2};  // participantId;eventId

// Currency symbol printed in front of monetary values.
inline constexpr const char* kCurrency{"RM"};

}  // namespace config

// =============================================================================
// 2. Text helper functions
//
// Stateless helpers are grouped in a namespace rather than a class because they
// share no data [Ref 1: "Getting Organized VI" - namespaces].
// =============================================================================
namespace textutils {

// Removes leading and trailing whitespace (spaces, tabs, '\r', '\n').
[[nodiscard]] std::string trim(const std::string& text);

// Returns an upper-case copy of the text, e.g. "p001" -> "P001".
[[nodiscard]] std::string toUpper(std::string text);

// Splits a record on the delimiter and trims every field. Empty fields are
// kept so malformed records can be detected by counting the fields.
[[nodiscard]] std::vector<std::string> split(const std::string& text, char delimiter);

// Converts text such as "150" or "99.50" into a double.
// Throws std::invalid_argument if the text is not a complete, finite number.
[[nodiscard]] double parseAmount(const std::string& text);

// Formats an amount with two decimal places, e.g. 150 -> "150.00".
[[nodiscard]] std::string formatAmount(double amount);

// Formats an amount with the currency symbol, e.g. 150 -> "RM 150.00".
[[nodiscard]] std::string formatCurrency(double amount);

}  // namespace textutils

// =============================================================================
// 3. EventType enumeration
//
// "enum class" is scoped and does not convert implicitly to int
// [Ref 1: "Programming with Class VII and VIII" - enum class].
// =============================================================================
enum class EventType {
    Workshop,
    Seminar,
    Competition
};

// Returns the text used in Event.txt, e.g. EventType::Workshop -> "Workshop".
[[nodiscard]] std::string toString(EventType eventType);

// Converts text such as "Workshop" (case-insensitive) into an EventType.
// Throws std::invalid_argument when the text is not a recognised event type.
[[nodiscard]] EventType parseEventType(const std::string& text);

// =============================================================================
// 4. Exception classes
//
//   std::runtime_error
//     +-- EventSystemException            (base for all application errors)
//           +-- InvalidEventException            (unknown event ID)
//           +-- InvalidParticipantException      (unknown participant ID)
//           +-- DuplicateRegistrationException   (participant already registered)
//           +-- RegistrationNotFoundException    (registration does not exist)
//           +-- FileProcessingException          (file cannot be read/written, bad record)
//
// Dedicated exception classes separate error detection (business classes) from
// error recovery (user interface) [Ref 1: "Programming with Class VII and
// VIII" - Throw Dedicated Exception Class; Ref 3: Core Guidelines E.14].
// =============================================================================

// Base class: lets callers catch every application error with one handler.
class EventSystemException : public std::runtime_error {
public:
    explicit EventSystemException(const std::string& message);
};

// Thrown when an event ID does not match any loaded event.
class InvalidEventException : public EventSystemException {
public:
    explicit InvalidEventException(const std::string& eventId);

    [[nodiscard]] const std::string& getEventId() const;

private:
    std::string eventId_;
};

// Thrown when a participant ID does not match any loaded participant.
class InvalidParticipantException : public EventSystemException {
public:
    explicit InvalidParticipantException(const std::string& participantId);

    [[nodiscard]] const std::string& getParticipantId() const;

private:
    std::string participantId_;
};

// Thrown when a participant tries to register for the same event twice.
class DuplicateRegistrationException : public EventSystemException {
public:
    DuplicateRegistrationException(const std::string& participantId, const std::string& eventId);

    [[nodiscard]] const std::string& getParticipantId() const;
    [[nodiscard]] const std::string& getEventId() const;

private:
    std::string participantId_;
    std::string eventId_;
};

// Thrown when a registration that does not exist is cancelled or looked up.
class RegistrationNotFoundException : public EventSystemException {
public:
    RegistrationNotFoundException(const std::string& participantId, const std::string& eventId);

    [[nodiscard]] const std::string& getParticipantId() const;
    [[nodiscard]] const std::string& getEventId() const;

private:
    std::string participantId_;
    std::string eventId_;
};

// Thrown when a data file cannot be opened, read or written, or when a record
// is malformed. Overloaded constructors let the error refer either to the
// whole file or to one specific line.
class FileProcessingException : public EventSystemException {
public:
    FileProcessingException(const std::string& fileName, const std::string& details);
    FileProcessingException(const std::string& fileName, std::size_t lineNumber,
                            const std::string& details);

    [[nodiscard]] const std::string& getFileName() const;
    [[nodiscard]] std::size_t getLineNumber() const;  // 0 when not line-specific

private:
    [[nodiscard]] static std::string buildMessage(const std::string& fileName,
                                                  std::size_t lineNumber,
                                                  const std::string& details);

    std::string fileName_;
    std::size_t lineNumber_{0};
};

// =============================================================================
// 5. Event hierarchy
//
// Event stores the data common to every event. Behaviour that differs between
// Workshop, Seminar and Competition is declared as pure virtual functions, so
// Event is abstract and objects are used through base-class pointers or
// references (runtime polymorphism) [Ref 1: "Programming with Class IV, V,
// VI" - Inheritance, Runtime Polymorphism; Ref 3: Core Guidelines C.35, C.67,
// C.128].
// =============================================================================
class Event {
public:
    // Virtual destructor: deleting a derived object through an Event pointer
    // (e.g. std::unique_ptr<Event>) runs the correct destructor.
    virtual ~Event() = default;

    // Events are owned through std::unique_ptr<Event> and never copied.
    // Deleting the copy operations prevents accidental object slicing.
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    // ----- Accessors (getters) -----
    [[nodiscard]] const std::string& getEventId() const;
    [[nodiscard]] const std::string& getEventName() const;
    [[nodiscard]] double getRegistrationFee() const;
    [[nodiscard]] std::string getEventTypeName() const;

    // ----- Mutators (setters) - validate before changing the object -----
    void setEventName(std::string eventName);
    void setRegistrationFee(double registrationFee);

    // ----- Polymorphic interface: every derived class must implement these -----
    [[nodiscard]] virtual EventType getEventType() const = 0;
    [[nodiscard]] virtual std::string getTicketPrefix() const = 0;
    [[nodiscard]] virtual std::string getSessionFormat() const = 0;

    // Prints a multi-line description; type-specific lines come from the
    // virtual functions above.
    void displayDetails(std::ostream& os) const;

    // Prints the column headings that match operator<< below.
    static void printTableHeader(std::ostream& os);

    // Friend stream operator: prints the event as one table row. It is a
    // friend so it can read the private data members directly.
    friend std::ostream& operator<<(std::ostream& os, const Event& event);

protected:
    // Only derived classes may create an Event (the class is abstract).
    Event(std::string eventId, std::string eventName, double registrationFee);

private:
    // Throws std::invalid_argument if the fee is negative.
    [[nodiscard]] static double validateFee(double registrationFee);

    std::string eventId_;
    std::string eventName_;
    double registrationFee_{0.0};
};

// Workshop IS-A Event: a hands-on training event.
// "final" prevents further derivation; "override" makes the compiler check
// that each function really overrides a virtual function of Event.
class Workshop final : public Event {
public:
    Workshop(std::string eventId, std::string eventName, double registrationFee);

    [[nodiscard]] EventType getEventType() const override;
    [[nodiscard]] std::string getTicketPrefix() const override;
    [[nodiscard]] std::string getSessionFormat() const override;
};

// Seminar IS-A Event: a talk delivered by speakers.
class Seminar final : public Event {
public:
    Seminar(std::string eventId, std::string eventName, double registrationFee);

    [[nodiscard]] EventType getEventType() const override;
    [[nodiscard]] std::string getTicketPrefix() const override;
    [[nodiscard]] std::string getSessionFormat() const override;
};

// Competition IS-A Event: a judged contest.
class Competition final : public Event {
public:
    Competition(std::string eventId, std::string eventName, double registrationFee);

    [[nodiscard]] EventType getEventType() const override;
    [[nodiscard]] std::string getTicketPrefix() const override;
    [[nodiscard]] std::string getSessionFormat() const override;
};

// =============================================================================
// 6. EventFactory
//
// Creates the correct Event subclass for an EventType and returns it through a
// base-class smart pointer, so the rest of the program never needs to know
// which derived class was created [Ref 4: Factory Method]. std::unique_ptr
// expresses exclusive ownership and frees the object automatically (RAII)
// [Ref 1: "Getting Organized VI" - unique_ptr, make_unique].
// =============================================================================
class EventFactory {
public:
    // Utility class with static members only - it is never instantiated.
    EventFactory() = delete;

    // Dynamically allocates a Workshop, Seminar or Competition.
    // Throws std::invalid_argument if the data is invalid (e.g. negative fee).
    [[nodiscard]] static std::unique_ptr<Event> createEvent(EventType eventType,
                                                            std::string eventId,
                                                            std::string eventName,
                                                            double registrationFee);
};

// =============================================================================
// 7. Participant
//
// Data members are private and only reachable through accessors/mutators
// (encapsulation / information hiding) [Ref 1: "Programming with Class I, II
// and III" - Encapsulation, Getters, Setters].
// =============================================================================
class Participant {
public:
    // Throws std::invalid_argument if the ID or name is empty.
    Participant(std::string participantId, std::string participantName);

    // ----- Accessors -----
    [[nodiscard]] const std::string& getParticipantId() const;
    [[nodiscard]] const std::string& getParticipantName() const;

    // ----- Mutator -----
    void setParticipantName(std::string participantName);

    // Prints the column headings that match operator<< below.
    static void printTableHeader(std::ostream& os);

    // Friend stream operator: prints the participant as one table row.
    friend std::ostream& operator<<(std::ostream& os, const Participant& participant);

private:
    std::string participantId_;
    std::string participantName_;
};

// =============================================================================
// 8. Ticket
//
// The admission ticket of a registration. A Ticket is part of a Registration
// (COMPOSITION): it is created inside the Registration and destroyed with it.
// The ticket number is derived from the event type prefix, event ID and
// participant ID (e.g. "WKS-E001-P001"), so the provided files need no extra data.
// =============================================================================
class Ticket {
public:
    // Overloaded constructors [Ref 1: "Programming with Class I, II and III"
    // - Constructor Overloading, Delegating Constructor]:
    //   1) a ticket with an explicit ticket number and amount;
    //   2) a ticket for an event/participant pair - delegates to (1).
    Ticket(std::string ticketNumber, double amountPaid);
    Ticket(const Event& event, const Participant& participant);

    [[nodiscard]] const std::string& getTicketNumber() const;
    [[nodiscard]] double getAmountPaid() const;

    // Friend stream operator: prints the ticket number.
    friend std::ostream& operator<<(std::ostream& os, const Ticket& ticket);

private:
    [[nodiscard]] static std::string buildTicketNumber(const Event& event,
                                                       const Participant& participant);

    std::string ticketNumber_;
    double amountPaid_{0.0};
};

// =============================================================================
// 9. Registration
//
// Links one Participant to one Event and owns the Ticket issued for it.
//   - Registration --> Participant : ASSOCIATION (non-owning pointer)
//   - Registration --> Event       : ASSOCIATION (non-owning pointer)
//   - Registration *-- Ticket      : COMPOSITION (data member)
// Pointers (not references) are stored so that Registration objects remain
// copy-assignable, which std::vector::erase() requires.
// =============================================================================
class Registration {
public:
    Registration(const Participant& participant, const Event& event);

    // ----- Accessors -----
    [[nodiscard]] const Participant& getParticipant() const;
    [[nodiscard]] const Event& getEvent() const;
    [[nodiscard]] const Ticket& getTicket() const;
    [[nodiscard]] const std::string& getParticipantId() const;
    [[nodiscard]] const std::string& getEventId() const;

    // Converts the registration into the Registration.txt format: "P001;E001".
    [[nodiscard]] std::string toRecord() const;

    // Prints a formatted admission ticket.
    void printTicket(std::ostream& os) const;

    // Prints the column headings that match operator<< below.
    static void printTableHeader(std::ostream& os);

    // ----- Overloaded operators (friend functions) -----
    // Two registrations are equal when they link the same participant to the
    // same event. Used with std::find to prevent duplicate registrations.
    friend bool operator==(const Registration& lhs, const Registration& rhs);
    friend bool operator!=(const Registration& lhs, const Registration& rhs);

    // Prints the registration as one table row.
    friend std::ostream& operator<<(std::ostream& os, const Registration& registration);

private:
    const Participant* participant_;  // association - not owned
    const Event* event_;              // association - not owned
    Ticket ticket_;                   // composition - owned
};

// =============================================================================
// 10. RegistrationStats
//
// Accumulates the number of registrations and the total fees for an event or
// a participant. Operator overloading makes the report code read naturally
// [Ref 1: "Programming with Class IV, V, VI" - Operator Overloading]:
//     stats += event;                // record one registration for 'event'
//     grandTotal = statsA + statsB;  // combine two sets of statistics
// =============================================================================
class RegistrationStats {
public:
    // Overloaded constructors: start empty, or start from known values.
    RegistrationStats() = default;
    RegistrationStats(int registrationCount, double totalFees);

    [[nodiscard]] int getRegistrationCount() const;
    [[nodiscard]] double getTotalFees() const;

    // Adds one registration for the given event and adds its fee to the total.
    RegistrationStats& operator+=(const Event& event);

    // Friend function: combines two sets of statistics (e.g. for a grand total).
    friend RegistrationStats operator+(const RegistrationStats& lhs, const RegistrationStats& rhs);

private:
    int registrationCount_{0};
    double totalFees_{0.0};
};

// =============================================================================
// 11. FileRecord and FileManager
//
// FileManager only deals with lines and fields; turning fields into objects is
// the job of RegistrationSystem (separation of concerns).
// =============================================================================

// One non-blank line of a data file, already split into trimmed fields.
struct FileRecord {
    std::size_t lineNumber{0};        // 1-based line number, used in error messages
    std::vector<std::string> fields;  // e.g. {"E001", "C++ Workshop", "Workshop", "150"}
};

class FileManager {
public:
    // Overloaded constructors: use the default directory (the current folder),
    // or a directory chosen by the user. The default constructor delegates.
    FileManager();
    explicit FileManager(std::string dataDirectory);

    [[nodiscard]] const std::string& getDataDirectory() const;

    // Returns "<dataDirectory>/<fileName>".
    [[nodiscard]] std::string buildPath(const std::string& fileName) const;

    // Returns true if the file exists and can be opened for reading.
    [[nodiscard]] bool fileExists(const std::string& fileName) const;

    // Reads every non-blank line and splits it on ';'.
    // Throws FileProcessingException if the file cannot be opened or read.
    [[nodiscard]] std::vector<FileRecord> readRecords(const std::string& fileName) const;

    // Writes one line per record via a temporary file that is then renamed,
    // so the original file is never left half-written.
    // Throws FileProcessingException on failure.
    void writeLines(const std::string& fileName, const std::vector<std::string>& lines) const;

private:
    std::string dataDirectory_;
};

// =============================================================================
// 12. RegistrationSystem
//
// Owns all events, participants and registrations, and provides the event,
// participant and registration management operations.
//   - RegistrationSystem *-- Event        : COMPOSITION via std::unique_ptr<Event>
//   - RegistrationSystem *-- Participant  : COMPOSITION (stored by value)
//   - RegistrationSystem *-- Registration : COMPOSITION (stored by value)
//   - RegistrationSystem *-- FileManager  : COMPOSITION (stored by value)
//
// Container choice (efficiency) [Ref 2: std::map, std::vector]:
//   - std::map gives O(log n) look-up by ID, keeps records sorted by ID and
//     never moves its elements, so pointers held by Registration stay valid.
//   - std::vector keeps registrations in insertion order, which is the order
//     in which they are written back to Registration.txt.
// =============================================================================
class RegistrationSystem {
public:
    explicit RegistrationSystem(FileManager fileManager);

    // The system owns unique resources and Registration objects point into its
    // containers, so copying it would be unsafe: copy operations are deleted.
    RegistrationSystem(const RegistrationSystem&) = delete;
    RegistrationSystem& operator=(const RegistrationSystem&) = delete;

    // ----- File processing -----
    // Loads Event.txt, Participant.txt and Registration.txt (in that order).
    // Malformed records are skipped and reported through getLoadWarnings().
    // Throws FileProcessingException if Event.txt or Participant.txt cannot be read.
    void loadAll();

    // Writes the current registrations back to Registration.txt.
    void saveRegistrations();

    // ----- Look-ups (IDs are case-insensitive, e.g. "p001" == "P001") -----
    [[nodiscard]] const Event& findEvent(const std::string& eventId) const;                    // throws InvalidEventException
    [[nodiscard]] const Participant& findParticipant(const std::string& participantId) const;  // throws InvalidParticipantException
    [[nodiscard]] const Registration& findRegistration(const std::string& participantId,
                                                       const std::string& eventId) const;      // throws RegistrationNotFoundException

    // ----- Registration management -----
    // Throws InvalidParticipantException, InvalidEventException or DuplicateRegistrationException.
    const Registration& registerParticipant(const std::string& participantId,
                                            const std::string& eventId);

    // Removes the registration and returns a copy of it (for confirmation messages).
    // Throws InvalidParticipantException, InvalidEventException or RegistrationNotFoundException.
    Registration cancelRegistration(const std::string& participantId, const std::string& eventId);

    // ----- Queries -----
    [[nodiscard]] std::vector<const Registration*> getRegistrationsForEvent(
        const std::string& eventId) const;
    [[nodiscard]] std::vector<const Registration*> getRegistrationsForParticipant(
        const std::string& participantId) const;

    [[nodiscard]] const std::map<std::string, std::unique_ptr<Event>>& getEvents() const;
    [[nodiscard]] const std::map<std::string, Participant>& getParticipants() const;
    [[nodiscard]] const std::vector<Registration>& getRegistrations() const;
    [[nodiscard]] const std::vector<std::string>& getLoadWarnings() const;
    [[nodiscard]] const std::string& getDataDirectory() const;
    [[nodiscard]] std::string getRegistrationFilePath() const;
    [[nodiscard]] bool hasUnsavedChanges() const;

private:
    void loadEvents();
    void loadParticipants();
    void loadRegistrations();

    // Adds a registration after checking for duplicates with operator==.
    const Registration& addRegistration(const Participant& participant, const Event& event);

    // Stores a warning about a record that could not be loaded.
    void recordSkippedLine(const FileProcessingException& problem);

    // Trims an ID and converts it to upper case.
    [[nodiscard]] static std::string normaliseId(const std::string& id);

    FileManager fileManager_;
    std::map<std::string, std::unique_ptr<Event>> events_;  // key: event ID
    std::map<std::string, Participant> participants_;       // key: participant ID
    std::vector<Registration> registrations_;
    std::vector<std::string> loadWarnings_;
    bool unsavedChanges_{false};
};

// =============================================================================
// 13. ReportGenerator
//
// Produces the four required reports:
//   (a) participants registered for a particular event
//   (b) registration summary for a participant
//   (c) total number of registrations for each event
//   (d) total registration fees collected for each event
// ReportGenerator --> RegistrationSystem is an ASSOCIATION: the generator only
// reads the data (const reference) and does not own it.
// =============================================================================
class ReportGenerator {
public:
    explicit ReportGenerator(const RegistrationSystem& system);

    // (a) Throws InvalidEventException if the event ID does not exist.
    void printParticipantsForEvent(std::ostream& os, const std::string& eventId) const;

    // (b) Throws InvalidParticipantException if the participant ID does not exist.
    void printParticipantSummary(std::ostream& os, const std::string& participantId) const;

    // (c)
    void printRegistrationCounts(std::ostream& os) const;

    // (d)
    void printFeesCollected(std::ostream& os) const;

private:
    // One RegistrationStats per event (events with no registrations included).
    [[nodiscard]] std::map<std::string, RegistrationStats> buildEventStatistics() const;

    // Adds every per-event statistic together with the overloaded operator+.
    [[nodiscard]] static RegistrationStats calculateGrandTotal(
        const std::map<std::string, RegistrationStats>& statistics);

    static void printTitle(std::ostream& os, const std::string& title);

    const RegistrationSystem& system_;
};

// =============================================================================
// 14. ConsoleMenu
//
// Text-based user interface: shows the menu, reads input, calls
// RegistrationSystem / ReportGenerator and handles the exceptions they throw.
//   - ConsoleMenu --> RegistrationSystem : ASSOCIATION (reference, not owned)
//   - ConsoleMenu *-- ReportGenerator    : COMPOSITION (data member)
// The input and output streams are injected through the constructor, so the
// menu can also be driven by a file or string stream (useful for testing).
// =============================================================================
class ConsoleMenu {
public:
    // Overloaded constructors: the first uses std::cin / std::cout and
    // delegates to the second.
    explicit ConsoleMenu(RegistrationSystem& system);
    ConsoleMenu(RegistrationSystem& system, std::istream& input, std::ostream& output);

    // Runs the main loop until the user chooses Exit or the input ends.
    void run();

private:
    enum class MenuOption {
        Exit = 0,
        DisplayEvents,
        DisplayParticipants,
        DisplayRegistrations,
        RegisterParticipant,
        CancelRegistration,
        PrintTicket,
        ReportParticipantsForEvent,
        ReportParticipantSummary,
        ReportRegistrationCounts,
        ReportFeesCollected,
        SaveChanges
    };
    static constexpr int kHighestOption{static_cast<int>(MenuOption::SaveChanges)};

    // ----- Screen output -----
    void printWelcome() const;
    void printMenu() const;
    void printHeading(const std::string& title) const;

    // ----- Input helpers -----
    // Shows the message and reads one trimmed line; std::nullopt means the
    // input has ended (e.g. Ctrl+D / Ctrl+Z or the end of a piped file).
    [[nodiscard]] std::optional<std::string> prompt(const std::string& message);
    [[nodiscard]] static std::optional<MenuOption> parseMenuOption(const std::string& text);
    [[nodiscard]] bool confirm(const std::string& question);

    // ----- Menu actions -----
    void handleOption(MenuOption option);
    void displayEvents() const;
    void displayParticipants() const;
    void displayRegistrations() const;
    void registerParticipant();
    void cancelRegistration();
    void printTicket();
    void reportParticipantsForEvent();
    void reportParticipantSummary();
    void saveChanges();
    [[nodiscard]] bool confirmExit();

    RegistrationSystem& system_;
    ReportGenerator reportGenerator_;
    std::istream& input_;
    std::ostream& output_;
};

#endif  // HEADER_HPP
