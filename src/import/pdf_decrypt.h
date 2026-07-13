/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause

     The MD5, RC4, and standard security handler key-derivation algorithms below
     were implemented from the PDF spec (ISO 32000-1, 7.6, "Encryption").
     For the revision 5/6 "hardened hash" key derivation, ISO 32000-2:2020, 7.6 was referenced.
     Also cross-checked for correctness against Mozilla's PDF.js (Apache License 2.0),
     specifically src/core/calculate_md5.js and src/core/crypto.js.
@{*/

#ifndef PDF_DECRYPT_H
#define PDF_DECRYPT_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace lily_of_the_valley
    {
    /// @brief Direction for an injected block-cipher functor (@c aes_cbc_functor).
    enum class cipher_direction
        {
        encrypt,
        decrypt
        };

    /// @brief Functor for performing raw AES-CBC encryption/decryption (no padding
    ///     added or removed by the functor itself; @c pdf_decryptor handles PDF's
    ///     PKCS#7 padding and IV placement conventions on top of this).
    /// @param key The AES key; its length (16 or 32 bytes) selects AES-128 or AES-256.
    /// @param initVector The 16-byte initialization vector.
    /// @param data The bytes to transform (must be a multiple of 16 bytes long).
    /// @param direction Whether to encrypt or decrypt @p data.
    /// @returns The transformed bytes.
    using aes_cbc_functor =
        std::function<std::string(std::string_view key, std::string_view initVector,
                                  std::string_view data, cipher_direction direction)>;

    /// @brief Functor for computing a SHA-2 message digest.
    /// @param data The bytes to hash.
    /// @param digestBits Which SHA-2 variant to use: 256, 384, or 512.
    /// @returns The binary digest (32, 48, or 64 bytes).
    using sha2_functor = std::function<std::string(std::string_view data, int digestBits)>;

    /// @brief Decrypts strings and streams in a PDF that uses the "Standard" security
    ///     handler, assuming an empty user password.
    /// @details This covers the common case of a PDF that is merely restricted from
    ///     editing, printing, etc. (i.e., it has an owner password), but opens for
    ///     reading without prompting for a password.\n
    ///     Supports:\n
    ///     - RC4 (algorithm versions `/V` 1 and 2, revisions `/R` 2 and 3, and
    ///       version 4 when its crypt filter's `/CFM` is `/V2`)\n
    ///     - AES-128 (version 4 with `/CFM` `/AESV2`)\n
    ///     - AES-256 (version 5, revisions 5/6, `/CFM` `/AESV3`)\n
    ///     AES support requires the caller to supply an @c aes_cbc_functor (and, for
    ///     revisions 5/6, a @c sha2_functor) since this class has no cipher/hashing
    ///     implementation of its own.
    /// @sa ISO 32000-1:2008, 7.6 ("Encryption"); ISO 32000-2:2020, 7.6 ("Encryption")
    ///     for the `/V` 5 (AES-256) additions.
    class pdf_decryptor
        {
      public:
        /// @brief Attempts to authenticate against the encryption dictionary's `/U`
        ///     value using an empty user password.
        /// @param ownerKey The raw bytes of the `/O` value.
        /// @param userKey The raw bytes of the `/U` value.
        /// @param userEncryptionKey The raw bytes of the `/UE` value (revisions 5/6
        ///     only; pass an empty string otherwise).
        /// @param documentId The raw bytes of the first element of the trailer's
        ///     `/ID` array (may be empty if the file has no `/ID`).
        /// @param permissions The `/P` value (a 32-bit signed permissions bitmask).
        /// @param revision The `/R` (revision) value. (2-6 are supported.)
        /// @param keyLengthBytes The file encryption key's length, in bytes (5-16;
        ///     ignored for revisions 5/6, which are always 32).
        /// @param encryptMetadata The `/EncryptMetadata` value (default @c true).
        /// @param cryptFilterMethod The resolved `/CF`/`StdCF`/`CFM` name (e.g.,
        ///     `"/V2"`, `"/AESV2"`, `"/AESV3"`), or empty if there is no crypt filter
        ///     (revisions 2/3).
        /// @param aesFunction Functor for AES-CBC encryption/decryption; required
        ///     when @p cryptFilterMethod is `"/AESV2"`/`"/AESV3"` or @p revision is 5/6.
        /// @param hashFunction Functor for SHA-2 hashing; required when @p revision
        ///     is 5 or 6.
        /// @returns The decryptor, or @c nullptr if @p revision isn't supported,
        ///     a required functor is missing, or authentication (with an empty
        ///     password) failed.
        [[nodiscard]]
        static std::unique_ptr<pdf_decryptor>
        create(std::string_view ownerKey, std::string_view userKey,
               std::string_view userEncryptionKey, std::string_view documentId, int32_t permissions,
               int revision, size_t keyLengthBytes, bool encryptMetadata = true,
               std::string_view cryptFilterMethod = {}, aes_cbc_functor aesFunction = {},
               sha2_functor hashFunction = {});

        /// @brief Decrypts a string or stream's raw bytes belonging to the given object.
        /// @param objectNumber The number of the (containing) indirect object.
        /// @param generation The generation number of the (containing) indirect object.
        /// @param bytes The encrypted bytes.
        /// @returns The decrypted bytes.
        [[nodiscard]]
        std::string decrypt(long objectNumber, long generation, std::string_view bytes) const;

        /// @brief Calculates the binary MD5 message digest of a byte sequence.
        /// @returns The 16-byte digest.
        [[nodiscard]]
        static std::string md5_digest(std::string_view data);

        /// @brief Encrypts or decrypts (a symmetric operation) a byte sequence with RC4.
        [[nodiscard]]
        static std::string rc4_crypt(std::string_view key, std::string_view data);

      private:
        /// @brief Which cipher @c decrypt() should use.
        enum class cipher_algorithm
            {
            rc4,
            aes
            };

        /// @brief Whether @c decrypt() derives a fresh per-object key (Algorithm 1)
        ///     or uses the file encryption key directly (revisions 5/6).
        enum class key_derivation_mode
            {
            per_object,
            file_key_direct
            };

        pdf_decryptor() = default;

        /// @brief Derives the per-object RC4/AES-128 key (Algorithm 1, or its
        ///     "sAlT"-suffixed AES variant, Algorithm 1.A) from the file encryption
        ///     key and an object's number/generation. Not used for revisions 5/6,
        ///     which use the file encryption key directly for every object.
        [[nodiscard]]
        std::string compute_object_key(long objectNumber, long generation,
                                       cipher_algorithm algorithm) const;

        /// @brief Computes ISO 32000-2's Algorithm 2.A (revision 5) or 2.B (revision 6,
        ///     the "hardened hash") over @p password + @p salt [+ @p userBytes].
        /// @returns The 32-byte hash, or an empty string if @p hashFunction or
        ///     (for revision 6) @p aesFunction is empty.
        [[nodiscard]]
        static std::string compute_hardened_hash(std::string_view password, std::string_view salt,
                                                 std::string_view userBytes, int revision,
                                                 const sha2_functor& hashFunction,
                                                 const aes_cbc_functor& aesFunction);

        std::string m_encryption_key;
        cipher_algorithm m_cipher{ cipher_algorithm::rc4 };
        key_derivation_mode m_key_derivation{ key_derivation_mode::per_object };
        aes_cbc_functor m_aes_decrypt;
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // PDF_DECRYPT_H
