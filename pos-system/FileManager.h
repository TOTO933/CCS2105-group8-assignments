#pragma once
#include <string>
#include "Inventory.h"
#include "Sale.h"

// Handles persistence of stock and sales to a flat text file.
//
// simulateFailureOnNextSave lets the demo/test suite deliberately force
// a storage failure (required deliverable: "file or storage handling
// with at least one deliberately tested failure") without relying on an
// unpredictable real disk error.
class FileManager {
    bool simulateFailureOnNextSave = false;

public:
    void armSimulatedFailure() { simulateFailureOnNextSave = true; }

    // Throws StorageException on failure (including the simulated one).
    void save(const std::string& path, const Inventory& inventory,
              const SalesLedger& ledger);

    // Throws StorageException if the file cannot be opened/parsed.
    void load(const std::string& path, Inventory& inventory, SalesLedger& ledger);
};
