#include "Sale.h"
#include "Exceptions.h"

std::string SalesLedger::newSaleId() {
    return "SALE-" + std::to_string(nextId++);
}

void SalesLedger::recordSale(const SaleRecord& record) {
    sales[record.saleId] = record;
}

SaleRecord& SalesLedger::getSaleForReturn(const std::string& id) {
    auto it = sales.find(id);
    if (it == sales.end()) {
        throw InvalidReturnException(id, "no such sale exists");
    }
    if (it->second.returned) {
        throw InvalidReturnException(id, "sale has already been returned");
    }
    return it->second;
}

std::vector<SaleRecord> SalesLedger::allSales() const {
    std::vector<SaleRecord> result;
    result.reserve(sales.size());
    for (const auto& [id, record] : sales) {
        result.push_back(record);
    }
    return result;
}

double Checkout::reserveBasket(const std::vector<SaleLine>& basket) {
    std::vector<SaleLine> reservedSoFar;
    double total = 0.0;

    for (const auto& line : basket) {
        try {
            inventory.reserveStock(line.productId, line.quantity);
        } catch (const POSException&) {
            // PART: propagation + re-throw. This function has enough
            // context to know "roll back everything reserved so far in
            // THIS basket", but not enough context to decide what the
            // cashier/UI should do next (retry? cancel the whole sale?
            // offer a substitute item?) - that decision belongs to
            // main(). So we partially handle here (the rollback) and
            // re-throw the original exception unchanged so the caller
            // still sees exactly what failed.
            releaseBasket(reservedSoFar);
            throw;
        }
        reservedSoFar.push_back(line);
        total += line.unitPrice * line.quantity;
    }

    return total;
}

void Checkout::confirmPayment(double total, double tendered) const {
    if (tendered < total) {
        throw InsufficientPaymentException(total, tendered);
    }
}

void Checkout::releaseBasket(const std::vector<SaleLine>& basket) {
    for (const auto& line : basket) {
        inventory.releaseStock(line.productId, line.quantity);
    }
}
