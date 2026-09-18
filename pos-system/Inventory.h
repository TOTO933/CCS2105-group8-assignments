#pragma once
#include <map>
#include <string>
#include <vector>
#include "Product.h"

// ---------------------------------------------------------------------
// Inventory owns the product catalogue and the ONLY code path allowed to
// change stock numbers. Centralising stock mutation here is what lets us
// guarantee invariant #3 ("stock must never become negative") in one
// place instead of scattering stock arithmetic across the program.
//
// reserveStock()/releaseStock() are the two halves of a tentative-commit
// pattern used by Checkout: reserve first, and if something later in the
// transaction fails (e.g. payment), the caller releases the same
// quantity back. Stock is only ever "permanently" gone once a sale is
// fully confirmed - see invariant #4 in DESIGN_AND_TESTING.md.
// ---------------------------------------------------------------------
class Inventory {
    std::map<std::string, Product> products;

public:
    // Throws DuplicateProductException, InvalidPriceException,
    // InvalidQuantityException.
    void registerProduct(const std::string& id, const std::string& name,
                          double price, int initialStock);

    // Throws ProductNotFoundException, InvalidQuantityException.
    void addStock(const std::string& id, int quantity);

    bool hasProduct(const std::string& id) const;

    // Throws ProductNotFoundException.
    const Product& getProduct(const std::string& id) const;

    // Throws ProductNotFoundException, InvalidQuantityException,
    // InsufficientStockException.
    void reserveStock(const std::string& id, int quantity);

    // Returns previously reserved stock. Assumes id is valid (caller
    // only releases what it successfully reserved).
    void releaseStock(const std::string& id, int quantity);

    std::vector<Product> listProducts() const;
};
