--[[
============================================================================
 CCS 2105 - Problem-Solving Exercises on Lua Coroutines
 QUESTION 8: Cooperative County Revenue Collection System (20 Marks)
============================================================================
 Scenario:
   Three fictional county services process parking payments, business
   permits and land rates. Each request moves through four stages:
       1. validation
       2. payment_verification
       3. receipt_generation
       4. completion
   One service may encounter a temporary network failure (typically at the
   payment_verification stage). The system must not block the other
   services while a request is retried.

 Design overview:
   - Each COUNTY SERVICE (parking, permits, land_rates) is modelled as its
     own coroutine with its own internal request queue and state (part a).
   - The service coroutine yields a status table after every stage so the
     scheduler always knows what happened (part b).
   - When a temporary network failure is detected, the request is NOT
     dropped. It is pushed to the back of the service's own queue and
     retried later, so other requests (and other services) keep moving
     (part c).
   - A central scheduler round-robins between the three service
     coroutines, resumes each one, reads back its status, and removes
     (terminates) any coroutine whose status is "dead". Requests that
     come back as "retry_needed" are retried up to a fixed retry limit
     (part d).
   - Part (e) is answered in the closing comment block at the end of the
     file.
============================================================================
]]

-- ---------------------------------------------------------------------
-- Configuration
-- ---------------------------------------------------------------------
local MAX_RETRIES = 3          -- sensible retry limit per request
local STAGES = { "validation", "payment_verification",
                 "receipt_generation", "completion" }

-- ---------------------------------------------------------------------
-- Helper: simulates whether a network call temporarily fails.
-- For reproducibility in this exercise we fail deterministically on the
-- 1st and 2nd attempt of specific requests instead of using math.random,
-- so behaviour is predictable when marking/demonstrating.
-- ---------------------------------------------------------------------
local function network_call_ok(service_name, request, attempt_number)
    if request.simulate_failures and attempt_number <= request.simulate_failures then
        return false
    end
    return true
end

-- ---------------------------------------------------------------------
-- PART (a): Independent service coroutine.
-- Each service owns its own queue and its own state (current request,
-- current stage, retry counts). Nothing here is shared mutable state
-- between services, so services are fully independent.
-- ---------------------------------------------------------------------
local function make_service_coroutine(service_name, request_queue)

    return coroutine.create(function()
        local queue = request_queue          -- service-local state
        local completed, failed_permanently = {}, {}

        while #queue > 0 do
            local request = table.remove(queue, 1)   -- FIFO pop
            request.attempts = (request.attempts or 0) + 1

            local stage_failed = false

            for _, stage in ipairs(STAGES) do
                request.stage = stage

                if stage == "payment_verification" then
                    -- This is where a temporary network failure can occur.
                    if not network_call_ok(service_name, request, request.attempts) then
                        stage_failed = true
                        -- PART (b): yield status even for a failed stage
                        coroutine.yield({
                            service = service_name,
                            id = request.id,
                            stage = stage,
                            status = "temporary_network_failure",
                            attempts = request.attempts,
                        })
                        break
                    end
                end

                -- PART (b): yield status information after every
                -- successful stage of processing.
                coroutine.yield({
                    service = service_name,
                    id = request.id,
                    stage = stage,
                    status = "stage_complete",
                    attempts = request.attempts,
                })
            end

            if stage_failed then
                -- PART (c): handle the temporary failure without blocking
                -- other requests/services.
                if request.attempts < MAX_RETRIES then
                    table.insert(queue, request)  -- requeue at the back
                    coroutine.yield({
                        service = service_name,
                        id = request.id,
                        stage = "payment_verification",
                        status = "retry_scheduled",
                        attempts = request.attempts,
                    })
                else
                    failed_permanently[#failed_permanently + 1] = request.id
                    coroutine.yield({
                        service = service_name,
                        id = request.id,
                        stage = "payment_verification",
                        status = "failed_permanently",
                        attempts = request.attempts,
                    })
                end
            else
                completed[#completed + 1] = request.id
            end
        end

        -- Final summary before the coroutine naturally dies.
        return {
            service = service_name,
            status = "service_finished",
            completed = completed,
            failed_permanently = failed_permanently,
        }
    end)
end

-- ---------------------------------------------------------------------
-- Sample request queues for the three services.
-- "simulate_failures = N" means the request will experience a temporary
-- network failure on its first N attempts before succeeding.
-- ---------------------------------------------------------------------
local parking_requests = {
    { id = "PARK-001" },
    { id = "PARK-002", simulate_failures = 1 },
    { id = "PARK-003" },
}

local permit_requests = {
    { id = "PERMIT-001" },
    { id = "PERMIT-002", simulate_failures = 4 }, -- will exceed MAX_RETRIES
}

local land_rate_requests = {
    { id = "RATE-001", simulate_failures = 2 },
    { id = "RATE-002" },
}

-- ---------------------------------------------------------------------
-- PART (d): Central scheduler.
-- Round-robins across the three service coroutines. Resumes each one,
-- reads the yielded status, and drops ("terminates") any coroutine whose
-- status becomes "dead" (never resumes a dead coroutine again). Retries
-- are handled naturally because a retried request simply reappears later
-- in that service's own queue - the scheduler does not need to know
-- about retries directly, it only needs to keep resuming live coroutines.
-- ---------------------------------------------------------------------
local function run_scheduler(services)
    local active = {}
    for name, co in pairs(services) do
        active[#active + 1] = { name = name, co = co }
    end

    print(string.format("%-10s %-10s %-22s %-22s %s",
        "SERVICE", "REQUEST", "STAGE", "STATUS", "ATTEMPT"))
    print(string.rep("-", 80))

    while #active > 0 do
        local still_active = {}

        for _, entry in ipairs(active) do
            if coroutine.status(entry.co) ~= "dead" then
                local ok, result = coroutine.resume(entry.co)

                if not ok then
                    -- Genuine coroutine error (not a business "temporary
                    -- failure") - report and drop this service safely.
                    print(string.format(
                        "[ERROR] service '%s' raised an error and was stopped: %s",
                        entry.name, tostring(result)))
                elseif coroutine.status(entry.co) == "dead" then
                    -- Coroutine ran to completion (its while-loop ended).
                    if type(result) == "table" and result.status == "service_finished" then
                        print(string.format(
                            "[DONE]  service '%s' finished. completed=%d, permanently_failed=%d",
                            entry.name, #result.completed, #result.failed_permanently))
                    end
                    -- PART (d): terminate - simply do not add it back to
                    -- `still_active`, so it is never resumed again.
                else
                    -- Coroutine is suspended: print its latest status and
                    -- keep it active for the next scheduling round.
                    print(string.format("%-10s %-10s %-22s %-22s %s",
                        result.service, result.id, result.stage,
                        result.status, tostring(result.attempts)))
                    still_active[#still_active + 1] = entry
                end
            end
        end

        active = still_active
    end

    print(string.rep("-", 80))
    print("All services processed. Scheduler exiting.")
end

-- ---------------------------------------------------------------------
-- Wire everything together and run.
-- ---------------------------------------------------------------------
local services = {
    parking    = make_service_coroutine("parking", parking_requests),
    permits    = make_service_coroutine("permits", permit_requests),
    land_rates = make_service_coroutine("land_rates", land_rate_requests),
}

run_scheduler(services)

--[[
============================================================================
 PART (e): How this design avoids an infinite retry loop
============================================================================
 1. Bounded attempts per request: every request carries its own
    `attempts` counter, incremented once per pass through the stage
    pipeline. A request is only requeued while
    `request.attempts < MAX_RETRIES` (here, 3). Once the counter reaches
    MAX_RETRIES the request is moved to `failed_permanently` and is never
    placed back on the queue - so no single request can be retried
    forever.

 2. Retry, don't restart, the loop: the service coroutine's `while
    #queue > 0 do` loop only continues as long as the queue is
    shrinking overall. Each iteration removes exactly one request from
    the front of the queue and, in the worst case, re-inserts it once at
    the back. Because every request has a hard retry ceiling, the total
    number of times any request can be re-inserted is bounded
    (MAX_RETRIES - 1), which means the queue length is bounded by
    (number of original requests) x MAX_RETRIES. The loop is therefore
    guaranteed to terminate.

 3. No shared/global retry state that could reset counters: because each
    service coroutine owns its own queue and each request owns its own
    `attempts` field, there is no external mechanism that could
    accidentally reset a request's retry count back to zero and restart
    the cycle.

 4. Scheduler-level safety net: even if a coroutine's internal logic
    misbehaved, the scheduler only resumes coroutines whose
    coroutine.status() is not "dead". A coroutine that returns from its
    function body naturally becomes dead and is dropped from `active`,
    so the top-level scheduling loop itself is also guaranteed to end
    once every coroutine has finished (successfully or via permanent
    failure) - it never re-adds a dead coroutine back into rotation.
============================================================================
]]
