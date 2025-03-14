/**
 * @file version.h
 * @brief Version information for the COIL assembler framework
 * @details This file defines constants and functions for retrieving
 *          version information about the COIL assembler framework.
 *
 * @version 0.1.0
 * @date 2025-03-14
 * @copyright Copyright (c) 2025 Low Level Team (LLT)
 */

#ifndef COIL_VERSION_H
#define COIL_VERSION_H

/**
 * @brief COIL assembler version major component
 */
#define COIL_VERSION_MAJOR 0

/**
 * @brief COIL assembler version minor component
 */
#define COIL_VERSION_MINOR 1

/**
 * @brief COIL assembler version patch component
 */
#define COIL_VERSION_PATCH 0

/**
 * @brief COIL assembler full version string
 */
#define COIL_VERSION "0.1.0"

/**
 * @brief COIL assembler build commit hash
 */
#define COIL_COMMIT "@COIL_COMMIT@"

/**
 * @brief Gets the COIL assembler version string
 * @return Version string in the format "MAJOR.MINOR.PATCH"
 */
const char* coil_version_string(void);

/**
 * @brief Gets the COIL assembler major version number
 * @return Major version number
 */
int coil_version_major(void);

/**
 * @brief Gets the COIL assembler minor version number
 * @return Minor version number
 */
int coil_version_minor(void);

/**
 * @brief Gets the COIL assembler patch version number
 * @return Patch version number
 */
int coil_version_patch(void);

/**
 * @brief Gets the COIL assembler build commit hash
 * @return Build commit hash
 */
const char* coil_version_commit(void);

/**
 * @brief Checks if the current version is compatible with the required version
 * @param required_major Required major version
 * @param required_minor Required minor version
 * @param required_patch Required patch version
 * @return 1 if compatible, 0 if not
 */
int coil_version_is_compatible(int required_major, int required_minor, int required_patch);

#endif /* COIL_VERSION_H */