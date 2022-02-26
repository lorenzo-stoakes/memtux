#pragma once

// Align `_n` to power-of-2 `_to`.
#define ALIGN(_n, _to) \
	((_n) & ~((_to) - 1))

// Align `_n` to power-of-2 `_to`, either doing nothing if `_n` is already
// aligned or aligning to _next_ multiple of `_to` otherwise.
#define ALIGN_UP(_n, _to) \
	ALIGN((_n) + (_to) - 1, (_to))

// Determine number of elements in array `_arr`.
#define ARRAY_SIZE(_arr) \
	(sizeof(_arr) / sizeof((_arr)[0]))
