#ifndef HISTORY_H_
#define HISTORY_H_

#include <stdbool.h>

typedef struct HistoryNode HistoryNode;
typedef struct History History;

#define HISTORY_MAX_STACK 128

/**
 * History node containing data and cleanup function.
 */
struct HistoryNode {
  void* data;             /**< Generic data pointer */
  void (*free_fn)(void*); /**< Function to free the data */
  HistoryNode* next;      /**< Pointer to next node */
};

/**
 * History manager for undo/redo operations.
 * Uses generic void* data with custom cleanup functions.
 */
struct History {
  HistoryNode* past;   /**< Stack of past states (for undo) */
  HistoryNode* future; /**< Stack of future states (for redo) */
  int present;         /**< Current state value (for scene history) */
};

/**
 * ---------------------------------------------------------------------------
 * History Lifecycle
 * ---------------------------------------------------------------------------
 */

/**
 * Create a new History manager.
 * @return Pointer to History, or NULL on allocation failure
 */
History* CreateHistory(void);

/**
 * Free History and its data.
 * @param history Pointer to History
 * @param free_fn Function to free data in stacks (can be NULL)
 */
void FreeHistory(History* history, void (*free_fn)(void*));

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
                 bool to_future);

/**
 * Pop state from history stack.
 * @param history Pointer to History
 * @param from_past If true, pop from past (undo); if false, pop from future (redo)
 * @return Data pointer (caller takes ownership), or NULL
 */
void* HistoryPop(History* history, bool from_past);

/**
 * Check if undo is available.
 * @param history Pointer to History
 * @return true if can undo
 */
bool HistoryCanUndo(const History* history);

/**
 * Check if redo is available.
 * @param history Pointer to History
 * @return true if can redo
 */
bool HistoryCanRedo(const History* history);

#endif /* HISTORY_H_ */
