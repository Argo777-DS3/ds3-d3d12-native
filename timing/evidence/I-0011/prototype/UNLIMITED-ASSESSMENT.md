# Unlimited architecture assessment

## Conclusion

The currently recovered central timing intervention does not expose a proved
genuine limiter-disabled state. I-0011 therefore presents `Unlimited` as
unavailable and performs no memory write when it is selected. It does not use
360 FPS, a larger arbitrary target, zero, infinity or a Present-only override
as a substitute.

This is a bounded architectural result, not a claim that no other undiscovered
engine mechanism could ever unlock cadence.

## Evidence

- I-0005 Pass B proves one atomic numeric representation:
  `requested FPS / enable = target / 1`. It changes broad-update cadence,
  distributed delta and presentation to overwhelmingly `Present(0, ...)`.
- Every accepted I-0005 through I-0010 session restores the original pair
  exactly as `30 / 0`. Stock operation under that state uses
  `Present(1, ...)` and approximately 60-FPS cadence, so it is native/VSync
  behavior rather than Unlimited.
- Static functions `DS3-1152-F-0084E140`, `0084E1B0` and `0084E210` branch on
  the same enable byte but expose no third or limiter-off state. The protected
  central update path has no validated sentinel value for the target float.
- I-0004's separate QPC presentation wait was inactive in stock and numeric
  tests. Disabling or bypassing it cannot establish an unlimited engine update
  cadence.

## Rejected substitutions

- A very high finite target is still a cap and violates the contract.
- `target=0` or non-finite values are undocumented and could produce invalid
  delta arithmetic; they fail the finite 30–360 controller contract.
- Forcing `Present(0, ...)` alone changes presentation but leaves the central
  engine cadence/delta unresolved and therefore is not Unlimited.
- Restoring `30/0` is correctly labeled `Native`, never `Unlimited`.

The deterministic test sequence `240 -> Unlimited -> 165 -> Unlimited` proves
that both Unlimited requests return `unsupported` without mutating the timing
pair, while both numeric requests pass through the one finite controller.
