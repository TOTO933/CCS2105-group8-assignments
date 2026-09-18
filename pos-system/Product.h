#pragma once
#include <string>

// A plain data holder. Business rules about products (uniqueness,
// price/stock validity) are enforced by Inventory, not here - this keeps
// Product a simple value type.
struct Product {
    std::string id;
    std::string name;
    double price = 0.0;
    int stock = 0;
};
