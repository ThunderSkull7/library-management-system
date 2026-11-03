#pragma once

#include "database/DatabaseConnection.hpp"

namespace lms::database {

/**
 * @brief RAII transaction manager for executing atomic SQLite transactions.
 *
 * Automatically issues BEGIN IMMEDIATE on construction. If not explicitly committed
 * via commit(), destructor issues ROLLBACK. Non-copyable, movable.
 */
class Transaction {
public:
    explicit Transaction(DatabaseConnection& connection);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    Transaction(Transaction&& other) noexcept;
    Transaction& operator=(Transaction&& other) noexcept;

    /**
     * @brief Commits the transaction.
     * @throws DatabaseException if commit fails or if already committed/rolled back.
     */
    void commit();

    /**
     * @brief Manually rolls back the transaction.
     */
    void rollback();

    /**
     * @brief Returns true if the transaction has been successfully committed.
     */
    [[nodiscard]] bool isCommitted() const noexcept { return committed_; }

    /**
     * @brief Returns true if the transaction is currently active (not committed and not rolled back).
     */
    [[nodiscard]] bool isActive() const noexcept { return active_; }

private:
    DatabaseConnection* connection_{nullptr};
    bool active_{false};
    bool committed_{false};
};

} // namespace lms::database
