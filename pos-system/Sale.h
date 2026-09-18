#pragma once
#include <map>
#include <string>
#include <vector>
#include "Inventory.h"

struct SaleLine {
    std::string productId;
    int quantity;
    double unitPrice;
};

struct SaleRecord {
    std::string saleId;
    std::vector<SaleLine> lines;
    double total = 0.0;
    bool returned = false;
};

// Keeps a record of every confirmed sale, needed both to display a
// history and to validate returns (business rule #6: "Returns must
// refer to a valid prior sale").
class SalesLedger {
    std::map<std::string, SaleRecord> sales;
    int nextId = 1;

public:
    std::string newSaleId();
    void recordSale(const SaleRecord& record);

    // Throws InvalidReturnException if the sale does not exist or has
    // already been returned.
    SaleRecord& getSaleForReturn(const std::string& id);

    std::vector<SaleRecord> allSales() const;
};

// ---------------------------------------------------------------------
// Checkout is where the "stock reduced immediately, payment validated
// after" critical-thinking scenario from the brief is resolved.
//
// Design decision (documented fully in DESIGN_AND_TESTING.md):
//   - The whole basket is treated as ATOMIC. reserveBasket() reserves
//     stock line by line; if any line fails (unknown product, bad
//     quantity, insufficient stock) every line already reserved in THIS
//     basket is released before the exception is allowed to propagate,
//     so a partially-available basket never leaves stock inconsistent.
//   - Stock is considered "committed" only once confirmPayment()
//     succeeds. If payment fails, main() is responsible for calling
//     releaseBasket() to restore the stock that reserveBasket() took -
//     this is the rollback path for the "stock reduced before payment"
//     scenario.
// ---------------------------------------------------------------------
class Checkout {
    Inventory& inventory;

public:
    explicit Checkout(Inventory& inv) : inventory(inv) {}

    // Throws ProductNotFoundException, InvalidQuantityException, or
    // InsufficientStockException (re-thrown after rolling back any
    // lines already reserved in this call - see .cpp for the
    // re-throw in action).
    double reserveBasket(const std::vector<SaleLine>& basket);

    // Throws InsufficientPaymentException. Does NOT touch stock; stock
    // was already committed by reserveBasket(). If this throws, the
    // caller must call releaseBasket() to roll the sale back.
    void confirmPayment(double total, double tendered) const;

    // Restores stock for every line in the basket. Used both for
    // rollback-on-payment-failure and for processing a return.
    void releaseBasket(const std::vector<SaleLine>& basket);
};
