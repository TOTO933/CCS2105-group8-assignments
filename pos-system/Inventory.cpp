#include "Inventory.h"
#include "Exceptions.h"

void Inventory::registerProduct(const std::string& id, const std::string& name,
                                 double price, int initialStock) {
    if (products.count(id)) {
        throw DuplicateProductException(id);
    }
    if (price <= 0.0) {
        throw InvalidPriceException(price);
    }
    if (initialStock < 0) {
        throw InvalidQuantityException(initialStock);
    }
    products[id] = Product{id, name, price, initialStock};
}

void Inventory::addStock(const std::string& id, int quantity) {
    auto it = products.find(id);
    if (it == products.end()) {
        throw ProductNotFoundException(id);
    }
    if (quantity <= 0) {
        throw InvalidQuantityException(quantity);
    }
    it->second.stock += quantity;
}

bool Inventory::hasProduct(const std::string& id) const {
    return products.count(id) > 0;
}

const Product& Inventory::getProduct(const std::string& id) const {
    auto it = products.find(id);
    if (it == products.end()) {
        throw ProductNotFoundException(id);
    }
    return it->second;
}

void Inventory::reserveStock(const std::string& id, int quantity) {
    auto it = products.find(id);
    if (it == products.end()) {
        throw ProductNotFoundException(id);
    }
    if (quantity <= 0) {
        throw InvalidQuantityException(quantity);
    }
    if (it->second.stock < quantity) {
        // Invariant #3 ("stock must never become negative") is enforced
        // right here: we refuse the reservation instead of letting stock
        // go negative and cleaning up afterwards.
        throw InsufficientStockException(id, quantity, it->second.stock);
    }
    it->second.stock -= quantity;
}

void Inventory::releaseStock(const std::string& id, int quantity) {
    auto it = products.find(id);
    if (it != products.end()) {
        it->second.stock += quantity;
    }
}

std::vector<Product> Inventory::listProducts() const {
    std::vector<Product> result;
    result.reserve(products.size());
    for (const auto& [id, product] : products) {
        result.push_back(product);
    }
    return result;
}
