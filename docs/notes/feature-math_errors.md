# Math operation refactor

The math operations have been refactored to use common code for error
checking. One of three different execution errors can now be thrown:
- "numeric: domain error"
- "numeric: range error (overflow)"
- "numeric: divide by zero"

The error checking depends solely on the finiteness or otherwise of the
operation's input(s) and output.

A domain error occurs when a math operation, given finite inputs, results
in not-a-number (NaN) - this is the case when the function is not defined
for the given inputs, for example `acos(2)`; or the output does not exist
in the extended real line (`ℝ ∪ {−∞, +∞}`), for example, `sqrt(-1)`.

A range error occurs when a math operation's output overflows given finite
inputs, i.e. when the result is greater than the maximum value of a 64-bit
floating point, for example `10^308 * 2`.

A divide by zero error occurs when a math operation causes division by zero
either directly, for example `1/0` or `0^-1` or as part of its computation,
for example `10 wrap 0`.

Math operations now do not throw execution errors when any of the inputs
are non-finite, for example neither of `(1^(-inf) + inf) / 2 = inf` or
`sqrt(-inf) = NaN` causes an execution error.
