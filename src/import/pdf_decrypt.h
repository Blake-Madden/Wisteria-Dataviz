/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause

     The MD5, RC4, and standard security handler key-derivation algorithms below
     were implemented from the PDF spec (ISO 32000-1, 7.6, "Encryption"), and were
     cross-checked for correctness against Mozilla's PDF.js (Apache License 2.0),
     specifically src/core/calculate_md5.js and src/core/crypto.js.
@{*/

#ifndef PDF_DECRYPT_H
#define PDF_DECRYPT_H

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace lily_of_the_valley
    {
    /// @brief Decrypts strings and streams in a PDF that uses the "Standard" security
    ///     handler, assuming an empty user password.
    /// @details This covers the common case of a PDF that is merely restricted from
    ///     editing, printing, etc. (i.e., it has an owner password), but opens for
    ///     reading without prompting for a password.\n
    ///     Only RC4-based encryption is supported: algorithm versions `/V` 1 and 2
    ///     (revisions `/R` 2 and 3), and version 4 (revision 4) when its crypt filter's
    ///     `/CFM` is `/V2` (RC4). AES-based encryption (`/CFM` `/AESV2` or `/AESV3`, and
    ///     `/V` 5 with revisions 5/6) is not currently supported.
    /// @sa ISO 32000-1:2008, 7.6 ("Encryption").
    class pdf_decryptor
        {
      public:
        /// @brief Attempts to authenticate against the encryption dictionary's `/U`
        ///     value using an empty user password.
        /// @param ownerKey The raw bytes of the `/O` value.
        /// @param userKey The raw bytes of the `/U` value.
        /// @param documentId The raw bytes of the first element of the trailer's
        ///     `/ID` array (may be empty if the file has no `/ID`).
        /// @param permissions The `/P` value (a 32-bit signed permissions bitmask).
        /// @param revision The `/R` (revision) value. Only 2-4 are supported.
        /// @param keyLengthBytes The file encryption key's length, in bytes (5-16).
        /// @param encryptMetadata The `/EncryptMetadata` value (default @c true).
        /// @returns The decryptor, or @c nullptr if @c revision isn't supported or
        ///     authentication (with an empty password) failed.
        [[nodiscard]]
        static std::unique_ptr<pdf_decryptor>
        create(std::string_view ownerKey, std::string_view userKey, std::string_view documentId,
               int32_t permissions, int revision, size_t keyLengthBytes,
               bool encryptMetadata = true);

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
        pdf_decryptor() = default;

        /// @brief Derives the per-object RC4 key (Algorithm 1) from the file
        ///     encryption key and an object's number/generation.
        [[nodiscard]]
        std::string compute_object_key(long objectNumber, long generation) const;

        std::string m_encryption_key;
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // PDF_DECRYPT_H
