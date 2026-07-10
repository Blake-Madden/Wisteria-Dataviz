///////////////////////////////////////////////////////////////////////////////
// Name:        pdf_decrypt.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pdf_decrypt.h"
#include <algorithm>
#include <array>
#include <utility>

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    // MD5 (RFC 1321) helpers
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    static uint32_t md5_left_rotate(const uint32_t value, const uint32_t bits)
        {
        return (value << bits) | (value >> (32 - bits));
        }

    //------------------------------------------------------------------
    static void md5_process_chunk(const unsigned char* chunk, uint32_t& a0, uint32_t& b0,
                                  uint32_t& c0, uint32_t& d0)
        {
        // the 64 constants T[i] = floor(abs(sin(i + 1)) x 2^32), per RFC 1321
        constexpr static std::array<uint32_t, 64> K{
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613,
            0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193,
            0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d,
            0x02441453, 0xd8a1e681, 0xe7d3fbc8, 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
            0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122,
            0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
            0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244,
            0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb,
            0xeb86d391
        };
        // per-round left-rotate amounts
        constexpr static std::array<uint32_t, 64> S{
            7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 5,  9,  14, 20, 5,  9,
            14, 20, 5,  9,  14, 20, 5,  9,  14, 20, 4,  11, 16, 23, 4,  11, 16, 23, 4,  11, 16, 23,
            4,  11, 16, 23, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21
        };

        std::array<uint32_t, 16> M{};
        for (size_t wordIndex = 0; wordIndex < 16; ++wordIndex)
            {
            M[wordIndex] = static_cast<uint32_t>(chunk[wordIndex * 4]) |
                           (static_cast<uint32_t>(chunk[(wordIndex * 4) + 1]) << 8) |
                           (static_cast<uint32_t>(chunk[(wordIndex * 4) + 2]) << 16) |
                           (static_cast<uint32_t>(chunk[(wordIndex * 4) + 3]) << 24);
            }

        uint32_t a{ a0 };
        uint32_t b{ b0 };
        uint32_t c{ c0 };
        uint32_t d{ d0 };
        for (uint32_t i = 0; i < 64; ++i)
            {
            uint32_t f{ 0 };
            uint32_t g{ 0 };
            if (i < 16)
                {
                f = (b & c) | (~b & d);
                g = i;
                }
            else if (i < 32)
                {
                f = (d & b) | (~d & c);
                g = ((5 * i) + 1) % 16;
                }
            else if (i < 48)
                {
                f = b ^ c ^ d;
                g = ((3 * i) + 5) % 16;
                }
            else
                {
                f = c ^ (b | ~d);
                g = (7 * i) % 16;
                }
            f += a + K[i] + M[g];
            a = d;
            d = c;
            c = b;
            b += md5_left_rotate(f, S[i]);
            }

        a0 += a;
        b0 += b;
        c0 += c;
        d0 += d;
        }

    //------------------------------------------------------------------
    std::string pdf_decryptor::md5_digest(const std::string_view data)
        {
        uint32_t a0{ 0x67452301 };
        uint32_t b0{ 0xefcdab89 };
        uint32_t c0{ 0x98badcfe };
        uint32_t d0{ 0x10325476 };

        // pad the message: an 0x80 byte, then zero bytes until the length is
        // 56 (mod 64), then the original bit length as a 64-bit little-endian integer
        std::string padded{ data };
        const uint64_t originalBitLength{ static_cast<uint64_t>(data.length()) * 8 };
        padded += static_cast<char>(0x80);
        while ((padded.length() % 64) != 56)
            {
            padded += static_cast<char>(0x00);
            }
        for (size_t byteIndex = 0; byteIndex < 8; ++byteIndex)
            {
            padded += static_cast<char>((originalBitLength >> (byteIndex * 8)) & 0xFF);
            }

        for (size_t chunkStart = 0; chunkStart < padded.length(); chunkStart += 64)
            {
            md5_process_chunk(reinterpret_cast<const unsigned char*>(padded.data()) + chunkStart,
                              a0, b0, c0, d0);
            }

        std::string digest;
        digest.reserve(16);
        for (const uint32_t word : { a0, b0, c0, d0 })
            {
            for (size_t byteIndex = 0; byteIndex < 4; ++byteIndex)
                {
                digest += static_cast<char>((word >> (byteIndex * 8)) & 0xFF);
                }
            }
        return digest;
        }

    //------------------------------------------------------------------
    // RC4
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    std::string pdf_decryptor::rc4_crypt(const std::string_view key, const std::string_view data)
        {
        if (key.empty())
            {
            return std::string{ data };
            }

        std::array<unsigned char, 256> s{};
        for (size_t i = 0; i < 256; ++i)
            {
            s[i] = static_cast<unsigned char>(i);
            }
        size_t j{ 0 };
        for (size_t i = 0; i < 256; ++i)
            {
            j = (j + s[i] + static_cast<unsigned char>(key[i % key.length()])) & 0xFF;
            std::swap(s[i], s[j]);
            }

        std::string output;
        output.reserve(data.length());
        size_t a{ 0 };
        size_t b{ 0 };
        for (const char byteValue : data)
            {
            a = (a + 1) & 0xFF;
            b = (b + s[a]) & 0xFF;
            std::swap(s[a], s[b]);
            output +=
                static_cast<char>(static_cast<unsigned char>(byteValue) ^ s[(s[a] + s[b]) & 0xFF]);
            }
        return output;
        }

    //------------------------------------------------------------------
    // SHA-2 hardened hash (ISO 32000-2, Algorithm 2.A / 2.B), revisions 5/6
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    std::string pdf_decryptor::compute_hardened_hash(const std::string_view password,
                                                     const std::string_view salt,
                                                     const std::string_view userBytes,
                                                     const int revision,
                                                     const sha2_functor& hashFunction,
                                                     const aes_cbc_functor& aesFunction)
        {
        if (!hashFunction)
            {
            return {};
            }
        std::string input{ password };
        input += salt;
        input += userBytes;
        std::string hashKey{ hashFunction(input, 256) };

        // revision 5 (Algorithm 2.A): a single, un-hardened SHA-256 round
        if (revision != 6)
            {
            return hashKey;
            }

        // revision 6 (Algorithm 2.B): the "hardened hash" loop
        if (!aesFunction)
            {
            return {};
            }
        int round{ 0 };
        int lastByte{ 0 };
        while (round < 64 || lastByte > (round - 32))
            {
            std::string combined{ password };
            combined += hashKey;
            combined += userBytes;
            std::string repeatedBuffer;
            repeatedBuffer.reserve(combined.length() * 64);
            for (int repetition = 0; repetition < 64; ++repetition)
                {
                repeatedBuffer += combined;
                }

            const std::string encrypted{ aesFunction(hashKey.substr(0, 16), hashKey.substr(16, 16),
                                                     repeatedBuffer, cipher_direction::encrypt) };
            if (encrypted.length() < 16)
                {
                return {};
                }
            unsigned int byteSum{ 0 };
            for (size_t byteIndex = 0; byteIndex < 16; ++byteIndex)
                {
                byteSum += static_cast<unsigned char>(encrypted[byteIndex]);
                }
            const unsigned int remainder{ byteSum % 3 };
            hashKey = hashFunction(encrypted, (remainder == 0) ? 256 :
                                              (remainder == 1) ? 384 :
                                                                 512);
            lastByte = static_cast<unsigned char>(encrypted.back());
            ++round;
            }
        return hashKey.substr(0, 32);
        }

    //------------------------------------------------------------------
    // standard security handler (empty user password)
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    std::unique_ptr<pdf_decryptor>
    pdf_decryptor::create(const std::string_view ownerKey, const std::string_view userKey,
                          const std::string_view userEncryptionKey,
                          const std::string_view documentId, const int32_t permissions,
                          const int revision, const size_t keyLengthBytes,
                          const bool encryptMetadata, const std::string_view cryptFilterMethod,
                          aes_cbc_functor aesFunction, sha2_functor hashFunction)
        {
        if (revision < 2 || revision > 6)
            {
            return nullptr;
            }

        // revisions 5/6 (/V 5, AES-256): entirely different, SHA-256-based key
        // derivation and authentication (ISO 32000-2, Algorithm 2.A/2.B). Only the
        // empty-user-password path is implemented, matching revisions 2-4 below.
        if (revision == 5 || revision == 6)
            {
            if (userKey.length() < 48 || userEncryptionKey.length() < 32 || !aesFunction ||
                !hashFunction)
                {
                return nullptr;
                }
            const std::string_view userHash{ userKey.substr(0, 32) };
            const std::string_view userValidationSalt{ userKey.substr(32, 8) };
            const std::string_view userKeySalt{ userKey.substr(40, 8) };

            const std::string computedHash{ pdf_decryptor::compute_hardened_hash(
                std::string_view{}, userValidationSalt, std::string_view{}, revision, hashFunction,
                aesFunction) };
            if (computedHash.length() < 32 || computedHash.compare(0, 32, userHash) != 0)
                {
                return nullptr;
                }

            const std::string intermediateKey{ pdf_decryptor::compute_hardened_hash(
                std::string_view{}, userKeySalt, std::string_view{}, revision, hashFunction,
                aesFunction) };
            if (intermediateKey.length() < 32)
                {
                return nullptr;
                }
            const std::string fileKey{ aesFunction(intermediateKey.substr(0, 32),
                                                   std::string(16, '\0'), userEncryptionKey,
                                                   cipher_direction::decrypt) };
            if (fileKey.length() < 32)
                {
                return nullptr;
                }

            std::unique_ptr<pdf_decryptor> decryptor{ new pdf_decryptor{} };
            decryptor->m_encryption_key = fileKey.substr(0, 32);
            decryptor->m_cipher = cipher_algorithm::aes;
            decryptor->m_key_derivation = key_derivation_mode::file_key_direct;
            decryptor->m_aes_decrypt = std::move(aesFunction);
            return decryptor;
            }

        // the standard 32-byte password padding string (ISO 32000-1, 7.6.3.3, Algorithm 2)
        constexpr static std::array<unsigned char, 32> PASSWORD_PADDING{
            0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41, 0x64, 0x00, 0x4E,
            0x56, 0xFF, 0xFA, 0x01, 0x08, 0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68,
            0x3E, 0x80, 0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A
        };
        const std::string paddingStr(PASSWORD_PADDING.cbegin(), PASSWORD_PADDING.cend());
        const size_t effectiveKeyLength{ std::clamp<size_t>(keyLengthBytes, 5, 16) };

        // Algorithm 2: compute the file encryption key. Since the user password is
        // assumed to be empty, the "padded password" is just the padding string in full.
        std::string hashInput{ paddingStr };
        hashInput += ownerKey;
        for (size_t byteIndex = 0; byteIndex < 4; ++byteIndex)
            {
            hashInput +=
                static_cast<char>((static_cast<uint32_t>(permissions) >> (byteIndex * 8)) & 0xFF);
            }
        hashInput += documentId;
        // Revision 4+ can optionally exclude metadata streams from encryption. That case
        // mixes in 4 extra 0xFF bytes when deriving the key.
        if (revision >= 4 && !encryptMetadata)
            {
            hashInput.append(4, static_cast<char>(0xFF));
            }
        std::string hash{ pdf_decryptor::md5_digest(hashInput) };
        if (revision >= 3)
            {
            for (int round = 0; round < 50; ++round)
                {
                hash = pdf_decryptor::md5_digest(hash.substr(0, effectiveKeyLength));
                }
            }
        std::string encryptionKey{ hash.substr(0, effectiveKeyLength) };

        // Algorithm 4 (revision 2) or Algorithm 5 (revision 3/4): recompute /U from the
        // (empty) user password and the encryption key just derived
        std::string computedU;
        if (revision == 2)
            {
            computedU = pdf_decryptor::rc4_crypt(encryptionKey, paddingStr);
            }
        else
            {
            std::string idHashInput{ paddingStr };
            idHashInput += documentId;
            computedU =
                pdf_decryptor::rc4_crypt(encryptionKey, pdf_decryptor::md5_digest(idHashInput));
            for (int round = 1; round <= 19; ++round)
                {
                std::string roundKey{ encryptionKey };
                for (char& keyByte : roundKey)
                    {
                    keyByte = static_cast<char>(static_cast<unsigned char>(keyByte) ^ round);
                    }
                computedU = pdf_decryptor::rc4_crypt(roundKey, computedU);
                }
            }

        // Algorithm 6: authenticate by comparing to the file's actual /U value. For
        // revision 3/4, only the first 16 bytes are significant (the rest is padding).
        const size_t compareLength{ (revision == 2) ? size_t{ 32 } : size_t{ 16 } };
        if (userKey.length() < compareLength || computedU.length() < compareLength ||
            userKey.compare(0, compareLength, computedU, 0, compareLength) != 0)
            {
            return nullptr;
            }

        if (cryptFilterMethod == "/AESV2" && !aesFunction)
            {
            return nullptr;
            }

        std::unique_ptr<pdf_decryptor> decryptor{ new pdf_decryptor{} };
        decryptor->m_encryption_key = std::move(encryptionKey);
        if (cryptFilterMethod == "/AESV2")
            {
            decryptor->m_cipher = cipher_algorithm::aes;
            decryptor->m_aes_decrypt = std::move(aesFunction);
            }
        return decryptor;
        }

    //------------------------------------------------------------------
    std::string pdf_decryptor::compute_object_key(const long objectNumber, const long generation,
                                                  const cipher_algorithm algorithm) const
        {
        // Algorithm 1 (or its AES variant, Algorithm 1.A): object key = first
        // min(n + 5, 16) bytes of MD5(file encryption key +
        //     low-order 3 bytes of object number (LE) +
        //     low-order 2 bytes of generation number (LE) +
        //     ["sAlT", if AES])
        std::string keyInput{ m_encryption_key };
        keyInput += static_cast<char>(objectNumber & 0xFF);
        keyInput += static_cast<char>((objectNumber >> 8) & 0xFF);
        keyInput += static_cast<char>((objectNumber >> 16) & 0xFF);
        keyInput += static_cast<char>(generation & 0xFF);
        keyInput += static_cast<char>((generation >> 8) & 0xFF);
        if (algorithm == cipher_algorithm::aes)
            {
            keyInput += "sAlT";
            }
        const std::string hash{ pdf_decryptor::md5_digest(keyInput) };
        return hash.substr(0, std::min<size_t>(m_encryption_key.length() + 5, 16));
        }

    //------------------------------------------------------------------
    std::string pdf_decryptor::decrypt(const long objectNumber, const long generation,
                                       const std::string_view bytes) const
        {
        if (bytes.empty())
            {
            return {};
            }

        if (m_cipher == cipher_algorithm::aes)
            {
            if (!m_aes_decrypt || bytes.length() < 16)
                {
                return {};
                }
            const std::string key{ (m_key_derivation == key_derivation_mode::file_key_direct) ?
                                       m_encryption_key :
                                       compute_object_key(objectNumber, generation,
                                                          cipher_algorithm::aes) };
            const std::string_view initVector{ bytes.substr(0, 16) };
            const std::string_view ciphertext{ bytes.substr(16) };
            std::string plaintext{ m_aes_decrypt(key, initVector, ciphertext,
                                                 cipher_direction::decrypt) };
            // strip PKCS#7 padding (ISO 32000-1, 7.6.2: 1-16 bytes, each equal to the pad length)
            if (!plaintext.empty())
                {
                const auto padLength{ static_cast<unsigned char>(plaintext.back()) };
                if (padLength >= 1 && padLength <= 16 && padLength <= plaintext.length())
                    {
                    plaintext.resize(plaintext.length() - padLength);
                    }
                }
            return plaintext;
            }

        return pdf_decryptor::rc4_crypt(
            compute_object_key(objectNumber, generation, cipher_algorithm::rc4), bytes);
        }
    } // namespace lily_of_the_valley
