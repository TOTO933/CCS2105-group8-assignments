 Group 8 — Supermarket Point-of-Sale and Inventory System

Second-Year Computer Science | C++ Exception Handling | Four-Day Group Challenge

Group Members

| Name | Registration Number |
|---|---|
| Margaret Wangeci | C026-01-0890/2025 |
| Annah Kioko | C026-01-2032/2025 |
| Martha Gitau | C026-01-0905/2025 |

---

## System Documentation

 1. Problem Analysis

The system is a console-based POS/inventory prototype for a Kenyan supermarket. It
registers products and stock, builds a sales basket, checks availability, processes
payment, confirms a sale, handles returns, and persists data — while guaranteeing that
stock and payment state never become inconsistent, even when something fails partway
through a transaction.

 2. Invariants

1. Product identifiers are unique (`DuplicateProductException` on violation).
2. Sale quantity must be positive (`InvalidQuantityException`).
3. Stock must never become negative — enforced centrally inside `Inventory::reserveStock`.
4. A failed sale must not permanently reduce stock — stock reserved for a basket that
   later fails (bad line, or payment shortfall) is always released back.
5. Payment must satisfy the sale total before a sale is confirmed
   (`InsufficientPaymentException`).
6. A return must refer to a valid, not-already-returned prior sale
   (`InvalidReturnException`).

 3. Class Responsibilities

| Class | Responsibility |
|---|---|
| `Product` | Plain data holder for a catalogue item. |
| `Inventory` | Owns the product catalogue; the only class allowed to mutate stock numbers; enforces invariants 1–3. |
| `Checkout` | Coordinates one sale transaction: atomic basket reservation, payment confirmation, rollback. Enforces invariant 4. |
| `SalesLedger` | Records confirmed sales and validates returns. Enforces invariant 6. |
| `FileManager` | Persists inventory + sales to disk; can simulate a storage failure on demand for testing. |

 4. Exception Design Table

| Exception | Carries | Thrown by | Typically handled |
|---|---|---|---|
| `ProductNotFoundException` | product id | `Inventory::getProduct/reserveStock` | `main()` sale/return handler |
| `DuplicateProductException` | product id | `Inventory::registerProduct` | `main()` registration handler |
| `InvalidQuantityException` | the bad quantity | `Inventory::addStock/reserveStock` | `main()` |
| `InsufficientStockException` | product id, requested, available | `Inventory::reserveStock` | `Checkout::reserveBasket` (rollback + re-throw) |
| `InvalidPriceException` | the bad price | `Inventory::registerProduct` | `main()` |
| `InsufficientPaymentException` | due, tendered | `Checkout::confirmPayment` | `performSale()` in `main.cpp` (rollback) |
| `InvalidReturnException` | sale id, reason | `SalesLedger::getSaleForReturn` | `performReturn()` |
| `StorageException` | failure detail | `FileManager::save/load` | `main()` menu loop |

All derive from `POSException` (itself derived from `std::exception`), so callers can
choose a specific typed `catch` where special recovery is needed, or a general
`catch (const POSException&)` as a domain-wide fallback — both patterns are used in
this project.

 5. Control Flow: Detection → Throw → Propagation → Handler → Recovery → Continuation

```
Inventory::reserveStock()          <- detects insufficient stock, THROWS
        |  (propagates up, uncaught)
Checkout::reserveBasket()          <- CATCHES POSException, releases any lines
        |                             already reserved in this basket (partial
        |                             handling), then RE-THROWS the original
        |                             exception unchanged
        v  (propagates up)
performSale() in main.cpp          <- final HANDLER: prints the failure,
                                       decides continuation = "sale aborted,
                                       return to menu, no stock lost"
```

A second path shows the "stock reduced before payment" scenario:

```
Checkout::reserveBasket()  succeeds, stock committed
Checkout::confirmPayment() THROWS InsufficientPaymentException (no stock touched)
        v
performSale()  <- CATCHES, calls checkout.releaseBasket(reserved) to restore
                   stock, then reports failure. Continuation: sale aborted,
                   stock is exactly as it was before the attempt.
```

 6. Critical-Thinking Answers (Group 8 scenario)

**Scenario:** stock is reduced immediately after the cashier selects products; payment
then fails.

1. **When is stock "committed" to a sale?** Only once `confirmPayment()` succeeds and
   the sale is recorded in `SalesLedger`. Before that, a reservation exists but is
   reversible.
2. **Stock exception vs payment exception — different recovery?** Yes. A stock
   exception (`InsufficientStockException`) is rolled back *inside* `reserveBasket()`
   itself, because nothing has been committed to the customer yet. A payment exception
   is rolled back *by the caller* (`performSale()`), because only the caller knows the
   reservation actually succeeded and needs releasing.
3. **Mixed basket (one available, one out of stock) — atomic?** Yes, the whole sale is
   atomic (see `reserveBasket`): the first failing line rolls back every line reserved
   before it, so a customer is never sold part of a basket.
4. **Propagated payment exception:** `InsufficientPaymentException` is thrown in
   `Checkout::confirmPayment()` and propagates, uncaught, all the way to
   `performSale()` — no intermediate function has enough context to decide the right
   recovery.
5. **File failure after payment but before persistence:** demonstrated by
   `FileManager::armSimulatedFailure()` / Test 20. In-memory state (inventory + ledger)
   is already correct at that point; only the durable copy failed, so recovery is
   "warn the operator, keep in-memory state, retry the save" — not "undo the sale".

 7. Test Matrix (20 tests, automated in `runTestSuite()` in `main.cpp`)

Run with menu option **8**. Each test prints its predicted outcome before executing,
then the actual outcome — all 20 predictions matched actual behaviour when last run
against this exact source.

| # | Category | Test | Predicted |
|---|---|---|---|
| 1 | Normal | Register P001 | success |
| 2 | Normal | Register P002 | success |
| 3 | Normal | Add stock to P001 | success |
| 4 | Normal | Valid one-item sale, exact payment | success |
| 5 | Normal | Valid return of that sale | success |
| 6 | Boundary | Sale qty == current stock | success, stock → 0 |
| 7 | Boundary | Sale qty == 0 | `InvalidQuantityException` |
| 8 | Boundary | Register at minimum price (0.01) | success |
| 9 | Boundary | Add exactly 1 unit of stock | success |
| 10 | Boundary | Request 1 more than available stock | `InsufficientStockException` |
| 11 | Exception | Sell unknown product | `ProductNotFoundException` |
| 12 | Exception | Register duplicate product code | `DuplicateProductException` |
| 13 | Exception | Register price ≤ 0 | `InvalidPriceException` |
| 14 | Exception | Tender less than total | `InsufficientPaymentException`, stock rolled back |
| 15 | Exception | Return nonexistent sale | `InvalidReturnException` |
| 16 | Multiple-error | Basket: 1 available + 1 out-of-stock line | atomic failure, first line's stock restored |
| 17 | Multiple-error | Basket: invalid qty + unknown product | first line's error reported |
| 18 | Recovery | Retry a normal sale after test 16 | success, stock was never lost |
| 19 | Recovery | Retry same basket after payment failure, correct payment | success |
| 20 | File/storage | Simulated save failure, then real retry | `StorageException`, then success |

 8. Reflection

Introducing exception handling changed the architecture from "check-then-act" scattered
through the UI layer into a design where **`Inventory` is the single authority on stock
validity** and **`Checkout` owns transaction atomicity**. This made the "reduce stock
first, validate payment after" business rule tractable: instead of pre-checking payment
possibility everywhere, each stage simply throws when it fails, and exactly one layer up
the call stack (the layer that has enough context) decides whether to roll back, retry,
or abort. Typed exceptions with embedded data (e.g. `InsufficientStockException` carrying
`requested`/`available`) meant handlers could report precise, actionable messages instead
of generic failure strings — and the atomic-basket rollback in `reserveBasket()` would
have been far harder to get right with return-code error checking, since every call site
would have needed to remember to unwind manually.

 9. Build & Run

```
g++ -std=c++17 -Wall -Iinclude src/*.cpp -o pos_system
./pos_system
```

Compiled and run successfully with g++ 13.3.0 on Ubuntu 24.04; all 20 automated tests
pass. Menu option 8 runs the full test matrix described above.

 10. File Structure

```
pos-system/
├── README.md
├── include/
│   ├── Exceptions.h
│   ├── FileManager.h
│   ├── Inventory.h
│   ├── Product.h
│   └── Sale.h
└── src/
    ├── FileManager.cpp
    ├── Inventory.cpp
    ├── Sale.cpp
    └── main.cpp
```
