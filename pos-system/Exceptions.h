#pragma once
#include <exception>
#include <string>

// ---------------------------------------------------------------------
// GROUP 8: Supermarket Point-of-Sale and Inventory System
// User-defined exception hierarchy.
//
// A common base (POSException) lets a caller write one catch-all handler
// for "anything POS-related went wrong", while each derived type carries
// the specific information a handler needs to react intelligently
// (which product, how much stock was available, etc.) rather than just
// a human-readable string.
// ---------------------------------------------------------------------

class POSException : public std::exception {
protected:
    std::string message;
public:
    explicit POSException(std::string msg) : message(std::move(msg)) {}
    const char* what() const noexcept override { return message.c_str(); }
};

// Thrown when a product code is not registered in the inventory.
class ProductNotFoundException : public POSException {
    std::string productId;
public:
    explicit ProductNotFoundException(std::string id)
        : POSException("Product not found: " + id), productId(std::move(id)) {}
    const std::string& getProductId() const { return productId; }
};

// Thrown when trying to register a product code that already exists.
class DuplicateProductException : public POSException {
    std::string productId;
public:
    explicit DuplicateProductException(std::string id)
        : POSException("Duplicate product code: " + id), productId(std::move(id)) {}
    const std::string& getProductId() const { return productId; }
};

// Thrown when a requested sale/stock quantity is not positive.
class InvalidQuantityException : public POSException {
    int quantity;
public:
    explicit InvalidQuantityException(int qty)
        : POSException("Invalid quantity: " + std::to_string(qty)), quantity(qty) {}
    int getQuantity() const { return quantity; }
};

// Thrown when a sale requests more units than are currently available.
// Carries enough information for a handler to explain the shortfall to
// the cashier without re-querying the inventory.
class InsufficientStockException : public POSException {
    std::string productId;
    int requested;
    int available;
public:
    InsufficientStockException(std::string id, int req, int avail)
        : POSException("Insufficient stock for " + id +
                        ": requested " + std::to_string(req) +
                        ", available " + std::to_string(avail)),
          productId(std::move(id)), requested(req), available(avail) {}
    const std::string& getProductId() const { return productId; }
    int getRequested() const { return requested; }
    int getAvailable() const { return available; }
};

// Thrown when a product is registered/updated with a non-positive price.
class InvalidPriceException : public POSException {
    double price;
public:
    explicit InvalidPriceException(double p)
        : POSException("Invalid price: " + std::to_string(p)), price(p) {}
    double getPrice() const { return price; }
};

// Thrown when the amount tendered by the customer is less than the sale
// total. Carries both figures so the handler can tell the cashier
// exactly how much more is owed.
class InsufficientPaymentException : public POSException {
    double due;
    double tendered;
public:
    InsufficientPaymentException(double due_, double tendered_)
        : POSException("Insufficient payment: due " + std::to_string(due_) +
                        ", tendered " + std::to_string(tendered_)),
          due(due_), tendered(tendered_) {}
    double getDue() const { return due; }
    double getShortfall() const { return due - tendered; }
};

// Thrown when a return does not correspond to a valid prior sale, or the
// sale has already been returned.
class InvalidReturnException : public POSException {
    std::string saleId;
public:
    InvalidReturnException(std::string id, const std::string& reason)
        : POSException("Invalid return for sale " + id + ": " + reason),
          saleId(std::move(id)) {}
    const std::string& getSaleId() const { return saleId; }
};

// Thrown when a save/load operation to the data file fails.
class StorageException : public POSException {
public:
    explicit StorageException(const std::string& detail)
        : POSException("Storage failure: " + detail) {}
};
