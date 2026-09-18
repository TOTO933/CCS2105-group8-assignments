 CCS 2105 — Lua Coroutines Exercise (Question 8)

 Cooperative County Revenue Collection System

 Group Members

| Name | Registration Number |
|---|---|
| Margaret Wangeci | C026-01/0890-2025 |
| Annah Kioko | C026-01-2032/2025 |
| Martha Gitau | C026-01-0905/2025 |

---

 System Documentation

 1. Problem Statement

Three fictional county services — **parking payments**, **business permits** and
**land rates** — each process requests through four stages: `validation`,
`payment_verification`, `receipt_generation`, `completion`. Any request may hit a
temporary network failure (typically during payment verification), and that failure
must not block the other two services or any other pending request.

 2. Design Overview

- **Independent service coroutines (part a).** Each service (`parking`, `permits`,
  `land_rates`) is its own `coroutine.create`, owning its own request queue and its own
  state (`attempts`, `stage`). No mutable state is shared between services, so they are
  genuinely independent.
- **Status yielded after every stage (part b).** `coroutine.yield()` fires after every
  stage of every request, returning a status table (`service`, `id`, `stage`, `status`,
  `attempts`) so the scheduler always has visibility into what just happened.
- **Non-blocking retry (part c).** When `payment_verification` fails, the request is
  **not** dropped — it is pushed to the back of its own service's queue and picked up
  again later, so the failure of one request never stalls the rest of that service or
  any other service.
- **Central scheduler (part d).** `run_scheduler()` round-robins across the three
  service coroutines, resumes each one, reads its status back, and drops
  (`still_active` no longer includes it) any coroutine whose `coroutine.status()`
  becomes `"dead"`, so a finished/dead coroutine is never resumed again. It also checks
  the boolean `ok` returned by `coroutine.resume()` to catch genuine coroutine errors
  safely, per the general instructions in the exercise sheet.
- **Bounded retries (part e).** `MAX_RETRIES = 3` per request. A request is only
  re-queued while `attempts < MAX_RETRIES`; once it hits the ceiling it is marked
  `failed_permanently` and removed from rotation for good. Because every request has a
  hard retry ceiling and no external mechanism can reset that counter, the total number
  of times any request can be re-queued is bounded, which guarantees the scheduling
  loop terminates. Full reasoning is in the closing comment block of the source file.

 3. How to Run

Requires a standard Lua interpreter (5.1+ or LuaJIT):
The program prints a running log of every stage transition for every request across
all three services, then a summary of completed vs. permanently-failed requests per
service.

4. Notes

- Two of the sample requests (`PARK-002`, `RATE-001`) are configured to simulate one or
  two temporary failures before succeeding, to demonstrate the retry path.
- One sample request (`PERMIT-002`) is configured to fail more times than
  `MAX_RETRIES`, to demonstrate the permanent-failure path.
