/**
 * Compact portable SHA-256 and RIPEMD-160.
 * Task-ID: phase-1 / foundation + phase-2 HTLC
 * Not an OpenSSL dependency — deterministic and embeddable for tests.
 */
#include "hive_native/util/types.hpp"
#include <cstring>

namespace hive_native {
namespace {

// -------------------- SHA-256 --------------------
inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
   static const uint32_t K[64] = {
      0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
      0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
      0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
      0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
      0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
      0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
      0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
      0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
   };
   uint32_t W[64];
   for(int i = 0; i < 16; ++i) {
      W[i] = (uint32_t(block[i*4]) << 24) | (uint32_t(block[i*4+1]) << 16) |
             (uint32_t(block[i*4+2]) << 8) | uint32_t(block[i*4+3]);
   }
   for(int i = 16; i < 64; ++i) {
      uint32_t s0 = rotr(W[i-15], 7) ^ rotr(W[i-15], 18) ^ (W[i-15] >> 3);
      uint32_t s1 = rotr(W[i-2], 17) ^ rotr(W[i-2], 19) ^ (W[i-2] >> 10);
      W[i] = W[i-16] + s0 + W[i-7] + s1;
   }
   uint32_t a=state[0],b=state[1],c=state[2],d=state[3],e=state[4],f=state[5],g=state[6],h=state[7];
   for(int i = 0; i < 64; ++i) {
      uint32_t S1 = rotr(e,6) ^ rotr(e,11) ^ rotr(e,25);
      uint32_t ch = (e & f) ^ ((~e) & g);
      uint32_t t1 = h + S1 + ch + K[i] + W[i];
      uint32_t S0 = rotr(a,2) ^ rotr(a,13) ^ rotr(a,22);
      uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
      uint32_t t2 = S0 + maj;
      h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
   }
   state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
   state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
}

} // namespace

sha256_t sha256(const uint8_t* data, size_t len) {
   uint32_t state[8] = {
      0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
      0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
   };
   uint8_t block[64];
   size_t offset = 0;
   while(len - offset >= 64) {
      sha256_transform(state, data + offset);
      offset += 64;
   }
   size_t rem = len - offset;
   std::memset(block, 0, 64);
   std::memcpy(block, data + offset, rem);
   block[rem] = 0x80;
   if(rem >= 56) {
      sha256_transform(state, block);
      std::memset(block, 0, 64);
   }
   uint64_t bitlen = uint64_t(len) * 8;
   for(int i = 0; i < 8; ++i)
      block[63 - i] = uint8_t((bitlen >> (8 * i)) & 0xff);
   sha256_transform(state, block);

   sha256_t out{};
   for(int i = 0; i < 8; ++i) {
      out[i*4]   = uint8_t((state[i] >> 24) & 0xff);
      out[i*4+1] = uint8_t((state[i] >> 16) & 0xff);
      out[i*4+2] = uint8_t((state[i] >> 8) & 0xff);
      out[i*4+3] = uint8_t(state[i] & 0xff);
   }
   return out;
}

// -------------------- RIPEMD-160 (compact) --------------------
namespace {

inline uint32_t rol(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

void ripemd160_compress(uint32_t h[5], const uint8_t block[64]) {
   static const int r1[80] = {
      0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
      7,4,13,1,10,6,15,3,12,0,9,5,2,14,11,8,
      3,10,14,4,9,15,8,1,2,7,0,6,13,11,5,12,
      1,9,11,10,0,8,12,4,13,3,7,15,14,5,6,2,
      4,0,5,9,7,12,2,10,14,1,3,8,11,6,15,13
   };
   static const int r2[80] = {
      5,14,7,0,9,2,11,4,13,6,15,8,1,10,3,12,
      6,11,3,7,0,13,5,10,14,15,8,12,4,9,1,2,
      15,5,1,3,7,14,6,9,11,8,12,2,10,0,4,13,
      8,6,4,1,3,11,15,0,5,12,2,13,9,7,10,14,
      12,15,10,4,1,5,8,7,6,2,13,14,0,3,9,11
   };
   static const int s1[80] = {
      11,14,15,12,5,8,7,9,11,13,14,15,6,7,9,8,
      7,6,8,13,11,9,7,15,7,12,15,9,11,7,13,12,
      11,13,6,7,14,9,13,15,14,8,13,6,5,12,7,5,
      11,12,14,15,14,15,9,8,9,14,5,6,8,6,5,12,
      9,15,5,11,6,8,13,12,5,12,13,14,11,8,5,6
   };
   static const int s2[80] = {
      8,9,9,11,13,15,15,5,7,7,8,11,14,14,12,6,
      9,13,15,7,12,8,9,11,7,7,12,7,6,15,13,11,
      9,7,15,11,8,6,6,14,12,13,5,14,13,13,7,5,
      15,5,8,11,14,14,6,14,6,9,12,9,12,5,15,8,
      8,5,12,9,12,5,14,6,8,13,6,5,15,13,11,11
   };
   static const uint32_t K1[5] = {0, 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xa953fd4e};
   static const uint32_t K2[5] = {0x50a28be6, 0x5c4dd124, 0x6d703ef3, 0x7a6d76e9, 0};

   auto f = [](int j, uint32_t x, uint32_t y, uint32_t z) -> uint32_t {
      if(j < 16) return x ^ y ^ z;
      if(j < 32) return (x & y) | (~x & z);
      if(j < 48) return (x | ~y) ^ z;
      if(j < 64) return (x & z) | (y & ~z);
      return x ^ (y | ~z);
   };

   uint32_t X[16];
   for(int i = 0; i < 16; ++i)
      X[i] = uint32_t(block[i*4]) | (uint32_t(block[i*4+1]) << 8) |
             (uint32_t(block[i*4+2]) << 16) | (uint32_t(block[i*4+3]) << 24);

   uint32_t al=h[0], bl=h[1], cl=h[2], dl=h[3], el=h[4];
   uint32_t ar=h[0], br=h[1], cr=h[2], dr=h[3], er=h[4];

   for(int j = 0; j < 80; ++j) {
      uint32_t t = rol(al + f(j, bl, cl, dl) + X[r1[j]] + K1[j/16], s1[j]) + el;
      al = el; el = dl; dl = rol(cl, 10); cl = bl; bl = t;
      t = rol(ar + f(79 - j, br, cr, dr) + X[r2[j]] + K2[j/16], s2[j]) + er;
      ar = er; er = dr; dr = rol(cr, 10); cr = br; br = t;
   }

   uint32_t t = h[1] + cl + dr;
   h[1] = h[2] + dl + er;
   h[2] = h[3] + el + ar;
   h[3] = h[4] + al + br;
   h[4] = h[0] + bl + cr;
   h[0] = t;
}

} // namespace

ripemd160_t ripemd160(const uint8_t* data, size_t len) {
   uint32_t h[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
   uint8_t block[64];
   size_t offset = 0;
   while(len - offset >= 64) {
      ripemd160_compress(h, data + offset);
      offset += 64;
   }
   size_t rem = len - offset;
   std::memset(block, 0, 64);
   std::memcpy(block, data + offset, rem);
   block[rem] = 0x80;
   if(rem >= 56) {
      ripemd160_compress(h, block);
      std::memset(block, 0, 64);
   }
   uint64_t bitlen = uint64_t(len) * 8;
   for(int i = 0; i < 8; ++i)
      block[56 + i] = uint8_t((bitlen >> (8 * i)) & 0xff);
   ripemd160_compress(h, block);

   ripemd160_t out{};
   for(int i = 0; i < 5; ++i) {
      out[i*4]   = uint8_t(h[i] & 0xff);
      out[i*4+1] = uint8_t((h[i] >> 8) & 0xff);
      out[i*4+2] = uint8_t((h[i] >> 16) & 0xff);
      out[i*4+3] = uint8_t((h[i] >> 24) & 0xff);
   }
   return out;
}

} // namespace hive_native
