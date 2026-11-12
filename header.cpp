/*
 * File        : header.cpp
 * Project     : Event Registration and Ticket Management System
 * Course      : XBCS2014N Advanced Programming
 * Description : Implementation of every function and class declared in
 *               header.hpp. Sections appear in the same order as in the header.
 *
 * External references are cited as [Ref n]; the full list, with an
 * explanation of how each one was used, is in README.md.
 */
#include "header.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <ostream>
#include <sstream>
#include <utility>

// -----------------------------------------------------------------------------
// Layout constants used only in this file. Each class has its own named group
// so that column widths printed by a header row and by operator<< always match.
// The anonymous namespace gives them internal linkage (private to this file).
// -----------------------------------------------------------------------------
namespace {

namespace eventTable {
constexpr int kIdWidth{8};
constexpr int kNameWidth{28};
constexpr int kTypeWidth{14};
constexpr int kFeeWidth{10};
}  // namespace eventTable

namespace participantTable {
constexpr int kIdWidth{8};
constexpr int kNameWidth{24};
}  // namespace participantTable

namespace registrationTable {
constexpr int kTicketWidth{16};
constexpr int kParticipantIdWidth{7};
constexpr int kParticipantNameWidth{20};
constexpr int kEventIdWidth{7};
constexpr int kEventNameWidth{28};
}  // namespace registrationTable

namespace ticketLayout {
constexpr std::size_t kInnerWidth{60};  // width of the text area inside the ticket box
constexpr int kLabelWidth{11};
}  // namespace ticketLayout

namespace reportTable {
constexpr int kNumberWidth{5};
constexpr int kIdWidth{8};
constexpr int kEventNameWidth{28};
constexpr int kPersonNameWidth{20};
constexpr int kTypeWidth{14};
constexpr int kFeeWidth{10};
constexpr int kCountWidth{15};
constexpr int kTotalWidth{17};
}  // namespace reportTable

constexpr int kBannerWidth{62};

// Byte-order mark that some Windows editors (e.g. Notepad) add to UTF-8 files.
constexpr const char* kUtf8ByteOrderMark{"\xEF\xBB\xBF"};

// Returns a line of '-' characters of the given width.
std::string divider(int width) {
    return std::string(static_cast<std::size_t>(width), '-');
}

}  // namespace

// =============================================================================
// 2. Text helper functions
// [Ref 2] std::isspace / std::toupper must be given an unsigned char value;
// std::find_if_not, std::stod, std::isfinite, std::fixed, std::setprecision.
// =============================================================================
namespace textutils {

std::string trim(const std::string& text) {
    // std::isspace has undefined behaviour for negative char values, so the
    // character is converted to unsigned char first.
    const auto isSpace = [](unsigned char character) { return std::isspace(character) != 0; };

    const auto first = std::find_if_not(text.begin(), text.end(), isSpace);
    const auto last = std::find_if_not(text.rbegin(), text.rend(), isSpace).base();

    return (first < last) ? std::string{first, last} : std::string{};
}

std::string toUpper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return text;
}

std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> fields;
    std::size_t start{0};

    while (true) {
        const std::size_t position{text.find(delimiter, start)};
        // When position is npos, substr() simply takes the rest of the string.
        fields.push_back(trim(text.substr(start, position - start)));
        if (position == std::string::npos) {
            break;
        }
        start = position + 1;
    }
    return fields;
}

double parseAmount(const std::string& text) {
    const std::string cleaned{trim(text)};
    std::size_t charactersUsed{0};
    double value{0.0};

    try {
        value = std::stod(cleaned, &charactersUsed);
    } catch (const std::logic_error&) {  // std::invalid_argument or std::out_of_range
        throw std::invalid_argument{"'" + text + "' is not a valid amount"};
    }

    // Reject partially numeric text such as "150abc" as well as NaN/infinity.
    if (charactersUsed != cleaned.size() || !std::isfinite(value)) {
        throw std::invalid_argument{"'" + text + "' is not a valid amount"};
    }
    return value;
}

std::string formatAmount(double amount) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << amount;
    return stream.str();
}

std::string formatCurrency(double amount) {
    return std::string{config::kCurrency} + ' ' + formatAmount(amount);
}

}  // namespace textutils

// =============================================================================
// 3. EventType conversions
// =============================================================================

std::string toString(EventType eventType) {
    switch (eventType) {
        case EventType::Workshop:
            return "Workshop";
        case EventType::Seminar:
            return "Seminar";
        case EventType::Competition:
            return "Competition";
    }
    return "Unknown";  // Not reachable: every enumerator is handled above.
}

EventType parseEventType(const std::string& text) {
    const std::string normalised{textutils::toUpper(textutils::trim(text))};

    if (normalised == "WORKSHOP") {
        return EventType::Workshop;
    }
    if (normalised == "SEMINAR") {
        return EventType::Seminar;
    }
    if (normalised == "COMPETITION") {
        return EventType::Competition;
    }
    throw std::invalid_argument{"unknown event type '" + text +
                                "' (expected Workshop, Seminar or Competition)"};
}

// =============================================================================
// 4. Exception classes - each constructor builds a clear message for what()
// =============================================================================

EventSystemException::EventSystemException(const std::string& message)
    : std::runtime_error{message} {}

InvalidEventException::InvalidEventException(const std::string& eventId)
    : EventSystemException{"Invalid event ID '" + eventId + "': no such event exists."},
      eventId_{eventId} {}

const std::string& InvalidEventException::getEventId() const {
    return eventId_;
}

InvalidParticipantException::InvalidParticipantException(const std::string& participantId)
    : EventSystemException{"Invalid participant ID '" + participantId +
                           "': no such participant exists."},
      participantId_{participantId} {}

const std::string& InvalidParticipantException::getParticipantId() const {
    return participantId_;
}

DuplicateRegistrationException::DuplicateRegistrationException(const std::string& participantId,
                                                               const std::string& eventId)
    : EventSystemException{"Duplicate registration: participant " + participantId +
                           " is already registered for event " + eventId + "."},
      participantId_{participantId},
      eventId_{eventId} {}

const std::string& DuplicateRegistrationException::getParticipantId() const {
    return participantId_;
}

const std::string& DuplicateRegistrationException::getEventId() const {
    return eventId_;
}

RegistrationNotFoundException::RegistrationNotFoundException(const std::string& participantId,
                                                             const std::string& eventId)
    : EventSystemException{"Registration not found: participant " + participantId +
                           " is not registered for event " + eventId + "."},
      participantId_{participantId},
      eventId_{eventId} {}

const std::string& RegistrationNotFoundException::getParticipantId() const {
    return participantId_;
}

const std::string& RegistrationNotFoundException::getEventId() const {
    return eventId_;
}

// Delegating constructor: a file-level error is a line-specific error with line 0
// [Ref 1: "Programming with Class I, II and III" - Delegating Constructor].
FileProcessingException::FileProcessingException(const std::string& fileName,
                                                 const std::string& details)
    : FileProcessingException{fileName, 0, details} {}

FileProcessingException::FileProcessingException(const std::string& fileName,
                                                 std::size_t lineNumber,
                                                 const std::string& details)
    : EventSystemException{buildMessage(fileName, lineNumber, details)},
      fileName_{fileName},
      lineNumber_{lineNumber} {}

const std::string& FileProcessingException::getFileName() const {
    return fileName_;
}

std::size_t FileProcessingException::getLineNumber() const {
    return lineNumber_;
}

std::string FileProcessingException::buildMessage(const std::string& fileName,
                                                  std::size_t lineNumber,
                                                  const std::string& details) {
    std::string message{"File '" + fileName + "'"};
    if (lineNumber > 0) {
        message += " (line " + std::to_string(lineNumber) + ")";
    }
    return message + ": " + details;
}

// =============================================================================
// 5. Event hierarchy
// [Ref 1] constructor initializer lists, std::move, [[nodiscard]] getters,
// friend stream operators, passing arguments to a base-class constructor.
// =============================================================================

// Member initializer list: members are initialised directly and the strings
// are moved instead of copied [Ref 1: "Programming with Class IV, V, VI" - move()].
Event::Event(std::string eventId, std::string eventName, double registrationFee)
    : eventId_{std::move(eventId)},
      eventName_{std::move(eventName)},
      registrationFee_{validateFee(registrationFee)} {
    if (eventId_.empty()) {
        throw std::invalid_argument{"event ID must not be empty"};
    }
    if (eventName_.empty()) {
        throw std::invalid_argument{"event name must not be empty"};
    }
}

const std::string& Event::getEventId() const {
    return eventId_;
}

const std::string& Event::getEventName() const {
    return eventName_;
}

double Event::getRegistrationFee() const {
    return registrationFee_;
}

std::string Event::getEventTypeName() const {
    // getEventType() is virtual: the call is resolved at run time to the
    // Workshop, Seminar or Competition implementation.
    return toString(getEventType());
}

void Event::setEventName(std::string eventName) {
    if (eventName.empty()) {
        throw std::invalid_argument{"event name must not be empty"};
    }
    eventName_ = std::move(eventName);
}

void Event::setRegistrationFee(double registrationFee) {
    registrationFee_ = validateFee(registrationFee);
}

void Event::displayDetails(std::ostream& os) const {
    os << "  Event ID   : " << eventId_ << '\n'
       << "  Event Name : " << eventName_ << '\n'
       << "  Event Type : " << getEventTypeName() << '\n'
       << "  Fee        : " << textutils::formatCurrency(registrationFee_) << '\n'
       << "  Format     : " << getSessionFormat() << '\n';
}

void Event::printTableHeader(std::ostream& os) {
    using namespace eventTable;
    const std::string feeHeading{std::string{"Fee ("} + config::kCurrency + ")"};
    os << std::left << std::setw(kIdWidth) << "ID" << std::setw(kNameWidth) << "Event Name"
       << std::setw(kTypeWidth) << "Type" << std::right << std::setw(kFeeWidth) << feeHeading
       << '\n'
       << divider(kIdWidth + kNameWidth + kTypeWidth + kFeeWidth) << '\n';
}

double Event::validateFee(double registrationFee) {
    if (registrationFee < 0.0) {
        throw std::invalid_argument{"registration fee cannot be negative"};
    }
    return registrationFee;
}

std::ostream& operator<<(std::ostream& os, const Event& event) {
    using namespace eventTable;
    os << std::left << std::setw(kIdWidth) << event.eventId_
       << std::setw(kNameWidth) << event.eventName_
       << std::setw(kTypeWidth) << event.getEventTypeName()
       << std::right << std::setw(kFeeWidth) << textutils::formatAmount(event.registrationFee_);
    return os;
}

// ----- Workshop -----
Workshop::Workshop(std::string eventId, std::string eventName, double registrationFee)
    : Event{std::move(eventId), std::move(eventName), registrationFee} {}

EventType Workshop::getEventType() const {
    return EventType::Workshop;
}

std::string Workshop::getTicketPrefix() const {
    return "WKS";
}

std::string Workshop::getSessionFormat() const {
    return "Hands-on practical session - bring your laptop";
}

// ----- Seminar -----
Seminar::Seminar(std::string eventId, std::string eventName, double registrationFee)
    : Event{std::move(eventId), std::move(eventName), registrationFee} {}

EventType Seminar::getEventType() const {
    return EventType::Seminar;
}

std::string Seminar::getTicketPrefix() const {
    return "SEM";
}

std::string Seminar::getSessionFormat() const {
    return "Expert talk with Q&A - e-certificate provided";
}

// ----- Competition -----
Competition::Competition(std::string eventId, std::string eventName, double registrationFee)
    : Event{std::move(eventId), std::move(eventName), registrationFee} {}

EventType Competition::getEventType() const {
    return EventType::Competition;
}

std::string Competition::getTicketPrefix() const {
    return "CMP";
}

std::string Competition::getSessionFormat() const {
    return "Judged contest - check in 30 minutes early";
}

// =============================================================================
// 6. EventFactory  [Ref 2: std::make_unique, std::unique_ptr; Ref 4: Factory Method]
// =============================================================================

std::unique_ptr<Event> EventFactory::createEvent(EventType eventType,
                                                 std::string eventId,
                                                 std::string eventName,
                                                 double registrationFee) {
    // std::make_unique allocates the derived object; the returned
    // std::unique_ptr<Derived> converts implicitly to std::unique_ptr<Event>.
    switch (eventType) {
        case EventType::Workshop:
            return std::make_unique<Workshop>(std::move(eventId), std::move(eventName),
                                              registrationFee);
        case EventType::Seminar:
            return std::make_unique<Seminar>(std::move(eventId), std::move(eventName),
                                             registrationFee);
        case EventType::Competition:
            return std::make_unique<Competition>(std::move(eventId), std::move(eventName),
                                                 registrationFee);
    }
    throw std::invalid_argument{"unsupported event type"};
}

// =============================================================================
// 7. Participant
// =============================================================================

Participant::Participant(std::string participantId, std::string participantName)
    : participantId_{std::move(participantId)}, participantName_{std::move(participantName)} {
    if (participantId_.empty()) {
        throw std::invalid_argument{"participant ID must not be empty"};
    }
    if (participantName_.empty()) {
        throw std::invalid_argument{"participant name must not be empty"};
    }
}

const std::string& Participant::getParticipantId() const {
    return participantId_;
}

const std::string& Participant::getParticipantName() const {
    return participantName_;
}

// Pass-by-value + std::move: an efficient setter for both lvalue and rvalue
// arguments [Ref 1: "Programming with Class IV, V, VI" - Efficient Setter Functions].
void Participant::setParticipantName(std::string participantName) {
    if (participantName.empty()) {
        throw std::invalid_argument{"participant name must not be empty"};
    }
    participantName_ = std::move(participantName);
}

void Participant::printTableHeader(std::ostream& os) {
    using namespace participantTable;
    os << std::left << std::setw(kIdWidth) << "ID" << "Participant Name" << '\n'
       << divider(kIdWidth + kNameWidth) << '\n';
}

std::ostream& operator<<(std::ostream& os, const Participant& participant) {
    using namespace participantTable;
    os << std::left << std::setw(kIdWidth) << participant.participantId_
       << participant.participantName_;
    return os;
}

// =============================================================================
// 8. Ticket
// =============================================================================

Ticket::Ticket(std::string ticketNumber, double amountPaid)
    : ticketNumber_{std::move(ticketNumber)}, amountPaid_{amountPaid} {
    if (ticketNumber_.empty()) {
        throw std::invalid_argument{"ticket number must not be empty"};
    }
    if (amountPaid_ < 0.0) {
        throw std::invalid_argument{"ticket amount cannot be negative"};
    }
}

// Delegating constructor: reuses the validation in the constructor above.
Ticket::Ticket(const Event& event, const Participant& participant)
    : Ticket{buildTicketNumber(event, participant), event.getRegistrationFee()} {}

const std::string& Ticket::getTicketNumber() const {
    return ticketNumber_;
}

double Ticket::getAmountPaid() const {
    return amountPaid_;
}

std::string Ticket::buildTicketNumber(const Event& event, const Participant& participant) {
    // getTicketPrefix() is virtual, so "WKS", "SEM" or "CMP" is chosen at run time.
    return event.getTicketPrefix() + "-" + event.getEventId() + "-" +
           participant.getParticipantId();
}

std::ostream& operator<<(std::ostream& os, const Ticket& ticket) {
    os << ticket.ticketNumber_;
    return os;
}

// =============================================================================
// 9. Registration
// [Ref 1] assert() for object invariants; operator overloading; friend functions.
// =============================================================================

namespace {
// Prints one "| label : value |" line of the ticket box.
void printTicketLine(std::ostream& os, const std::string& label, const std::string& value) {
    using namespace ticketLayout;
    std::ostringstream text;
    text << std::left << std::setw(kLabelWidth) << label << " : " << value;
    os << "| " << std::left << std::setw(static_cast<int>(kInnerWidth)) << text.str() << " |\n";
}
}  // namespace

Registration::Registration(const Participant& participant, const Event& event)
    : participant_{&participant}, event_{&event}, ticket_{event, participant} {}

const Participant& Registration::getParticipant() const {
    // Invariant: a registration always refers to a participant.
    assert(participant_ != nullptr);
    return *participant_;
}

const Event& Registration::getEvent() const {
    // Invariant: a registration always refers to an event.
    assert(event_ != nullptr);
    return *event_;
}

const Ticket& Registration::getTicket() const {
    return ticket_;
}

const std::string& Registration::getParticipantId() const {
    return getParticipant().getParticipantId();
}

const std::string& Registration::getEventId() const {
    return getEvent().getEventId();
}

std::string Registration::toRecord() const {
    return getParticipantId() + config::kFieldDelimiter + getEventId();
}

void Registration::printTicket(std::ostream& os) const {
    using namespace ticketLayout;
    const Event& event = getEvent();
    const Participant& participant = getParticipant();

    const std::string border{"+" + std::string(kInnerWidth + 2, '-') + "+"};
    const std::string title{"EVENT ADMISSION TICKET"};
    const std::size_t leftPadding{(kInnerWidth - title.size()) / 2};

    os << border << '\n'
       << "| " << std::string(leftPadding, ' ') << std::left
       << std::setw(static_cast<int>(kInnerWidth - leftPadding)) << title << " |\n"
       << border << '\n';

    printTicketLine(os, "Ticket No", ticket_.getTicketNumber());
    printTicketLine(os, "Participant",
                    participant.getParticipantName() + " (" + participant.getParticipantId() + ")");
    printTicketLine(os, "Event", event.getEventName() + " (" + event.getEventId() + ")");
    printTicketLine(os, "Event Type", event.getEventTypeName());  // virtual dispatch
    printTicketLine(os, "Fee Paid", textutils::formatCurrency(ticket_.getAmountPaid()));
    printTicketLine(os, "Format", event.getSessionFormat());      // virtual dispatch

    os << border << '\n';
}

void Registration::printTableHeader(std::ostream& os) {
    using namespace registrationTable;
    os << std::left << std::setw(kTicketWidth) << "Ticket No" << std::setw(kParticipantIdWidth)
       << "P.ID" << std::setw(kParticipantNameWidth) << "Participant" << std::setw(kEventIdWidth)
       << "E.ID" << "Event" << '\n'
       << divider(kTicketWidth + kParticipantIdWidth + kParticipantNameWidth + kEventIdWidth +
                  kEventNameWidth)
       << '\n';
}

bool operator==(const Registration& lhs, const Registration& rhs) {
    // The ticket is derived data, so only the participant and event IDs are compared.
    return lhs.participant_->getParticipantId() == rhs.participant_->getParticipantId() &&
           lhs.event_->getEventId() == rhs.event_->getEventId();
}

bool operator!=(const Registration& lhs, const Registration& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Registration& registration) {
    using namespace registrationTable;
    os << std::left << std::setw(kTicketWidth) << registration.ticket_  // Ticket's operator<<
       << std::setw(kParticipantIdWidth) << registration.participant_->getParticipantId()
       << std::setw(kParticipantNameWidth) << registration.participant_->getParticipantName()
       << std::setw(kEventIdWidth) << registration.event_->getEventId()
       << registration.event_->getEventName();
    return os;
}

// =============================================================================
// 10. RegistrationStats
// =============================================================================

RegistrationStats::RegistrationStats(int registrationCount, double totalFees)
    : registrationCount_{registrationCount}, totalFees_{totalFees} {
    if (registrationCount_ < 0 || totalFees_ < 0.0) {
        throw std::invalid_argument{"registration statistics cannot be negative"};
    }
}

int RegistrationStats::getRegistrationCount() const {
    return registrationCount_;
}

double RegistrationStats::getTotalFees() const {
    return totalFees_;
}

RegistrationStats& RegistrationStats::operator+=(const Event& event) {
    ++registrationCount_;
    totalFees_ += event.getRegistrationFee();
    return *this;  // return *this so that calls can be chained
}

RegistrationStats operator+(const RegistrationStats& lhs, const RegistrationStats& rhs) {
    // As a friend, operator+ can read the private members of both operands.
    return RegistrationStats{lhs.registrationCount_ + rhs.registrationCount_,
                             lhs.totalFees_ + rhs.totalFees_};
}

// =============================================================================
// 11. FileManager
// [Ref 2] std::ifstream / std::ofstream, std::getline, std::rename, std::remove.
// [Ref 1] "Do not use assert() for file opening errors: use if statements or
//         exceptions" (Getting Organized III-V).
// =============================================================================

FileManager::FileManager() : FileManager{config::kDefaultDataDirectory} {}

FileManager::FileManager(std::string dataDirectory) : dataDirectory_{std::move(dataDirectory)} {
    if (dataDirectory_.empty()) {
        dataDirectory_ = ".";
    }
}

const std::string& FileManager::getDataDirectory() const {
    return dataDirectory_;
}

std::string FileManager::buildPath(const std::string& fileName) const {
    const char last{dataDirectory_.back()};
    if (last == '/' || last == '\\') {
        return dataDirectory_ + fileName;
    }
    return dataDirectory_ + "/" + fileName;
}

bool FileManager::fileExists(const std::string& fileName) const {
    const std::ifstream file{buildPath(fileName)};
    return file.good();
}

std::vector<FileRecord> FileManager::readRecords(const std::string& fileName) const {
    const std::string path{buildPath(fileName)};
    std::ifstream input{path};
    if (!input) {
        throw FileProcessingException{path, "cannot open file for reading"};
    }

    std::vector<FileRecord> records;
    std::string line;
    std::size_t lineNumber{0};

    while (std::getline(input, line)) {
        ++lineNumber;
        if (lineNumber == 1 && line.compare(0, 3, kUtf8ByteOrderMark) == 0) {
            line.erase(0, 3);
        }
        // Blank lines are ignored. trim() also removes a trailing '\r', so files
        // saved with Windows (CRLF) line endings are read correctly.
        if (textutils::trim(line).empty()) {
            continue;
        }
        records.push_back(FileRecord{lineNumber, textutils::split(line, config::kFieldDelimiter)});
    }

    // eof() is the normal way for the loop to end; bad() means a real I/O error.
    if (input.bad()) {
        throw FileProcessingException{path, "an error occurred while reading the file"};
    }
    return records;
}

void FileManager::writeLines(const std::string& fileName,
                             const std::vector<std::string>& lines) const {
    const std::string path{buildPath(fileName)};
    const std::string temporaryPath{path + ".tmp"};

    {   // Inner scope: the stream is closed (RAII) before the file is renamed.
        std::ofstream output{temporaryPath, std::ios::out | std::ios::trunc};
        if (!output) {
            throw FileProcessingException{temporaryPath, "cannot open file for writing"};
        }
        // Same layout as the provided file: one record per line, no trailing newline.
        for (std::size_t index{0}; index < lines.size(); ++index) {
            output << lines[index];
            if (index + 1 < lines.size()) {
                output << '\n';
            }
        }
        output.flush();
        if (!output) {
            throw FileProcessingException{temporaryPath, "an error occurred while writing the file"};
        }
    }

    // On POSIX systems std::rename replaces the old file in one step. On Windows
    // it fails if the target exists, so the old file is removed and we retry.
    if (std::rename(temporaryPath.c_str(), path.c_str()) != 0) {
        std::remove(path.c_str());
        if (std::rename(temporaryPath.c_str(), path.c_str()) != 0) {
            throw FileProcessingException{path, "could not replace the file with the updated data"};
        }
    }
}

// =============================================================================
// 12. RegistrationSystem
// [Ref 2] std::map::find / try_emplace, std::find, std::transform, std::back_inserter.
// [Ref 3] E.2 "Throw an exception to signal that a function can't perform its task".
// =============================================================================

RegistrationSystem::RegistrationSystem(FileManager fileManager)
    : fileManager_{std::move(fileManager)} {}

void RegistrationSystem::loadAll() {
    // Registrations point to events and participants, so they are cleared
    // first and loaded last.
    registrations_.clear();
    loadWarnings_.clear();

    loadEvents();
    loadParticipants();
    loadRegistrations();

    unsavedChanges_ = false;
}

void RegistrationSystem::loadEvents() {
    events_.clear();

    for (const FileRecord& record : fileManager_.readRecords(config::kEventFileName)) {
        try {
            if (record.fields.size() != config::kEventFieldCount) {
                throw std::invalid_argument{
                    "expected 4 fields (eventId;eventName;eventType;registrationFee) but found " +
                    std::to_string(record.fields.size())};
            }
            const std::string eventId{normaliseId(record.fields[0])};
            const EventType eventType{parseEventType(record.fields[2])};
            const double registrationFee{textutils::parseAmount(record.fields[3])};

            if (events_.count(eventId) != 0) {
                throw std::invalid_argument{"duplicate event ID '" + eventId + "'"};
            }
            // The factory returns a Workshop, Seminar or Competition through a
            // std::unique_ptr<Event> (base-class pointer).
            events_.emplace(eventId, EventFactory::createEvent(eventType, eventId, record.fields[1],
                                                               registrationFee));
        } catch (const std::invalid_argument& error) {
            recordSkippedLine(
                FileProcessingException{config::kEventFileName, record.lineNumber, error.what()});
        }
    }
}

void RegistrationSystem::loadParticipants() {
    participants_.clear();

    for (const FileRecord& record : fileManager_.readRecords(config::kParticipantFileName)) {
        try {
            if (record.fields.size() != config::kParticipantFieldCount) {
                throw std::invalid_argument{
                    "expected 2 fields (participantId;participantName) but found " +
                    std::to_string(record.fields.size())};
            }
            const std::string participantId{normaliseId(record.fields[0])};

            // try_emplace only constructs the Participant if the key is new.
            if (!participants_.try_emplace(participantId, participantId, record.fields[1]).second) {
                throw std::invalid_argument{"duplicate participant ID '" + participantId + "'"};
            }
        } catch (const std::invalid_argument& error) {
            recordSkippedLine(FileProcessingException{config::kParticipantFileName,
                                                      record.lineNumber, error.what()});
        }
    }
}

void RegistrationSystem::loadRegistrations() {
    registrations_.clear();

    // A missing Registration.txt is not fatal: the system starts with no
    // registrations and the file is created on the next save.
    if (!fileManager_.fileExists(config::kRegistrationFileName)) {
        loadWarnings_.push_back(std::string{config::kRegistrationFileName} +
                                " was not found - starting with no registrations.");
        return;
    }

    for (const FileRecord& record : fileManager_.readRecords(config::kRegistrationFileName)) {
        try {
            if (record.fields.size() != config::kRegistrationFieldCount) {
                throw std::invalid_argument{"expected 2 fields (participantId;eventId) but found " +
                                            std::to_string(record.fields.size())};
            }
            const Participant& participant = findParticipant(record.fields[0]);
            const Event& event = findEvent(record.fields[1]);
            addRegistration(participant, event);
        } catch (const std::invalid_argument& error) {
            recordSkippedLine(FileProcessingException{config::kRegistrationFileName,
                                                      record.lineNumber, error.what()});
        } catch (const EventSystemException& error) {
            // Unknown participant/event ID or a duplicate line in the file.
            recordSkippedLine(FileProcessingException{config::kRegistrationFileName,
                                                      record.lineNumber, error.what()});
        }
    }
}

void RegistrationSystem::saveRegistrations() {
    std::vector<std::string> lines;
    lines.reserve(registrations_.size());
    std::transform(registrations_.cbegin(), registrations_.cend(), std::back_inserter(lines),
                   [](const Registration& registration) { return registration.toRecord(); });

    fileManager_.writeLines(config::kRegistrationFileName, lines);
    unsavedChanges_ = false;
}

const Event& RegistrationSystem::findEvent(const std::string& eventId) const {
    const std::string key{normaliseId(eventId)};
    const auto found = events_.find(key);
    if (found == events_.end()) {
        throw InvalidEventException{key};
    }
    return *found->second;
}

const Participant& RegistrationSystem::findParticipant(const std::string& participantId) const {
    const std::string key{normaliseId(participantId)};
    const auto found = participants_.find(key);
    if (found == participants_.end()) {
        throw InvalidParticipantException{key};
    }
    return found->second;
}

const Registration& RegistrationSystem::findRegistration(const std::string& participantId,
                                                         const std::string& eventId) const {
    const Participant& participant = findParticipant(participantId);
    const Event& event = findEvent(eventId);
    const Registration target{participant, event};

    // std::find compares elements with the overloaded operator==.
    const auto found = std::find(registrations_.cbegin(), registrations_.cend(), target);
    if (found == registrations_.cend()) {
        throw RegistrationNotFoundException{participant.getParticipantId(), event.getEventId()};
    }
    return *found;
}

const Registration& RegistrationSystem::registerParticipant(const std::string& participantId,
                                                            const std::string& eventId) {
    const Participant& participant = findParticipant(participantId);
    const Event& event = findEvent(eventId);
    const Registration& added = addRegistration(participant, event);
    unsavedChanges_ = true;
    return added;
}

Registration RegistrationSystem::cancelRegistration(const std::string& participantId,
                                                    const std::string& eventId) {
    const Participant& participant = findParticipant(participantId);
    const Event& event = findEvent(eventId);
    const Registration target{participant, event};

    const auto position = std::find(registrations_.begin(), registrations_.end(), target);
    if (position == registrations_.end()) {
        throw RegistrationNotFoundException{participant.getParticipantId(), event.getEventId()};
    }

    Registration cancelled{*position};  // keep a copy for the confirmation message
    registrations_.erase(position);
    unsavedChanges_ = true;
    return cancelled;
}

const Registration& RegistrationSystem::addRegistration(const Participant& participant,
                                                        const Event& event) {
    Registration candidate{participant, event};

    // Duplicate check uses the overloaded operator== of Registration.
    if (std::find(registrations_.cbegin(), registrations_.cend(), candidate) !=
        registrations_.cend()) {
        throw DuplicateRegistrationException{participant.getParticipantId(), event.getEventId()};
    }
    registrations_.push_back(std::move(candidate));
    return registrations_.back();
}

std::vector<const Registration*> RegistrationSystem::getRegistrationsForEvent(
    const std::string& eventId) const {
    const std::string key{normaliseId(eventId)};
    std::vector<const Registration*> matches;
    for (const Registration& registration : registrations_) {
        if (registration.getEventId() == key) {
            matches.push_back(&registration);
        }
    }
    return matches;
}

std::vector<const Registration*> RegistrationSystem::getRegistrationsForParticipant(
    const std::string& participantId) const {
    const std::string key{normaliseId(participantId)};
    std::vector<const Registration*> matches;
    for (const Registration& registration : registrations_) {
        if (registration.getParticipantId() == key) {
            matches.push_back(&registration);
        }
    }
    return matches;
}

const std::map<std::string, std::unique_ptr<Event>>& RegistrationSystem::getEvents() const {
    return events_;
}

const std::map<std::string, Participant>& RegistrationSystem::getParticipants() const {
    return participants_;
}

const std::vector<Registration>& RegistrationSystem::getRegistrations() const {
    return registrations_;
}

const std::vector<std::string>& RegistrationSystem::getLoadWarnings() const {
    return loadWarnings_;
}

const std::string& RegistrationSystem::getDataDirectory() const {
    return fileManager_.getDataDirectory();
}

std::string RegistrationSystem::getRegistrationFilePath() const {
    return fileManager_.buildPath(config::kRegistrationFileName);
}

bool RegistrationSystem::hasUnsavedChanges() const {
    return unsavedChanges_;
}

void RegistrationSystem::recordSkippedLine(const FileProcessingException& problem) {
    loadWarnings_.push_back(std::string{problem.what()} + " -> record skipped");
}

std::string RegistrationSystem::normaliseId(const std::string& id) {
    return textutils::toUpper(textutils::trim(id));
}

// =============================================================================
// 13. ReportGenerator
// [Ref 2] std::accumulate (used with the overloaded operator+), std::setw.
// =============================================================================

namespace {
// Returns "1.", "2.", ... for the row-number column.
std::string rowLabel(int rowNumber) {
    return std::to_string(rowNumber) + ".";
}

// Returns e.g. "Fee (RM)".
std::string currencyHeading(const std::string& label) {
    return label + " (" + config::kCurrency + ")";
}
}  // namespace

ReportGenerator::ReportGenerator(const RegistrationSystem& system) : system_{system} {}

// (a) Participants registered for a particular event
void ReportGenerator::printParticipantsForEvent(std::ostream& os,
                                                const std::string& eventId) const {
    using namespace reportTable;
    const Event& event = system_.findEvent(eventId);  // may throw InvalidEventException
    const auto registrations = system_.getRegistrationsForEvent(event.getEventId());

    printTitle(os, "Participants Registered for " + event.getEventId() + " - " +
                       event.getEventName());
    event.displayDetails(os);
    os << '\n';

    if (registrations.empty()) {
        os << "No participants are currently registered for this event.\n";
        return;
    }

    os << std::left << std::setw(kNumberWidth) << "No." << std::setw(kIdWidth) << "ID"
       << std::setw(kPersonNameWidth) << "Participant Name" << "Ticket No" << '\n'
       << divider(kNumberWidth + kIdWidth + kPersonNameWidth + 16) << '\n';

    int rowNumber{0};
    for (const Registration* registration : registrations) {
        const Participant& participant = registration->getParticipant();
        os << std::left << std::setw(kNumberWidth) << rowLabel(++rowNumber)
           << std::setw(kIdWidth) << participant.getParticipantId()
           << std::setw(kPersonNameWidth) << participant.getParticipantName()
           << registration->getTicket() << '\n';
    }
    os << "\nTotal participants registered: " << registrations.size() << '\n';
}

// (b) Registration summary for a participant
void ReportGenerator::printParticipantSummary(std::ostream& os,
                                              const std::string& participantId) const {
    using namespace reportTable;
    const Participant& participant = system_.findParticipant(participantId);  // may throw
    const auto registrations = system_.getRegistrationsForParticipant(participant.getParticipantId());

    printTitle(os, "Registration Summary for " + participant.getParticipantId() + " - " +
                       participant.getParticipantName());

    if (registrations.empty()) {
        os << "This participant has not registered for any events.\n";
        return;
    }

    os << std::left << std::setw(kNumberWidth) << "No." << std::setw(kIdWidth) << "ID"
       << std::setw(kEventNameWidth) << "Event Name" << std::setw(kTypeWidth) << "Type"
       << std::right << std::setw(kFeeWidth) << currencyHeading("Fee") << "  " << std::left
       << "Ticket No" << '\n'
       << divider(kNumberWidth + kIdWidth + kEventNameWidth + kTypeWidth + kFeeWidth + 2 + 13)
       << '\n';

    RegistrationStats participantStats;  // starts at zero (default constructor)
    int rowNumber{0};
    for (const Registration* registration : registrations) {
        const Event& event = registration->getEvent();
        participantStats += event;  // overloaded operator+=

        os << std::left << std::setw(kNumberWidth) << rowLabel(++rowNumber)
           << std::setw(kIdWidth) << event.getEventId()
           << std::setw(kEventNameWidth) << event.getEventName()
           << std::setw(kTypeWidth) << event.getEventTypeName()
           << std::right << std::setw(kFeeWidth) << textutils::formatAmount(event.getRegistrationFee())
           << "  " << registration->getTicket() << '\n';
    }

    os << '\n'
       << "Events registered  : " << participantStats.getRegistrationCount() << '\n'
       << "Total fees payable : " << textutils::formatCurrency(participantStats.getTotalFees())
       << '\n';
}

// (c) Total number of registrations for each event
void ReportGenerator::printRegistrationCounts(std::ostream& os) const {
    using namespace reportTable;
    const auto statistics = buildEventStatistics();
    const int tableWidth{kIdWidth + kEventNameWidth + kTypeWidth + kCountWidth};

    printTitle(os, "Total Number of Registrations for Each Event");
    os << std::left << std::setw(kIdWidth) << "ID" << std::setw(kEventNameWidth) << "Event Name"
       << std::setw(kTypeWidth) << "Type" << std::right << std::setw(kCountWidth) << "Registrations"
       << '\n'
       << divider(tableWidth) << '\n';

    for (const auto& entry : statistics) {
        const Event& event = system_.findEvent(entry.first);
        os << std::left << std::setw(kIdWidth) << event.getEventId()
           << std::setw(kEventNameWidth) << event.getEventName()
           << std::setw(kTypeWidth) << event.getEventTypeName()
           << std::right << std::setw(kCountWidth) << entry.second.getRegistrationCount() << '\n';
    }

    const RegistrationStats grandTotal{calculateGrandTotal(statistics)};
    os << divider(tableWidth) << '\n'
       << std::left << std::setw(kIdWidth + kEventNameWidth + kTypeWidth) << "TOTAL"
       << std::right << std::setw(kCountWidth) << grandTotal.getRegistrationCount() << '\n';
}

// (d) Total registration fees collected for each event
void ReportGenerator::printFeesCollected(std::ostream& os) const {
    using namespace reportTable;
    const auto statistics = buildEventStatistics();
    const int tableWidth{kIdWidth + kEventNameWidth + kFeeWidth + kCountWidth + kTotalWidth};

    printTitle(os, "Total Registration Fees Collected for Each Event");
    os << std::left << std::setw(kIdWidth) << "ID" << std::setw(kEventNameWidth) << "Event Name"
       << std::right << std::setw(kFeeWidth) << currencyHeading("Fee")
       << std::setw(kCountWidth) << "Registrations"
       << std::setw(kTotalWidth) << currencyHeading("Collected") << '\n'
       << divider(tableWidth) << '\n';

    for (const auto& entry : statistics) {
        const Event& event = system_.findEvent(entry.first);
        os << std::left << std::setw(kIdWidth) << event.getEventId()
           << std::setw(kEventNameWidth) << event.getEventName()
           << std::right << std::setw(kFeeWidth) << textutils::formatAmount(event.getRegistrationFee())
           << std::setw(kCountWidth) << entry.second.getRegistrationCount()
           << std::setw(kTotalWidth) << textutils::formatAmount(entry.second.getTotalFees()) << '\n';
    }

    const RegistrationStats grandTotal{calculateGrandTotal(statistics)};
    os << divider(tableWidth) << '\n'
       << std::left << std::setw(kIdWidth + kEventNameWidth + kFeeWidth) << "GRAND TOTAL"
       << std::right << std::setw(kCountWidth) << grandTotal.getRegistrationCount()
       << std::setw(kTotalWidth) << textutils::formatAmount(grandTotal.getTotalFees()) << '\n';
}

std::map<std::string, RegistrationStats> ReportGenerator::buildEventStatistics() const {
    std::map<std::string, RegistrationStats> statistics;

    // Start every event at zero so events without registrations still appear.
    for (const auto& entry : system_.getEvents()) {
        statistics.emplace(entry.first, RegistrationStats{});
    }
    // Each registration adds one to the count and its fee to the total.
    for (const Registration& registration : system_.getRegistrations()) {
        statistics[registration.getEventId()] += registration.getEvent();  // operator+=
    }
    return statistics;
}

RegistrationStats ReportGenerator::calculateGrandTotal(
    const std::map<std::string, RegistrationStats>& statistics) {
    // std::accumulate repeatedly applies the overloaded operator+ (friend function).
    return std::accumulate(statistics.cbegin(), statistics.cend(), RegistrationStats{},
                           [](const RegistrationStats& runningTotal, const auto& entry) {
                               return runningTotal + entry.second;
                           });
}

void ReportGenerator::printTitle(std::ostream& os, const std::string& title) {
    os << '\n' << title << '\n' << std::string(title.size(), '=') << '\n';
}

// =============================================================================
// 14. ConsoleMenu
//
// Error handling strategy: the business classes THROW exceptions when an
// operation cannot be completed; the menu CATCHES them, explains the problem
// and lets the program continue [Ref 1: "Programming with Class VII and VIII"
// - exceptions separate error detection from error recovery].
// [Ref 2] std::optional, std::getline, std::stoi, std::count_if.
// =============================================================================

ConsoleMenu::ConsoleMenu(RegistrationSystem& system) : ConsoleMenu{system, std::cin, std::cout} {}

ConsoleMenu::ConsoleMenu(RegistrationSystem& system, std::istream& input, std::ostream& output)
    : system_{system}, reportGenerator_{system}, input_{input}, output_{output} {}

void ConsoleMenu::run() {
    printWelcome();

    bool running{true};
    while (running) {
        printMenu();
        const std::optional<std::string> choiceText = prompt("Enter your choice: ");
        if (!choiceText) {
            output_ << "\nInput ended - exiting the program.\n";
            if (system_.hasUnsavedChanges()) {
                output_ << "Warning: unsaved changes were not written to the file.\n";
            }
            break;
        }

        const std::optional<MenuOption> option = parseMenuOption(*choiceText);
        if (!option) {
            output_ << "Invalid choice '" << *choiceText << "'. Please enter a number from 0 to "
                    << kHighestOption << ".\n";
            continue;
        }
        if (*option == MenuOption::Exit) {
            running = !confirmExit();
            continue;
        }

        // Each action may throw; the most specific handlers come first.
        try {
            handleOption(*option);
        } catch (const InvalidParticipantException& error) {
            output_ << "Error: " << error.what() << "\n       Tip: option 2 lists all participant IDs.\n";
        } catch (const InvalidEventException& error) {
            output_ << "Error: " << error.what() << "\n       Tip: option 1 lists all event IDs.\n";
        } catch (const DuplicateRegistrationException& error) {
            output_ << "Error: " << error.what()
                    << "\n       A participant may register for many events, but only once per event.\n";
        } catch (const RegistrationNotFoundException& error) {
            output_ << "Error: " << error.what() << '\n';
        } catch (const FileProcessingException& error) {
            output_ << "File error: " << error.what() << '\n';
        } catch (const EventSystemException& error) {
            output_ << "Error: " << error.what() << '\n';
        }
    }
    output_ << "Thank you for using the Event Registration and Ticket Management System.\n";
}

void ConsoleMenu::printWelcome() const {
    const std::string line(static_cast<std::size_t>(kBannerWidth), '=');
    output_ << line << '\n'
            << "        EVENT REGISTRATION AND TICKET MANAGEMENT SYSTEM\n"
            << line << '\n'
            << "Data directory : " << system_.getDataDirectory() << '\n'
            << "Loaded         : " << system_.getEvents().size() << " events, "
            << system_.getParticipants().size() << " participants, "
            << system_.getRegistrations().size() << " registrations\n";

    const auto& warnings = system_.getLoadWarnings();
    if (!warnings.empty()) {
        output_ << "\nWarnings while loading data (" << warnings.size() << "):\n";
        for (const std::string& warning : warnings) {
            output_ << "  - " << warning << '\n';
        }
    }
}

void ConsoleMenu::printMenu() const {
    output_ << '\n'
            << "------------------------- MAIN MENU -------------------------\n"
            << " Event & Participant Management\n"
            << "    1. Display all events\n"
            << "    2. Display all participants\n"
            << " Registration Management\n"
            << "    3. Display all registrations\n"
            << "    4. Register a participant for an event\n"
            << "    5. Cancel a registration\n"
            << "    6. Print a ticket\n"
            << " Reports\n"
            << "    7. Participants registered for an event\n"
            << "    8. Registration summary for a participant\n"
            << "    9. Total registrations for each event\n"
            << "   10. Total registration fees collected for each event\n"
            << " File\n"
            << "   11. Save changes to " << config::kRegistrationFileName
            << (system_.hasUnsavedChanges() ? "   [* unsaved changes]" : "") << '\n'
            << "    0. Exit\n"
            << "-------------------------------------------------------------\n";
}

void ConsoleMenu::printHeading(const std::string& title) const {
    output_ << '\n' << title << '\n' << std::string(title.size(), '=') << '\n';
}

std::optional<std::string> ConsoleMenu::prompt(const std::string& message) {
    output_ << message << std::flush;
    std::string line;
    if (!std::getline(input_, line)) {
        return std::nullopt;
    }
    return textutils::trim(line);
}

std::optional<ConsoleMenu::MenuOption> ConsoleMenu::parseMenuOption(const std::string& text) {
    try {
        std::size_t charactersUsed{0};
        const int value{std::stoi(text, &charactersUsed)};
        if (charactersUsed == text.size() && value >= 0 && value <= kHighestOption) {
            return static_cast<MenuOption>(value);
        }
    } catch (const std::logic_error&) {
        // Not a number (std::invalid_argument) or too large (std::out_of_range).
    }
    return std::nullopt;
}

bool ConsoleMenu::confirm(const std::string& question) {
    while (true) {
        const auto answer = prompt(question + " (Y/N): ");
        if (!answer) {
            return false;
        }
        const std::string choice{textutils::toUpper(*answer)};
        if (choice == "Y" || choice == "YES") {
            return true;
        }
        if (choice == "N" || choice == "NO") {
            return false;
        }
        output_ << "Please answer Y or N.\n";
    }
}

void ConsoleMenu::handleOption(MenuOption option) {
    switch (option) {
        case MenuOption::DisplayEvents:
            displayEvents();
            break;
        case MenuOption::DisplayParticipants:
            displayParticipants();
            break;
        case MenuOption::DisplayRegistrations:
            displayRegistrations();
            break;
        case MenuOption::RegisterParticipant:
            registerParticipant();
            break;
        case MenuOption::CancelRegistration:
            cancelRegistration();
            break;
        case MenuOption::PrintTicket:
            printTicket();
            break;
        case MenuOption::ReportParticipantsForEvent:
            reportParticipantsForEvent();
            break;
        case MenuOption::ReportParticipantSummary:
            reportParticipantSummary();
            break;
        case MenuOption::ReportRegistrationCounts:
            reportGenerator_.printRegistrationCounts(output_);
            break;
        case MenuOption::ReportFeesCollected:
            reportGenerator_.printFeesCollected(output_);
            break;
        case MenuOption::SaveChanges:
            saveChanges();
            break;
        case MenuOption::Exit:
            break;  // handled in run()
    }
}

void ConsoleMenu::displayEvents() const {
    printHeading("Available Events");
    Event::printTableHeader(output_);

    const auto& events = system_.getEvents();
    for (const auto& entry : events) {
        // entry.second is a std::unique_ptr<Event>; *entry.second is an Event&
        // that refers to a Workshop, Seminar or Competition object.
        output_ << *entry.second << '\n';
    }

    // Runtime polymorphism: getEventType() is resolved through the base-class pointer.
    const auto countOfType = [&events](EventType type) {
        return std::count_if(events.cbegin(), events.cend(), [type](const auto& entry) {
            return entry.second->getEventType() == type;
        });
    };
    output_ << "\nTotal events: " << events.size() << "  (Workshops: "
            << countOfType(EventType::Workshop) << ", Seminars: " << countOfType(EventType::Seminar)
            << ", Competitions: " << countOfType(EventType::Competition) << ")\n";
}

void ConsoleMenu::displayParticipants() const {
    printHeading("Participant Records");
    Participant::printTableHeader(output_);
    for (const auto& entry : system_.getParticipants()) {
        output_ << entry.second << '\n';  // overloaded operator<< (friend)
    }
    output_ << "\nTotal participants: " << system_.getParticipants().size() << '\n';
}

void ConsoleMenu::displayRegistrations() const {
    printHeading("Registration Records");
    const auto& registrations = system_.getRegistrations();
    if (registrations.empty()) {
        output_ << "There are no registrations.\n";
        return;
    }
    Registration::printTableHeader(output_);
    for (const Registration& registration : registrations) {
        output_ << registration << '\n';  // overloaded operator<< (friend)
    }
    output_ << "\nTotal registrations: " << registrations.size() << '\n';
}

void ConsoleMenu::registerParticipant() {
    printHeading("Register a Participant for an Event");

    const auto participantId = prompt("Enter participant ID (e.g. P001): ");
    if (!participantId) {
        return;
    }
    // Validate the participant straight away so the user gets early feedback.
    const Participant& participant = system_.findParticipant(*participantId);
    output_ << "Participant: " << participant.getParticipantName() << '\n';

    const auto eventId = prompt("Enter event ID (e.g. E001): ");
    if (!eventId) {
        return;
    }
    const Registration& registration =
        system_.registerParticipant(participant.getParticipantId(), *eventId);

    output_ << "\nRegistration successful! The following ticket has been issued:\n";
    registration.printTicket(output_);
    output_ << "Remember to save your changes (option 11).\n";
}

void ConsoleMenu::cancelRegistration() {
    printHeading("Cancel a Registration");

    const auto participantId = prompt("Enter participant ID (e.g. P001): ");
    if (!participantId) {
        return;
    }
    const Participant& participant = system_.findParticipant(*participantId);
    const auto current = system_.getRegistrationsForParticipant(participant.getParticipantId());
    if (current.empty()) {
        output_ << participant.getParticipantName() << " has no registrations to cancel.\n";
        return;
    }

    output_ << "Current registrations for " << participant.getParticipantName() << ":\n";
    for (const Registration* registration : current) {
        output_ << "  - " << registration->getEventId() << "  "
                << registration->getEvent().getEventName() << "  (ticket "
                << registration->getTicket() << ")\n";
    }

    const auto eventId = prompt("Enter the event ID to cancel: ");
    if (!eventId) {
        return;
    }
    // findRegistration() throws if the participant is not registered for this event.
    const Registration& existing =
        system_.findRegistration(participant.getParticipantId(), *eventId);
    if (!confirm("Cancel " + participant.getParticipantName() + "'s registration for " +
                 existing.getEvent().getEventName() + "?")) {
        output_ << "Cancellation aborted - nothing was changed.\n";
        return;
    }

    const Registration cancelled =
        system_.cancelRegistration(participant.getParticipantId(), *eventId);
    output_ << "Registration cancelled. Ticket " << cancelled.getTicket()
            << " is no longer valid.\n"
            << "Remember to save your changes (option 11).\n";
}

void ConsoleMenu::printTicket() {
    printHeading("Print a Ticket");

    const auto participantId = prompt("Enter participant ID (e.g. P001): ");
    if (!participantId) {
        return;
    }
    const auto eventId = prompt("Enter event ID (e.g. E001): ");
    if (!eventId) {
        return;
    }
    const Registration& registration = system_.findRegistration(*participantId, *eventId);
    output_ << '\n';
    registration.printTicket(output_);
}

void ConsoleMenu::reportParticipantsForEvent() {
    const auto eventId = prompt("Enter event ID (e.g. E001): ");
    if (!eventId) {
        return;
    }
    reportGenerator_.printParticipantsForEvent(output_, *eventId);
}

void ConsoleMenu::reportParticipantSummary() {
    const auto participantId = prompt("Enter participant ID (e.g. P001): ");
    if (!participantId) {
        return;
    }
    reportGenerator_.printParticipantSummary(output_, *participantId);
}

void ConsoleMenu::saveChanges() {
    system_.saveRegistrations();  // may throw FileProcessingException
    output_ << "Saved " << system_.getRegistrations().size() << " registration(s) to '"
            << system_.getRegistrationFilePath() << "'.\n";
}

bool ConsoleMenu::confirmExit() {
    if (!system_.hasUnsavedChanges()) {
        return true;
    }
    while (true) {
        const auto answer =
            prompt("You have unsaved changes. Save before exiting? (Y = save, N = discard, C = cancel): ");
        if (!answer) {
            return true;
        }
        const std::string choice{textutils::toUpper(*answer)};
        if (choice == "Y") {
            try {
                saveChanges();
                return true;
            } catch (const FileProcessingException& error) {
                output_ << "File error: " << error.what() << "\nThe program will not exit.\n";
                return false;
            }
        }
        if (choice == "N") {
            output_ << "Changes discarded.\n";
            return true;
        }
        if (choice == "C") {
            return false;
        }
        output_ << "Please enter Y, N or C.\n";
    }
}
