/*
 * message.c
 *
 *  Created on: 2026. 11. 12.
 *      Author: kccistc
 */

/* include */
#include "message.h"

/* define */

/* typedef */

/* variable declarations */

/* variable definitions */

/* function declarations */

/* function definitions */
bool comm_mailbox_init(void)
{
    /* Called before any task starts. CubeMX still generates a uint32_t queue;
     * replace it with the value mailbox from the preserved RTOS_QUEUES block. */
    QueueHandle_t mailbox = xQueueCreate(1, sizeof(comm_data_t));
    if (mailbox == NULL) return false;
    if (commQueHandle != NULL) vQueueDelete(commQueHandle);
    commQueHandle = mailbox;
    return true;
}
