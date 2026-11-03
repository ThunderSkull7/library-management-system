#include "models/Loan.hpp"

#include <utility>

namespace lms::models {

Loan::Loan(int copyId,
           int memberId,
           std::string issuedOn,
           std::string dueOn,
           std::optional<std::string> returnedOn,
           int fineMinorUnits,
           bool finePaid)
    : id_(0),
      copyId_(copyId),
      memberId_(memberId),
      issuedOn_(std::move(issuedOn)),
      dueOn_(std::move(dueOn)),
      returnedOn_(std::move(returnedOn)),
      fineMinorUnits_(fineMinorUnits),
      finePaid_(finePaid) {}

Loan::Loan(int id,
           int copyId,
           int memberId,
           std::string issuedOn,
           std::string dueOn,
           std::optional<std::string> returnedOn,
           int fineMinorUnits,
           bool finePaid)
    : id_(id),
      copyId_(copyId),
      memberId_(memberId),
      issuedOn_(std::move(issuedOn)),
      dueOn_(std::move(dueOn)),
      returnedOn_(std::move(returnedOn)),
      fineMinorUnits_(fineMinorUnits),
      finePaid_(finePaid) {}

} // namespace lms::models
