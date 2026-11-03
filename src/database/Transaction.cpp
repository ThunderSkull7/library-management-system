#include "database/Transaction.hpp"
#include "database/DatabaseException.hpp"

#include <utility>

namespace lms::database {

Transaction::Transaction(DatabaseConnection& connection)
    : connection_(&connection), active_(false), committed_(false) {
    if (!connection_->isOpen()) {
        throw DatabaseException("Cannot begin transaction: database connection is closed.");
    }
    connection_->execute("BEGIN IMMEDIATE;");
    active_ = true;
}

Transaction::~Transaction() {
    if (active_ && !committed_ && connection_ && connection_->isOpen()) {
        try {
            connection_->execute("ROLLBACK;");
        } catch (...) {
            // Suppress rollback errors during destruction to preserve active exceptions
        }
    }
}

Transaction::Transaction(Transaction&& other) noexcept
    : connection_(std::exchange(other.connection_, nullptr)),
      active_(std::exchange(other.active_, false)),
      committed_(std::exchange(other.committed_, false)) {}

Transaction& Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        if (active_ && !committed_ && connection_ && connection_->isOpen()) {
            try {
                connection_->execute("ROLLBACK;");
            } catch (...) {}
        }
        connection_ = std::exchange(other.connection_, nullptr);
        active_ = std::exchange(other.active_, false);
        committed_ = std::exchange(other.committed_, false);
    }
    return *this;
}

void Transaction::commit() {
    if (!active_ || committed_) {
        throw DatabaseException("Cannot commit transaction: transaction is not active or already finished.");
    }
    connection_->execute("COMMIT;");
    committed_ = true;
    active_ = false;
}

void Transaction::rollback() {
    if (!active_ || committed_) {
        return;
    }
    try {
        connection_->execute("ROLLBACK;");
    } catch (...) {
        active_ = false;
        throw;
    }
    active_ = false;
}

} // namespace lms::database
