#include "database/DatabaseConnection.hpp"
#include "database/SchemaManager.hpp"
#include "models/Book.hpp"
#include "models/BookCopy.hpp"
#include "models/Member.hpp"
#include "models/MemberStatus.hpp"
#include "repositories/BookCopyRepository.hpp"
#include "repositories/BookRepository.hpp"
#include "repositories/MemberRepository.hpp"
#include "services/MemberService.hpp"
#include "utils/ValidationException.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>

namespace {

class MemberServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        connection_ = std::make_unique<lms::database::DatabaseConnection>(":memory:");
        schemaManager_ = std::make_unique<lms::database::SchemaManager>(*connection_);
        schemaManager_->initializeSchema();

        memberRepo_ = std::make_unique<lms::repositories::MemberRepository>(*connection_);
        bookRepo_ = std::make_unique<lms::repositories::BookRepository>(*connection_);
        copyRepo_ = std::make_unique<lms::repositories::BookCopyRepository>(*connection_);
        memberService_ = std::make_unique<lms::services::MemberService>(*memberRepo_);
    }

    void TearDown() override {
        memberService_.reset();
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
    std::unique_ptr<lms::services::MemberService> memberService_;
};

TEST_F(MemberServiceTest, RegisterValidMember) {
    int id = memberService_->registerMember("  Ada Lovelace  ", "  2026-01-15  ", "  ada@lovelace.org  ", "  555-1234  ");
    EXPECT_GT(id, 0);

    auto member = memberService_->getMember(id);
    EXPECT_EQ(member.getFullName(), "Ada Lovelace");
    EXPECT_EQ(member.getJoinedOn(), "2026-01-15");
    ASSERT_TRUE(member.getEmail().has_value());
    EXPECT_EQ(*member.getEmail(), "ada@lovelace.org");
    ASSERT_TRUE(member.getPhone().has_value());
    EXPECT_EQ(*member.getPhone(), "555-1234");
    EXPECT_EQ(member.getStatus(), lms::models::MemberStatus::Active);
}

TEST_F(MemberServiceTest, RejectEmptyOrWhitespaceNames) {
    EXPECT_THROW(
        memberService_->registerMember("", "2026-01-15"),
        lms::utils::ValidationException);

    EXPECT_THROW(
        memberService_->registerMember("    \t\n  ", "2026-01-15"),
        lms::utils::ValidationException);
}

TEST_F(MemberServiceTest, ValidateEmailFormat) {
    // Valid emails
    EXPECT_NO_THROW(memberService_->registerMember("User 1", "2026-01-01", "user1@example.com"));
    EXPECT_NO_THROW(memberService_->registerMember("User 2", "2026-01-01", "john.doe@sub.domain.co"));

    // Invalid emails
    EXPECT_THROW(memberService_->registerMember("Bad 1", "2026-01-01", "plainaddress"), lms::utils::ValidationException);
    EXPECT_THROW(memberService_->registerMember("Bad 2", "2026-01-01", "@missinguser.com"), lms::utils::ValidationException);
    EXPECT_THROW(memberService_->registerMember("Bad 3", "2026-01-01", "missingdomain@"), lms::utils::ValidationException);
    EXPECT_THROW(memberService_->registerMember("Bad 4", "2026-01-01", "user@nodot"), lms::utils::ValidationException);
    EXPECT_THROW(memberService_->registerMember("Bad 5", "2026-01-01", "user@.dotfirst"), lms::utils::ValidationException);
    EXPECT_THROW(memberService_->registerMember("Bad 6", "2026-01-01", "user @domain.com"), lms::utils::ValidationException);
    EXPECT_THROW(memberService_->registerMember("Bad 7", "2026-01-01", "user@domain.com."), lms::utils::ValidationException);
}

TEST_F(MemberServiceTest, RejectDuplicateEmail) {
    memberService_->registerMember("Alice", "2026-01-01", "duplicate@test.com");

    EXPECT_THROW(
        memberService_->registerMember("Bob", "2026-01-02", "duplicate@test.com"),
        lms::utils::ValidationException);
}

TEST_F(MemberServiceTest, AcceptOmittedEmail) {
    int id1 = memberService_->registerMember("Patron 1", "2026-01-01", std::nullopt);
    int id2 = memberService_->registerMember("Patron 2", "2026-01-01", std::nullopt);

    EXPECT_GT(id1, 0);
    EXPECT_GT(id2, 0);
    EXPECT_FALSE(memberService_->getMember(id1).getEmail().has_value());
    EXPECT_FALSE(memberService_->getMember(id2).getEmail().has_value());
}

TEST_F(MemberServiceTest, ValidateOptionalPhoneInput) {
    // Valid phone
    int id = memberService_->registerMember("User", "2026-01-01", std::nullopt, "  +1-800-555-0199  ");
    EXPECT_EQ(*memberService_->getMember(id).getPhone(), "+1-800-555-0199");

    // Whitespace only phone should be rejected
    EXPECT_THROW(
        memberService_->registerMember("User", "2026-01-01", std::nullopt, "   "),
        lms::utils::ValidationException);
}

TEST_F(MemberServiceTest, RejectInvalidDates) {
    EXPECT_THROW(memberService_->registerMember("User", "invalid-date"), lms::utils::ValidationException);
    EXPECT_THROW(memberService_->registerMember("User", "2026/01/01"), lms::utils::ValidationException);
    EXPECT_THROW(memberService_->registerMember("User", "2026-13-01"), lms::utils::ValidationException);
    EXPECT_THROW(memberService_->registerMember("User", "2026-00-10"), lms::utils::ValidationException);
    EXPECT_THROW(memberService_->registerMember("User", "2026-02-29"), lms::utils::ValidationException); // Not leap year
    EXPECT_THROW(memberService_->registerMember("User", "2026-04-31"), lms::utils::ValidationException); // April has 30
}

TEST_F(MemberServiceTest, AcceptValidLeapYearDates) {
    EXPECT_NO_THROW(memberService_->registerMember("User", "2024-02-29"));
}

TEST_F(MemberServiceTest, RetrieveNonexistentMemberThrowsNotFound) {
    EXPECT_THROW((void)memberService_->getMember(9999), lms::utils::NotFoundException);
}

TEST_F(MemberServiceTest, UpdateMemberDetails) {
    int id = memberService_->registerMember("Initial Name", "2026-01-01", "init@test.com", "111");

    lms::models::Member updated(id, "Updated Name", "2026-01-05", "updated@test.com", "222", lms::models::MemberStatus::Active);
    EXPECT_NO_THROW(memberService_->updateMember(updated));

    auto retrieved = memberService_->getMember(id);
    EXPECT_EQ(retrieved.getFullName(), "Updated Name");
    EXPECT_EQ(retrieved.getJoinedOn(), "2026-01-05");
    EXPECT_EQ(*retrieved.getEmail(), "updated@test.com");
    EXPECT_EQ(*retrieved.getPhone(), "222");

    // Update nonexistent member throws NotFound
    lms::models::Member nonexistent(9999, "Name", "2026-01-01");
    EXPECT_THROW(memberService_->updateMember(nonexistent), lms::utils::NotFoundException);
}

TEST_F(MemberServiceTest, UpdateWithDuplicateEmailThrowsValidation) {
    int id1 = memberService_->registerMember("Member 1", "2026-01-01", "m1@test.com");
    int id2 = memberService_->registerMember("Member 2", "2026-01-02", "m2@test.com");

    lms::models::Member conflictUpdate(id2, "Member 2", "2026-01-02", "m1@test.com");
    EXPECT_THROW(memberService_->updateMember(conflictUpdate), lms::utils::ValidationException);

    // Updating member with their own email should succeed
    lms::models::Member selfEmailUpdate(id1, "Member 1 Renamed", "2026-01-01", "m1@test.com");
    EXPECT_NO_THROW(memberService_->updateMember(selfEmailUpdate));
}

TEST_F(MemberServiceTest, SuspendReactivateAndCloseMember) {
    int id = memberService_->registerMember("Patron", "2026-01-01");
    EXPECT_EQ(memberService_->getMember(id).getStatus(), lms::models::MemberStatus::Active);

    // Suspend
    memberService_->updateStatus(id, lms::models::MemberStatus::Suspended);
    EXPECT_EQ(memberService_->getMember(id).getStatus(), lms::models::MemberStatus::Suspended);

    // Reactivate
    memberService_->updateStatus(id, lms::models::MemberStatus::Active);
    EXPECT_EQ(memberService_->getMember(id).getStatus(), lms::models::MemberStatus::Active);

    // Close
    memberService_->updateStatus(id, lms::models::MemberStatus::Closed);
    EXPECT_EQ(memberService_->getMember(id).getStatus(), lms::models::MemberStatus::Closed);

    // Update status of nonexistent member
    EXPECT_THROW(memberService_->updateStatus(9999, lms::models::MemberStatus::Active), lms::utils::NotFoundException);
}

TEST_F(MemberServiceTest, SearchAndFilterMethods) {
    memberService_->registerMember("Sherlock Holmes", "2026-01-01", "sherlock@221b.co.uk", std::nullopt, lms::models::MemberStatus::Active);
    memberService_->registerMember("John Watson", "2026-01-02", "watson@221b.co.uk", std::nullopt, lms::models::MemberStatus::Suspended);
    memberService_->registerMember("Mycroft Holmes", "2026-01-03", "mycroft@diogenes.co.uk", std::nullopt, lms::models::MemberStatus::Active);

    EXPECT_EQ(memberService_->searchMembersByName("Holmes").size(), 2);
    EXPECT_EQ(memberService_->searchMembersByName("Watson").size(), 1);
    EXPECT_EQ(memberService_->filterMembersByStatus(lms::models::MemberStatus::Active).size(), 2);
    EXPECT_EQ(memberService_->filterMembersByStatus(lms::models::MemberStatus::Suspended).size(), 1);

    auto found = memberService_->getMemberByEmail("sherlock@221b.co.uk");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->getFullName(), "Sherlock Holmes");

    EXPECT_THROW((void)memberService_->getMemberByEmail("   "), lms::utils::ValidationException);
}

TEST_F(MemberServiceTest, DeleteMemberSafety) {
    int memberId = memberService_->registerMember("Member To Delete", "2026-01-01");
    int bookId = bookRepo_->addBook(lms::models::Book("Book", "Author"));
    int copyId = copyRepo_->addCopy(lms::models::BookCopy(bookId, "ACC-MEM-DEL"));

    // 1. Cannot delete member with active loan
    connection_->execute(
        "INSERT INTO loans (id, copy_id, member_id, issued_on, due_on, returned_on) "
        "VALUES (100, " + std::to_string(copyId) + ", " + std::to_string(memberId) + ", '2026-01-01', '2026-01-15', NULL);");

    EXPECT_THROW(memberService_->deleteMember(memberId), lms::utils::ValidationException);

    // 2. Cannot delete member even after loan is returned (preserves loan history)
    connection_->execute("UPDATE loans SET returned_on = '2026-01-10' WHERE id = 100;");
    EXPECT_THROW(memberService_->deleteMember(memberId), lms::utils::ValidationException);

    // 3. Member without loan history can be deleted safely
    int freshId = memberService_->registerMember("Fresh Member", "2026-01-01");
    EXPECT_NO_THROW(memberService_->deleteMember(freshId));
    EXPECT_THROW((void)memberService_->getMember(freshId), lms::utils::NotFoundException);

    // 4. Nonexistent member throws NotFoundException
    EXPECT_THROW(memberService_->deleteMember(9999), lms::utils::NotFoundException);
}

} // namespace

