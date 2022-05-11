// Copyright (c) 2020 The Zano developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "RIPEMD160_helper.h"
#include "auto_val_init.h"
extern "C" {
#include "RIPEMD160.h"
}

#define RMDsize 160

namespace crypto {

  void RIPEMD160_hash(const void *data, size_t length_size_t, hash160 &h)
  {

    dword         MDbuf[RMDsize / 32] = {0};   /* contains (A, B, C, D(, E))   */
    byte*         hashcode = (byte*)&h;        /* hashcode[RMDsize / 8];          for final hash-value         */
    dword         X[16] = {0};                 /* current 16-word chunk        */
    unsigned int  i = 0;                       /* counter                      */
    dword         length = static_cast<dword>(length_size_t);                  /* length in bytes of message   */
    dword         nbytes = 0;                  /* # of bytes not yet processed */
    byte*         message = (byte*)data;

    /* initialize */
    MDinit(MDbuf);
    //length = (dword)strlen((char *)message);

    /* process message in 16-word chunks */
    for (nbytes = length; nbytes > 63; nbytes -= 64) {
      for (i = 0; i < 16; i++) {
        X[i] = BYTES_TO_DWORD(message);
        message += 4;
      }
      compress(MDbuf, X);
    }/* length mod 64 bytes left */

    /* finish: */
    MDfinish(MDbuf, message, length, 0);

    for (i = 0; i < RMDsize / 8; i += 4) {
      hashcode[i] = (byte)MDbuf[i >> 2];         /* implicit cast to byte  */
      hashcode[i + 1] = (byte)(MDbuf[i >> 2] >> 8);  /*  extracts the 8 least  */
      hashcode[i + 2] = (byte)(MDbuf[i >> 2] >> 16);  /*  significant bits.     */
      hashcode[i + 3] = (byte)(MDbuf[i >> 2] >> 24);
    } 
  }

  hash160 RIPEMD160_hash(const void *data, size_t length)
  {
    hash160 h = AUTO_VAL_INIT(h);
    RIPEMD160_hash(data, length, h);
    return h;
  }

  hash RIPEMD160_hash_256(const void *data, size_t length)
  {
    hash160 h = RIPEMD160_hash(data, length);
    hash h256 = AUTO_VAL_INIT(h256);
    memcpy(&h256, &h, sizeof(h));
    return h256;
  }

}
