#ifndef FIX_MINGW_H
#define FIX_MINGW_H

// Говорим компилятору, что эти проблемные встроенные функции уже определены,
// чтобы он пропустил их реализацию в intrin-impl.h и не ругался на nested function.
#define __faststorefence
#define __stosq
#define _interlockedbittestandset64
#define _interlockedbittestandreset64
#define _interlockedbittestandcomplement64
#define InterlockedBitTestAndSet64
#define InterlockedBitTestAndReset64
#define InterlockedBitTestAndComplement64
#define _InterlockedAnd64
#define _InterlockedOr64
#define _InterlockedXor64

#endif