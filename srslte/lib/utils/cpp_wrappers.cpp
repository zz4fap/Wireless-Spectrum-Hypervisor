#include "srslte/utils/cpp_wrappers.h"

//************************** Sync Circular buffer ********************************
void sync_cb_make(sync_cb_handle* handle, uint64_t size) {
  *handle = new sync_cb_t;
  (*handle)->sync_cb_ptr = new boost::circular_buffer<short_ue_sync_t>(size);
}

// Free all allocated resources.
void sync_cb_free(sync_cb_handle* handle) {
  boost::circular_buffer<short_ue_sync_t> *ptr = ((*handle)->sync_cb_ptr);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

// Push ue sync structure into vector.
void sync_cb_push_back(sync_cb_handle handle, short_ue_sync_t* const short_ue_sync) {
  handle->sync_cb_ptr->push_back(*short_ue_sync);
}

// Read element from vector.
void sync_cb_read(sync_cb_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync) {
  *short_ue_sync = (*handle->sync_cb_ptr)[index];
}

void sync_cb_front(sync_cb_handle handle, short_ue_sync_t* const short_ue_sync) {
  *short_ue_sync = handle->sync_cb_ptr->front();
}

void sync_cb_pop_front(sync_cb_handle handle) {
  handle->sync_cb_ptr->pop_front();
}

bool sync_cb_empty(sync_cb_handle handle) {
  return handle->sync_cb_ptr->empty();
}

int sync_cb_size(sync_cb_handle handle) {
  return static_cast<int>(handle->sync_cb_ptr->size());
}

//************************** Sync vector ********************************
void sync_vector_make(sync_vector_handle* handle) {
  *handle = new sync_vector_t;
  (*handle)->sync_vector_ptr = new std::vector<short_ue_sync_t>;
}

// Free all allocated resources.
void sync_vector_free(sync_vector_handle* handle) {
  std::vector<short_ue_sync_t> *ptr = ((*handle)->sync_vector_ptr);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

// Reserve a number of positions.
void sync_vector_reserve(sync_vector_handle handle, uint64_t size) {
  handle->sync_vector_ptr->reserve(size);
}

// Push ue sync structure into vector.
void sync_vector_push(sync_vector_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync) {
  handle->sync_vector_ptr->push_back(*short_ue_sync);
}

// Read element from vector.
void sync_vector_read(sync_vector_handle handle, uint64_t index, short_ue_sync_t* const short_ue_sync) {
  *short_ue_sync = (*handle->sync_vector_ptr)[index];
}

void sync_vector_pop_back(sync_vector_handle handle) {
  handle->sync_vector_ptr->pop_back();
}

bool sync_vector_empty(sync_vector_handle handle) {
  return handle->sync_vector_ptr->empty();
}

int sync_vector_size(sync_vector_handle handle) {
  return static_cast<int>(handle->sync_vector_ptr->size());
}

//************************** Tx basic control Circular buffer ********************************
void tx_cb_make(tx_cb_handle* handle, uint64_t size) {
  *handle = new tx_cb_t;
  (*handle)->tx_cb_ptr = new boost::circular_buffer<slot_ctrl_t>(size);
}

// Free all allocated resources.
void tx_cb_free(tx_cb_handle* handle) {
  boost::circular_buffer<slot_ctrl_t> *ptr = ((*handle)->tx_cb_ptr);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

// Push ue sync structure into vector.
void tx_cb_push_back(tx_cb_handle handle, slot_ctrl_t* const slot_ctrl) {
  handle->tx_cb_ptr->push_back(*slot_ctrl);
}

// Read element from vector.
void tx_cb_read(tx_cb_handle handle, uint64_t index, slot_ctrl_t* const slot_ctrl) {
  *slot_ctrl = (*handle->tx_cb_ptr)[index];
}

void tx_cb_front(tx_cb_handle handle, slot_ctrl_t* const slot_ctrl) {
  *slot_ctrl = handle->tx_cb_ptr->front();
}

void tx_cb_pop_front(tx_cb_handle handle) {
  handle->tx_cb_ptr->pop_front();
}

bool tx_cb_empty(tx_cb_handle handle) {
  return handle->tx_cb_ptr->empty();
}

int tx_cb_size(tx_cb_handle handle) {
  return static_cast<int>(handle->tx_cb_ptr->size());
}

// ******************* Semaphore FIFO for Hypervisor Tx ************************
void vphy_tx_semaphore_cb_make(vphy_tx_semaphore_cb_handle* handle, uint64_t size) {
  *handle = new vphy_tx_semaphore_cb_t;
  (*handle)->vphy_tx_slot_done_semaphore_cb_ptr = new boost::circular_buffer<sem_t*>(size);
}

// Free all allocated resources.
void vphy_tx_semaphore_cb_free(vphy_tx_semaphore_cb_handle* handle) {
  boost::circular_buffer<sem_t*> *ptr = ((*handle)->vphy_tx_slot_done_semaphore_cb_ptr);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

bool vphy_tx_semaphore_cb_empty(vphy_tx_semaphore_cb_handle handle) {
  return handle->vphy_tx_slot_done_semaphore_cb_ptr->empty();
}

uint32_t vphy_tx_semaphore_cb_size(vphy_tx_semaphore_cb_handle handle) {
  return static_cast<int>(handle->vphy_tx_slot_done_semaphore_cb_ptr->size());
}

void vphy_tx_semaphore_cb_push_back(vphy_tx_semaphore_cb_handle handle, sem_t* const semaphore) {
  handle->vphy_tx_slot_done_semaphore_cb_ptr->push_back(semaphore);
}

// Read element from circular buffer.
sem_t* vphy_tx_semaphore_cb_read(vphy_tx_semaphore_cb_handle handle, uint64_t index) {
  sem_t* sem;
  sem = (*handle->vphy_tx_slot_done_semaphore_cb_ptr)[index];
  return sem;
}

sem_t* vphy_tx_semaphore_cb_front(vphy_tx_semaphore_cb_handle handle) {
  sem_t* sem;
  sem = handle->vphy_tx_slot_done_semaphore_cb_ptr->front();
  return sem;
}

void vphy_tx_semaphore_cb_pop_front(vphy_tx_semaphore_cb_handle handle) {
  handle->vphy_tx_slot_done_semaphore_cb_ptr->pop_front();
}

//************************** PHY control Circular buffer ********************************
void phy_control_cb_make(phy_control_cb_handle* handle, uint64_t size) {
  *handle = new phy_control_cb_t;
  (*handle)->phy_control_cb_ptr = new boost::circular_buffer<phy_ctrl_t>(size);
}

// Free all allocated resources.
void phy_control_cb_free(phy_control_cb_handle* handle) {
  boost::circular_buffer<phy_ctrl_t> *ptr = ((*handle)->phy_control_cb_ptr);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

// Push ue sync structure into vector.
void phy_control_cb_push_back(phy_control_cb_handle handle, phy_ctrl_t* const phy_ctrl) {
  handle->phy_control_cb_ptr->push_back(*phy_ctrl);
}

// Read element from vector.
void phy_control_cb_read(phy_control_cb_handle handle, uint64_t index, phy_ctrl_t* const phy_ctrl) {
  *phy_ctrl = (*handle->phy_control_cb_ptr)[index];
}

void phy_control_cb_front(phy_control_cb_handle handle, phy_ctrl_t* const phy_ctrl) {
  *phy_ctrl = handle->phy_control_cb_ptr->front();
}

void phy_control_cb_pop_front(phy_control_cb_handle handle) {
  handle->phy_control_cb_ptr->pop_front();
}

bool phy_control_cb_empty(phy_control_cb_handle handle) {
  return handle->phy_control_cb_ptr->empty();
}

int phy_control_cb_size(phy_control_cb_handle handle) {
  return static_cast<int>(handle->phy_control_cb_ptr->size());
}

//************************** Channel Buffer Context Circular Buffer ********************************
void channel_buffer_ctx_cb_make(chan_buf_ctx_cb_handle* handle, uint64_t size) {
  *handle = new chan_buf_ctx_cb_t;
  (*handle)->chan_buf_ctx_cb_ptr = new boost::circular_buffer<channel_buffer_context_t>(size);
}

// Free all allocated resources.
void channel_buffer_ctx_cb_free(chan_buf_ctx_cb_handle* handle) {
  boost::circular_buffer<channel_buffer_context_t> *ptr = ((*handle)->chan_buf_ctx_cb_ptr);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

// Push ue sync structure into vector.
void channel_buffer_ctx_cb_push_back(chan_buf_ctx_cb_handle handle, channel_buffer_context_t* const chan_buf_ctx) {
  handle->chan_buf_ctx_cb_ptr->push_back(*chan_buf_ctx);
}

// Read element from vector.
void channel_buffer_ctx_cb_read(chan_buf_ctx_cb_handle handle, uint64_t index, channel_buffer_context_t* const chan_buf_ctx) {
  *chan_buf_ctx = (*handle->chan_buf_ctx_cb_ptr)[index];
}

void channel_buffer_ctx_cb_front(chan_buf_ctx_cb_handle handle, channel_buffer_context_t* const chan_buf_ctx) {
  *chan_buf_ctx = handle->chan_buf_ctx_cb_ptr->front();
}

void channel_buffer_ctx_cb_pop_front(chan_buf_ctx_cb_handle handle) {
  handle->chan_buf_ctx_cb_ptr->pop_front();
}

bool channel_buffer_ctx_cb_empty(chan_buf_ctx_cb_handle handle) {
  return handle->chan_buf_ctx_cb_ptr->empty();
}

int channel_buffer_ctx_cb_size(chan_buf_ctx_cb_handle handle) {
  return static_cast<int>(handle->chan_buf_ctx_cb_ptr->size());
}

//************************** Input IQ Samples Buffer Context Circular Buffer ********************************
void input_buffer_ctx_cb_make(in_buf_ctx_cb_handle* handle, uint64_t size) {
  *handle = new in_buf_ctx_cb_t;
  (*handle)->in_buf_ctx_cb_ptr = new boost::circular_buffer<uint32_t>(size);
}

// Free all allocated resources.
void input_buffer_ctx_cb_free(in_buf_ctx_cb_handle* handle) {
  boost::circular_buffer<uint32_t> *ptr = ((*handle)->in_buf_ctx_cb_ptr);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

// Push input IQ sample buffer counter into circular buffer.
void input_buffer_ctx_cb_push_back(in_buf_ctx_cb_handle handle, uint32_t in_buffer_cnt) {
  handle->in_buf_ctx_cb_ptr->push_back(in_buffer_cnt);
}

// Read element from vector.
uint32_t input_buffer_ctx_cb_read(in_buf_ctx_cb_handle handle, uint64_t index) {
  return (*handle->in_buf_ctx_cb_ptr)[index];
}

uint32_t input_buffer_ctx_cb_front(in_buf_ctx_cb_handle handle) {
  return handle->in_buf_ctx_cb_ptr->front();
}

void input_buffer_ctx_cb_pop_front(in_buf_ctx_cb_handle handle) {
  handle->in_buf_ctx_cb_ptr->pop_front();
}

bool input_buffer_ctx_cb_empty(in_buf_ctx_cb_handle handle) {
  return handle->in_buf_ctx_cb_ptr->empty();
}

int input_buffer_ctx_cb_size(in_buf_ctx_cb_handle handle) {
  return static_cast<int>(handle->in_buf_ctx_cb_ptr->size());
}
