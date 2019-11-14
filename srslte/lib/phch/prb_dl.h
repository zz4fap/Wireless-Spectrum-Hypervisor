/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 Software Radio Systems Limited
 *
 * \section LICENSE
 *
 * This file is part of the srsLTE library.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

 #include <stdio.h>
 #define __STDC_FORMAT_MACROS
 #include <inttypes.h>
 #include <stdint.h>
 #include <complex.h>    /* Standard Library of Complex Numbers */
 #include <string.h>
 #include <stdlib.h>
 #include <stdbool.h>

#include "srslte/config.h"

void prb_cp_ref(cf_t **input, cf_t **output, int offset, int nof_refs, int nof_intervals, bool advance_input);
void prb_cp(cf_t **input, cf_t **output, int nof_prb);
void prb_cp_half(cf_t **input, cf_t **output, int nof_prb);
void prb_put_ref_(cf_t **input, cf_t **output, int offset, int nof_refs, int nof_intervals);

// *****************************************************************************
// Implementations for McF-TDMA - START
// *****************************************************************************
// Copy data symbols to OFDM symbol without CRS respecting DC subcarrier.
// Physical Resource Block (PRB) copy.
void prb_cp_with_dc(cf_t **input, cf_t **output, int nof_prb, uint32_t radio_fft_len, uint32_t init_pos);

// Copy data symbols to OFDM symbol with CRS respecting DC subcarrier.
void prb_cp_ref_with_dc(cf_t **input, cf_t **output, int offset, int nof_refs, int nof_intervals, bool advance_output, uint32_t nof_prb, uint32_t radio_fft_len, uint32_t init_pos);

// Copy data symbols to OFDM symbol without CRS respecting DC subcarrier.
void prb_cp_half_with_dc(cf_t **input, cf_t **output, int nof_prb, uint32_t radio_fft_len, uint32_t init_pos);

// *****************************************************************************
// Implementations for McF-TDMA - END
// *****************************************************************************
