export const PRINT_CONTROL_DEBOUNCE_MS = 2000
let lastPrintControlAt = 0

/** Returns true when the action should be ignored (debounced). */
export function printControlDebounced(): boolean {
  const now = Date.now()
  if (now - lastPrintControlAt < PRINT_CONTROL_DEBOUNCE_MS) return true
  lastPrintControlAt = now
  return false
}
