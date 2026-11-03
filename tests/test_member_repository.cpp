#include "database/DatabaseConnection.hpp"
#include "database/DatabaseException.hpp"
#include "database/SchemaManager.hpp"
#include "models/Book.hpp"
#include "models/BookCopy.hpp"
#include "models/Member.hpp"
#include "models/MemberStatus.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"
#include "repositories/MemberRepository.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

class MemberRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        connection_ = std::make_unique<lms::database::DatabaseConnection>(":memory:");
        schemaManager_ = std::make_unique<lms::database::SchemaManager>(*connection_);
        schemaManager_->initializeSchema();

        memberRepo_ = std::make_unique<lms::repositories::MemberRepository>(*connection_);
        bookRepo_ = std::make_unique<lms::repositories::BookRepository>(*connection_);
        copyRepo_ = std::make_unique<lms::repositories::BookCopyRepository>(*connection_);
    }

    void TearDown() override {
        copyRepo_.reset();
        bookRepo_.reset();
        memberRepo_.reset();
        schemaManager_.reset();
        connection_.reset();
    }

    std::unique_ptr<lms::database::DatabaseConnection> connection_;
    std::unique_ptr<lms::database::SchemaManager> schemaManager_;
    std::unique_ptr<lms::repositories::MemberRepository> memberRepo_;
    std::unique_ptr<lms::repositories::BookRepository> bookRepo_;
    std::unique_ptr<lms::repositories::BookCopyRepository> copyRepo_;
};

TEST_F(MemberRepositoryTest, AddAndRetrieveMemberById) {
    lms::models::Member member("Ada Lovelace", "2026-01-10", "ada@computing.org", "+44-1234567", lms::models::MemberStatus::Active);
    int id = memberRepo_->addMember(member);
    EXPECT_GT(id, 0);

    auto retrieved = memberRepo_->getMemberById(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getId(), id);
    EXPECT_EQ(retrieved->getFullName(), "Ada Lovelace");
    EXPECT_EQ(retrieved->getJoinedOn(), "2026-01-10");
    ASSERT_TRUE(retrieved->getEmail().has_value());
    EXPECT_EQ(*retrieved->getEmail(), "ada@computing.org");
    ASSERT_TRUE(retrieved->getPhone().has_value());
    EXPECT_EQ(*retrieved->getPhone(), "+44-1234567");
    EXPECT_EQ(retrieved->getStatus(), lms::models::MemberStatus::Active);
}

TEST_F(MemberRepositoryTest, RetrieveMemberByEmail) {
    memberRepo_->addMember(lms::models::Member("Alan Turing", "2026-01-12", "alan@bletchley.uk"));

    auto retrieved = memberRepo_->getMemberByEmail("alan@bletchley.uk");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getFullName(), "Alan Turing");

    EXPECT_FALSE(memberRepo_->getMemberByEmail("nonexistent@domain.com").has_value());
}

TEST_F(MemberRepositoryTest, GetAllMembers) {
    EXPECT_TRUE(memberRepo_->getAllMembers().empty());

    memberRepo_->addMember(lms::models::Member("Member 1", "2026-01-01"));
    memberRepo_->addMember(lms::models::Member("Member 2", "2026-01-02"));
    memberRepo_->addMember(lms::models::Member("Member 3", "2026-01-03"));

    auto all = memberRepo_->getAllMembers();
    EXPECT_EQ(all.size(), 3);
    EXPECT_EQ(all[0].getFullName(), "Member 1");
    EXPECT_EQ(all[1].getFullName(), "Member 2");
    EXPECT_EQ(all[2].getFullName(), "Member 3");
}

TEST_F(MemberRepositoryTest, SearchByName) {
    memberRepo_->addMember(lms::models::Member("Grace Hopper", "2026-01-01"));
    memberRepo_->addMember(lms::models::Member("Margaret Hamilton", "2026-01-02"));
    memberRepo_->addMember(lms::models::Member("Grace Kelly", "2026-01-03"));

    auto results = memberRepo_->searchByName("Grace");
    EXPECT_EQ(results.size(), 2);

    results = memberRepo_->searchByName("hamilton");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].getFullName(), "Margaret Hamilton");

    results = memberRepo_->searchByName("NotFound");
    EXPECT_TRUE(results.empty());
}

TEST_F(MemberRepositoryTest, FilterByStatus) {
    memberRepo_->addMember(lms::models::Member("User A", "2026-01-01", std::nullopt, std::nullopt, lms::models::MemberStatus::Active));
    memberRepo_->addMember(lms::models::Member("User B", "2026-01-02", std::nullopt, std::nullopt, lms::models::MemberStatus::Suspended));
    memberRepo_->addMember(lms::models::Member("User C", "2026-01-03", std::nullopt, std::nullopt, lms::models::MemberStatus::Active));
    memberRepo_->addMember(lms::models::Member("User D", "2026-01-04", std::nullopt, std::nullopt, lms::models::MemberStatus::Closed));

    auto active = memberRepo_->searchByStatus(lms::models::MemberStatus::Active);
    EXPECT_EQ(active.size(), 2);

    auto suspended = memberRepo_->searchByStatus(lms::models::MemberStatus::Suspended);
    EXPECT_EQ(suspended.size(), 1);
    EXPECT_EQ(suspended[0].getFullName(), "User B");

    auto closed = memberRepo_->searchByStatus(lms::models::MemberStatus::Closed);
    EXPECT_EQ(closed.size(), 1);
}

TEST_F(MemberRepositoryTest, UpdateMemberInformation) {
    int id = memberRepo_->addMember(lms::models::Member("Original Name", "2026-01-01", "orig@test.com", "111"));

    lms::models::Member updated(id, "Updated Name", "2026-01-01", "new@test.com", "222", lms::models::MemberStatus::Suspended);
    EXPECT_TRUE(memberRepo_->updateMember(updated));

    auto retrieved = memberRepo_->getMemberById(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getFullName(), "Updated Name");
    EXPECT_EQ(*retrieved->getEmail(), "new@test.com");
    EXPECT_EQ(*retrieved->getPhone(), "222");
    EXPECT_EQ(retrieved->getStatus(), lms::models::MemberStatus::Suspended);

    // Update non-existent member
    lms::models::Member nonExistent(9999, "Name", "2026-01-01");
    EXPECT_FALSE(memberRepo_->updateMember(nonExistent));
}

TEST_F(MemberRepositoryTest, UpdateMemberStatusDirectly) {
    int id = memberRepo_->addMember(lms::models::Member("Member A", "2026-01-01", std::nullopt, std::nullopt, lms::models::MemberStatus::Active));

    EXPECT_TRUE(memberRepo_->updateMemberStatus(id, lms::models::MemberStatus::Closed));

    auto retrieved = memberRepo_->getMemberById(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getStatus(), lms::models::MemberStatus::Closed);

    EXPECT_FALSE(memberRepo_->updateMemberStatus(9999, lms::models::MemberStatus::Active));
}

TEST_F(MemberRepositoryTest, RejectDuplicateNonNullEmail) {
    memberRepo_->addMember(lms::models::Member("Member 1", "2026-01-01", "duplicate@test.com"));

    EXPECT_THROW(
        memberRepo_->addMember(lms::models::Member("Member 2", "2026-01-02", "duplicate@test.com")),
        lms::database::DatabaseException);
}

TEST_F(MemberRepositoryTest, AllowMultipleNullEmails) {
    EXPECT_NO_THROW(memberRepo_->addMember(lms::models::Member("No Email 1", "2026-01-01", std::nullopt)));
    EXPECT_NO_THROW(memberRepo_->addMember(lms::models::Member("No Email 2", "2026-01-02", std::nullopt)));

    auto all = memberRepo_->getAllMembers();
    EXPECT_EQ(all.size(), 2);
}

TEST_F(MemberRepositoryTest, MissingMemberReturnsNullopt) {
    EXPECT_FALSE(memberRepo_->getMemberById(9999).has_value());
    EXPECT_FALSE(memberRepo_->getMemberByEmail("missing@test.com").has_value());
}

TEST_F(MemberRepositoryTest, DeleteMemberWithoutLoanHistorySucceeds) {
    int id = memberRepo_->addMember(lms::models::Member("Delete Candidate", "2026-01-01"));
    EXPECT_TRUE(memberRepo_->deleteMember(id));
    EXPECT_FALSE(memberRepo_->getMemberById(id).has_value());
    EXPECT_FALSE(memberRepo_->deleteMember(id)); // already deleted
}

TEST_F(MemberRepositoryTest, PreventDeleteWhenLoanHistoryExists) {
    int memberId = memberRepo_->addMember(lms::models::Member("Borrower", "2026-01-01"));
    int bookId = bookRepo_->addBook(lms::models::Book("Book", "Author"));
    int copyId = copyRepo_->addCopy(lms::models::BookCopy(bookId, "ACC-LOAN-01"));

    // Add loan record for this member
    connection_->execute(
        "INSERT INTO loans (copy_id, member_id, issued_on, due_on, returned_on) "
        "VALUES (" + std::to_string(copyId) + ", " + std::to_string(memberId) + ", '2026-01-01', '2026-01-15', '2026-01-10');");

    EXPECT_TRUE(memberRepo_->hasLoanHistory(memberId));
    EXPECT_FALSE(memberRepo_->hasActiveLoan(memberId));

    // Deleting member must fail
    EXPECT_THROW(memberRepo_->deleteMember(memberId), lms::database::DatabaseException);
    EXPECT_TRUE(memberRepo_->getMemberById(memberId).has_value());
}

TEST_F(MemberRepositoryTest, ActiveLoanCheckForMember) {
    int memberId = memberRepo_->addMember(lms::models::Member("Active Borrower", "2026-01-01"));
    int bookId = bookRepo_->addBook(lms::models::Book("Book", "Author"));
    int copyId = copyRepo_->addCopy(lms::models::BookCopy(bookId, "ACC-ACT-01"));

    EXPECT_FALSE(memberRepo_->hasActiveLoan(memberId));

    // Add active loan
    connection_->execute(
        "INSERT INTO loans (copy_id, member_id, issued_on, due_on, returned_on) "
        "VALUES (" + std::to_string(copyId) + ", " + std::to_string(memberId) + ", '2026-01-01', '2026-01-15', NULL);");

    EXPECT_TRUE(memberRepo_->hasActiveLoan(memberId));
    EXPECT_TRUE(memberRepo_->hasLoanHistory(memberId));
}

TEST_F(MemberRepositoryTest, PreserveRecordsAfterRepeatedOperations) {
    int id1 = memberRepo_->addMember(lms::models::Member("Member 1", "2026-01-01", "m1@test.com"));
    int id2 = memberRepo_->addMember(lms::models::Member("Member 2", "2026-01-02", "m2@test.com"));

    memberRepo_->updateMemberStatus(id1, lms::models::MemberStatus::Suspended);

    auto m1 = memberRepo_->getMemberById(id1);
    auto m2 = memberRepo_->getMemberById(id2);

    ASSERT_TRUE(m1.has_value());
    ASSERT_TRUE(m2.has_value());
    EXPECT_EQ(m1->getStatus(), lms::models::MemberStatus::Suspended);
    EXPECT_EQ(m2->getStatus(), lms::models::MemberStatus::Active);
    EXPECT_EQ(*m2->getEmail(), "m2@test.com");
}

} // namespace

