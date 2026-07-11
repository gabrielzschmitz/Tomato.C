#include "history.h"

#include <stdlib.h>

#include "error.h"

/* PRIVATE HISTORY FUNCTIONS */
/* History Operations */
static void pushStack(HistoryNode** stack, void* data, void (*free_fn)(void*));
static HistoryNode* popStack(HistoryNode** stack);
static int getStackCount(const HistoryNode* stack);

/**
 * ---------------------------------------------------------------------------
 * History Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create a new History manager.
 * @return Pointer to History, or NULL on allocation failure
 */
History* CreateHistory(void) {
  History* history = (History*)malloc(sizeof(History));
  if (!history) { LogError("CreateHistory", MALLOC_ERROR); return NULL; }
  history->past = NULL;
  history->future = NULL;
  history->present = -1;
  return history;
}

/**
 * Free History and all its data.
 * @param history Pointer to History
 * @param free_fn Function to free data in stacks (can be NULL)
 */
void FreeHistory(History* history, void (*free_fn)(void*)) {
  if (!history) return;

  /* Free past stack with custom free_fn */
  while (history->past) {
    HistoryNode* node = history->past;
    history->past = node->next;
    if (free_fn && node->data) free_fn(node->data);
    free(node);
  }

  /* Free future stack with custom free_fn */
  while (history->future) {
    HistoryNode* node = history->future;
    history->future = node->next;
    if (free_fn && node->data) free_fn(node->data);
    free(node);
  }

  free(history);
}

/**
 * ---------------------------------------------------------------------------
 * History Operations
 * ---------------------------------------------------------------------------
 */

/**
 * Push current state to history.
 * @param history Pointer to History
 * @param data Pointer to data snapshot (caller retains ownership)
 * @param free_fn Function to free the data (can be NULL)
 * @param to_future If true, push to future stack; if false, push to past stack
 */
void HistoryPush(History* history, void* data, void (*free_fn)(void*),
                 bool to_future) {
  if (!history || !data) return;

  /* Determine which stack to push to */
  HistoryNode** stack = to_future ? &history->future : &history->past;

  /* Check stack limit */
  int stack_count = getStackCount(*stack);
  if (stack_count >= HISTORY_MAX_STACK) {
    /* Remove oldest */
    HistoryNode* oldest = *stack;
    while (oldest->next && oldest->next->next) oldest = oldest->next;
    if (oldest->next) {
      if (oldest->next->free_fn && oldest->next->data)
        oldest->next->free_fn(oldest->next->data);
      free(oldest->next);
      oldest->next = NULL;
    }
  }

  pushStack(stack, data, free_fn);
}

/**
 * Pop state from history stack.
 * @param history Pointer to History
 * @param from_past If true, pop from past (undo); if false, pop from future (redo)
 * @return Data pointer (caller takes ownership), or NULL
 */
void* HistoryPop(History* history, bool from_past) {
  if (!history) return NULL;

  HistoryNode** stack = from_past ? &history->past : &history->future;
  HistoryNode* node = popStack(stack);

  if (!node) return NULL;

  void* data = node->data;
  /* Don't call free_fn - caller takes ownership */
  free(node);
  return data;
}

/**
 * Check if undo is available.
 * @param history Pointer to History
 * @return true if can undo
 */
bool HistoryCanUndo(const History* history) {
  return history && history->past != NULL;
}

/**
 * Check if redo is available.
 * @param history Pointer to History
 * @return true if can redo
 */
bool HistoryCanRedo(const History* history) {
  return history && history->future != NULL;
}

/**
 * Push a node onto a stack.
 * @param stack Pointer to the stack pointer
 * @param data Data to push
 * @param free_fn Function to free the data
 */
static void pushStack(HistoryNode** stack, void* data, void (*free_fn)(void*)) {
  HistoryNode* node = (HistoryNode*)malloc(sizeof(HistoryNode));
  if (!node) {
    LogError("pushStack", MALLOC_ERROR);
    if (free_fn) free_fn(data);
    return;
  }
  node->data = data;
  node->free_fn = free_fn;
  node->next = *stack;
  *stack = node;
}

/**
 * Pop a node from a stack.
 * @param stack Pointer to the stack pointer
 * @return The popped node (caller takes ownership of data), or NULL
 */
static HistoryNode* popStack(HistoryNode** stack) {
  if (!*stack) return NULL;
  HistoryNode* node = *stack;
  *stack = node->next;
  return node;
}

/**
 * Get the count of nodes in a stack.
 * @param stack Pointer to the stack
 * @return Number of nodes
 */
static int getStackCount(const HistoryNode* stack) {
  int count = 0;
  while (stack) {
    count++;
    stack = stack->next;
  }
  return count;
}
