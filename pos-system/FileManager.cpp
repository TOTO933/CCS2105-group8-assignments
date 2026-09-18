#include "FileManager.h"
#include "Exceptions.h"
#include <fstream>
#include <sstream>

void FileManager::save(const std::string& path, const Inventory& inventory,
                        const SalesLedger& ledger) {
    if (simulateFailureOnNextSave) {
        simulateFailureOnNextSave = false;
        // Deliberately-tested failure: simulates a disk/permission error
        // occurring AFTER payment has been confirmed but BEFORE the
        // final record is persisted - exactly the "file failure after
        // payment but before final stock persistence" scenario the
        // brief asks Group 8 to demonstrate. In-memory state (inventory
        // and ledger) is already correct at this point; only the
        // durable copy on disk failed to update, so the recovery is
        // "warn the operator and retry the save", not "undo the sale".
        throw StorageException("simulated disk write failure while saving '" + path + "'");
    }

    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        throw StorageException("could not open '" + path + "' for writing");
    }

    for (const auto& p : inventory.listProducts()) {
        out << "PRODUCT|" << p.id << "|" << p.name << "|" << p.price << "|" << p.stock << "\n";
    }
    for (const auto& s : ledger.allSales()) {
        out << "SALE|" << s.saleId << "|" << (s.returned ? 1 : 0) << "|" << s.total;
        for (const auto& line : s.lines) {
            out << "|" << line.productId << ":" << line.quantity << ":" << line.unitPrice;
        }
        out << "\n";
    }

    if (!out.good()) {
        throw StorageException("write error while saving '" + path + "'");
    }
}

static std::vector<std::string> splitPipes(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string part;
    while (std::getline(ss, part, '|')) {
        parts.push_back(part);
    }
    return parts;
}

void FileManager::load(const std::string& path, Inventory& inventory, SalesLedger& ledger) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw StorageException("could not open '" + path + "' for reading");
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto parts = splitPipes(line);
        if (parts.empty()) continue;

        try {
            if (parts[0] == "PRODUCT" && parts.size() >= 5) {
                const std::string& id = parts[1];
                const std::string& name = parts[2];
                double price = std::stod(parts[3]);
                int stock = std::stoi(parts[4]);
                if (!inventory.hasProduct(id)) {
                    inventory.registerProduct(id, name, price, stock);
                }
            } else if (parts[0] == "SALE" && parts.size() >= 4) {
                SaleRecord record;
                record.saleId = parts[1];
                record.returned = (parts[2] == "1");
                record.total = std::stod(parts[3]);
                for (size_t i = 4; i < parts.size(); ++i) {
                    std::stringstream ls(parts[i]);
                    std::string productId, qtyStr, priceStr;
                    std::getline(ls, productId, ':');
                    std::getline(ls, qtyStr, ':');
                    std::getline(ls, priceStr, ':');
                    record.lines.push_back(SaleLine{productId, std::stoi(qtyStr), std::stod(priceStr)});
                }
                ledger.recordSale(record);
            }
        } catch (const std::exception& e) {
            // A single malformed line shouldn't take down the whole
            // load - but the operator does need to know the file is
            // not fully trustworthy.
            throw StorageException(std::string("corrupt record in '") + path + "': " + e.what());
        }
    }
}
