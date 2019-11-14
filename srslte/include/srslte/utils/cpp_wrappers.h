#ifndef _CPP_WRAPPERS_H_
#define _CPP_WRAPPERS_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <semaphore.h>

#include "../intf/intf.h"

#ifdef __cplusplus

#include <math.h>
#include <stdio.h>
#include <iostream>
#include <stdexcept>
#include <queue>
#include <map>
#include <vector>
#include <boost/circular_buffer.hpp>

#include "srslte/ue/ue_sync.h"
#include "../intf/intf.h"

struct sync_vector_t {
  std::vector<short_ue_sync_t>* sync_vector_ptr;
};

struct sync_cb_t {
  boost::circular_buffer<short_ue_sync_t>* sync_cb_ptr;
};

struct tx_cb_t {
  boost::circular_buffer<slot_ctrl_t>* tx_cb_ptr;
};

struct vphy_tx_semaphore_cb_t {
  boost::circular_buffer<sem_t*>* vphy_tx_slot_done_semaphore_cb_ptr;
};

struct phy_control_cb_t {
  boost::circular_buffer<phy_ctrl_t>* phy_control_cb_ptr;
};

struct chan_buf_ctx_cb_t {
  boost::circular_buffer<channel_buffer_context_t>* chan_buf_ctx_cb_ptr;
};

struct in_buf_ctx_cb_t {
  boost::circular_buffer<uint32_t>* in_buf_ctx_cb_ptr;
};

extern "C" {
#else
struct sync_vector_t;
struct sync_cb_t;
struct tx_cb_t;
struct vphy_tx_semaphore_cb_t;
struct phy_control_cb_t;
struct chan_buf_ctx_cb_t;
struct in_buf_ctx_cb_t;
#endif

#include <inttypes.h>

#include "srslte/ue/ue_sync.h"
#include "../intf/intf.h"

typedef struct sync_vector_t* sync_vector_handle;
typedef struct sync_cb_t* sync_cb_handle;
typedef struct tx_cb_t* tx_cb_handle;
typedef struct vphy_tx_semaphore_cb_t* vphy_tx_semaphore_cb_handle;
typedef struct phy_control_cb_t* phy_control_cb_handle;
typedef struct chan_buf_ctx_cb_t* chan_buf_ctx_cb_handle;
typedef struct in_buf_ctx_cb_t* in_buf_ctx_cb_handle;

//******************************************************************************
SRSLTE_API void sync_cb_make(sync_cb_handle* handle, uint64_t size);

SRSLTE_API void sync_cb_free(sync_cb_handle* handle);

SRSLTE_API void sync_cb_push_back(sync_cb_handle handle, short_ue_sync_t* const short_ue_sync);

SRSLTE_API void sync_cb_read(sync_cb_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync);

SRSLTE_API void sync_cb_front(sync_cb_handle handle, short_ue_sync_t* const short_ue_sync);

SRSLTE_API void sync_cb_pop_front(sync_cb_handle handle);

SRSLTE_API bool sync_cb_empty(sync_cb_handle handle);

SRSLTE_API int sync_cb_size(sync_cb_handle handle);
//******************************************************************************

//******************************************************************************
SRSLTE_API void sync_vector_make(sync_vector_handle* handle);

SRSLTE_API void sync_vector_free(sync_vector_handle* handle);

SRSLTE_API void sync_vector_push(sync_vector_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync);

SRSLTE_API void sync_vector_read(sync_vector_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync);

SRSLTE_API void sync_vector_pop_back(sync_vector_handle handle);

SRSLTE_API bool sync_vector_empty(sync_vector_handle handle);

SRSLTE_API int sync_vector_size(sync_vector_handle handle);

SRSLTE_API void sync_vector_reserve(sync_vector_handle handle, uint64_t size);
//******************************************************************************

//******************************************************************************
SRSLTE_API void tx_cb_make(tx_cb_handle* handle, uint64_t size);

SRSLTE_API void tx_cb_free(tx_cb_handle* handle);

SRSLTE_API void tx_cb_push_back(tx_cb_handle handle, slot_ctrl_t* const slot_ctrl);

SRSLTE_API void tx_cb_read(tx_cb_handle handle, uint64_t index, slot_ctrl_t* const slot_ctrl);

SRSLTE_API void tx_cb_front(tx_cb_handle handle, slot_ctrl_t* const slot_ctrl);

SRSLTE_API void tx_cb_pop_front(tx_cb_handle handle);

SRSLTE_API bool tx_cb_empty(tx_cb_handle handle);

SRSLTE_API int tx_cb_size(tx_cb_handle handle);
//******************************************************************************

// *****************************************************************************
SRSLTE_API void vphy_tx_semaphore_cb_make(vphy_tx_semaphore_cb_handle* handle, uint64_t size);

SRSLTE_API void vphy_tx_semaphore_cb_free(vphy_tx_semaphore_cb_handle* handle);

SRSLTE_API bool vphy_tx_semaphore_cb_empty(vphy_tx_semaphore_cb_handle handle);

SRSLTE_API void vphy_tx_semaphore_cb_push_back(vphy_tx_semaphore_cb_handle handle, sem_t* const semaphore);

SRSLTE_API sem_t* vphy_tx_semaphore_cb_read(vphy_tx_semaphore_cb_handle handle, uint64_t index);

SRSLTE_API sem_t* vphy_tx_semaphore_cb_front(vphy_tx_semaphore_cb_handle handle);

SRSLTE_API void vphy_tx_semaphore_cb_pop_front(vphy_tx_semaphore_cb_handle handle);

SRSLTE_API uint32_t vphy_tx_semaphore_cb_size(vphy_tx_semaphore_cb_handle handle);
//*****************************************************************************

//******************************************************************************
SRSLTE_API void phy_control_cb_make(phy_control_cb_handle* handle, uint64_t size);

SRSLTE_API void phy_control_cb_free(phy_control_cb_handle* handle);

SRSLTE_API void phy_control_cb_push_back(phy_control_cb_handle handle, phy_ctrl_t* const phy_ctrl);

SRSLTE_API void phy_control_cb_read(phy_control_cb_handle handle, uint64_t index, phy_ctrl_t* const phy_ctrl);

SRSLTE_API void phy_control_cb_front(phy_control_cb_handle handle, phy_ctrl_t* const phy_ctrl);

SRSLTE_API void phy_control_cb_pop_front(phy_control_cb_handle handle);

SRSLTE_API bool phy_control_cb_empty(phy_control_cb_handle handle);

SRSLTE_API int phy_control_cb_size(phy_control_cb_handle handle);
//******************************************************************************

//******************************************************************************
SRSLTE_API void channel_buffer_ctx_cb_make(chan_buf_ctx_cb_handle* handle, uint64_t size);

SRSLTE_API void channel_buffer_ctx_cb_free(chan_buf_ctx_cb_handle* handle);

SRSLTE_API void channel_buffer_ctx_cb_push_back(chan_buf_ctx_cb_handle handle, channel_buffer_context_t* const chan_buf_ctx);

SRSLTE_API void channel_buffer_ctx_cb_read(chan_buf_ctx_cb_handle handle, uint64_t index, channel_buffer_context_t* const chan_buf_ctx);

SRSLTE_API void channel_buffer_ctx_cb_front(chan_buf_ctx_cb_handle handle, channel_buffer_context_t* const chan_buf_ctx);

SRSLTE_API void channel_buffer_ctx_cb_pop_front(chan_buf_ctx_cb_handle handle);

SRSLTE_API bool channel_buffer_ctx_cb_empty(chan_buf_ctx_cb_handle handle);

SRSLTE_API int channel_buffer_ctx_cb_size(chan_buf_ctx_cb_handle handle);
//******************************************************************************

//******************************************************************************
SRSLTE_API void input_buffer_ctx_cb_make(in_buf_ctx_cb_handle* handle, uint64_t size);

SRSLTE_API void input_buffer_ctx_cb_free(in_buf_ctx_cb_handle* handle);

SRSLTE_API void input_buffer_ctx_cb_push_back(in_buf_ctx_cb_handle handle, uint32_t in_buffer_cnt);

SRSLTE_API uint32_t input_buffer_ctx_cb_read(in_buf_ctx_cb_handle handle, uint64_t index);

SRSLTE_API uint32_t input_buffer_ctx_cb_front(in_buf_ctx_cb_handle handle);

SRSLTE_API void input_buffer_ctx_cb_pop_front(in_buf_ctx_cb_handle handle);

SRSLTE_API bool input_buffer_ctx_cb_empty(in_buf_ctx_cb_handle handle);

SRSLTE_API int input_buffer_ctx_cb_size(in_buf_ctx_cb_handle handle);
//******************************************************************************

#ifdef __cplusplus
}
#endif

#endif /* _CPP_WRAPPERS_H_ */
