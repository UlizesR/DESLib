; Test program for RET error handling
; This program tries to call RET without a corresponding CALL

PRINTS "=== RET Error Test ==="
PRINTS "Testing RET without CALL (should cause stack underflow)"
PRINTS ""

; Try to return without any call
RET

PRINTS "This should not be printed!"
HALT
